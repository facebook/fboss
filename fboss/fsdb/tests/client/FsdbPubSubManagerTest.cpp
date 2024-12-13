// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include "fboss/fsdb/client/FsdbPubSubManager.h"
#include <fboss/fsdb/client/FsdbStreamClient.h>
#include <gtest/gtest.h>
#include "fboss/fsdb/oper/ExtendedPathBuilder.h"
#include "fboss/fsdb/tests/client/FsdbTestClients.h"
#include "fboss/fsdb/tests/utils/FsdbTestServer.h"
#include "fboss/lib/CommonUtils.h"
#include "fboss/lib/thrift_service_client/ConnectionOptions.h"

#include <folly/io/async/ScopedEventBaseThread.h>
#include <folly/synchronization/Baton.h>
#include <thread>

namespace facebook::fboss::fsdb::test {
namespace {
bool isConnected(FsdbStreamClient::State state) {
  return state == FsdbStreamClient::State::CONNECTED;
}
constexpr auto kPublisherId = "fsdb_test_publisher";
const std::vector<std::string> kPublishRoot{"agent"};
const uint32_t kGrHoldTimeInSec = 2;
} // namespace
template <typename TestParam>
class FsdbPubSubManagerTest : public ::testing::Test {
 public:
  void SetUp() override {
    fsdbTestServer_ = std::make_unique<FsdbTestServer>();
    std::vector<std::string> publishRoot;
    pubSubManager_ = std::make_unique<FsdbPubSubManager>(kPublisherId);
  }
  void TearDown() override {
    pubSubManager_.reset();
    // Publisher connections should be removed
    // from server
    checkPublishing(false /*isStats*/, 0);
    checkPublishing(true /*isStats*/, 0);
    WITH_RETRIES({
      EXPECT_EVENTUALLY_EQ(fsdbTestServer_->getActiveSubscriptions().size(), 0);
    });

    fsdbTestServer_.reset();
  }

 protected:
  folly::Synchronized<std::map<std::string, FsdbErrorCode>>
      subscriptionLastDisconnectReason;
  void updateSubscriptionLastDisconnectReason(
      SubscriptionType subscriptionType,
      bool isStats) {
    auto subscriptionInfoList = this->pubSubManager_->getSubscriptionInfo();
    for (const auto& subscriptionInfo : subscriptionInfoList) {
      if (subscriptionType == subscriptionInfo.subscriptionType &&
          isStats == subscriptionInfo.isStats) {
        auto reason = subscriptionInfo.disconnectReason;
        subscriptionLastDisconnectReason.withWLock([&](auto& map) {
          std::string subscriberId = isStats ? "STAT" : "State";
          subscriberId +=
              fmt::format("_{}", subscriptionTypeToStr[subscriptionType]);
          map[subscriberId] = reason;
        });
        return;
      }
    }
  }
  void checkDisconnectReason(FsdbErrorCode expected) {
    WITH_RETRIES({
      auto reasons = subscriptionLastDisconnectReason.rlock();
      for (const auto& [key, value] : *reasons) {
        EXPECT_EVENTUALLY_EQ(value, expected);
      }
    });
  }

  FsdbStreamClient::FsdbStreamStateChangeCb stateChangeCb() {
    return [this](auto /*oldState*/, auto newState) {
      if (isConnected(newState)) {
        connectionSync_.post();
      }
    };
  }
  SubscriptionStateChangeCb subscrStateChangeCb() {
    return [this](
               SubscriptionState /*oldState*/,
               SubscriptionState /*newState*/,
               std::optional<bool> /*initialSyncHasData*/) {};
  }
  template <typename SubUnit>
  SubscriptionStateChangeCb subscrStateChangeCb(
      folly::Synchronized<std::vector<SubUnit>>& subUnits,
      std::optional<std::function<void()>> onDisconnect = std::nullopt) {
    return [this, &onDisconnect, &subUnits](
               SubscriptionState /*oldState*/,
               SubscriptionState newState,
               std::optional<bool> /*initialSyncHasData*/) {
      if (onDisconnect.has_value() && isDisconnected(newState)) {
        onDisconnect.value()();
      }
      if (!isConnected(newState)) {
        subUnits.wlock()->clear();
      }
    };
  }
  template <typename SubUnit>
  FsdbStreamClient::FsdbStreamStateChangeCb stateChangeCb(
      std::function<void()> onDisconnect,
      folly::Synchronized<std::vector<SubUnit>>& subUnits) {
    return [&](auto /*oldState*/, auto newState) {
      if (newState == FsdbStreamClient::State::DISCONNECTED) {
        onDisconnect();
      }
      if (!isConnected(newState)) {
        subUnits.wlock()->clear();
      }
    };
  }

  SubscriptionStateChangeCb subscriptionStateChangeCb() {
    return [this](
               SubscriptionState /*oldState*/,
               SubscriptionState newState,
               std::optional<bool> /*initialSyncHasData*/) {
      auto state = subscriptionState_.wlock();
      *state = newState;
    };
  }
  folly::Synchronized<SubscriptionState> subscriptionState_{
      SubscriptionState::DISCONNECTED};
  SubscriptionState getSubscriptionState() {
    return *subscriptionState_.rlock();
  }

  void createPublishers() {
    // Create both state and stat publishers. Having both in a
    // binary is a common use case.
    if constexpr (std::is_same_v<
                      typename TestParam::PublisherT,
                      FsdbDeltaPublisher>) {
      pubSubManager_->createStatDeltaPublisher(
          kPublishRoot, stateChangeCb(), fsdbTestServer_->getFsdbPort());
    } else if constexpr (std::is_same_v<
                             typename TestParam::PublisherT,
                             FsdbStatePublisher>) {
      pubSubManager_->createStatPathPublisher(
          kPublishRoot, stateChangeCb(), fsdbTestServer_->getFsdbPort());
    } else {
      pubSubManager_->createStatPatchPublisher(
          kPublishRoot, stateChangeCb(), fsdbTestServer_->getFsdbPort());
    }
    connectionSync_.wait();
    connectionSync_.reset();
    if constexpr (std::is_same_v<
                      typename TestParam::PublisherT,
                      FsdbDeltaPublisher>) {
      pubSubManager_->createStateDeltaPublisher(
          kPublishRoot, stateChangeCb(), fsdbTestServer_->getFsdbPort());
    } else if constexpr (std::is_same_v<
                             typename TestParam::PublisherT,
                             FsdbStatePublisher>) {
      pubSubManager_->createStatePathPublisher(
          kPublishRoot, stateChangeCb(), fsdbTestServer_->getFsdbPort());
    } else {
      pubSubManager_->createStatePatchPublisher(
          kPublishRoot, stateChangeCb(), fsdbTestServer_->getFsdbPort());
    }
    connectionSync_.wait();
    connectionSync_.reset();
    checkPublishing(false /*isStats*/);
    checkPublishing(true /*isStats*/);
  }
  void createPublisher(bool isStats, PubSubType type) {
    switch (type) {
      case PubSubType::DELTA:
        if (isStats) {
          pubSubManager_->createStateDeltaPublisher(
              kPublishRoot, stateChangeCb(), fsdbTestServer_->getFsdbPort());
        } else {
          pubSubManager_->createStateDeltaPublisher(
              kPublishRoot, stateChangeCb(), fsdbTestServer_->getFsdbPort());
        }
        break;
      case PubSubType::PATH:
        if (isStats) {
          pubSubManager_->createStatPathPublisher(
              kPublishRoot, stateChangeCb(), fsdbTestServer_->getFsdbPort());
        } else {
          pubSubManager_->createStatePathPublisher(
              kPublishRoot, stateChangeCb(), fsdbTestServer_->getFsdbPort());
        }
        break;
      case PubSubType::PATCH:
        if (isStats) {
          pubSubManager_->createStatPatchPublisher(
              kPublishRoot, stateChangeCb(), fsdbTestServer_->getFsdbPort());
        } else {
          pubSubManager_->createStatePatchPublisher(
              kPublishRoot, stateChangeCb(), fsdbTestServer_->getFsdbPort());
        }
        break;
    }
    connectionSync_.wait();
    connectionSync_.reset();
    checkPublishing(isStats);
  }
  void checkPublishing(bool isStats, int expectCount = 1) {
    WITH_RETRIES({
      auto metadata = fsdbTestServer_->getPublisherRootMetadata(
          *kPublishRoot.begin(), isStats);
      if (expectCount) {
        ASSERT_EVENTUALLY_TRUE(metadata);
        EXPECT_EVENTUALLY_EQ(metadata->numOpenConnections, expectCount);
      } else {
        EXPECT_EVENTUALLY_FALSE(metadata.has_value());
      }
    });
  }

  void publish(const cfg::AgentConfig& cfg) {
    publish(cfg, false);
  }
  void publish(const folly::F14FastMap<std::string, HwPortStats>& stats) {
    publish(stats, true);
  }

  void publishAndVerifyConfig(
      const std::map<std::string, std::string>& cmdLinArgs) {
    auto config = makeAgentConfig(cmdLinArgs);
    publish(config);
    WITH_RETRIES({
      auto fsdbOperRoot = fsdbTestServer_->serviceHandler().operRootExpensive();
      EXPECT_EVENTUALLY_EQ(*fsdbOperRoot.agent()->config(), config);
    });
  }
  void publishAndVerifyStats(int64_t inBytes) {
    auto portStats = makePortStats(inBytes);
    publish(portStats);
    WITH_RETRIES({
      auto fsdbOperRoot =
          fsdbTestServer_->serviceHandler().operStatsRootExpensive();
      EXPECT_EVENTUALLY_EQ(*fsdbOperRoot.agent()->hwPortStats(), portStats);
    });
  }
  bool pubSubStats() const {
    return TestParam::PubSubStats;
  }
  std::vector<std::string> subscriptionPath() const {
    return kPublishRoot;
  }
  std::vector<ExtendedOperPath> extSubscriptionPaths() const {
    using StateRootMembers =
        apache::thrift::reflect_struct<FsdbOperStateRoot>::member;
    using AgentRootMembers = apache::thrift::reflect_struct<AgentData>::member;
    using AgentConfigRootMembers =
        apache::thrift::reflect_struct<cfg::AgentConfig>::member;
    ExtendedOperPath path =
        ext_path_builder::raw(StateRootMembers::agent::id::value)
            .raw(AgentRootMembers::config::id::value)
            .raw(AgentConfigRootMembers::defaultCommandLineArgs::id::value)
            .any()
            .get();
    return {std::move(path)};
  }

  std::string addStatDeltaSubscription(
      FsdbDeltaSubscriber::FsdbOperDeltaUpdateCb operDeltaUpdate,
      SubscriptionStateChangeCb stChangeCb) {
    return pubSubManager_->addStatDeltaSubscription(
        subscriptionPath(),
        stChangeCb,
        operDeltaUpdate,
        utils::ConnectionOptions("::1", fsdbTestServer_->getFsdbPort()));
  }
  std::string addStateDeltaSubscription(
      FsdbDeltaSubscriber::FsdbOperDeltaUpdateCb operDeltaUpdate,
      SubscriptionStateChangeCb stChangeCb) {
    return pubSubManager_->addStateDeltaSubscription(
        subscriptionPath(),
        stChangeCb,
        operDeltaUpdate,
        utils::ConnectionOptions("::1", fsdbTestServer_->getFsdbPort()));
  }
  void addSubscriptions(
      FsdbDeltaSubscriber::FsdbOperDeltaUpdateCb operDeltaUpdate) {
    addStatDeltaSubscription(operDeltaUpdate, subscrStateChangeCb());
    addStateDeltaSubscription(operDeltaUpdate, subscrStateChangeCb());
  }
  std::string addStatPathSubscription(
      FsdbStateSubscriber::FsdbOperStateUpdateCb operPathUpdate,
      SubscriptionStateChangeCb stChangeCb) {
    return pubSubManager_->addStatPathSubscription(
        subscriptionPath(),
        stChangeCb,
        operPathUpdate,
        utils::ConnectionOptions("::1", fsdbTestServer_->getFsdbPort()));
  }
  std::string addStatePathSubscription(
      FsdbStateSubscriber::FsdbOperStateUpdateCb operPathUpdate,
      SubscriptionStateChangeCb stChangeCb) {
    return pubSubManager_->addStatePathSubscription(
        subscriptionPath(),
        stChangeCb,
        operPathUpdate,
        utils::ConnectionOptions("::1", fsdbTestServer_->getFsdbPort()));
  }
  std::string addStatePatchSubscription(
      const std::vector<std::string> path,
      FsdbPatchSubscriber::FsdbOperPatchUpdateCb operPatchUpdate,
      SubscriptionStateChangeCb stChangeCb) {
    fsdb::RawOperPath p;
    p.path() = path;
    return pubSubManager_->addStatePatchSubscription(
        {{42, p}},
        stChangeCb,
        operPatchUpdate,
        utils::ConnectionOptions("::1", fsdbTestServer_->getFsdbPort()));
  }
  void addStatePathSubscriptionWithGrHoldTime(
      FsdbStateSubscriber::FsdbOperStateUpdateCb operPathUpdate,
      SubscriptionStateChangeCb stChangeCb,
      uint32_t grHoldTimeSec) {
    auto subscribeStats = false;
    utils::ConnectionOptions connOpts{"::1", fsdbTestServer_->getFsdbPort()};
    SubscriptionOptions opts{
        pubSubManager_->getClientId(), subscribeStats, grHoldTimeSec};
    pubSubManager_->addStatePathSubscription(
        std::move(opts),
        subscriptionPath(),
        stChangeCb,
        operPathUpdate,
        std::move(connOpts));
  }
  std::string addStateExtDeltaSubscription(
      FsdbExtDeltaSubscriber::FsdbOperDeltaUpdateCb operDeltaCb,
      SubscriptionStateChangeCb stChangeCb) {
    utils::ConnectionOptions connOpts{"::1", fsdbTestServer_->getFsdbPort()};
    return pubSubManager_->addStateExtDeltaSubscription(
        extSubscriptionPaths(), stChangeCb, operDeltaCb, std::move(connOpts));
  }
  void addSubscriptions(
      FsdbStateSubscriber::FsdbOperStateUpdateCb operPathUpdate) {
    addStatPathSubscription(operPathUpdate, subscrStateChangeCb());
    addStatePathSubscription(operPathUpdate, subscrStateChangeCb());
  }
  FsdbDeltaSubscriber::FsdbOperDeltaUpdateCb makeOperDeltaCb(
      folly::Synchronized<std::vector<OperDelta>>& deltas) {
    return [&deltas](OperDelta&& delta) { deltas.wlock()->push_back(delta); };
  }
  FsdbStateSubscriber::FsdbOperStateUpdateCb makeOperStateCb(
      folly::Synchronized<std::vector<OperState>>& states) {
    return [&states](OperState&& state) { states.wlock()->push_back(state); };
  }
  FsdbExtDeltaSubscriber::FsdbOperDeltaUpdateCb makeExtOperDeltaCb(
      folly::Synchronized<std::vector<OperSubDeltaUnit>>& deltas) {
    return [&deltas](OperSubDeltaUnit&& delta) {
      deltas.wlock()->push_back(delta);
    };
  }
  FsdbPatchSubscriber::FsdbOperPatchUpdateCb makePatchUpdateCb(
      folly::Synchronized<std::vector<SubscriberChunk>>& patches) {
    return [&patches](SubscriberChunk&& patch) {
      patches.wlock()->push_back(patch);
    };
  }

 private:
  template <typename PubT>
  void publish(const PubT& toPublish, bool isStat) {
    if constexpr (std::is_same_v<typename TestParam::PubUnitT, OperDelta>) {
      auto delta = makeDelta(toPublish);
      isStat ? pubSubManager_->publishStat(std::move(delta))
             : pubSubManager_->publishState(std::move(delta));
    } else if constexpr (std::is_same_v<
                             typename TestParam::PubUnitT,
                             OperState>) {
      auto state = makeState(toPublish);
      isStat ? pubSubManager_->publishStat(std::move(state))
             : pubSubManager_->publishState(std::move(state));
    } else {
      auto patch = makePatch(toPublish);
      isStat ? pubSubManager_->publishStat(std::move(patch))
             : pubSubManager_->publishState(std::move(patch));
    }
  }

 protected:
  template <typename SubUnit>
  void assertQueue(
      folly::Synchronized<std::vector<SubUnit>>& queue,
      int expectedSize) const {
    WITH_RETRIES_N(kRetries, {
      ASSERT_EVENTUALLY_EQ(queue.rlock()->size(), expectedSize);
      // Copy the queue, to guard against it changing during iteration
      auto q = *queue.rlock();
      for (const auto& unit : q) {
        ASSERT_GT(*unit.metadata()->lastConfirmedAt(), 0);
      }
    });
  }
  static auto constexpr kRetries = 60;
  std::unique_ptr<FsdbTestServer> fsdbTestServer_;
  std::unique_ptr<FsdbPubSubManager> pubSubManager_;
  folly::Baton<> connectionSync_;
};

using TestTypes = ::testing::Types<
    DeltaPubSubForState,
    StatePubSubForState,
    DeltaPubSubForStats,
    StatePubSubForStats>;

TYPED_TEST_SUITE(FsdbPubSubManagerTest, TestTypes);

TYPED_TEST(FsdbPubSubManagerTest, pubSub) {
  folly::Synchronized<std::vector<OperDelta>> deltas;
  this->createPublishers();
  this->addSubscriptions(this->makeOperDeltaCb(deltas));
  // Initial sync only after first publish
  this->publishAndVerifyStats(1);
  this->publishAndVerifyConfig({{"foo", "bar"}});
  this->assertQueue(deltas, 2);
}

TYPED_TEST(FsdbPubSubManagerTest, publishMultipleSubscribers) {
  this->createPublishers();
  folly::Synchronized<std::vector<OperDelta>> deltas;
  folly::Synchronized<std::vector<OperState>> states;
  this->addSubscriptions(this->makeOperDeltaCb(deltas));
  this->addSubscriptions(this->makeOperStateCb(states));
  // Publish
  this->publish(makePortStats(1));
  this->publish(makeAgentConfig({{"foo", "bar"}}));
  // Initial sync only after first publish
  this->assertQueue(deltas, 2);
  this->assertQueue(states, 2);
}

TYPED_TEST(FsdbPubSubManagerTest, publisherDropCausesSubscriberReset) {
  this->createPublishers();
  folly::Synchronized<std::vector<OperDelta>> statDeltas, stateDeltas;
  folly::Synchronized<std::vector<OperState>> statPaths, statePaths;
  this->addStatDeltaSubscription(
      this->makeOperDeltaCb(statDeltas), this->subscrStateChangeCb(statDeltas));
  this->addStateDeltaSubscription(
      this->makeOperDeltaCb(stateDeltas),
      this->subscrStateChangeCb(stateDeltas));
  this->addStatPathSubscription(
      this->makeOperStateCb(statPaths), this->subscrStateChangeCb(statPaths));
  this->addStatePathSubscription(
      this->makeOperStateCb(statePaths), this->subscrStateChangeCb(statePaths));
  // Publish
  this->publish(makePortStats(1));
  this->assertQueue(statDeltas, 1);
  this->assertQueue(statPaths, 1);
  this->publish(makeAgentConfig({{"foo", "bar"}}));
  this->assertQueue(stateDeltas, 1);
  this->assertQueue(statePaths, 1);
  // Publisher resets should be noticed by subscribers
  this->pubSubManager_->removeStatDeltaPublisher();
  this->pubSubManager_->removeStatPathPublisher();
  this->assertQueue(statDeltas, 0);
  this->assertQueue(statPaths, 0);
  // State subscriptions remain as is
  this->assertQueue(stateDeltas, 1);
  this->assertQueue(statePaths, 1);
  this->pubSubManager_->removeStateDeltaPublisher();
  this->pubSubManager_->removeStatePathPublisher();
  this->assertQueue(stateDeltas, 0);
  this->assertQueue(statePaths, 0);
  // Reset pubsub manager while local vector objects are in scope, since
  // we reference them in state change callbacks
  this->pubSubManager_.reset();
}

TYPED_TEST(FsdbPubSubManagerTest, publishMultipleSubscribersPruneSome) {
  this->createPublishers();
  folly::Synchronized<std::vector<OperDelta>> deltas;
  folly::Synchronized<std::vector<OperState>> states;
  this->addSubscriptions(this->makeOperDeltaCb(deltas));
  this->addSubscriptions(this->makeOperStateCb(states));
  // Publish
  this->publish(makePortStats(1));
  this->publish(makeAgentConfig({{"foo", "bar"}}));
  // Initial sync only after first publish
  this->assertQueue(deltas, 2);
  this->assertQueue(states, 2);
  this->pubSubManager_->removeStatDeltaSubscription(
      this->subscriptionPath(), "::1");
  this->pubSubManager_->removeStateDeltaSubscription(
      this->subscriptionPath(), "::1");
  this->publish(makePortStats(2));
  this->publish(makeAgentConfig({{"bar", "baz"}}));
  this->assertQueue(deltas, 2);
  this->assertQueue(states, 4);
}

TYPED_TEST(FsdbPubSubManagerTest, subscriberAppError) {
  this->createPublishers();
  folly::Synchronized<std::vector<OperDelta>> statDeltas, stateDeltas;
  folly::Synchronized<std::vector<OperState>> statPaths, statePaths;
  std::map<std::tuple<bool, std::string>, std::string> subKeys;
  std::map<std::string, bool> exceptionThrown;

  exceptionThrown["StatDelta"] = false;
  subKeys[std::make_tuple(true, "StatDelta")] = this->addStatDeltaSubscription(
      [&exceptionThrown](OperDelta&&) {
        if (!exceptionThrown["StatDelta"]) {
          exceptionThrown["StatDelta"] = true;
          throw std::runtime_error("test app exception");
        }
      },
      this->subscrStateChangeCb(statDeltas));
  exceptionThrown["StateDelta"] = false;
  subKeys[std::make_tuple(false, "StateDelta")] =
      this->addStateDeltaSubscription(
          [&exceptionThrown](OperDelta&&) {
            if (!exceptionThrown["StateDelta"]) {
              exceptionThrown["StateDelta"] = true;
              throw std::runtime_error("test app exception");
            }
          },
          this->subscrStateChangeCb(stateDeltas));
  exceptionThrown["StatPath"] = false;
  subKeys[std::make_tuple(true, "StatPath")] = this->addStatPathSubscription(
      [&exceptionThrown](OperState&&) {
        if (!exceptionThrown["StatPath"]) {
          exceptionThrown["StatPath"] = true;
          throw std::runtime_error("test app exception");
        }
      },
      this->subscrStateChangeCb(statPaths));
  exceptionThrown["StatePath"] = false;
  subKeys[std::make_tuple(false, "StatePath")] = this->addStatePathSubscription(
      [&exceptionThrown](OperState&&) {
        if (!exceptionThrown["StatePath"]) {
          exceptionThrown["StatePath"] = true;
          throw std::runtime_error("test app exception");
        }
      },
      this->subscrStateChangeCb(statePaths));

  // check: no dataCbErrors till publish
  WITH_RETRIES_N(this->kRetries, {
    // In tests, we don't start the publisher threads
    fb303::ThreadCachedServiceData::get()->publishStats();
    for (auto [sub, key] : subKeys) {
      auto counterPrefix =
          this->pubSubManager_->getSubscriberStatsPrefix(std::get<0>(sub), key);
      EXPECT_EVENTUALLY_EQ(
          fb303::ServiceData::get()->getCounter(
              counterPrefix + ".disconnectReason.dataCbError.sum.60"),
          0);
    }
  });

  // Publish
  this->publish(makePortStats(1));
  this->publish(makeAgentConfig({{"foo", "bar"}}));

  // check: disconnectReason.dataCbError for all subscription types
  WITH_RETRIES_N(this->kRetries, {
    // In tests, we don't start the publisher threads
    fb303::ThreadCachedServiceData::get()->publishStats();
    for (auto [sub, key] : subKeys) {
      auto counterPrefix =
          this->pubSubManager_->getSubscriberStatsPrefix(std::get<0>(sub), key);
      EXPECT_EVENTUALLY_EQ(
          fb303::ServiceData::get()->getCounter(
              counterPrefix + ".disconnectReason.dataCbError.sum.60"),
          1);
    }
  });

  // Reset pubsub manager while local vector objects are in scope, since
  // we reference them in state change callbacks
  this->pubSubManager_.reset();
}

template <typename TestParam>
class FsdbPubSubManagerGRTest : public FsdbPubSubManagerTest<TestParam> {};

using GRTestTypes = ::testing::Types<
    DeltaPubSubForState,
    StatePubSubForState,
    DeltaPubSubForStats,
    StatePubSubForStats>;

TYPED_TEST_SUITE(FsdbPubSubManagerGRTest, GRTestTypes);

TYPED_TEST(FsdbPubSubManagerGRTest, verifySubscriptionDisconnectOnPublisherGR) {
  this->createPublishers();
  folly::Synchronized<std::vector<OperDelta>> statDeltas, stateDeltas;
  folly::Synchronized<std::vector<OperState>> statPaths, statePaths;
  this->addStatDeltaSubscription(
      this->makeOperDeltaCb(statDeltas),
      this->subscrStateChangeCb(stateDeltas, [this]() {
        this->updateSubscriptionLastDisconnectReason(
            SubscriptionType::DELTA, true);
      }));
  this->addStatPathSubscription(
      this->makeOperStateCb(statPaths),
      this->subscrStateChangeCb(stateDeltas, [this]() {
        this->updateSubscriptionLastDisconnectReason(
            SubscriptionType::PATH, true);
      }));
  this->addStateDeltaSubscription(
      this->makeOperDeltaCb(stateDeltas),
      this->subscrStateChangeCb(stateDeltas, [this]() {
        this->updateSubscriptionLastDisconnectReason(
            SubscriptionType::DELTA, false);
      }));
  SubscriptionStateChangeCb stChangeCb =
      this->subscrStateChangeCb(statePaths, [this]() {
        this->updateSubscriptionLastDisconnectReason(
            SubscriptionType::PATH, false);
      });
  this->addStatePathSubscription(this->makeOperStateCb(statePaths), stChangeCb);
  // Publish
  this->publish(makePortStats(1));
  this->assertQueue(statDeltas, 1);
  this->assertQueue(statPaths, 1);
  this->publish(makeAgentConfig({{"foo", "bar"}}));
  this->assertQueue(stateDeltas, 1);
  this->assertQueue(statePaths, 1);
  // verify Publisher disconnect for GR is noticed by subscribers
  this->pubSubManager_->removeStatDeltaPublisher(true);
  this->pubSubManager_->removeStatPathPublisher(true);
  this->pubSubManager_->removeStateDeltaPublisher(true);
  this->pubSubManager_->removeStatePathPublisher(true);
  this->checkDisconnectReason(FsdbErrorCode::PUBLISHER_GR_DISCONNECT);
  // reconnect publishers, and disconnect without GR
  this->createPublishers();
  this->publish(makeAgentConfig({{"foo", "bar"}}));
  this->publish(makePortStats(1));
  this->pubSubManager_->removeStatDeltaPublisher();
  this->pubSubManager_->removeStatPathPublisher();
  this->pubSubManager_->removeStateDeltaPublisher();
  this->pubSubManager_->removeStatePathPublisher();
  this->checkDisconnectReason(FsdbErrorCode::ALL_PUBLISHERS_GONE);
  // Reset pubsub manager while local vector objects are in scope,
  // since we reference them in state change callbacks
  this->pubSubManager_.reset();
}

template <typename TestParam>
class FsdbPubSubManagerGRHoldTest : public FsdbPubSubManagerTest<TestParam> {};

using GRHoldTestTypes = ::testing::Types<DeltaPubSubForState>;

TYPED_TEST_SUITE(FsdbPubSubManagerGRHoldTest, GRHoldTestTypes);

TYPED_TEST(FsdbPubSubManagerGRHoldTest, verifySubscriptionDisconnect) {
  folly::Synchronized<std::vector<OperState>> statePaths;
  this->createPublisher(false, PubSubType::DELTA);
  this->addStatePathSubscriptionWithGrHoldTime(
      this->makeOperStateCb(statePaths), this->subscriptionStateChangeCb(), 0);
  // Publish
  this->publish(makeAgentConfig({{"foo", "bar"}}));
  WITH_RETRIES({
    EXPECT_EVENTUALLY_EQ(
        this->getSubscriptionState(), SubscriptionState::CONNECTED);
  });
  this->assertQueue(statePaths, 1);
  // verify Publisher disconnect for GR is noticed by subscribers
  this->pubSubManager_->removeStateDeltaPublisher(true);
  WITH_RETRIES({
    EXPECT_EVENTUALLY_EQ(
        this->getSubscriptionState(), SubscriptionState::DISCONNECTED);
  });

  // Reset pubsub manager while local vector objects are in scope,
  // since we reference them in state change callbacks
  this->pubSubManager_.reset();
}

TYPED_TEST(FsdbPubSubManagerGRHoldTest, verifyGRHoldTimeExpiry) {
  folly::Synchronized<std::vector<OperState>> statePaths;
  this->createPublisher(false, PubSubType::DELTA);
  this->addStatePathSubscriptionWithGrHoldTime(
      this->makeOperStateCb(statePaths),
      this->subscriptionStateChangeCb(),
      kGrHoldTimeInSec);
  // Publish
  this->publish(makeAgentConfig({{"foo", "bar"}}));
  this->assertQueue(statePaths, 1);
  // verify GR hold time expiry
  this->pubSubManager_->removeStateDeltaPublisher(true);
  WITH_RETRIES({
    EXPECT_EVENTUALLY_EQ(
        this->getSubscriptionState(),
        SubscriptionState::DISCONNECTED_GR_HOLD_EXPIRED);
  });

  // Reset pubsub manager while local vector objects are in scope,
  // since we reference them in state change callbacks
  this->pubSubManager_.reset();
}

TYPED_TEST(FsdbPubSubManagerGRHoldTest, verifyResyncWithinGRHoldTime) {
  folly::Synchronized<std::vector<OperState>> statePaths;
  this->createPublisher(false, PubSubType::DELTA);
  this->addStatePathSubscriptionWithGrHoldTime(
      this->makeOperStateCb(statePaths),
      this->subscriptionStateChangeCb(),
      kGrHoldTimeInSec);
  this->publish(makeAgentConfig({{"foo", "bar"}}));
  this->assertQueue(statePaths, 1);
  this->pubSubManager_->removeStateDeltaPublisher(true);
  WITH_RETRIES({
    EXPECT_EVENTUALLY_EQ(
        this->getSubscriptionState(), SubscriptionState::DISCONNECTED_GR_HOLD);
  });
  // reconnect publisher, and verify publisher reconnect within GR hold time
  this->createPublisher(false, PubSubType::DELTA);
  this->publish(makeAgentConfig({{"foo", "bar"}}));
  // verify subscriber state stays connected, and never disconnected
  WITH_RETRIES({
    EXPECT_EVENTUALLY_EQ(
        this->getSubscriptionState(), SubscriptionState::CONNECTED);
  });

  // Reset pubsub manager while local vector objects are in scope,
  // since we reference them in state change callbacks
  this->pubSubManager_.reset();
}

TYPED_TEST(FsdbPubSubManagerGRHoldTest, verifyRHoldTimeExpiryOnInitialConnect) {
  folly::Synchronized<std::vector<OperState>> statePaths;
  // create subscription that will not receive initial sync
  this->addStatePathSubscriptionWithGrHoldTime(
      this->makeOperStateCb(statePaths),
      this->subscriptionStateChangeCb(),
      kGrHoldTimeInSec);
  // verify GR hold time expiry
  WITH_RETRIES({
    EXPECT_EVENTUALLY_EQ(
        this->getSubscriptionState(),
        SubscriptionState::DISCONNECTED_GR_HOLD_EXPIRED);
  });

  // Reset pubsub manager while local vector objects are in scope,
  // since we reference them in state change callbacks
  this->pubSubManager_.reset();
}

TYPED_TEST(FsdbPubSubManagerTest, pubSubExtDelta) {
  folly::Synchronized<std::vector<OperSubDeltaUnit>> deltas;
  this->createPublishers();
  this->addStateExtDeltaSubscription(
      this->makeExtOperDeltaCb(deltas), this->subscrStateChangeCb());
  // Initial sync only after first publish
  this->publishAndVerifyConfig({{"foo", "bar"}});
  WITH_RETRIES(ASSERT_EVENTUALLY_EQ(deltas.rlock()->size(), 1));
}

template <typename TestParam>
class FsdbPubSubManagerPatchApiTest : public FsdbPubSubManagerTest<TestParam> {
};

using PatchApiTestTypes = ::testing::Types<PatchPubSubForState>;

TYPED_TEST_SUITE(FsdbPubSubManagerPatchApiTest, PatchApiTestTypes);

TYPED_TEST(FsdbPubSubManagerPatchApiTest, verifyEmptyInitialResponse) {
  folly::Synchronized<std::vector<SubscriberChunk>> received;
  bool initialResponseReceived = false;
  bool isDataExpected;
  auto streamStateCb = [&](SubscriptionState /*oldState*/,
                           SubscriptionState newState,
                           std::optional<bool> initialSyncHasData) {
    if (newState == SubscriptionState::CONNECTED) {
      EXPECT_TRUE(initialSyncHasData.has_value());
      EXPECT_EQ(initialSyncHasData.value(), isDataExpected);
      initialResponseReceived = true;
    }
  };
  this->createPublishers();
  this->publishAndVerifyConfig({{"foo", "bar"}});

  // subscribe for path that is not published
  isDataExpected = false;
  this->addStatePatchSubscription(
      {"agent", "fsdbSubscriptions", "foo"},
      this->makePatchUpdateCb(received),
      streamStateCb);
  WITH_RETRIES_N(
      this->kRetries, { EXPECT_EVENTUALLY_TRUE(initialResponseReceived); });
  EXPECT_EQ(received.rlock()->size(), 0);

  // Subscribe for published path
  isDataExpected = true;
  this->pubSubManager_->removeStatePatchSubscription(
      this->subscriptionPath(), "::1");
  initialResponseReceived = false;
  this->addStatePatchSubscription(
      kPublishRoot, this->makePatchUpdateCb(received), streamStateCb);
  WITH_RETRIES_N(
      this->kRetries, { EXPECT_EVENTUALLY_TRUE(initialResponseReceived); });
  // check for initial chunk
  WITH_RETRIES_N(
      this->kRetries, { ASSERT_EVENTUALLY_EQ(received.rlock()->size(), 1); });
}

} // namespace facebook::fboss::fsdb::test
