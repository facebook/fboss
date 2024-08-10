// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/DsfSubscriber.h"
#include "fboss/agent/AgentFeatures.h"
#include "fboss/agent/DsfStateUpdaterUtil.h"
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
#include "fboss/fsdb/common/Flags.h"
#include "fboss/fsdb/if/FsdbModel.h"
#include "fboss/fsdb/if/gen-cpp2/fsdb_common_types.h"
#include "fboss/thrift_cow/nodes/Serializer.h"

#include <memory>

using namespace facebook::fboss;
namespace {

std::map<folly::IPAddress, folly::IPAddress> getDsfSessionIps(
    const std::set<folly::CIDRNetwork>& localIpsSorted,
    const std::set<folly::CIDRNetwork>& loopbackIpsSorted) {
  auto maxElems = std::min(
      FLAGS_dsf_num_parallel_sessions_per_remote_interface_node,
      static_cast<uint32_t>(
          std::min(localIpsSorted.size(), loopbackIpsSorted.size())));

  // Copy first n elements of loopbackIps
  std::map<folly::IPAddress, folly::IPAddress> dsfSessionIps;
  auto sitr = localIpsSorted.begin();
  auto ditr = loopbackIpsSorted.begin();
  for (auto i = 0; i < maxElems; ++i) {
    dsfSessionIps.insert({sitr->first, ditr->first});
    sitr++;
    ditr++;
  }
  return dsfSessionIps;
}

} // anonymous namespace

namespace facebook::fboss {

using ThriftMapTypeClass = apache::thrift::type_class::map<
    apache::thrift::type_class::integral,
    apache::thrift::type_class::structure>;

DsfSubscriber::DsfSubscriber(SwSwitch* sw)
    : sw_(sw),
      localNodeName_(getLocalHostnameUqdn()),
      streamConnectPool_(std::make_unique<folly::IOThreadPoolExecutor>(
          1,
          std::make_shared<folly::NamedThreadFactory>(
              "DsfSubscriberStreamConnect"))),
      streamServePool_(std::make_unique<folly::IOThreadPoolExecutor>(
          1,
          std::make_shared<folly::NamedThreadFactory>(
              "DsfSubscriberStreamServe"))),
      hwUpdatePool_(std::make_unique<folly::IOThreadPoolExecutor>(
          1,
          std::make_shared<folly::NamedThreadFactory>("DsfHwUpdate"))) {
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

void DsfSubscriber::updateWithRollbackProtection(
    const std::string& nodeName,
    const std::map<SwitchID, std::shared_ptr<SystemPortMap>>&
        switchId2SystemPorts,
    const std::map<SwitchID, std::shared_ptr<InterfaceMap>>& switchId2Intfs) {
  auto hasNoLocalSwitchId = [this, nodeName](const auto& switchId2Objects) {
    for (const auto& [switchId, _] : switchId2Objects) {
      if (this->isLocal(switchId)) {
        throw FbossError(
            "Got updates for a local switch ID, from: ",
            nodeName,
            " id: ",
            switchId);
      }
    }
  };

  hasNoLocalSwitchId(switchId2SystemPorts);
  hasNoLocalSwitchId(switchId2Intfs);

  auto updateDsfStateFn =
      [this, nodeName, switchId2SystemPorts, switchId2Intfs](
          const std::shared_ptr<SwitchState>& in) {
        auto out = DsfStateUpdaterUtil::getUpdatedState(
            in,
            sw_->getScopeResolver(),
            sw_->getRib(),
            switchId2SystemPorts,
            switchId2Intfs);

        if (FLAGS_dsf_subscriber_cache_updated_state) {
          cachedState_ = out;
        }

        if (!FLAGS_dsf_subscriber_skip_hw_writes) {
          return out;
        }

        return std::shared_ptr<SwitchState>{};
      };

  sw_->getRib()->updateStateInRibThread([this, nodeName, updateDsfStateFn]() {
    sw_->updateStateWithHwFailureProtection(
        folly::sformat("Update state for node: {}", nodeName),
        updateDsfStateFn);
  });
}

void DsfSubscriber::stateUpdated(const StateDelta& stateDelta) {
  if (!FLAGS_dsf_subscribe) {
    return;
  }

  // Setup Fsdb subscriber if we have switch ids of type VOQ
  auto voqSwitchIds =
      sw_->getSwitchInfoTable().getSwitchIdsOfType(cfg::SwitchType::VOQ);
  if (!voqSwitchIds.size()) {
    // No processing needed on non VOQ switches
    return;
  }
  // Should never get here if we don't have voq switch Ids
  CHECK(voqSwitchIds.size());
  auto isInterfaceNode = [](const std::shared_ptr<DsfNode>& node) {
    return node->getType() == cfg::DsfNodeType::INTERFACE_NODE;
  };
  auto getLocalIps =
      [&voqSwitchIds](const std::shared_ptr<SwitchState>& state) {
        for (const auto& switchId : voqSwitchIds) {
          auto localDsfNode = state->getDsfNodes()->getNodeIf(switchId);
          CHECK(localDsfNode);
          if (localDsfNode->getLoopbackIpsSorted().size()) {
            return localDsfNode->getLoopbackIpsSorted();
          }
        }
        throw FbossError("Could not find loopback IP for any local VOQ switch");
      };
  auto addDsfNode = [&](const std::shared_ptr<DsfNode>& node) {
    // No need to setup subscriptions to (local) yourself
    // Only IN nodes have control plane, so ignore non IN DSF nodes
    if (isLocal(node->getSwitchId()) || !isInterfaceNode(node)) {
      return;
    }
    auto nodeName = node->getName();
    auto nodeSwitchId = node->getSwitchId();

    auto localIps = getLocalIps(stateDelta.newState());
    for (const auto& [srcIPAddr, dstIPAddr] :
         getDsfSessionIps(localIps, node->getLoopbackIpsSorted())) {
      auto dstIP = dstIPAddr.str();
      auto srcIP = srcIPAddr.str();
      XLOG(DBG2) << "Setting up DSF subscriptions to:: " << nodeName
                 << " dstIP: " << dstIP << " from: " << localNodeName_
                 << " srcIP: " << srcIP;

      auto remoteEndpoint = makeRemoteEndpoint(nodeName, dstIPAddr);

      fsdb::SubscriptionOptions opts{
          folly::sformat("{}_{}:agent", localNodeName_, dstIPAddr.str()),
          false /* subscribeStats */,
          FLAGS_dsf_gr_hold_time};
      auto subscriptionsWlock = subscriptions_.wlock();
      if (subscriptionsWlock->find(remoteEndpoint) !=
          subscriptionsWlock->end()) {
        // Session already exists. This maybe a node with multiple
        // VOQ switch ASICs (e.g. MTIA)
        continue;
      }
      subscriptionsWlock->emplace(
          remoteEndpoint,
          std::make_unique<DsfSubscription>(
              std::move(opts),
              streamConnectPool_->getEventBase(),
              streamServePool_->getEventBase(),
              hwUpdatePool_->getEventBase(),
              localNodeName_,
              nodeName,
              srcIPAddr,
              dstIPAddr,
              sw_,
              [this, nodeName, nodeSwitchId]() { // GrHoldExpiredCb
                // There is a single DSF subscription to every remote Interface
                // Node even
                // if the remote Interface Node is a multi ASIC system.
                // Thus, when GR hold timer expires for a specific switchID,
                // process every switchID (every ASIC) on that remote Interface
                // Node.
                processGRHoldTimerExpired(
                    nodeName,
                    getAllSwitchIDsForSwitch(
                        this->sw_->getState()->getDsfNodes(), nodeSwitchId));
              }));
    }
  };
  auto rmDsfNode = [&](const std::shared_ptr<DsfNode>& node) {
    // No need to setup subscriptions to (local) yourself
    // Only IN nodes have control plane, so ignore non IN DSF nodes
    if (isLocal(node->getSwitchId()) || !isInterfaceNode(node)) {
      return;
    }
    auto nodeName = node->getName();

    auto localIps = getLocalIps(stateDelta.oldState());
    for (const auto& [srcIPAddr, dstIPAddr] :
         getDsfSessionIps(localIps, node->getLoopbackIpsSorted())) {
      auto dstIP = dstIPAddr.str();
      XLOG(DBG2) << "Removing DSF subscriptions to:: " << nodeName
                 << " dstIP: " << dstIP;

      subscriptions_.wlock()->erase(makeRemoteEndpoint(nodeName, dstIPAddr));
    }
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

void DsfSubscriber::processGRHoldTimerExpired(
    const std::string& nodeName,
    const std::set<SwitchID>& allNodeSwitchIDs) {
  auto updateDsfStateFn = [this, nodeName, allNodeSwitchIDs](
                              const std::shared_ptr<SwitchState>& in) {
    bool changed{false};
    auto out = in->clone();

    auto remoteSystemPorts = out->getRemoteSystemPorts()->modify(&out);
    for (auto& [_, remoteSystemPortMap] : *remoteSystemPorts) {
      for (auto& [_, remoteSystemPort] : *remoteSystemPortMap) {
        // GR timeout expired for an Interface Node.
        // Mark all remote system ports synced over control plane (i.e.
        // DYNAMIC) as STALE for every switchID on that Interface Node
        // if FLAGS_dsf_flush_remote_sysports_and_rifs_on_gr is not set.
        // Otherwise, remove those remote system ports.
        if (allNodeSwitchIDs.count(remoteSystemPort->getSwitchId()) > 0 &&
            remoteSystemPort->getRemoteSystemPortType().has_value() &&
            remoteSystemPort->getRemoteSystemPortType().value() ==
                RemoteSystemPortType::DYNAMIC_ENTRY) {
          if (FLAGS_dsf_flush_remote_sysports_and_rifs_on_gr) {
            remoteSystemPorts->removeNode(remoteSystemPort->getID());
          } else {
            auto clonedNode = remoteSystemPort->isPublished()
                ? remoteSystemPort->clone()
                : remoteSystemPort;
            clonedNode->setRemoteLivenessStatus(LivenessStatus::STALE);
            remoteSystemPorts->updateNode(
                clonedNode, sw_->getScopeResolver()->scope(clonedNode));
          }

          changed = true;
        }
      }
    }

    auto remoteInterfaces = out->getRemoteInterfaces()->modify(&out);
    for (auto& [_, remoteInterfaceMap] : *remoteInterfaces) {
      for (auto& [_, remoteInterface] : *remoteInterfaceMap) {
        const auto& remoteSystemPort = in->getRemoteSystemPorts()->getNodeIf(
            *remoteInterface->getSystemPortID());

        if (remoteSystemPort) {
          auto switchID = remoteSystemPort->getSwitchId();
          // GR timeout expired for an Interface Node.
          // Mark all remote interfaces synced over control plane (i.e.
          // DYNAMIC) as STALE for every switchID on that Interface Node,
          // if FLAGS_dsf_flush_remote_sysports_and_rifs_on_gr is not set.
          // Otherwise, remove those remote rifs.
          // Always remove all the neighbor entries on that interface or else
          // we will end up blackholing the traffic.
          if (allNodeSwitchIDs.count(switchID) > 0 &&
              remoteInterface->getRemoteInterfaceType().has_value() &&
              remoteInterface->getRemoteInterfaceType().value() ==
                  RemoteInterfaceType::DYNAMIC_ENTRY) {
            if (FLAGS_dsf_flush_remote_sysports_and_rifs_on_gr) {
              remoteInterfaces->removeNode(remoteInterface->getID());
            } else {
              auto clonedNode = remoteInterface->isPublished()
                  ? remoteInterface->clone()
                  : remoteInterface;
              clonedNode->setRemoteLivenessStatus(LivenessStatus::STALE);
              clonedNode->setArpTable(state::NeighborEntries{});
              clonedNode->setNdpTable(state::NeighborEntries{});

              remoteInterfaces->updateNode(
                  clonedNode, sw_->getScopeResolver()->scope(clonedNode, out));
            }

            changed = true;
          }
        }
      }
    }

    if (changed) {
      return out;
    }
    return std::shared_ptr<SwitchState>{};
  };

  sw_->getRib()->updateStateInRibThread([this, nodeName, updateDsfStateFn]() {
    sw_->updateStateWithHwFailureProtection(
        folly::sformat("Update state for node: {}", nodeName),
        updateDsfStateFn);
  });
}

void DsfSubscriber::stop() {
  sw_->unregisterStateObserver(this);
  // make sure all subscribers are stopped before thread cleanup
  subscriptions_.wlock()->clear();
}

std::vector<DsfSessionThrift> DsfSubscriber::getDsfSessionsThrift() const {
  auto lockedSubscriptions = subscriptions_.rlock();
  std::vector<DsfSessionThrift> thriftSessions;
  thriftSessions.reserve(lockedSubscriptions->size());
  for (const auto& [_, subscription] : *lockedSubscriptions) {
    thriftSessions.emplace_back(subscription->dsfSessionThrift());
  }
  return thriftSessions;
}

} // namespace facebook::fboss
