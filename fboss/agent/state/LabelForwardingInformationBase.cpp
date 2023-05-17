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

LabelForwardingInformationBase* LabelForwardingInformationBase::programLabel(
    std::shared_ptr<SwitchState>* state,
    Label label,
    ClientID client,
    AdminDistance distance,
    LabelNextHopSet nexthops) {
  if (!MultiLabelForwardingInformationBase::isValidNextHopSet(nexthops)) {
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
    MultiLabelForwardingInformationBase::resolve(newEntry);
    writableLabelFib->addNode(newEntry);
  } else {
    auto entryToUpdate = entry->clone();
    entryToUpdate->update(
        client, LabelNextHopEntry(std::move(nexthops), distance));
    entryToUpdate->setResolved(*entryToUpdate->getBestEntry().second);
    XLOG(DBG2) << "updated label:" << label.value() << " nhops: " << nextHopsStr
               << "nhop count:" << nexthopCount
               << " in label forwarding information base for client:"
               << static_cast<int>(client);
    writableLabelFib->updateNode(entryToUpdate);
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
  auto entryToUpdate = entry->clone();
  entryToUpdate->delEntryForClient(client);
  XLOG(DBG2) << "removed label:" << label.value()
             << " from label forwarding information base for client:"
             << static_cast<int>(client);

  if (entryToUpdate->getEntryForClients().isEmpty()) {
    XLOG(DBG2) << "Purging empty forwarding entry for label:" << label.value();
    writableLabelFib->removeNode(entry);
  } else {
    entryToUpdate->setResolved(*(entryToUpdate->getBestEntry().second));
    writableLabelFib->updateNode(std::move(entryToUpdate));
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
      auto entryToModify = entry->clone();
      entryToModify->delEntryForClient(client);
      if (entryToModify->getEntryForClients().isEmpty()) {
        XLOG(DBG1) << "Purging empty forwarding entry for label:"
                   << entry->getID();
        iter = writableLabelFib->erase(iter);
        continue;
      } else {
        entryToModify->setResolved(*(entryToModify->getBestEntry().second));
      }
      writableLabelFib->updateNode(std::move(entryToModify));
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

bool MultiLabelForwardingInformationBase::isValidNextHopSet(
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

MultiLabelForwardingInformationBase*
MultiLabelForwardingInformationBase::modify(
    std::shared_ptr<SwitchState>* state) {
  return SwitchState::modify<switch_state_tags::labelFibMap>(state);
}

template class ThriftMapNode<
    LabelForwardingInformationBase,
    LabelForwardingInformationBaseTraits>;

} // namespace facebook::fboss
