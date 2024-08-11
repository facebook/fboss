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

std::string makeRemoteEndpoint(
    const std::string& remoteNode,
    const folly::IPAddress& remoteIP) {
  return DsfSubscription::makeRemoteEndpoint(remoteNode, remoteIP);
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
              getAllSwitchIDsForSwitch(
                  sw_->getState()->getDsfNodes(), nodeSwitchId),
              srcIPAddr,
              dstIPAddr,
              sw_));
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
