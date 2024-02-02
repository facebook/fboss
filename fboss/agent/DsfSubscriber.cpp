// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/DsfSubscriber.h"
#include <fb303/ServiceData.h>
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
#include "fboss/fsdb/client/FsdbPubSubManager.h"
#include "fboss/fsdb/common/Flags.h"
#include "fboss/fsdb/if/FsdbModel.h"
#include "fboss/fsdb/if/gen-cpp2/fsdb_common_types.h"
#include "fboss/thrift_cow/nodes/Serializer.h"

#include <memory>

DEFINE_bool(dsf_subscriber_skip_hw_writes, false, "Skip writing to HW");
DEFINE_bool(
    dsf_subscriber_cache_updated_state,
    false,
    "Cache switch state after update by dsf subsriber");
DEFINE_uint32(
    dsf_gr_hold_time,
    0,
    "GR hold time for FSDB DsfSubscription in sec");

namespace {
const thriftpath::RootThriftPath<facebook::fboss::fsdb::FsdbOperStateRoot>
    stateRoot;
} // anonymous namespace

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

const auto& DsfSubscriber::getSystemPortsPath() {
  static auto path = stateRoot.agent().switchState().systemPortMaps();
  return path;
}

const auto& DsfSubscriber::getInterfacesPath() {
  static auto path = stateRoot.agent().switchState().interfaceMaps();
  return path;
}

auto DsfSubscriber::getDsfSubscriptionsPath(const std::string& localNodeName) {
  static auto path = stateRoot.agent().fsdbSubscriptions();
  return path[localNodeName];
}

std::vector<std::vector<std::string>> DsfSubscriber::getAllSubscribePaths(
    const std::string& localNodeName) {
  return {
      getSystemPortsPath().tokens(),
      getInterfacesPath().tokens(),
      getDsfSubscriptionsPath(localNodeName).tokens()};
}

bool DsfSubscriber::isLocal(SwitchID nodeSwitchId) const {
  auto localSwitchIds = sw_->getSwitchInfoTable().getSwitchIDs();
  return localSwitchIds.find(nodeSwitchId) != localSwitchIds.end();
}

void DsfSubscriber::scheduleUpdate(
    const std::string& nodeName,
    SwitchID nodeSwitchId,
    const std::map<SwitchID, std::shared_ptr<SystemPortMap>>&
        switchId2SystemPorts,
    const std::map<SwitchID, std::shared_ptr<InterfaceMap>>& switchId2Intfs) {
  auto hasNoLocalSwitchId = [this, nodeName](const auto& switchId2Objects) {
    for (const auto& [switchId, _] : switchId2Objects) {
      if (this->isLocal(switchId)) {
        throw FbossError(
            " Got updates for a local switch ID, from: ",
            nodeName,
            " id: ",
            switchId);
      }
    }
  };

  hasNoLocalSwitchId(switchId2SystemPorts);
  hasNoLocalSwitchId(switchId2Intfs);

  auto updateDsfStateFn =
      [this, nodeName, nodeSwitchId, switchId2SystemPorts, switchId2Intfs](
          const std::shared_ptr<SwitchState>& in) {
        auto out = DsfStateUpdaterUtil::getUpdatedState(
            in, sw_->getScopeResolver(), switchId2SystemPorts, switchId2Intfs);

        if (FLAGS_dsf_subscriber_cache_updated_state) {
          cachedState_ = out;
        }

        if (!FLAGS_dsf_subscriber_skip_hw_writes) {
          return out;
        }

        return std::shared_ptr<SwitchState>{};
      };

  sw_->updateState(
      folly::sformat("Update state for node: {}", nodeName),
      std::move(updateDsfStateFn));
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

  auto getServerOptions = [&voqSwitchIds](
                              const auto& dstIP, const auto& state) {
    // Use loopback IP of any local VOQ switch as src for FSDB subscriptions
    // TODO: Evaluate what we should do if one or more VOQ switches go down
    auto getLocalIp = [&voqSwitchIds, &state]() {
      for (const auto& switchId : voqSwitchIds) {
        auto localDsfNode = state->getDsfNodes()->getNodeIf(switchId);
        CHECK(localDsfNode);
        if (localDsfNode->getLoopbackIpsSorted().size()) {
          return (*localDsfNode->getLoopbackIpsSorted().begin()).first.str();
        }
      }
      throw FbossError("Could not find loopback IP for any local VOQ switch");
    };
    // Subscribe to FSDB of DSF node in the cluster with:
    //  dstIP = inband IP of that DSF node
    //  dstPort = FSDB port
    //  srcIP = self inband IP
    //  priority = CRITICAL
    auto serverOptions = fsdb::FsdbStreamClient::ServerOptions(
        dstIP,
        FLAGS_fsdbPort,
        getLocalIp(),
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

    dsfSessions_.wlock()->emplace(nodeName, nodeName);
    for (const auto& network : node->getLoopbackIpsSorted()) {
      auto dstIP = network.first.str();
      XLOG(DBG2) << "Setting up DSF subscriptions to : " << nodeName
                 << " dstIP: " << dstIP;

      // Subscription is not established until state becomes CONNECTED
      this->sw_->stats()->failedDsfSubscription(1);

      auto subscriberId = folly::sformat("{}_{}:agent", localNodeName_, dstIP);
      fsdb::FsdbExtStateSubscriber::SubscriptionOptions opts{
          subscriberId, false /* subscribeStats */, FLAGS_dsf_gr_hold_time};
      fsdbPubSubMgr_->addStatePathSubscription(
          std::move(opts),
          getAllSubscribePaths(localNodeName_),
          [this, nodeName](
              fsdb::FsdbExtStateSubscriber::SubscriptionState oldState,
              fsdb::FsdbExtStateSubscriber::SubscriptionState newState) {
            handleFsdbSubscriptionStateUpdate(nodeName, oldState, newState);
          },
          [this, nodeName, nodeSwitchId](
              fsdb::OperSubPathUnit&& operStateUnit) {
            handleFsdbUpdate(nodeSwitchId, nodeName, std::move(operStateUnit));
          },
          getServerOptions(dstIP, stateDelta.newState()));
    }
  };
  auto rmDsfNode = [&](const std::shared_ptr<DsfNode>& node) {
    // No need to setup subscriptions to (local) yourself
    // Only IN nodes have control plane, so ignore non IN DSF nodes
    if (isLocal(node->getSwitchId()) || !isInterfaceNode(node)) {
      return;
    }
    auto nodeName = node->getName();
    dsfSessions_.wlock()->erase(nodeName);

    for (const auto& network : node->getLoopbackIpsSorted()) {
      auto dstIP = network.first.str();
      XLOG(DBG2) << "Removing DSF subscriptions to : " << nodeName
                 << " dstIP: " << dstIP;

      if (fsdbPubSubMgr_->getStatePathSubsriptionState(
              getAllSubscribePaths(localNodeName_), dstIP) !=
          fsdb::FsdbStreamClient::State::CONNECTED) {
        // Subscription was not established - decrement failedDSF counter.
        this->sw_->stats()->failedDsfSubscription(-1);
      }

      fsdbPubSubMgr_->removeStatePathSubscription(
          getAllSubscribePaths(localNodeName_), dstIP);
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

void DsfSubscriber::handleFsdbSubscriptionStateUpdate(
    const std::string& nodeName,
    fsdb::FsdbExtStateSubscriber::SubscriptionState oldState,
    fsdb::FsdbExtStateSubscriber::SubscriptionState newState) {
  if (XLOG_IS_ON(DBG2)) {
    XLOG(DBG2)
        << "DsfSubscriber: " << nodeName << ": subscription state changed "
        << fsdb::FsdbExtStateSubscriber::subscriptionStateToString(oldState)
        << " -> "
        << fsdb::FsdbExtStateSubscriber::subscriptionStateToString(newState);
  }

  auto oldThriftState = fsdb::FsdbExtStateSubscriber::isConnected(oldState)
      ? fsdb::FsdbSubscriptionState::CONNECTED
      : fsdb::FsdbSubscriptionState::DISCONNECTED;
  auto newThriftState = fsdb::FsdbExtStateSubscriber::isConnected(newState)
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
    auto lockedDsfSessions = this->dsfSessions_.wlock();
    if (auto it = lockedDsfSessions->find(nodeName);
        it != lockedDsfSessions->end()) {
      it->second.localSubStateChanged(newThriftState);
    }
  }
}

void DsfSubscriber::handleFsdbUpdate(
    SwitchID nodeSwitchId,
    const std::string& nodeName,
    fsdb::OperSubPathUnit&& operStateUnit) {
  std::map<SwitchID, std::shared_ptr<SystemPortMap>> switchId2SystemPorts;
  std::map<SwitchID, std::shared_ptr<InterfaceMap>> switchId2Intfs;

  for (const auto& change : *operStateUnit.changes()) {
    if (getSystemPortsPath().matchesPath(*change.path()->path())) {
      XLOG(DBG2) << " Got sys port update from : " << nodeName;
      MultiSwitchSystemPortMap mswitchSysPorts;
      mswitchSysPorts.fromThrift(thrift_cow::deserialize<
                                 MultiSwitchSystemPortMapTypeClass,
                                 MultiSwitchSystemPortMapThriftType>(
          fsdb::OperProtocol::BINARY, *change.state()->contents()));
      for (const auto& [id, sysPortMap] : mswitchSysPorts) {
        auto matcher = HwSwitchMatcher(id);
        switchId2SystemPorts[matcher.switchId()] = sysPortMap;
      }
    } else if (getInterfacesPath().matchesPath(*change.path()->path())) {
      XLOG(DBG2) << " Got rif update from : " << nodeName;
      MultiSwitchInterfaceMap mswitchIntfs;
      mswitchIntfs.fromThrift(thrift_cow::deserialize<
                              MultiSwitchInterfaceMapTypeClass,
                              MultiSwitchInterfaceMapThriftType>(
          fsdb::OperProtocol::BINARY, *change.state()->contents()));
      for (const auto& [id, intfMap] : mswitchIntfs) {
        auto matcher = HwSwitchMatcher(id);
        switchId2Intfs[matcher.switchId()] = intfMap;
      }
    } else if (getDsfSubscriptionsPath(localNodeName_)
                   .matchesPath(*change.path()->path())) {
      XLOG(DBG2) << " Got dsf sub update from : " << nodeName;

      using targetType = fsdb::FsdbSubscriptionState;
      using targetTypeClass = apache::thrift::type_class::enumeration;

      auto newRemoteState =
          thrift_cow::deserialize<targetTypeClass, targetType>(
              fsdb::OperProtocol::BINARY, *change.state()->contents());

      auto lockedDsfSessions = this->dsfSessions_.wlock();
      if (auto it = lockedDsfSessions->find(nodeName);
          it != lockedDsfSessions->end()) {
        it->second.remoteSubStateChanged(newRemoteState);
      }
    } else {
      throw FbossError(
          " Got unexpected state update for : ",
          folly::join("/", *change.path()->path()),
          " from node: ",
          nodeName);
    }
  }
  scheduleUpdate(nodeName, nodeSwitchId, switchId2SystemPorts, switchId2Intfs);
}

void DsfSubscriber::stop() {
  sw_->unregisterStateObserver(this);
  fsdbPubSubMgr_.reset();
}

std::vector<DsfSessionThrift> DsfSubscriber::getDsfSessionsThrift() const {
  auto lockedSessions = dsfSessions_.rlock();
  std::vector<DsfSessionThrift> thriftSessions;
  for (const auto& [key, value] : *lockedSessions) {
    thriftSessions.emplace_back(value.toThrift());
  }
  return thriftSessions;
}

} // namespace facebook::fboss
