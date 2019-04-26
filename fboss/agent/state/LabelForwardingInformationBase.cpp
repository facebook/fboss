// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/state/LabelForwardingInformationBase.h"
#include "fboss/agent/state/NodeMap-defs.h"
#include "fboss/agent/state/SwitchState.h"

#include <folly/logging/xlog.h>

namespace facebook {
namespace fboss {

LabelForwardingInformationBase::LabelForwardingInformationBase() {}

LabelForwardingInformationBase::~LabelForwardingInformationBase() {}

const std::shared_ptr<LabelForwardingEntry>&
LabelForwardingInformationBase::getLabelForwardingEntry(
    Label labelFib) const {
  return getNode(labelFib);
}

std::shared_ptr<LabelForwardingEntry>
LabelForwardingInformationBase::getLabelForwardingEntryIf(
    Label labelFib) const {
  return getNodeIf(labelFib);
}

std::shared_ptr<LabelForwardingInformationBase>
LabelForwardingInformationBase::fromFollyDynamic(const folly::dynamic& json) {
  auto labelFib = std::make_shared<LabelForwardingInformationBase>();
  if (json.isNull()) {
    return labelFib;
  }
  for (const auto& entry : json[BaseT::kEntries]) {
    labelFib->addNode(LabelForwardingEntry::fromFollyDynamic(entry));
  }
  return labelFib;
}

LabelForwardingInformationBase* LabelForwardingInformationBase::programLabel(
    std::shared_ptr<SwitchState>* state,
    Label label,
    ClientID client,
    AdminDistance distance,
    LabelNextHopSet nexthops) {
  for (const auto& nexthop : nexthops) {
    if (!nexthop.labelForwardingAction()) {
      throw FbossError(
          "label next hop ", nexthop.str(), " has no forwarding action");
    }
    if (nexthop.labelForwardingAction()->type() ==
        LabelForwardingAction::LabelForwardingType::POP_AND_LOOKUP) {
      /* pop and lookup forwarding action does not have and need interface id
      as well as next hop address, accordingly it is always valid. however
      there must be only one next hop with pop and lookup */
      if (nexthops.size() > 1) {
        throw FbossError("pop label forwarding action with multiple next hops");
      } else {
        continue;
      }
    }

    if (!nexthop.isResolved()) {
      throw FbossError("invalid label next hop", nexthop.str());
    }
  }

  auto* writableLabelFib = modify(state);
  auto entry = writableLabelFib->getLabelForwardingEntryIf(label);

  if (!entry) {
    XLOG(DBG3) << "programmed label:" << label
               << " in label forwarding information base for client:" << client;

    writableLabelFib->addNode(std::make_shared<LabelForwardingEntry>(
        label, client, LabelNextHopEntry(std::move(nexthops), distance)));
  } else {
    auto* entryToUpdate = entry->modify(state);
    entryToUpdate->update(
        client, LabelNextHopEntry(std::move(nexthops), distance));
    XLOG(DBG3) << "updated label:" << label
               << " in label forwarding information base for client:" << client;
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
        label,
        " which does not exist in Label Information Base");
  }
  auto* entryToUpdate = entry->modify(state);
  entryToUpdate->delEntryForClient(client);
  XLOG(DBG3) << "removed label:" << label
             << " from label forwarding information base for client:" << client;

  if (entryToUpdate->isEmpty()) {
    XLOG(DBG3) << "Purging empty forwarding entry for label:" << label;
    writableLabelFib->removeNode(entry);
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
    if (iter->second->getEntryForClient(client)) {
      auto* entry = iter->second->modify(state);
      entry->delEntryForClient(client);
      if (entry->isEmpty()) {
        XLOG(DBG1) << "Purging empty forwarding entry for label:"
                   << entry->getID();
        iter = writableLabelFib->writableNodes().erase(iter);
        continue;
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

FBOSS_INSTANTIATE_NODE_MAP(
    LabelForwardingInformationBase,
    LabelForwardingRoute);

} // namespace fboss
} // namespace facebook
