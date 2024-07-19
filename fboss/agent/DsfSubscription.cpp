// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/DsfSubscription.h"
#include "fboss/agent/AgentFeatures.h"
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
    fsdb::FsdbPubSubManager* pubSubManager,
    std::string localNodeName,
    std::string remoteNodeName,
    SwitchID remoteNodeSwitchId,
    folly::IPAddress localIp,
    folly::IPAddress remoteIp,
    SwitchStats* stats,
    DsfSubscriberStateCb dsfSubscriberStateCb,
    GrHoldExpiredCb grHoldExpiredCb,
    StateUpdateCb stateUpdateCb)
    : fsdbPubSubMgr_(std::move(pubSubManager)),
      localNodeName_(std::move(localNodeName)),
      remoteNodeName_(std::move(remoteNodeName)),
      remoteNodeSwitchId_(std::move(remoteNodeSwitchId)),
      localIp_(std::move(localIp)),
      remoteIp_(std::move(remoteIp)),
      stats_(stats),
      dsfSubscriberStateCb_(std::move(dsfSubscriberStateCb)),
      grHoldExpiredCb_(std::move(grHoldExpiredCb)),
      stateUpdateCb_(std::move(stateUpdateCb)),
      session_(makeRemoteEndpoint(remoteNodeName_, remoteIp_)) {
  // Subscription is not established until state becomes CONNECTED
  stats_->failedDsfSubscription(remoteNodeSwitchId_, remoteNodeName_, 1);
  auto remoteIpStr = remoteIp_.str();
  auto subscriberId =
      folly::sformat("{}_{}:agent", localNodeName_, remoteIpStr);
  fsdb::SubscriptionOptions opts{
      subscriberId, false /* subscribeStats */, FLAGS_dsf_gr_hold_time};
  fsdbPubSubMgr_->addStatePathSubscription(
      std::move(opts),
      getAllSubscribePaths(localNodeName_, localIp_),
      [this](
          fsdb::SubscriptionState oldState, fsdb::SubscriptionState newState) {
        handleFsdbSubscriptionStateUpdate(oldState, newState);
      },
      [this](fsdb::OperSubPathUnit&& operStateUnit) {
        handleFsdbUpdate(std::move(operStateUnit));
      },
      getServerOptions(localIp_.str(), remoteIpStr));
}

DsfSubscription::~DsfSubscription() {
  if (getStreamState() != fsdb::FsdbStreamClient::State::CONNECTED) {
    // Subscription was not established - decrement failedDSF counter.
    stats_->failedDsfSubscription(remoteNodeSwitchId_, remoteNodeName_, -1);
  }
  fsdbPubSubMgr_->removeStatePathSubscription(
      getAllSubscribePaths(localNodeName_, localIp_), remoteIp_.str());
}

fsdb::FsdbStreamClient::State DsfSubscription::getStreamState() const {
  return fsdbPubSubMgr_->getStatePathSubsriptionState(
      getAllSubscribePaths(localNodeName_, localIp_), remoteIp_.str());
}

std::string DsfSubscription::makeRemoteEndpoint(
    const std::string& remoteNode,
    const folly::IPAddress& remoteIP) {
  return folly::sformat("{}::{}", remoteNode, remoteIP.str());
}

void DsfSubscription::handleFsdbSubscriptionStateUpdate(
    fsdb::SubscriptionState,
    fsdb::SubscriptionState) {
  // TODO: handle update and forward to caller
}

void DsfSubscription::handleFsdbUpdate(fsdb::OperSubPathUnit&&) {
  // TODO: handle update and forward to caller
}
} // namespace facebook::fboss
