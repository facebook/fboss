// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/DsfStateUpdaterUtil.h"

#include "fboss/agent/state/StateDelta.h"

namespace facebook::fboss {

std::shared_ptr<SwitchState> DsfStateUpdaterUtil::getUpdatedState(
    const std::shared_ptr<SwitchState>& in,
    const SwitchIdScopeResolver* scopeResolver,
    const std::map<SwitchID, std::shared_ptr<SystemPortMap>>&
        switchId2SystemPorts,
    const std::map<SwitchID, std::shared_ptr<InterfaceMap>>& switchId2Intfs) {
  bool changed{false};
  auto out = in->clone();

  auto skipProgramming = [&](const auto& nbrEntryIter) -> bool {
    // Local neighbor entry on one DSF node is remote neighbor entry on
    // every other DSF node. Thus, for neighbor entry received from other
    // DSF nodes, set isLocal = False before programming it.
    // Also, link local only has significance for Servers directly
    // connected to Interface Node. Thus, skip programming remote link
    // local neighbors.
    if (nbrEntryIter->second->getIP().isLinkLocal()) {
      return true;
    }

    // Only program neighbor entries that are REACHABLE on the DSF node
    // that resolved it.
    if (nbrEntryIter->second->getState() != NeighborState::REACHABLE) {
      return true;
    }

    return false;
  };

  auto updateResolvedTimestamp = [&](const auto& oldTable,
                                     const auto& nbrEntryIter) {
    // If dynamic neighbor entry got added the first time, update
    // the resolved timestamp.
    if (nbrEntryIter->second->getType() ==
            state::NeighborEntryType::DYNAMIC_ENTRY &&
        (!oldTable ||
         (std::as_const(*oldTable).find(nbrEntryIter->second->getID())) ==
             oldTable->cend())) {
      nbrEntryIter->second->setResolvedSince(
          static_cast<int64_t>(std::time(nullptr)));
    } else {
      // Retain the resolved timestamp from the old entry.
      nbrEntryIter->second->setResolvedSince(
          oldTable->at(nbrEntryIter->second->getID())->getResolvedSince());
    }
  };

  auto updateNeighborEntry = [&](const auto& oldTable,
                                 const auto& clonedTable) {
    auto nbrEntryIter = clonedTable->begin();
    while (nbrEntryIter != clonedTable->end()) {
      if (skipProgramming(nbrEntryIter)) {
        nbrEntryIter = clonedTable->erase(nbrEntryIter);
      } else {
        // Entries received from remote are non-Local on current node
        nbrEntryIter->second->setIsLocal(false);
        // Entries received from remote always need to be programmed
        nbrEntryIter->second->setNoHostRoute(false);
        updateResolvedTimestamp(oldTable, nbrEntryIter);
        ++nbrEntryIter;
      }
    }
  };

  auto makeRemoteSysPort = [&](const auto& /*oldNode*/, const auto& newNode) {
    return newNode;
  };
  auto makeRemoteRif = [&](const auto& oldNode, const auto& newNode) {
    auto clonedNode = newNode->clone();

    if (newNode->isPublished()) {
      clonedNode->setArpTable(newNode->getArpTable()->toThrift());
      clonedNode->setNdpTable(newNode->getNdpTable()->toThrift());
    }

    updateNeighborEntry(
        oldNode ? oldNode->getArpTable() : nullptr, clonedNode->getArpTable());
    updateNeighborEntry(
        oldNode ? oldNode->getNdpTable() : nullptr, clonedNode->getNdpTable());

    return clonedNode;
  };

  auto processDelta = [&]<typename MapT>(
                          auto& delta, MapT* mapToUpdate, auto& makeRemote) {
    DeltaFunctions::forEachChanged(
        delta,
        [&](const auto& oldNode, const auto& newNode) {
          if (*oldNode != *newNode) {
            // Compare contents as we reconstructed
            // map from deserialized FSDB
            // subscriptions. So can't just rely on
            // pointer comparison here.
            auto clonedNode = makeRemote(oldNode, newNode);
            if constexpr (std::is_same_v<MapT, MultiSwitchSystemPortMap>) {
              mapToUpdate->updateNode(
                  clonedNode, scopeResolver->scope(clonedNode));
            } else {
              mapToUpdate->updateNode(
                  clonedNode, scopeResolver->scope(clonedNode, in));
            }
            changed = true;
          }
        },
        [&](const auto& newNode) {
          auto clonedNode =
              makeRemote(std::decay_t<decltype(newNode)>{nullptr}, newNode);
          if constexpr (std::is_same_v<MapT, MultiSwitchSystemPortMap>) {
            mapToUpdate->addNode(clonedNode, scopeResolver->scope(clonedNode));
          } else {
            mapToUpdate->addNode(
                clonedNode, scopeResolver->scope(clonedNode, in));
          }
          changed = true;
        },
        [&](const auto& rmNode) {
          mapToUpdate->removeNode(rmNode);
          changed = true;
        });
  };

  for (const auto& [nodeSwitchId, newSysPorts] : switchId2SystemPorts) {
    XLOG(DBG2) << "SwitchId: " << static_cast<int64_t>(nodeSwitchId)
               << " updated # of sys ports: " << newSysPorts->size();

    auto origSysPorts = out->getSystemPorts(nodeSwitchId);
    ThriftMapDelta<SystemPortMap> delta(origSysPorts.get(), newSysPorts.get());
    auto remoteSysPorts = out->getRemoteSystemPorts()->modify(&out);
    processDelta(delta, remoteSysPorts, makeRemoteSysPort);
  }

  for (const auto& [nodeSwitchId, newRifs] : switchId2Intfs) {
    XLOG(DBG2) << "SwitchId: " << static_cast<int64_t>(nodeSwitchId)
               << " updated # of intfs: " << newRifs->size();

    auto origRifs = out->getInterfaces(nodeSwitchId);
    InterfaceMapDelta delta(origRifs.get(), newRifs.get());
    auto remoteRifs = out->getRemoteInterfaces()->modify(&out);
    processDelta(delta, remoteRifs, makeRemoteRif);
  }

  if (changed) {
    return out;
  }
  return std::shared_ptr<SwitchState>{};
}

} // namespace facebook::fboss
