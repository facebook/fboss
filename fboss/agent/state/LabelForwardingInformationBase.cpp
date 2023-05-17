// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/state/LabelForwardingInformationBase.h"
#include <fboss/agent/state/LabelForwardingEntry.h>
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
  const auto& fwd = entry->getForwardInfo();

  if (fwd.getAction() == LabelNextHopEntry::Action::DROP ||
      fwd.getAction() == LabelNextHopEntry::Action::TO_CPU) {
    return;
  }
  auto entries = entry->cref<switch_state_tags::nexthopsmulti>()->clone();
  // only interface routes and v6 ll routes will have interface id
  for (auto& clientEntry : entry->getEntryForClients()) {
    if (clientEntry.first == ClientID::INTERFACE_ROUTE) {
      continue;
    }
    RouteNextHopSet nhSet;
    const auto& rNHE = *clientEntry.second;
    for (auto& nh : rNHE.getNextHopSet()) {
      const auto& addr = nh.addr();
      if (addr.isV6() && addr.isLinkLocal()) {
        nhSet.emplace(nh);
      } else {
        nhSet.emplace(UnresolvedNextHop(
            nh.addr(), nh.weight(), nh.labelForwardingAction()));
      }
    }
    entries->update(
        clientEntry.first,
        RouteNextHopEntry(nhSet, rNHE.getAdminDistance(), rNHE.getCounterID()));
  }
  entry->ref<switch_state_tags::nexthopsmulti>() = entries;
  entry->setResolved(fwd);
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
  auto entry = writableLabelFib->getNodeIf(label.value());
  auto nexthopCount = nexthops.size();
  std::string nextHopsStr{};
  toAppend(nexthops, &nextHopsStr);

  if (!entry) {
    XLOG(DBG2) << "programmed label:" << label.value()
               << " nhops: " << nextHopsStr << " nhop count:" << nexthopCount
               << " in label forwarding information base for client:"
               << static_cast<int>(client);
    auto newEntry =
        std::make_shared<LabelForwardingEntry>(LabelForwardingEntry::makeThrift(
            label, client, LabelNextHopEntry(std::move(nexthops), distance)));
    resolve(newEntry);
    writableLabelFib->addNode(newEntry);
  } else {
    auto* entryToUpdate = modifyLabelEntry(state, entry);
    entryToUpdate->update(
        client, LabelNextHopEntry(std::move(nexthops), distance));
    entryToUpdate->setResolved(*entryToUpdate->getBestEntry().second);
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
  auto entry = writableLabelFib->getNodeIf(label.value());
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
    entryToUpdate->setResolved(*(entryToUpdate->getBestEntry().second));
  }
  return writableLabelFib;
}

LabelForwardingInformationBase*
LabelForwardingInformationBase::purgeEntriesForClient(
    std::shared_ptr<SwitchState>* state,
    ClientID client) {
  auto* writableLabelFib = modify(state);
  auto iter = writableLabelFib->begin();
  while (iter != writableLabelFib->end()) {
    auto entry = iter->second;
    if (entry->getEntryForClient(client)) {
      auto entryToModify = modifyLabelEntry(state, entry);
      entryToModify->delEntryForClient(client);
      if (entryToModify->getEntryForClients().isEmpty()) {
        XLOG(DBG1) << "Purging empty forwarding entry for label:"
                   << entry->getID();
        iter = writableLabelFib->erase(iter);
        continue;
      } else {
        entryToModify->setResolved(*(entryToModify->getBestEntry().second));
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

  auto newFib = this->clone();
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

template class ThriftMapNode<
    LabelForwardingInformationBase,
    LabelForwardingInformationBaseTraits>;

} // namespace facebook::fboss
