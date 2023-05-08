// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/DsfSubscriber.h"
#include <fb303/ServiceData.h>
#include "fboss/agent/HwSwitchMatcher.h"
#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/SwitchIdScopeResolver.h"
#include "fboss/agent/Utils.h"
#include "fboss/agent/state/DsfNode.h"
#include "fboss/agent/state/InterfaceMap.h"
#include "fboss/agent/state/StateDelta.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/state/SystemPortMap.h"
#include "fboss/fsdb/client/FsdbPubSubManager.h"
#include "fboss/fsdb/common/Flags.h"
#include "fboss/thrift_cow/nodes/Serializer.h"

#include <memory>

DEFINE_bool(dsf_subscriber_skip_hw_writes, false, "Skip writing to HW");
DEFINE_bool(
    dsf_subscriber_cache_updated_state,
    false,
    "Cache switch state after update by dsf subsriber");

namespace facebook::fboss {

using ThriftMapTypeClass = apache::thrift::type_class::map<
    apache::thrift::type_class::integral,
    apache::thrift::type_class::structure>;

DsfSubscriber::DsfSubscriber(SwSwitch* sw) : sw_(sw) {
  sw_->registerStateObserver(this, "DSFSubscriber");
}

DsfSubscriber::~DsfSubscriber() {
  stop();
}

bool DsfSubscriber::isLocal(SwitchID nodeSwitchId) const {
  auto localSwitchIds = sw_->getSwitchInfoTable().getSwitchIDs();
  return localSwitchIds.find(nodeSwitchId) != localSwitchIds.end();
}

void DsfSubscriber::scheduleUpdate(
    const std::shared_ptr<SystemPortMap>& newSysPorts,
    const std::shared_ptr<InterfaceMap>& newRifs,
    const std::string& nodeName,
    SwitchID nodeSwitchId) {
  XLOG(DBG2) << " For , switchId: " << static_cast<int64_t>(nodeSwitchId)
             << " got,"
             << " updated # of sys ports: "
             << (newSysPorts ? newSysPorts->size() : 0)
             << " updated # of rifs: " << (newRifs ? newRifs->size() : 0);
  sw_->updateState(
      folly::sformat("Update state for node: {}", nodeName),
      [this, newSysPorts, newRifs, nodeSwitchId, nodeName](
          const std::shared_ptr<SwitchState>& in) {
        if (isLocal(nodeSwitchId)) {
          throw FbossError(
              " Got updates for a local switch ID, from: ",
              nodeName,
              " id: ",
              nodeSwitchId);
        }
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

        auto makeRemoteSysPort = [&](const auto& /*oldNode*/,
                                     const auto& newNode) { return newNode; };
        auto makeRemoteRif = [&](const auto& /*oldNode*/, const auto& newNode) {
          auto clonedNode = newNode->clone();

          if (newNode->isPublished()) {
            clonedNode->setArpTable(newNode->getArpTable()->toThrift());
            clonedNode->setNdpTable(newNode->getNdpTable()->toThrift());
          }

          auto arpEntryIter = (*clonedNode->getArpTable()).begin();
          while (arpEntryIter != (*clonedNode->getArpTable()).end()) {
            if (skipProgramming(arpEntryIter)) {
              arpEntryIter = (*clonedNode->getArpTable()).erase(arpEntryIter);
            } else {
              arpEntryIter->second->setIsLocal(false);
              ++arpEntryIter;
            }
          }

          auto ndpEntryIter = (*clonedNode->getNdpTable()).begin();
          while (ndpEntryIter != (*clonedNode->getNdpTable()).end()) {
            if (skipProgramming(ndpEntryIter)) {
              ndpEntryIter = (*clonedNode->getNdpTable()).erase(ndpEntryIter);
            } else {
              ndpEntryIter->second->setIsLocal(false);
              ++ndpEntryIter;
            }
          }

          return clonedNode;
        };

        auto processDelta = [&]<typename MapT>(
                                auto& delta,
                                MapT* mapToUpdate,
                                auto& makeRemote) {
          DeltaFunctions::forEachChanged(
              delta,
              [&](const auto& oldNode, const auto& newNode) {
                if (*oldNode != *newNode) {
                  // Compare contents as we reconstructed
                  // map from deserialized FSDB
                  // subscriptions. So can't just rely on
                  // pointer comparison here.
                  auto clonedNode = makeRemote(oldNode, newNode);
                  if constexpr (std::
                                    is_same_v<MapT, MultiSwitchSystemPortMap>) {
                    mapToUpdate->updateNode(
                        clonedNode, sw_->getScopeResolver()->scope(clonedNode));
                  } else {
                    mapToUpdate->updateNode(clonedNode);
                  }
                  changed = true;
                }
              },
              [&](const auto& newNode) {
                auto clonedNode = makeRemote(
                    std::decay_t<decltype(newNode)>{nullptr}, newNode);
                if constexpr (std::is_same_v<MapT, MultiSwitchSystemPortMap>) {
                  mapToUpdate->addNode(
                      clonedNode, sw_->getScopeResolver()->scope(clonedNode));
                } else {
                  mapToUpdate->addNode(clonedNode);
                }
                changed = true;
              },
              [&](const auto& rmNode) {
                mapToUpdate->removeNode(rmNode);
                changed = true;
              });
        };

        if (newSysPorts) {
          auto origSysPorts = out->getSystemPorts(nodeSwitchId);
          ThriftMapDelta<SystemPortMap> delta(
              origSysPorts.get(), newSysPorts.get());
          auto remoteSysPorts =
              out->getMultiSwitchRemoteSystemPorts()->modify(&out);
          processDelta(delta, remoteSysPorts, makeRemoteSysPort);
        }
        if (newRifs) {
          auto origRifs = out->getInterfaces(nodeSwitchId);
          InterfaceMapDelta delta(origRifs.get(), newRifs.get());
          auto remoteRifs = out->getRemoteInterfaces()->modify(&out);
          processDelta(delta, remoteRifs, makeRemoteRif);
        }
        if (FLAGS_dsf_subscriber_cache_updated_state) {
          cachedState_ = out;
        }
        if (changed && !FLAGS_dsf_subscriber_skip_hw_writes) {
          return out;
        }
        return std::shared_ptr<SwitchState>{};
      });
}

void DsfSubscriber::stateUpdated(const StateDelta& stateDelta) {
  // Setup Fsdb subscriber if we have switch ids of type VOQ
  auto voqSwitchIds =
      sw_->getSwitchInfoTable().getSwitchIdsOfType(cfg::SwitchType::VOQ);
  if (voqSwitchIds.size()) {
    if (!fsdbPubSubMgr_) {
      fsdbPubSubMgr_ = std::make_unique<fsdb::FsdbPubSubManager>(folly::sformat(
          "{}:agent:{}",
          getLocalHostname(),
          fb303::ServiceData::get()->getAliveSince().count()));
    }
  } else {
    if (fsdbPubSubMgr_) {
      // If we had a fsdbManager, it implies that we went from having VOQ
      // switches to no VOQ switches. This is not supported.
      XLOG(FATAL)
          << " Transition from VOQ to non-VOQ swtich type is not supported";
    }
    // No processing needed on non VOQ switches
    return;
  }
  // Should never get here if we don't have voq switch Ids
  CHECK(voqSwitchIds.size());
  auto isInterfaceNode = [](const std::shared_ptr<DsfNode>& node) {
    return node->getType() == cfg::DsfNodeType::INTERFACE_NODE;
  };
  auto getLoopbackIp = [](const std::shared_ptr<DsfNode>& node) {
    auto network = folly::IPAddress::createNetwork(
        (*node->getLoopbackIps()->cbegin())->toThrift(),
        -1 /*default CIDR*/,
        false /*apply mask*/);
    return network.first.str();
  };

  auto getServerOptions = [&voqSwitchIds, getLoopbackIp](
                              const std::shared_ptr<DsfNode>& node,
                              const auto& state) {
    // Use loopback IP of any local VOQ switch as src for FSDB subscriptions
    // TODO: Evaluate what we should do if one or more VOQ switches go down
    auto localDsfNode = state->getDsfNodes()->getNodeIf(*voqSwitchIds.begin());
    CHECK(localDsfNode);
    CHECK(localDsfNode->getLoopbackIpsSorted().size() != 0);

    // Subscribe to FSDB of DSF node in the cluster with:
    //  dstIP = inband IP of that DSF node
    //  dstPort = FSDB port
    //  srcIP = self inband IP
    auto serverOptions = fsdb::FsdbStreamClient::ServerOptions(
        getLoopbackIp(node),
        FLAGS_fsdbPort,
        (*localDsfNode->getLoopbackIpsSorted().begin()).first.str());

    return serverOptions;
  };

  auto addDsfNode = [&](const std::shared_ptr<DsfNode>& node) {
    // No need to setup subscriptions to (local) yourself
    // Only IN nodes have control plane, so ignore non IN DSF nodes
    if (isLocal(node->getSwitchId()) || !isInterfaceNode(node)) {
      return;
    }
    auto nodeName = node->getName();
    auto nodeSwitchId = node->getSwitchId();
    XLOG(DBG2) << " Setting up DSF subscriptions to : " << nodeName;
    fsdbPubSubMgr_->addStatePathSubscription(
        {getSystemPortsPath(), getInterfacesPath()},
        [nodeName](auto /*oldState*/, auto newState) {
          XLOG(DBG2) << (newState == fsdb::FsdbStreamClient::State::CONNECTED
                             ? "Connected to:"
                             : "Disconnected from: ")
                     << nodeName;
        },
        [this, nodeName, nodeSwitchId](fsdb::OperSubPathUnit&& operStateUnit) {
          std::shared_ptr<SystemPortMap> newSysPorts;
          std::shared_ptr<InterfaceMap> newRifs;
          for (const auto& change : *operStateUnit.changes()) {
            if (change.path()->path() == getSystemPortsPath()) {
              XLOG(DBG2) << " Got sys port update from : " << nodeName;
              MultiSwitchSystemPortMap mswitchSysPorts;
              mswitchSysPorts.fromThrift(thrift_cow::deserialize<
                                         MultiSwitchSystemPortMapTypeClass,
                                         MultiSwitchSystemPortMapThriftType>(
                  fsdb::OperProtocol::BINARY, *change.state()->contents()));
              newSysPorts = std::make_shared<SystemPortMap>();
              for (const auto& [_, sysPortMap] :
                   std::as_const(mswitchSysPorts)) {
                for (const auto& [_, sysPort] : std::as_const(*sysPortMap)) {
                  newSysPorts->addNode(sysPort);
                }
              }
            } else if (change.path()->path() == getInterfacesPath()) {
              XLOG(DBG2) << " Got rif update from : " << nodeName;
              newRifs = std::make_shared<InterfaceMap>();
              newRifs->fromThrift(
                  thrift_cow::
                      deserialize<ThriftMapTypeClass, InterfaceMapThriftType>(
                          fsdb::OperProtocol::BINARY,
                          *change.state()->contents()));
            } else {
              throw FbossError(
                  " Got unexpected state update for : ",
                  folly::join("/", *change.path()->path()),
                  " from node: ",
                  nodeName);
            }
          }
          scheduleUpdate(newSysPorts, newRifs, nodeName, nodeSwitchId);
        },
        getServerOptions(node, stateDelta.newState()));
  };
  auto rmDsfNode = [&](const std::shared_ptr<DsfNode>& node) {
    // No need to setup subscriptions to (local) yourself
    // Only IN nodes have control plane, so ignore non IN DSF nodes
    if (isLocal(node->getSwitchId()) || !isInterfaceNode(node)) {
      return;
    }
    XLOG(DBG2) << " Removing DSF subscriptions to : " << node->getName();
    fsdbPubSubMgr_->removeStatePathSubscription(
        {getSystemPortsPath(), getInterfacesPath()}, getLoopbackIp(node));
  };
  DeltaFunctions::forEachChanged(
      stateDelta.getDsfNodesDelta(),
      [&](auto oldNode, auto newNode) {
        rmDsfNode(oldNode);
        addDsfNode(newNode);
      },
      addDsfNode,
      rmDsfNode);
}

void DsfSubscriber::stop() {
  sw_->unregisterStateObserver(this);
  fsdbPubSubMgr_.reset();
}

} // namespace facebook::fboss
