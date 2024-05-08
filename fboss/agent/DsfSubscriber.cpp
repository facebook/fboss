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
#include "fboss/fsdb/client/FsdbPubSubManager.h"
#include "fboss/fsdb/common/Flags.h"
#include "fboss/fsdb/if/FsdbModel.h"
#include "fboss/fsdb/if/gen-cpp2/fsdb_common_types.h"
#include "fboss/thrift_cow/nodes/Serializer.h"

#include <memory>

using namespace facebook::fboss;
namespace {
const thriftpath::RootThriftPath<facebook::fboss::fsdb::FsdbOperStateRoot>
    stateRoot;

fsdb::FsdbStreamClient::ServerOptions getServerOptions(
    const std::string& srcIP,
    const std::string& dstIP) {
  // Subscribe to FSDB of DSF node in the cluster with:
  //  dstIP = inband IP of that DSF node
  //  dstPort = FSDB port
  //  srcIP = self inband IP
  //  priority = CRITICAL
  return fsdb::FsdbStreamClient::ServerOptions(
      dstIP, FLAGS_fsdbPort, srcIP, fsdb::FsdbStreamClient::Priority::CRITICAL);
}

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

const auto& getSystemPortsPath() {
  static auto path = stateRoot.agent().switchState().systemPortMaps();
  return path;
}

const auto& getInterfacesPath() {
  static auto path = stateRoot.agent().switchState().interfaceMaps();
  return path;
}

auto getDsfSubscriptionsPath(const std::string& localNodeName) {
  static auto path = stateRoot.agent().fsdbSubscriptions();
  return path[localNodeName];
}

std::vector<std::vector<std::string>> getAllSubscribePaths(
    const std::string& localNodeName,
    const folly::IPAddress& localIP) {
  return {
      getSystemPortsPath().tokens(),
      getInterfacesPath().tokens(),
      // When subscribing to remote node - localNodeName, localIP is remote
      getDsfSubscriptionsPath(
          DsfSubscriber::makeRemoteEndpoint(localNodeName, localIP))
          .tokens()};
}

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

std::string DsfSubscriber::makeRemoteEndpoint(
    const std::string& remoteNode,
    const folly::IPAddress& remoteIP) {
  return folly::sformat("{}::{}", remoteNode, remoteIP.str());
}

bool DsfSubscriber::isLocal(SwitchID nodeSwitchId) const {
  auto localSwitchIds = sw_->getSwitchInfoTable().getSwitchIDs();
  return localSwitchIds.find(nodeSwitchId) != localSwitchIds.end();
}

void DsfSubscriber::scheduleUpdate(
    const std::string& nodeName,
    const SwitchID& nodeSwitchId,
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
      [this, nodeName, nodeSwitchId, switchId2SystemPorts, switchId2Intfs](
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

  sw_->updateState(
      folly::sformat("Update state for node: {}", nodeName),
      std::move(updateDsfStateFn));
}

void DsfSubscriber::stateUpdated(const StateDelta& stateDelta) {
  if (!FLAGS_dsf_subscribe) {
    return;
  }

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
          << "Transition from VOQ to non-VOQ swtich type is not supported";
    }
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
    // Use loopback IP of any local VOQ switch as src for FSDB subscriptions
    // TODO: Evaluate what we should do if one or more VOQ switches go down
    auto localIp = localIps.begin()->first.str();
    for (const auto& [srcIPAddr, dstIPAddr] :
         getDsfSessionIps(localIps, node->getLoopbackIpsSorted())) {
      auto dstIP = dstIPAddr.str();
      auto srcIP = srcIPAddr.str();
      XLOG(DBG2) << "Setting up DSF subscriptions to:: " << nodeName
                 << " dstIP: " << dstIP << " from: " << localNodeName_
                 << " srcIP: " << srcIP;

      // Subscription is not established until state becomes CONNECTED
      this->sw_->stats()->failedDsfSubscription(nodeSwitchId, nodeName, 1);
      auto remoteEndpoint = makeRemoteEndpoint(nodeName, dstIPAddr);
      dsfSessions_.wlock()->emplace(remoteEndpoint, DsfSession(remoteEndpoint));

      auto subscriberId = folly::sformat("{}_{}:agent", localNodeName_, dstIP);
      fsdb::FsdbExtStateSubscriber::SubscriptionOptions opts{
          subscriberId, false /* subscribeStats */, FLAGS_dsf_gr_hold_time};
      fsdbPubSubMgr_->addStatePathSubscription(
          std::move(opts),
          getAllSubscribePaths(localNodeName_, srcIPAddr),
          [this, nodeName, dstIPAddr = dstIPAddr, nodeSwitchId](
              fsdb::SubscriptionState oldState,
              fsdb::SubscriptionState newState) {
            handleFsdbSubscriptionStateUpdate(
                nodeName, dstIPAddr, nodeSwitchId, oldState, newState);
          },
          [this,
           nodeName,
           nodeSwitchId,
           srcIPAddr = srcIPAddr,
           dstIPAddr = dstIPAddr](fsdb::OperSubPathUnit&& operStateUnit) {
            handleFsdbUpdate(
                srcIPAddr,
                nodeSwitchId,
                nodeName,
                dstIPAddr,
                std::move(operStateUnit));
          },
          getServerOptions(localIp, dstIP));
    }
  };
  auto rmDsfNode = [&](const std::shared_ptr<DsfNode>& node) {
    // No need to setup subscriptions to (local) yourself
    // Only IN nodes have control plane, so ignore non IN DSF nodes
    if (isLocal(node->getSwitchId()) || !isInterfaceNode(node)) {
      return;
    }
    auto nodeName = node->getName();
    auto nodeSwitchId = node->getSwitchId();

    auto localIps = getLocalIps(stateDelta.oldState());
    for (const auto& [srcIPAddr, dstIPAddr] :
         getDsfSessionIps(localIps, node->getLoopbackIpsSorted())) {
      auto dstIP = dstIPAddr.str();
      XLOG(DBG2) << "Removing DSF subscriptions to:: " << nodeName
                 << " dstIP: " << dstIP;
      auto remoteEndpoint = makeRemoteEndpoint(nodeName, dstIPAddr);
      dsfSessions_.wlock()->erase(remoteEndpoint);

      if (fsdbPubSubMgr_->getStatePathSubsriptionState(
              getAllSubscribePaths(localNodeName_, srcIPAddr), dstIP) !=
          fsdb::FsdbStreamClient::State::CONNECTED) {
        // Subscription was not established - decrement failedDSF counter.
        this->sw_->stats()->failedDsfSubscription(nodeSwitchId, nodeName, -1);
      }

      fsdbPubSubMgr_->removeStatePathSubscription(
          getAllSubscribePaths(localNodeName_, srcIPAddr), dstIP);
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
        // DYNAMIC) as STALE for every switchID on that Interface Node.
        if (allNodeSwitchIDs.count(remoteSystemPort->getSwitchId()) > 0 &&
            remoteSystemPort->getRemoteSystemPortType().has_value() &&
            remoteSystemPort->getRemoteSystemPortType().value() ==
                RemoteSystemPortType::DYNAMIC_ENTRY) {
          auto clonedNode = remoteSystemPort->isPublished()
              ? remoteSystemPort->clone()
              : remoteSystemPort;
          clonedNode->setRemoteLivenessStatus(LivenessStatus::STALE);
          remoteSystemPorts->updateNode(
              clonedNode, sw_->getScopeResolver()->scope(clonedNode));
          changed = true;
        }
      }
    }

    auto remoteInterfaces = out->getRemoteInterfaces()->modify(&out);
    for (auto& [_, remoteInterfaceMap] : *remoteInterfaces) {
      for (auto& [_, remoteInterface] : *remoteInterfaceMap) {
        const auto& remoteSystemPort =
            remoteSystemPorts->getNodeIf(*remoteInterface->getSystemPortID());

        if (remoteSystemPort) {
          auto switchID = remoteSystemPort->getSwitchId();
          // GR timeout expired for an Interface Node.
          // Mark all remote interfaces synced over control plane (i.e.
          // DYNAMIC) as STALE for every switchID on that Interface Node,
          // Remove all the neighbor entries on that interface.
          if (allNodeSwitchIDs.count(switchID) > 0 &&

              remoteInterface->getRemoteInterfaceType().has_value() &&
              remoteInterface->getRemoteInterfaceType().value() ==
                  RemoteInterfaceType::DYNAMIC_ENTRY) {
            auto clonedNode = remoteInterface->isPublished()
                ? remoteInterface->clone()
                : remoteInterface;
            clonedNode->setRemoteLivenessStatus(LivenessStatus::STALE);
            clonedNode->setArpTable(state::NeighborEntries{});
            clonedNode->setNdpTable(state::NeighborEntries{});

            remoteInterfaces->updateNode(
                clonedNode, sw_->getScopeResolver()->scope(clonedNode, out));
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

  sw_->updateState(
      folly::sformat(
          "Update state on GR Hold Timer expired for node: {}", nodeName),
      std::move(updateDsfStateFn));
}

void DsfSubscriber::handleFsdbSubscriptionStateUpdate(
    const std::string& remoteNodeName,
    const folly::IPAddress& remoteIP,
    const SwitchID& remoteSwitchId,
    fsdb::SubscriptionState oldState,
    fsdb::SubscriptionState newState) {
  auto remoteEndpoint = makeRemoteEndpoint(remoteNodeName, remoteIP);
  XLOG(DBG2) << "DsfSubscriber: " << remoteEndpoint
             << " SwitchID: " << static_cast<int>(remoteSwitchId)
             << ": subscription state changed "
             << fsdb::subscriptionStateToString(oldState) << " -> "
             << fsdb::subscriptionStateToString(newState);

  auto oldThriftState = fsdb::isConnected(oldState)
      ? fsdb::FsdbSubscriptionState::CONNECTED
      : fsdb::FsdbSubscriptionState::DISCONNECTED;
  auto newThriftState = fsdb::isConnected(newState)
      ? fsdb::FsdbSubscriptionState::CONNECTED
      : fsdb::FsdbSubscriptionState::DISCONNECTED;

  if (oldThriftState != newThriftState) {
    if (newThriftState == fsdb::FsdbSubscriptionState::CONNECTED) {
      this->sw_->stats()->failedDsfSubscription(
          remoteSwitchId, remoteNodeName, -1);
    } else {
      this->sw_->stats()->failedDsfSubscription(
          remoteSwitchId, remoteNodeName, 1);
    }

    this->sw_->updateDsfSubscriberState(
        remoteEndpoint, oldThriftState, newThriftState);
    auto lockedDsfSessions = this->dsfSessions_.wlock();
    if (auto it = lockedDsfSessions->find(remoteEndpoint);
        it != lockedDsfSessions->end()) {
      it->second.localSubStateChanged(newThriftState);
    }
  }

  if (fsdb::isGRHoldExpired(newState)) {
    // There is a single DSF subscription to every remote Interface Node even
    // if the remote Interface Node is a multi ASIC system.
    // Thus, when GR hold timer expires for a specific switchID, process every
    // switchID (every ASIC) on that remote Interface Node.
    processGRHoldTimerExpired(
        remoteNodeName,
        getAllSwitchIDsForSwitch(
            this->sw_->getState()->getDsfNodes(), remoteSwitchId));
  }
}

void DsfSubscriber::handleFsdbUpdate(
    const folly::IPAddress& localIP,
    SwitchID remoteSwitchId,
    const std::string& remoteNodeName,
    const folly::IPAddress& remoteIP,
    fsdb::OperSubPathUnit&& operStateUnit) {
  std::map<SwitchID, std::shared_ptr<SystemPortMap>> switchId2SystemPorts;
  std::map<SwitchID, std::shared_ptr<InterfaceMap>> switchId2Intfs;

  for (const auto& change : *operStateUnit.changes()) {
    if (getSystemPortsPath().matchesPath(*change.path()->path())) {
      XLOG(DBG2) << "Got sys port update from : " << remoteNodeName;
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
      XLOG(DBG2) << "Got rif update from : " << remoteNodeName;
      MultiSwitchInterfaceMap mswitchIntfs;
      mswitchIntfs.fromThrift(thrift_cow::deserialize<
                              MultiSwitchInterfaceMapTypeClass,
                              MultiSwitchInterfaceMapThriftType>(
          fsdb::OperProtocol::BINARY, *change.state()->contents()));
      for (const auto& [id, intfMap] : mswitchIntfs) {
        auto matcher = HwSwitchMatcher(id);
        switchId2Intfs[matcher.switchId()] = intfMap;
      }
    } else if (getDsfSubscriptionsPath(
                   makeRemoteEndpoint(localNodeName_, localIP))
                   .matchesPath(*change.path()->path())) {
      XLOG(DBG2) << "Got dsf sub update from : " << remoteNodeName;

      using targetType = fsdb::FsdbSubscriptionState;
      using targetTypeClass = apache::thrift::type_class::enumeration;

      auto newRemoteState =
          thrift_cow::deserialize<targetTypeClass, targetType>(
              fsdb::OperProtocol::BINARY, *change.state()->contents());

      auto lockedDsfSessions = this->dsfSessions_.wlock();
      if (auto it = lockedDsfSessions->find(
              makeRemoteEndpoint(remoteNodeName, remoteIP));
          it != lockedDsfSessions->end()) {
        it->second.remoteSubStateChanged(newRemoteState);
      }
    } else {
      throw FbossError(
          "Got unexpected state update for : ",
          folly::join("/", *change.path()->path()),
          " from node: ",
          remoteNodeName);
    }
  }
  scheduleUpdate(
      remoteNodeName, remoteSwitchId, switchId2SystemPorts, switchId2Intfs);
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
