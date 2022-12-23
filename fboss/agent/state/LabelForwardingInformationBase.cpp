// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/state/LabelForwardingInformationBase.h"
#include <fboss/agent/state/LabelForwardingEntry.h>
#include "fboss/agent/rib/RoutingInformationBase.h"
#include "fboss/agent/state/NodeMap-defs.h"
#include "fboss/agent/state/RouteNextHopEntry.h"
#include "fboss/agent/state/SwitchState.h"

#include <folly/logging/xlog.h>

// Enable label route handling in Routing Information Base
// which allows recursive resolution of mpls routes
DEFINE_bool(mpls_rib, true, "Enable mpls rib");

namespace facebook::fboss {

namespace {
auto constexpr kIncomingLabel = "topLabel";
auto constexpr kLabelNextHop = "labelNextHop";
auto constexpr kLabelNextHopsByClient = "labelNextHopMulti";
} // namespace

LabelForwardingInformationBase::LabelForwardingInformationBase() {}

LabelForwardingInformationBase::~LabelForwardingInformationBase() {}

const std::shared_ptr<LabelForwardingEntry>&
LabelForwardingInformationBase::getLabelForwardingEntry(Label labelFib) const {
  return getNode(labelFib);
}

std::shared_ptr<LabelForwardingEntry>
LabelForwardingInformationBase::getLabelForwardingEntryIf(
    Label labelFib) const {
  return getNodeIf(labelFib);
}

// Save entries in old format till code to parse new format is in prod
std::shared_ptr<LabelForwardingInformationBase>
LabelForwardingInformationBase::fromFollyDynamicLegacy(
    const folly::dynamic& json) {
  auto labelFib = std::make_shared<LabelForwardingInformationBase>();
  if (json.isNull()) {
    return labelFib;
  }
  for (const auto& entry : json[kEntries]) {
    labelFib->addNode(labelEntryFromFollyDynamic(entry));
  }
  return labelFib;
}

std::shared_ptr<LabelForwardingEntry>
LabelForwardingInformationBase::labelEntryFromFollyDynamic(
    folly::dynamic entry) {
  std::shared_ptr<LabelForwardingEntry> labelEntry;
  if (entry.find(kIncomingLabel) != entry.items().end()) {
    labelEntry = fromFollyDynamicOldFormat(entry);
  } else {
    labelEntry = LabelForwardingEntry::fromFollyDynamic(entry);
  }
  if (FLAGS_mpls_rib) {
    noRibToRibEntryConvertor(labelEntry);
  }
  return labelEntry;
}

std::shared_ptr<LabelForwardingEntry>
LabelForwardingInformationBase::fromFollyDynamicOldFormat(folly::dynamic json) {
  auto topLabel = static_cast<MplsLabel>(json[kIncomingLabel].asInt());
  auto entry = std::make_shared<LabelForwardingEntry>(topLabel);
  auto labelNextHopsByClient(LabelNextHopsByClient::fromFollyDynamicLegacy(
      json[kLabelNextHopsByClient]));
  for (const auto& clientEntry : labelNextHopsByClient) {
    entry->update(clientEntry.first, RouteNextHopEntry(clientEntry.second));
  }
  entry->setResolved(
      LabelNextHopEntry::fromFollyDynamicLegacy(json[kLabelNextHop]));
  return entry;
}

folly::dynamic LabelForwardingInformationBase::toFollyDynamicOldFormat(
    std::shared_ptr<LabelForwardingEntry> entry) {
  folly::dynamic json = folly::dynamic::object;
  json[kIncomingLabel] = static_cast<int>(entry->getID().value());
  json[kLabelNextHop] = entry->getForwardInfo().toFollyDynamicLegacy();
  json[kLabelNextHopsByClient] =
      entry->getEntryForClients().toFollyDynamicLegacy();
  return json;
}

// when rib is enabled, the client entries are stored as received
// from producer of route. ie interfaces will not be resolved
// unless it is a v6 link local or interface route. With no rib,
// thrift handler layer will fill in interface id for all client
// entries. This method converts a no rib entry to a rib entry
// and is used for warmboot upgrade from no rib to rib case.
void LabelForwardingInformationBase::noRibToRibEntryConvertor(
    std::shared_ptr<LabelForwardingEntry>& entry) {
  CHECK(!entry->isPublished());
  // cache fwdinfo before modifying the route
  auto fwd = entry->getForwardInfo();

  if (fwd.getAction() == LabelNextHopEntry::Action::DROP ||
      fwd.getAction() == LabelNextHopEntry::Action::TO_CPU) {
    return;
  }
  // only interface routes and v6 ll routes will have interface id
  for (auto& clientEntry : entry->getEntryForClients()) {
    if (clientEntry.first == ClientID::INTERFACE_ROUTE) {
      continue;
    }
    RouteNextHopSet nhSet;
    auto rNHE = RouteNextHopEntry(clientEntry.second);
    for (auto& nh : rNHE.getNextHopSet()) {
      const auto& addr = nh.addr();
      if (addr.isV6() && addr.isLinkLocal()) {
        nhSet.emplace(nh);
      } else {
        nhSet.emplace(UnresolvedNextHop(
            nh.addr(), nh.weight(), nh.labelForwardingAction()));
      }
    }
    entry->update(
        clientEntry.first,
        RouteNextHopEntry(nhSet, rNHE.getAdminDistance(), rNHE.getCounterID()));
  }
  entry->setResolved(std::move(fwd));
}

LabelForwardingInformationBase* LabelForwardingInformationBase::programLabel(
    std::shared_ptr<SwitchState>* state,
    Label label,
    ClientID client,
    AdminDistance distance,
    LabelNextHopSet nexthops) {
  if (!isValidNextHopSet(nexthops)) {
    throw FbossError("invalid label next hop");
  }

  auto* writableLabelFib = modify(state);
  auto entry = writableLabelFib->getLabelForwardingEntryIf(label);
  auto nexthopCount = nexthops.size();
  std::string nextHopsStr{};
  toAppend(nexthops, &nextHopsStr);

  if (!entry) {
    XLOG(DBG2) << "programmed label:" << label.value()
               << " nhops: " << nextHopsStr << " nhop count:" << nexthopCount
               << " in label forwarding information base for client:"
               << static_cast<int>(client);
    auto newEntry = std::make_shared<LabelForwardingEntry>(
        label, client, LabelNextHopEntry(std::move(nexthops), distance));
    resolve(newEntry);
    writableLabelFib->addNode(newEntry);
  } else {
    auto* entryToUpdate = modifyLabelEntry(state, entry);
    entryToUpdate->update(
        client, LabelNextHopEntry(std::move(nexthops), distance));
    entryToUpdate->setResolved(
        RouteNextHopEntry(*entryToUpdate->getBestEntry().second));
    XLOG(DBG2) << "updated label:" << label.value() << " nhops: " << nextHopsStr
               << "nhop count:" << nexthopCount
               << " in label forwarding information base for client:"
               << static_cast<int>(client);
  }
  return writableLabelFib;
}

LabelForwardingInformationBase* LabelForwardingInformationBase::unprogramLabel(
    std::shared_ptr<SwitchState>* state,
    Label label,
    ClientID client) {
  auto* writableLabelFib = modify(state);
  auto entry = writableLabelFib->getLabelForwardingEntryIf(label);
  if (!entry) {
    throw FbossError(
        "request to delete a label ",
        label.value(),
        " which does not exist in Label Information Base");
  }
  auto* entryToUpdate = modifyLabelEntry(state, entry);
  entryToUpdate->delEntryForClient(client);
  XLOG(DBG2) << "removed label:" << label.value()
             << " from label forwarding information base for client:"
             << static_cast<int>(client);

  if (entryToUpdate->getEntryForClients().isEmpty()) {
    XLOG(DBG2) << "Purging empty forwarding entry for label:" << label.value();
    writableLabelFib->removeNode(entry);
  } else {
    auto entryThrift = entryToUpdate->getBestEntry().second;
    entryToUpdate->setResolved(RouteNextHopEntry(*entryThrift));
  }
  return writableLabelFib;
}

LabelForwardingInformationBase*
LabelForwardingInformationBase::purgeEntriesForClient(
    std::shared_ptr<SwitchState>* state,
    ClientID client) {
  auto* writableLabelFib = modify(state);
  auto iter = writableLabelFib->writableNodes().begin();
  while (iter != writableLabelFib->writableNodes().end()) {
    auto entry = iter->second;
    if (entry->getEntryForClient(client)) {
      auto entryToModify = modifyLabelEntry(state, entry);
      entryToModify->delEntryForClient(client);
      if (entryToModify->getEntryForClients().isEmpty()) {
        XLOG(DBG1) << "Purging empty forwarding entry for label:"
                   << entry->getID().label();
        iter = writableLabelFib->writableNodes().erase(iter);
        continue;
      } else {
        auto entryThrift =
            RouteNextHopEntry(*(entryToModify->getBestEntry().second));
        entryToModify->setResolved(std::move(entryThrift));
      }
    }
    ++iter;
  }
  return writableLabelFib;
}

LabelForwardingInformationBase* LabelForwardingInformationBase::modify(
    std::shared_ptr<SwitchState>* state) {
  if (!isPublished()) {
    CHECK(!(*state)->isPublished());
    return this;
  }

  auto newFib = clone();
  auto* ptr = newFib.get();
  (*state)->resetLabelForwardingInformationBase(std::move(newFib));
  return ptr;
}

bool LabelForwardingInformationBase::isValidNextHopSet(
    const LabelNextHopSet& nexthops) {
  for (const auto& nexthop : nexthops) {
    if (!nexthop.labelForwardingAction()) {
      XLOG(ERR) << "missing label forwarding action in " << nexthop.str();
      return false;
    }
    if (nexthop.isPopAndLookup() && nexthops.size() > 1) {
      /* pop and lookup forwarding action does not have and need interface id
      as well as next hop address, accordingly it is always valid. however
      there must be only one next hop with pop and lookup */
      XLOG(ERR) << "nexthop set with pop and lookup exceed size 1";
      return false;
    } else if (!nexthop.isPopAndLookup() && !nexthop.isResolved()) {
      XLOG(ERR) << "next hop is not resolved, " << nexthop.str();
      return false;
    }
  }
  return true;
}

std::shared_ptr<LabelForwardingEntry>
LabelForwardingInformationBase::cloneLabelEntry(
    std::shared_ptr<LabelForwardingEntry> entry) {
  return entry->clone();
}

LabelForwardingEntry* LabelForwardingInformationBase::modifyLabelEntry(
    std::shared_ptr<SwitchState>* state,
    std::shared_ptr<LabelForwardingEntry> entry) {
  if (!entry->isPublished()) {
    CHECK(!(*state)->isPublished());
    return entry.get();
  }

  LabelForwardingInformationBase* labelFib =
      (*state)->getLabelForwardingInformationBase()->modify(state);
  auto newEntry = entry->clone();
  auto* ptr = newEntry.get();
  labelFib->updateNode(std::move(newEntry));
  return ptr;
}

FBOSS_INSTANTIATE_NODE_MAP(
    LabelForwardingInformationBase,
    LabelForwardingRoute);

} // namespace facebook::fboss
