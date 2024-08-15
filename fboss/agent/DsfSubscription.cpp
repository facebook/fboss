// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/DsfSubscription.h"
#include "fboss/agent/AgentFeatures.h"
#include "fboss/agent/DsfStateUpdaterUtil.h"
#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/SwitchStats.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/fsdb/if/FsdbModel.h"
#include "fboss/fsdb/if/gen-cpp2/fsdb_common_types.h"
#include "fboss/thrift_cow/nodes/Serializer.h"

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
          DsfSubscription::makeRemoteEndpoint(localNodeName, localIP))
          .tokens()};
}
auto constexpr kDsfCtrlLogPrefix = "DSF_CTRL_EVENT: ";
} // namespace

namespace facebook::fboss {

DsfSubscription::DsfSubscription(
    fsdb::SubscriptionOptions options,
    folly::EventBase* reconnectEvb,
    folly::EventBase* subscriberEvb,
    folly::EventBase* hwUpdateEvb,
    std::string localNodeName,
    std::string remoteNodeName,
    std::set<SwitchID> remoteNodeSwitchIds,
    folly::IPAddress localIp,
    folly::IPAddress remoteIp,
    SwSwitch* sw)
    : opts_(std::move(options)),
      hwUpdateEvb_(hwUpdateEvb),
      fsdbPubSubMgr_(new fsdb::FsdbPubSubManager(
          opts_.clientId_,
          reconnectEvb,
          subscriberEvb)),
      localNodeName_(std::move(localNodeName)),
      remoteNodeName_(std::move(remoteNodeName)),
      remoteNodeSwitchIds_(std::move(remoteNodeSwitchIds)),
      localIp_(std::move(localIp)),
      remoteIp_(std::move(remoteIp)),
      sw_(sw),
      session_(makeRemoteEndpoint(remoteNodeName_, remoteIp_)) {
  // Subscription is not established until state becomes CONNECTED
  sw->stats()->failedDsfSubscription(remoteNodeName_, 1);
  fsdbPubSubMgr_->addStatePathSubscription(
      fsdb::SubscriptionOptions(opts_),
      getAllSubscribePaths(localNodeName_, localIp_),
      [this](
          fsdb::SubscriptionState oldState, fsdb::SubscriptionState newState) {
        handleFsdbSubscriptionStateUpdate(oldState, newState);
      },
      [this](fsdb::OperSubPathUnit&& operStateUnit) {
        handleFsdbUpdate(std::move(operStateUnit));
      },
      getServerOptions(localIp_.str(), remoteIp_.str()));
  XLOG(DBG2) << kDsfCtrlLogPrefix
             << "added subscription for : " << remoteEndpointStr();
}

DsfSubscription::~DsfSubscription() {
  if (getStreamState() != fsdb::FsdbStreamClient::State::CONNECTED) {
    // Subscription was not established - decrement failedDSF counter.
    sw_->stats()->failedDsfSubscription(remoteNodeName_, -1);
  }
  fsdbPubSubMgr_->removeStatePathSubscription(
      getAllSubscribePaths(localNodeName_, localIp_), remoteIp_.str());
  XLOG(DBG2) << kDsfCtrlLogPrefix
             << "removed subscription for : " << remoteEndpointStr();
}

std::string DsfSubscription::remoteEndpointStr() const {
  static const std::string kRemoteEndpoint =
      remoteNodeName_ + "_" + remoteIp_.str();
  return kRemoteEndpoint;
}
fsdb::FsdbStreamClient::State DsfSubscription::getStreamState() const {
  return fsdbPubSubMgr_->getStatePathSubsriptionState(
      getAllSubscribePaths(localNodeName_, localIp_), remoteIp_.str());
}

const fsdb::SubscriptionInfo DsfSubscription::getSubscriptionInfo() const {
  // Since we own our own pub sub mgr, there should always be exactly one
  // subscription
  return fsdbPubSubMgr_->getSubscriptionInfo()[0];
}

std::string DsfSubscription::makeRemoteEndpoint(
    const std::string& remoteNode,
    const folly::IPAddress& remoteIP) {
  return folly::sformat("{}::{}", remoteNode, remoteIP.str());
}

void DsfSubscription::handleFsdbSubscriptionStateUpdate(
    fsdb::SubscriptionState oldState,
    fsdb::SubscriptionState newState) {
  auto remoteEndpoint = makeRemoteEndpoint(remoteNodeName_, remoteIp_);
  XLOG(DBG2) << "DsfSubscriber: " << remoteEndpoint
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
      sw_->stats()->failedDsfSubscription(remoteNodeName_, -1);
    } else {
      sw_->stats()->failedDsfSubscription(remoteNodeName_, 1);
    }

    sw_->updateDsfSubscriberState(
        remoteEndpoint, oldThriftState, newThriftState);
    session_.localSubStateChanged(newThriftState);
  }

  if (fsdb::isGRHoldExpired(newState)) {
    processGRHoldTimerExpired();
  }
}

void DsfSubscription::handleFsdbUpdate(fsdb::OperSubPathUnit&& operStateUnit) {
  DsfUpdate dsfUpdate;
  for (const auto& change : *operStateUnit.changes()) {
    if (getSystemPortsPath().matchesPath(*change.path()->path())) {
      XLOG(DBG2) << "Got sys port update from : " << remoteNodeName_;
      MultiSwitchSystemPortMap mswitchSysPorts;
      mswitchSysPorts.fromThrift(thrift_cow::deserialize<
                                 MultiSwitchSystemPortMapTypeClass,
                                 MultiSwitchSystemPortMapThriftType>(
          fsdb::OperProtocol::BINARY, *change.state()->contents()));
      for (const auto& [id, sysPortMap] : mswitchSysPorts) {
        auto matcher = HwSwitchMatcher(id);
        dsfUpdate.switchId2SystemPorts[matcher.switchId()] = sysPortMap;
      }
    } else if (getInterfacesPath().matchesPath(*change.path()->path())) {
      XLOG(DBG2) << "Got rif update from : " << remoteNodeName_;
      MultiSwitchInterfaceMap mswitchIntfs;
      mswitchIntfs.fromThrift(thrift_cow::deserialize<
                              MultiSwitchInterfaceMapTypeClass,
                              MultiSwitchInterfaceMapThriftType>(
          fsdb::OperProtocol::BINARY, *change.state()->contents()));
      for (const auto& [id, intfMap] : mswitchIntfs) {
        auto matcher = HwSwitchMatcher(id);
        dsfUpdate.switchId2Intfs[matcher.switchId()] = intfMap;
      }
    } else if (getDsfSubscriptionsPath(
                   makeRemoteEndpoint(localNodeName_, localIp_))
                   .matchesPath(*change.path()->path())) {
      XLOG(DBG2) << "Got dsf sub update from : " << remoteNodeName_;

      using targetType = fsdb::FsdbSubscriptionState;
      using targetTypeClass = apache::thrift::type_class::enumeration;

      auto newRemoteState =
          thrift_cow::deserialize<targetTypeClass, targetType>(
              fsdb::OperProtocol::BINARY, *change.state()->contents());
      session_.remoteSubStateChanged(newRemoteState);
    } else {
      throw FbossError(
          "Got unexpected state update for : ",
          folly::join("/", *change.path()->path()),
          " from node: ",
          remoteNodeName_);
    }
  }
  bool needsScheduling = false;
  {
    auto nextDsfUpdateWlock = nextDsfUpdate_.wlock();
    // If nextDsfUpdate is not empty, then just overwrite
    // nextDsfUpdate with latest info but don't schedule
    // another lambda on the hwUpdateThread. The idea here
    // is that since nextDsfUpdate_ was not empty it implies
    // we have already scheduled a update on the hwUpdateEvb
    // and that update will now simply consume the latest
    // contents.
    needsScheduling = nextDsfUpdateWlock->isEmpty();
    *nextDsfUpdateWlock = std::move(dsfUpdate);
  }
  /*
   * Schedule updates async on hwUpdateEvb, so we don't
   * keep the streamEventEvb blocked waiting on HW updates.
   * Doing everything on streamEvb slows down convergence
   * when multiple session have simultaneous updates.
   * This is most acutely felt during bootup, where several
   * hundred sessions come up close together and wait on
   * each other for initial sync to complete.
   */
  if (needsScheduling) {
    hwUpdateEvb_->runInEventBaseThread([this]() {
      DsfUpdate update;
      {
        auto nextDsfUpdateWlock = nextDsfUpdate_.wlock();
        if (nextDsfUpdateWlock->isEmpty()) {
          // Update was already done or cancelled
          return;
        }
        update = std::move(*nextDsfUpdateWlock);
        // At this point nextDsfUpdate should be empty
        CHECK(nextDsfUpdateWlock->isEmpty());
      }
      try {
        updateWithRollbackProtection(
            update.switchId2SystemPorts, update.switchId2Intfs);

      } catch (std::exception& e) {
        // TODO handle failures
        XLOG(FATAL) << " update failed: " << e.what();
      }
    });
  }
}

bool DsfSubscription::isLocal(SwitchID nodeSwitchId) const {
  auto localSwitchIds = sw_->getSwitchInfoTable().getSwitchIDs();
  return localSwitchIds.find(nodeSwitchId) != localSwitchIds.end();
}

void DsfSubscription::updateWithRollbackProtection(
    const std::map<SwitchID, std::shared_ptr<SystemPortMap>>&
        switchId2SystemPorts,
    const std::map<SwitchID, std::shared_ptr<InterfaceMap>>& switchId2Intfs) {
  auto hasNoLocalSwitchId = [this](const auto& switchId2Objects) {
    for (const auto& [switchId, _] : switchId2Objects) {
      if (this->isLocal(switchId)) {
        throw FbossError(
            "Got updates for a local switch ID, from: ",
            localNodeName_,
            " id: ",
            switchId);
      }
    }
  };

  hasNoLocalSwitchId(switchId2SystemPorts);
  hasNoLocalSwitchId(switchId2Intfs);

  auto updateDsfStateFn = [this, switchId2SystemPorts, switchId2Intfs](
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

  sw_->getRib()->updateStateInRibThread([this, updateDsfStateFn]() {
    sw_->updateStateWithHwFailureProtection(
        folly::sformat("Update state for node: {}", localNodeName_),
        updateDsfStateFn);
  });
}

void DsfSubscription::processGRHoldTimerExpired() {
  sw_->stats()->dsfSessionGrExpired();
  XLOG(DBG2) << kDsfCtrlLogPrefix << "GR expired for : " << remoteEndpointStr();
  auto updateDsfStateFn = [this](const std::shared_ptr<SwitchState>& in) {
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
        if (remoteNodeSwitchIds_.count(remoteSystemPort->getSwitchId()) > 0 &&
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
          if (remoteNodeSwitchIds_.count(switchID) > 0 &&
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

  sw_->getRib()->updateStateInRibThread([this, updateDsfStateFn]() {
    sw_->updateStateWithHwFailureProtection(
        folly::sformat("Update state for node: {}", remoteNodeName_),
        updateDsfStateFn);
  });
}
} // namespace facebook::fboss
