// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/DsfStateUpdaterUtil.h"

#include "fboss/agent/rib/ForwardingInformationBaseUpdater.h"
#include "fboss/agent/rib/RoutingInformationBase.h"
#include "fboss/agent/state/StateDelta.h"

namespace {

typedef std::pair<facebook::fboss::InterfaceID, folly::IPAddress> IntfAddress;
typedef boost::container::flat_map<folly::CIDRNetwork, IntfAddress> IntfRoute;
typedef boost::container::flat_map<facebook::fboss::RouterID, IntfRoute>
    IntfRouteTable;

std::shared_ptr<facebook::fboss::SwitchState> updateFibForRemoteConnectedRoutes(
    const facebook::fboss::SwitchIdScopeResolver* resolver,
    facebook::fboss::RouterID vrf,
    const facebook::fboss::IPv4NetworkToRouteMap& v4NetworkToRoute,
    const facebook::fboss::IPv6NetworkToRouteMap& v6NetworkToRoute,
    const facebook::fboss::LabelToRouteMap& labelToRoute,
    void* cookie) {
  facebook::fboss::ForwardingInformationBaseUpdater fibUpdater(
      resolver, vrf, v4NetworkToRoute, v6NetworkToRoute, labelToRoute);

  auto nextStatePtr =
      static_cast<std::shared_ptr<facebook::fboss::SwitchState>*>(cookie);

  fibUpdater(*nextStatePtr);
  return *nextStatePtr;
}

void updateRemoteConnectedRoutes(
    std::shared_ptr<facebook::fboss::SwitchState>& state,
    const facebook::fboss::SwitchIdScopeResolver* scopeResolver,
    facebook::fboss::RoutingInformationBase* rib) {
  IntfRouteTable remoteIntfRouteTables;
  auto addInterfaceRoute = [&](const auto interfaces) {
    for (const auto& [intfId, intf] : *interfaces) {
      // On the same box, local interface of mpu0 will be added
      // as remote interface of mpu1 (and vice versa). Therefore
      // skipping those when processing remote interfaces.
      if (state->getInterfaces()->getNodeIf(intfId)) {
        continue;
      }
      for (const auto& [addr, mask] : std::as_const(*intf->getAddresses())) {
        const auto ipAddr = folly::IPAddress(addr);
        // Skip link-local addresses in directly-connected routes
        if (ipAddr.isV6() && ipAddr.isLinkLocal()) {
          continue;
        }
        remoteIntfRouteTables[intf->getRouterID()].emplace(
            folly::IPAddress::createNetwork(folly::to<std::string>(
                addr, "/", static_cast<int>(mask->cref()))),
            std::make_pair(intf->getID(), ipAddr));
      }
    }
  };

  addInterfaceRoute(std::as_const(*state->getRemoteInterfaces()).getAllNodes());

  rib->reconfigureRemoteInterfaceRoutes(
      scopeResolver,
      remoteIntfRouteTables,
      &updateFibForRemoteConnectedRoutes,
      static_cast<void*>(&state));
};

} // namespace

namespace facebook::fboss {

std::shared_ptr<SwitchState> DsfStateUpdaterUtil::getUpdatedState(
    const std::shared_ptr<SwitchState>& in,
    const SwitchIdScopeResolver* scopeResolver,
    RoutingInformationBase* rib,
    const std::map<SwitchID, std::shared_ptr<SystemPortMap>>&
        switchId2SystemPorts,
    const std::map<SwitchID, std::shared_ptr<InterfaceMap>>& switchId2Intfs) {
  bool changed{false};
  bool intfChanged{false};
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
        XLOG(DBG2) << "Skip programming remote neighbor: "
                   << nbrEntryIter->second->str();
        nbrEntryIter = clonedTable->erase(nbrEntryIter);
      } else {
        // Entries received from remote are non-Local on current node
        nbrEntryIter->second->setIsLocal(false);
        // Entries received from remote always need to be programmed
        nbrEntryIter->second->setNoHostRoute(false);
        updateResolvedTimestamp(oldTable, nbrEntryIter);
        XLOG(DBG2) << "Program remote neighbor: "
                   << nbrEntryIter->second->str();
        ++nbrEntryIter;
      }
    }
  };

  auto makeRemoteSysPort = [&](const auto& oldNode, const auto& newNode) {
    /*
     * RemoteSystemPorts synced from a remote peer R contain:
     *  o RemoteSystemPorts corresponding to remote recycle ports.
     *    - These are already programmed statically based on DSF node config
     *      and are used to bring up the control plane. Thus, don't overwrite
     *      statically programmed entries.
     *  o RemoteSystemPorts corresponding to remote NIF ports.
     *    - remoteSystemPortType and remoteLivenessStatus is applicable only for
     *      remoteSystemPorts.
     *    - However, these ports are localSystemPorts for peer R, and thus
     *      remoteSystemPortType and remoteLivenessStatus is not set. Set those.
     *    - remoteSystemPortType = DYNAMIC as these are synced dynamically.
     *    - remoteLivenessStatus = LIVE as these are synced from control plane.
     *      The remoteLivenessStatus will be changed to STALE on GR timeout.
     */
    CHECK(!oldNode || oldNode->getID() == newNode->getID());
    if (oldNode && oldNode->getRemoteSystemPortType().has_value() &&
        oldNode->getRemoteSystemPortType().value() ==
            RemoteSystemPortType::STATIC_ENTRY) {
      XLOG(DBG2)
          << "Skip overwriting STATIC remoteSystemPorts: " << " STATIC: "
          << apache::thrift::SimpleJSONSerializer::serialize<std::string>(
                 oldNode->toThrift())
          << " non-STATIC: "
          << apache::thrift::SimpleJSONSerializer::serialize<std::string>(
                 newNode->toThrift());
      return oldNode;
    }

    auto clonedNode = newNode->isPublished() ? newNode->clone() : newNode;
    clonedNode->setRemoteSystemPortType(RemoteSystemPortType::DYNAMIC_ENTRY);
    clonedNode->setRemoteLivenessStatus(LivenessStatus::LIVE);

    return clonedNode;
  };
  auto makeRemoteRif = [&](const auto& oldNode, const auto& newNode) {
    CHECK(!oldNode || oldNode->getID() == newNode->getID());
    if (oldNode && oldNode->getRemoteInterfaceType().has_value() &&
        oldNode->getRemoteInterfaceType().value() ==
            RemoteInterfaceType::STATIC_ENTRY) {
      XLOG(DBG2)
          << "Skip overwriting STATIC remoteInterface: " << " STATIC: "
          << apache::thrift::SimpleJSONSerializer::serialize<std::string>(
                 oldNode->toThrift())
          << " non-STATIC: "
          << apache::thrift::SimpleJSONSerializer::serialize<std::string>(
                 newNode->toThrift());
      return oldNode;
    }

    auto clonedNode = newNode->clone();
    clonedNode->setRemoteInterfaceType(RemoteInterfaceType::DYNAMIC_ENTRY);
    clonedNode->setRemoteLivenessStatus(LivenessStatus::LIVE);

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
    intfChanged |= (delta.begin() != delta.end());
  }

  if (intfChanged) {
    updateRemoteConnectedRoutes(out, scopeResolver, rib);
  }

  if (changed) {
    return out;
  }
  return std::shared_ptr<SwitchState>{};
}

} // namespace facebook::fboss
