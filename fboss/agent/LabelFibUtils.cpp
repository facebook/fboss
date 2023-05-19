// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/LabelFibUtils.h"

#include "fboss/agent/SwitchIdScopeResolver.h"
#include "fboss/agent/state/SwitchState.h"

namespace facebook::fboss {

std::shared_ptr<SwitchState> programLabel(
    const SwitchIdScopeResolver& resolver,
    const std::shared_ptr<SwitchState>& state,
    Label label,
    ClientID client,
    AdminDistance distance,
    LabelNextHopSet nexthops) {
  if (!MultiLabelForwardingInformationBase::isValidNextHopSet(nexthops)) {
    throw FbossError("invalid label next hop");
  }

  auto newState = state->clone();
  auto* writableLabelFib =
      newState->getLabelForwardingInformationBase()->modify(&newState);

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
    writableLabelFib->addNode(newEntry, resolver.scope(newEntry));
  } else {
    auto entryToUpdate = entry->clone();
    entryToUpdate->update(
        client, LabelNextHopEntry(std::move(nexthops), distance));
    entryToUpdate->setResolved(*entryToUpdate->getBestEntry().second);
    XLOG(DBG2) << "updated label:" << label.value() << " nhops: " << nextHopsStr
               << "nhop count:" << nexthopCount
               << " in label forwarding information base for client:"
               << static_cast<int>(client);
    writableLabelFib->updateNode(entryToUpdate, resolver.scope(entryToUpdate));
  }
  return newState;
}

std::shared_ptr<SwitchState> unprogramLabel(
    const SwitchIdScopeResolver& resolver,
    const std::shared_ptr<SwitchState>& state,
    Label label,
    ClientID client) {
  auto newState = state->clone();
  auto* writableLabelFib =
      newState->getLabelForwardingInformationBase()->modify(&newState);

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
    writableLabelFib->updateNode(entryToUpdate, resolver.scope(entryToUpdate));
  }
  return newState;
}

std::shared_ptr<SwitchState> purgeEntriesForClient(
    const SwitchIdScopeResolver& resolver,
    const std::shared_ptr<SwitchState>& state,
    ClientID client) {
  auto newState = state->clone();
  auto* writableLabelFibs =
      newState->getLabelForwardingInformationBase()->modify(&newState);

  auto miter = writableLabelFibs->begin();
  while (miter != writableLabelFibs->end()) {
    auto writableLabelFib = miter->second;
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
        writableLabelFibs->updateNode(
            entryToModify, resolver.scope(entryToModify));
      }
      ++iter;
    }
    miter++;
  }
  return newState;
}

} // namespace facebook::fboss
