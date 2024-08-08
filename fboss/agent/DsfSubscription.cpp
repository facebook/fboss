// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/DsfSubscription.h"
#include "fboss/agent/AgentFeatures.h"
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
} // namespace

namespace facebook::fboss {

DsfSubscription::DsfSubscription(
    fsdb::SubscriptionOptions options,
    folly::EventBase* reconnectEvb,
    folly::EventBase* subscriberEvb,
    folly::EventBase* hwUpdateEvb,
    std::string localNodeName,
    std::string remoteNodeName,
    folly::IPAddress localIp,
    folly::IPAddress remoteIp,
    SwSwitch* sw,
    GrHoldExpiredCb grHoldExpiredCb,
    StateUpdateCb stateUpdateCb)
    : opts_(std::move(options)),
      hwUpdateEvb_(hwUpdateEvb),
      fsdbPubSubMgr_(new fsdb::FsdbPubSubManager(
          opts_.clientId_,
          reconnectEvb,
          subscriberEvb)),
      localNodeName_(std::move(localNodeName)),
      remoteNodeName_(std::move(remoteNodeName)),
      localIp_(std::move(localIp)),
      remoteIp_(std::move(remoteIp)),
      sw_(sw),
      grHoldExpiredCb_(std::move(grHoldExpiredCb)),
      stateUpdateCb_(std::move(stateUpdateCb)),
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
}

DsfSubscription::~DsfSubscription() {
  if (getStreamState() != fsdb::FsdbStreamClient::State::CONNECTED) {
    // Subscription was not established - decrement failedDSF counter.
    sw_->stats()->failedDsfSubscription(remoteNodeName_, -1);
  }
  fsdbPubSubMgr_->removeStatePathSubscription(
      getAllSubscribePaths(localNodeName_, localIp_), remoteIp_.str());
}

fsdb::FsdbStreamClient::State DsfSubscription::getStreamState() const {
  return fsdbPubSubMgr_->getStatePathSubsriptionState(
      getAllSubscribePaths(localNodeName_, localIp_), remoteIp_.str());
}

const fsdb::FsdbPubSubManager::SubscriptionInfo
DsfSubscription::getSubscriptionInfo() const {
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

    dsfSubscriberStateCb_(oldThriftState, newThriftState);
    session_.localSubStateChanged(newThriftState);
  }

  if (fsdb::isGRHoldExpired(newState)) {
    grHoldExpiredCb_();
  }
}

void DsfSubscription::handleFsdbUpdate(fsdb::OperSubPathUnit&& operStateUnit) {
  std::map<SwitchID, std::shared_ptr<SystemPortMap>> switchId2SystemPorts;
  std::map<SwitchID, std::shared_ptr<InterfaceMap>> switchId2Intfs;

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
        switchId2SystemPorts[matcher.switchId()] = sysPortMap;
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
        switchId2Intfs[matcher.switchId()] = intfMap;
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
  stateUpdateCb_(switchId2SystemPorts, switchId2Intfs);
}
} // namespace facebook::fboss
