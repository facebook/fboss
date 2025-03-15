// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/fsdb/benchmarks/FsdbBenchmarkTestHelper.h"
#include "fboss/thrift_cow/nodes/Types.h"

namespace {

const thriftpath::RootThriftPath<facebook::fboss::fsdb::FsdbOperStateRoot>
    stateRoot;
const thriftpath::RootThriftPath<facebook::fboss::fsdb::FsdbOperStatsRoot>
    statsRoot;
const std::vector<std::string> kPublishRoot{"agent"};
const uint32_t kStateServeIntervalMs = 50;
const uint32_t kStatsServeIntervalMs = 50;

} // anonymous namespace

namespace facebook::fboss::fsdb::test {
using FsdbOperStateRootMembers =
    apache::thrift::reflect_struct<FsdbOperStateRoot>::member;

void FsdbBenchmarkTestHelper::setup(int32_t numSubscriptions) {
  fsdbTestServer_ = std::make_unique<fsdb::test::FsdbTestServer>(
      0, kStateServeIntervalMs, kStatsServeIntervalMs);
  FLAGS_fsdbPort = fsdbTestServer_->getFsdbPort();
  for (int i = 0; i < numSubscriptions; i++) {
    const std::string clientId = folly::to<std::string>("agent_", i);
    pubsubMgrs_.push_back(std::make_unique<fsdb::FsdbPubSubManager>(clientId));
    std::vector<std::atomic_bool> subs(numSubscriptions);
    for (auto& sub : subs) {
      sub = false;
    }
    subscriptionConnected_ = std::move(subs);
  }
}

void FsdbBenchmarkTestHelper::publishPath(
    const AgentStats& stats,
    uint64_t stamp) {
  fsdb::OperState state;
  state.set_contents(
      apache::thrift::BinarySerializer::serialize<std::string>(stats));
  state.protocol() = fsdb::OperProtocol::BINARY;
  state.metadata() = fsdb::OperMetadata();
  state.metadata()->lastConfirmedAt() = stamp;
  pubsubMgrs_.at(0)->publishStat(std::move(state));
}

void FsdbBenchmarkTestHelper::publishStatePatch(
    const state::SwitchState& state,
    uint64_t stamp) {
  AgentData agentData;
  agentData.switchState() = state;

  auto prevState = std::make_shared<thrift_cow::ThriftStructNode<AgentData>>();
  auto switchState =
      std::make_shared<thrift_cow::ThriftStructNode<AgentData>>(agentData);
  auto basePath = {
      folly::to<std::string>(FsdbOperStateRootMembers::agent::id::value)};

  auto patch =
      thrift_cow::PatchBuilder::build(prevState, switchState, basePath);

  patch.metadata() = fsdb::OperMetadata();
  patch.metadata()->lastConfirmedAt() = stamp;
  pubsubMgrs_.at(0)->publishState(std::move(patch));
}

void FsdbBenchmarkTestHelper::startPublisher(bool stats) {
  auto stateChangeCb = [this](
                           fsdb::FsdbStreamClient::State /* oldState */,
                           fsdb::FsdbStreamClient::State newState) {
    if (newState == fsdb::FsdbStreamClient::State::CONNECTED) {
      publisherConnected_.post();
      readyForPublishing_.store(true);
    } else {
      readyForPublishing_.store(false);
    }
  };

  if (stats) {
    // agent is Path publisher for stats
    pubsubMgrs_.at(0)->createStatPathPublisher(
        getAgentStatsPath(), std::move(stateChangeCb));
  } else {
    // Create patch publisher for agent state since DFS uses patch
    pubsubMgrs_.at(0)->createStatePatchPublisher(
        kPublishRoot, std::move(stateChangeCb));
  }
}

void FsdbBenchmarkTestHelper::waitForPublisherConnected() {
  publisherConnected_.wait();
}

void FsdbBenchmarkTestHelper::stopPublisher(bool gr) {
  readyForPublishing_.store(false);
  pubsubMgrs_.at(0)->removeStatPathPublisher(gr);
}

void FsdbBenchmarkTestHelper::TearDown() {
  fsdbTestServer_.reset();
  if (!pubsubMgrs_.empty()) {
    stopPublisher();
    CHECK(!readyForPublishing_.load());
    for (auto& pubsubMgr : pubsubMgrs_) {
      pubsubMgr.reset();
    }
  }
}

std::optional<fboss::fsdb::FsdbOperTreeMetadata>
FsdbBenchmarkTestHelper::getPublisherRootMetadata(bool isStats) {
  auto metadata =
      fsdbTestServer_->getPublisherRootMetadata(*kPublishRoot.begin(), isStats);
  return metadata;
}

std::vector<std::string> FsdbBenchmarkTestHelper::getAgentStatsPath() {
  return statsRoot.agent().tokens();
}

void FsdbBenchmarkTestHelper::addSubscription(
    const fsdb::FsdbStateSubscriber::FsdbOperStateUpdateCb& stateUpdateCb) {
  auto stateChangeCb = [this](
                           fsdb::SubscriptionState /*oldState*/,
                           fsdb::SubscriptionState newState,
                           std::optional<bool> /*initialSyncHasData*/) {
    if (newState == fsdb::SubscriptionState::CONNECTED) {
      subscriptionConnected_.at(0).store(true);
    } else {
      subscriptionConnected_.at(0).store(false);
    }
  };

  // agent is Path publisher for stats
  pubsubMgrs_.at(0)->addStatPathSubscription(
      getAgentStatsPath(), stateChangeCb, stateUpdateCb);
}

void FsdbBenchmarkTestHelper::addStatePatchSubscription(
    const fsdb::FsdbPatchSubscriber::FsdbOperPatchUpdateCb& stateUpdateCb,
    int32_t subscriberID) {
  auto stateChangeCb = [this, subscriberID](
                           fsdb::SubscriptionState /*oldState*/,
                           fsdb::SubscriptionState newState,
                           std::optional<bool> /*initialSyncHasData*/) {
    if (newState == fsdb::SubscriptionState::CONNECTED) {
      subscriptionConnected_.at(subscriberID).store(true);
    } else {
      subscriptionConnected_.at(subscriberID).store(false);
    }
  };

  fsdb::RawOperPath p;
  p.path() = kPublishRoot;
  // Create patch publisher for agent state since DFS uses patch
  pubsubMgrs_.at(subscriberID)
      ->addStatePatchSubscription(
          {{subscriberID, p}},
          stateChangeCb,
          stateUpdateCb,
          utils::ConnectionOptions("::1", fsdbTestServer_->getFsdbPort()));
}

bool FsdbBenchmarkTestHelper::isSubscriptionConnected(int32_t subscriberId) {
  return subscriptionConnected_.at(subscriberId).load();
}

void FsdbBenchmarkTestHelper::waitForAllSubscribersConnected(
    int32_t numSubscribers) {
  if (numSubscribers > subscriptionConnected_.size()) {
    throw std::runtime_error(
        "Waiting on more subscribers than number of subscribers created");
  }
  for (int i = 0; i < numSubscribers; i++) {
    while (!isSubscriptionConnected(i)) {
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
  }
  return;
}

void FsdbBenchmarkTestHelper::removeSubscription(
    bool stats,
    int32_t subscriberID) {
  if (stats) {
    pubsubMgrs_.at(0)->removeStatPathSubscription(getAgentStatsPath());
  } else {
    pubsubMgrs_.at(subscriberID)
        ->removeStatePatchSubscription(kPublishRoot, "::1");
  }
}

} // namespace facebook::fboss::fsdb::test
