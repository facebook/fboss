// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/DsfSubscriber.h"
#include <fb303/ServiceData.h>
#include "fboss/agent/HwSwitchMatcher.h"
#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/SwitchIdScopeResolver.h"
#include "fboss/agent/SwitchStats.h"
#include "fboss/agent/Utils.h"
#include "fboss/agent/state/DsfNode.h"
#include "fboss/agent/state/InterfaceMap.h"
#include "fboss/agent/state/StateDelta.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/state/SystemPortMap.h"
#include "fboss/fsdb/client/FsdbPubSubManager.h"
#include "fboss/fsdb/common/Flags.h"
#include "fboss/fsdb/if/gen-cpp2/fsdb_common_types.h"
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

DsfSubscriber::DsfSubscriber(SwSwitch* sw)
    : sw_(sw), localNodeName_(getLocalHostnameUqdn()) {
  // TODO(aeckert): add dedicated config field for localNodeName
  sw_->registerStateObserver(this, "DsfSubscriber");
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
                oldTable->at(nbrEntryIter->second->getID())
                    ->getResolvedSince());
          }
        };

        auto updateNeighborEntry = [&](const auto& oldTable,
                                       const auto& clonedTable) {
          auto nbrEntryIter = clonedTable->begin();
          while (nbrEntryIter != clonedTable->end()) {
            if (skipProgramming(nbrEntryIter)) {
              nbrEntryIter = clonedTable->erase(nbrEntryIter);
            } else {
              nbrEntryIter->second->setIsLocal(false);
              updateResolvedTimestamp(oldTable, nbrEntryIter);
              ++nbrEntryIter;
            }
          }
        };

        auto makeRemoteSysPort = [&](const auto& /*oldNode*/,
                                     const auto& newNode) { return newNode; };
        auto makeRemoteRif = [&](const auto& oldNode, const auto& newNode) {
          auto clonedNode = newNode->clone();

          if (newNode->isPublished()) {
            clonedNode->setArpTable(newNode->getArpTable()->toThrift());
            clonedNode->setNdpTable(newNode->getNdpTable()->toThrift());
          }

          updateNeighborEntry(
              oldNode ? oldNode->getArpTable() : nullptr,
              clonedNode->getArpTable());
          updateNeighborEntry(
              oldNode ? oldNode->getNdpTable() : nullptr,
              clonedNode->getNdpTable());

          return clonedNode;
        };

        auto processDelta = [&]<typename MapT>(
                                auto& delta,
                                MapT* mapToUpdate,
                                auto& makeRemote) {
          const auto& scopeResolver = sw_->getScopeResolver();
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
                        clonedNode, scopeResolver->scope(clonedNode));
                  } else {
                    mapToUpdate->updateNode(
                        clonedNode,
                        scopeResolver->scope(clonedNode, sw_->getState()));
                  }
                  changed = true;
                }
              },
              [&](const auto& newNode) {
                auto clonedNode = makeRemote(
                    std::decay_t<decltype(newNode)>{nullptr}, newNode);
                if constexpr (std::is_same_v<MapT, MultiSwitchSystemPortMap>) {
                  mapToUpdate->addNode(
                      clonedNode, scopeResolver->scope(clonedNode));
                } else {
                  mapToUpdate->addNode(
                      clonedNode,
                      scopeResolver->scope(clonedNode, sw_->getState()));
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
          auto remoteSysPorts = out->getRemoteSystemPorts()->modify(&out);
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
      fsdbPubSubMgr_ = std::make_unique<fsdb::FsdbPubSubManager>(
          folly::sformat("{}:agent", localNodeName_));
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
    //  priority = CRITICAL
    auto serverOptions = fsdb::FsdbStreamClient::ServerOptions(
        getLoopbackIp(node),
        FLAGS_fsdbPort,
        (*localDsfNode->getLoopbackIpsSorted().begin()).first.str(),
        fsdb::FsdbStreamClient::Priority::CRITICAL);

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
    // Subscription is not established until state becomes CONNECTED
    this->sw_->stats()->failedDsfSubscription(1);
    XLOG(DBG2) << " Setting up DSF subscriptions to : " << nodeName;
    fsdbPubSubMgr_->addStatePathSubscription(
        {getSystemPortsPath(), getInterfacesPath()},
        [this, nodeName](auto oldState, auto newState) {
          if (XLOG_IS_ON(DBG2)) {
            switch (newState) {
              case fsdb::FsdbStreamClient::State::CONNECTING:
                XLOG(DBG2) << "Try connecting to " << nodeName;
                break;
              case fsdb::FsdbStreamClient::State::CONNECTED:
                XLOG(DBG2) << "Connected to " << nodeName;
                break;
              case fsdb::FsdbStreamClient::State::DISCONNECTED:
                XLOG(DBG2) << "Disconnected from " << nodeName;
                break;
              case fsdb::FsdbStreamClient::State::CANCELLED:
                XLOG(DBG2) << "Cancelled " << nodeName;
                break;
            }
          }

          auto oldThriftState =
              (oldState == fsdb::FsdbStreamClient::State::CONNECTED)
              ? fsdb::FsdbSubscriptionState::CONNECTED
              : fsdb::FsdbSubscriptionState::DISCONNECTED;
          auto newThriftState =
              (newState == fsdb::FsdbStreamClient::State::CONNECTED)
              ? fsdb::FsdbSubscriptionState::CONNECTED
              : fsdb::FsdbSubscriptionState::DISCONNECTED;

          if (oldThriftState != newThriftState) {
            if (newThriftState == fsdb::FsdbSubscriptionState::CONNECTED) {
              this->sw_->stats()->failedDsfSubscription(-1);
            } else {
              this->sw_->stats()->failedDsfSubscription(1);
            }

            this->sw_->updateDsfSubscriberState(
                nodeName, oldThriftState, newThriftState);
          }
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
              newSysPorts = mswitchSysPorts.getAllNodes();
            } else if (change.path()->path() == getInterfacesPath()) {
              XLOG(DBG2) << " Got rif update from : " << nodeName;
              MultiSwitchInterfaceMap mswitchIntfs;
              mswitchIntfs.fromThrift(thrift_cow::deserialize<
                                      MultiSwitchInterfaceMapTypeClass,
                                      MultiSwitchInterfaceMapThriftType>(
                  fsdb::OperProtocol::BINARY, *change.state()->contents()));
              newRifs = mswitchIntfs.getAllNodes();
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
    if (fsdbPubSubMgr_->getStatePathSubsriptionState(
            {getSystemPortsPath(), getInterfacesPath()}, getLoopbackIp(node)) !=
        fsdb::FsdbStreamClient::State::CONNECTED) {
      // Subscription was not established - decrement failedDSF counter.
      this->sw_->stats()->failedDsfSubscription(-1);
    }
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
