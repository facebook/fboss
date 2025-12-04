// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/fsdb/client/FsdbPatchPublisher.h"
#include "fboss/fsdb/client/instantiations/FsdbCowStateSubManager.h"
#include "fboss/fsdb/client/instantiations/FsdbCowStatsSubManager.h"
#include "fboss/fsdb/common/Utils.h"
#include "fboss/fsdb/oper/ExtendedPathBuilder.h"
#include "fboss/fsdb/tests/utils/FsdbTestServer.h"
#include "fboss/lib/CommonUtils.h"
#include "fboss/lib/thrift_service_client/ConnectionOptions.h"

#include <folly/logging/Init.h>
#include <folly/logging/LogLevel.h>
#include <folly/logging/LoggerDB.h>
#include <folly/logging/xlog.h>
#include <gtest/gtest.h>

FOLLY_INIT_LOGGING_CONFIG("fboss=DBG5; default:async=true");

namespace facebook::fboss::fsdb::test {

class TestAgentPublisher {
 public:
  TestAgentPublisher(bool isStats, utils::ConnectionOptions serverOptions)
      : isStats_(isStats),
        connectionOptions_(std::move(serverOptions)),
        publisherThread_(
            std::make_unique<folly::ScopedEventBaseThread>(
                "test-publisher-stream")),
        reconnectThread_(
            std::make_unique<folly::ScopedEventBaseThread>(
                "test-publisher-reconnect")) {}

  ~TestAgentPublisher() {
    disconnect();
  }

  void connect() {
    folly::Baton b;
    publisher_ = std::unique_ptr<FsdbPatchPublisher>(new FsdbPatchPublisher(
        "agent",
        {"agent"},
        publisherThread_->getEventBase(),
        reconnectThread_->getEventBase(),
        isStats_,
        [&](auto, auto newState) {
          if (newState == FsdbPatchPublisher::State::CONNECTED) {
            b.post();
          }
        }));
    publisher_->setConnectionOptions(connectionOptions_);
    b.wait();
  }

  void disconnect() {
    publisher_.reset();
  }

  template <typename PathT, typename Node = typename PathT::DataT>
  void publish(PathT path, Node data) {
    CHECK(publisher_);
    Patch patch;
    patch.basePath() = path.idTokens();
    thrift_cow::PatchNode rootPatch;
    auto prot = OperProtocol::BINARY;
    rootPatch.set_val(thrift_cow::serializeBuf<typename PathT::TC>(prot, data));
    patch.patch() = std::move(rootPatch);
    patch.protocol() = prot;
    ASSERT_TRUE(publisher_->write(std::move(patch)));
  }

 private:
  bool isStats_;
  std::unique_ptr<FsdbPatchPublisher> publisher_;
  utils::ConnectionOptions connectionOptions_;
  std::unique_ptr<folly::ScopedEventBaseThread> publisherThread_;
  std::unique_ptr<folly::ScopedEventBaseThread> reconnectThread_;
};

template <bool IsStats>
struct DataFactory;

template <>
struct DataFactory<true /* IsStats */> {
  const auto& root() {
    static const thriftpath::RootThriftPath<FsdbOperStatsRoot> root;
    return root;
  }

  auto path1() {
    return root().agent().phyStats();
  }

  std::map<std::string, phy::PhyStats> data1(std::string val) {
    std::map<std::string, phy::PhyStats> stats;
    stats[val] = phy::PhyStats();
    stats[val].timeCollected() = 123;
    return stats;
  }

  std::map<std::string, phy::PhyStats> fetchData1(
      const FsdbOperStatsRoot& root) {
    return *root.agent()->phyStats();
  }

  // path2 is designed to be inside a container so it doesn't exists initially
  static constexpr auto path2Key = "key1";
  auto path2() {
    return root().agent().hwTrunkStats()[path2Key];
  }

  HwTrunkStats data2(int val) {
    HwTrunkStats stats;
    stats.capacity_() = val;
    return stats;
  }

  HwTrunkStats fetchData2(const FsdbOperStatsRoot& root) {
    return root.agent()->hwTrunkStats()->at(path2Key);
  }
};

template <>
struct DataFactory<false /* IsStats */> {
  const auto& root() {
    static const thriftpath::RootThriftPath<FsdbOperStateRoot> root;
    return root;
  }

  auto path1() {
    return root().agent().config().defaultCommandLineArgs();
  }

  std::map<std::string, std::string> data1(std::string val) {
    std::map<std::string, std::string> cliArgs;
    cliArgs[val] = val;
    return cliArgs;
  }

  std::map<std::string, std::string> fetchData1(const FsdbOperStateRoot& root) {
    return *root.agent()->config()->defaultCommandLineArgs();
  }

  // path2 is designed to be inside a container so it doesn't exists initially
  static constexpr auto path2Key = 42;
  auto path2() {
    return root()
        .agent()
        .config()
        .sw()
        .switchSettings()
        .switchIdToSwitchInfo()[path2Key];
  }

  cfg::SwitchInfo data2(int val) {
    cfg::SwitchInfo info;
    info.switchIndex() = val;
    return info;
  }

  cfg::SwitchInfo fetchData2(const FsdbOperStateRoot& root) {
    return root.agent()
        ->config()
        ->sw()
        ->switchSettings()
        ->switchIdToSwitchInfo()
        ->at(path2Key);
  }
};

template <typename SubscriberT>
class FsdbSubManagerTest : public ::testing::Test,
                           public DataFactory<SubscriberT::IsStats> {
 public:
  using Subscriber = SubscriberT;
  static constexpr bool IsStats = Subscriber::IsStats;

  void SetUp() override {
    createFsdbServerAndPublisher();

    folly::LoggerDB::get().setLevel("fboss.thrift_cow", folly::LogLevel::DBG5);
    folly::LoggerDB::get().setLevel("fboss.fsdb", folly::LogLevel::DBG5);
  }

  bool isSubscribed(const std::string& clientId) {
    if (fsdbTestServer_) {
      auto subs = fsdbTestServer_->getActiveSubscriptions();
      for (const auto& [_, info] : subs) {
        for (const auto& subscription : info) {
          if (subscription.subscriberId() == clientId) {
            return true;
          }
        }
      }
    }
    return false;
  }

  bool publisherReady(const std::string& root) {
    if (fsdbTestServer_) {
      auto metadata = fsdbTestServer_->getPublisherRootMetadata(root, IsStats);
      return metadata.has_value();
    }
    return false;
  }

  TestAgentPublisher& testPublisher() {
    return *testPublisher_;
  }

  std::unique_ptr<Subscriber> createSubscriber(
      std::string clientId,
      int grHoldTimer = 0) {
    CHECK(connectionOptions_);
    SubscriptionOptions options(clientId, IsStats, grHoldTimer);
    return std::make_unique<Subscriber>(
        std::move(options), *connectionOptions_);
  }

  template <typename PathT>
  std::unique_ptr<Subscriber>
  createSubscriber(std::string clientId, PathT path, int grHoldTimer = 0) {
    auto subscriber = createSubscriber(std::move(clientId), grHoldTimer);
    subscriber->addPath(std::move(path));
    return subscriber;
  }

  std::vector<ExtendedOperPath> extSubscriptionPaths() const {
    if constexpr (IsStats) {
      using StatsRootMembers =
          apache::thrift::reflect_struct<FsdbOperStatsRoot>::member;
      using AgentStatsRootMembers =
          apache::thrift::reflect_struct<AgentStats>::member;
      using PhyStatsMembers =
          apache::thrift::reflect_struct<phy::PhyStats>::member;
      ExtendedOperPath path =
          ext_path_builder::raw(StatsRootMembers::agent::id::value)
              .raw(AgentStatsRootMembers::phyStats::id::value)
              .any()
              .raw(PhyStatsMembers::timeCollected::id::value)
              .get();
      return {std::move(path)};
    } else {
      using StateRootMembers =
          apache::thrift::reflect_struct<FsdbOperStateRoot>::member;
      using AgentRootMembers =
          apache::thrift::reflect_struct<AgentData>::member;
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
  }

  std::unique_ptr<Subscriber> createExtendedSubscriber(
      std::string clientId,
      std::vector<ExtendedOperPath> paths) {
    auto subscriber = createSubscriber(std::move(clientId));
    for (auto& path : paths) {
      subscriber->addExtendedPath(std::move(path));
    }
    return subscriber;
  }

  template <typename PathT, typename DataT>
  void connectPublisherAndPublish(PathT path, DataT data) {
    this->testPublisher().connect();
    this->testPublisher().publish(std::move(path), std::move(data));
    WITH_RETRIES(ASSERT_EVENTUALLY_TRUE(this->publisherReady("agent")));
  }

  void createFsdbServerAndPublisher() {
    fsdbTestServer_ = std::make_unique<FsdbTestServer>();
    connectionOptions_ =
        utils::ConnectionOptions("::1", fsdbTestServer_->getFsdbPort());
    testPublisher_ =
        std::make_unique<TestAgentPublisher>(IsStats, *connectionOptions_);
  }

  void killFsdb() {
    fsdbTestServer_.reset();
  }

 private:
  std::unique_ptr<FsdbTestServer> fsdbTestServer_;
  std::unique_ptr<TestAgentPublisher> testPublisher_;
  std::optional<utils::ConnectionOptions> connectionOptions_;
};

TEST(FsdbSubManagerTest, subscriberIdParsing) {
  SubscriberId agentId = "agent";
  EXPECT_EQ(subscriberId2ClientId(agentId).client(), FsdbClient::AGENT);
  EXPECT_EQ(subscriberId2ClientId(agentId).instanceId(), "");
  EXPECT_EQ(string2FsdbClient(agentId), FsdbClient::AGENT);
  EXPECT_EQ(fsdbClient2string(string2FsdbClient(agentId)), agentId);

  SubscriberId foo = "foo";
  EXPECT_EQ(subscriberId2ClientId(foo).client(), FsdbClient::UNSPECIFIED);
  EXPECT_EQ(subscriberId2ClientId(foo).instanceId(), "");
  EXPECT_EQ(string2FsdbClient(foo), FsdbClient::UNSPECIFIED);

  SubscriberId agentWithInstance = "agent:some_instance";
  EXPECT_EQ(
      subscriberId2ClientId(agentWithInstance).client(), FsdbClient::AGENT);
  EXPECT_EQ(
      subscriberId2ClientId(agentWithInstance).instanceId(), "some_instance");
  EXPECT_EQ(string2FsdbClient(agentWithInstance), FsdbClient::UNSPECIFIED);
}

using SubscriberTypes =
    ::testing::Types<FsdbCowStateSubManager, FsdbCowStatsSubManager>;

TYPED_TEST_SUITE(FsdbSubManagerTest, SubscriberTypes);

TYPED_TEST(FsdbSubManagerTest, subUnsub) {
  auto subscriber = this->createSubscriber("test", this->root().agent());
  auto boundData = subscriber->subscribeBound();
  WITH_RETRIES(EXPECT_EVENTUALLY_TRUE(this->isSubscribed("test")));
  subscriber.reset();
  WITH_RETRIES(EXPECT_EVENTUALLY_FALSE(this->isSubscribed("test")));
}

TYPED_TEST(FsdbSubManagerTest, subUnsubNoFsdb) {
  this->killFsdb();
  auto subscriber = this->createSubscriber("test", this->root().agent());
  auto boundData = subscriber->subscribeBound();
  sleep(3);
  subscriber.reset();
}

TYPED_TEST(FsdbSubManagerTest, subBeforeFsdbUp) {
  this->killFsdb();
  auto subscriber = this->createSubscriber("test", this->root().agent());
  auto boundData = subscriber->subscribeBound();
  sleep(3);
  this->createFsdbServerAndPublisher();
  sleep(3);
  subscriber.reset();
}

TYPED_TEST(FsdbSubManagerTest, subUpdateUnsub) {
  auto data1 = this->data1("foo");
  this->connectPublisherAndPublish(this->path1(), data1);
  auto subscriber = this->createSubscriber("test", this->root().agent());
  auto boundData = subscriber->subscribeBound();
  WITH_RETRIES({
    ASSERT_EVENTUALLY_TRUE(this->isSubscribed("test"));
    ASSERT_EVENTUALLY_TRUE(*boundData.rlock());
    EXPECT_EVENTUALLY_EQ(
        this->fetchData1((*boundData.rlock())->toThrift()), data1);
  });
  // update data and verify receieved
  data1 = this->data1("bar");
  this->testPublisher().publish(this->path1(), data1);
  WITH_RETRIES(EXPECT_EVENTUALLY_EQ(
      this->fetchData1((*boundData.rlock())->toThrift()), data1));
  subscriber.reset();
  WITH_RETRIES(EXPECT_EVENTUALLY_FALSE(this->isSubscribed("test")));
}

TYPED_TEST(FsdbSubManagerTest, subBeforePublisher) {
  auto subscriber = this->createSubscriber("test", this->root().agent());
  auto boundData = subscriber->subscribeBound();
  WITH_RETRIES(EXPECT_EVENTUALLY_TRUE(this->isSubscribed("test")));
  EXPECT_FALSE(*boundData.rlock());
  auto data1 = this->data1("foo");
  this->connectPublisherAndPublish(this->path1(), data1);
  WITH_RETRIES({
    ASSERT_EVENTUALLY_TRUE(*boundData.rlock());
    EXPECT_EVENTUALLY_EQ(
        this->fetchData1((*boundData.rlock())->toThrift()), data1);
  });
  subscriber.reset();
  WITH_RETRIES(EXPECT_EVENTUALLY_FALSE(this->isSubscribed("test")));
}

TYPED_TEST(FsdbSubManagerTest, subMultiPath) {
  auto data1 = this->data1("foo");
  auto data2 = this->data2(123);
  this->connectPublisherAndPublish(this->path1(), data1);
  auto subscriber = this->createSubscriber("test");
  SubscriptionKey path1SubKey = subscriber->addPath(this->path1());
  SubscriptionKey path2SubKey = subscriber->addPath(this->path2());

  bool publishedData2 = false;
  int numUpdates = 0;
  subscriber->subscribe([&](auto update) {
    numUpdates++;
    EXPECT_EQ(update.updatedKeys.size(), 1);
    EXPECT_EQ(update.updatedPaths.size(), 1);
    auto expectedKey = publishedData2 ? path2SubKey : path1SubKey;
    auto expectedPath =
        publishedData2 ? this->path2().idTokens() : this->path1().idTokens();
    EXPECT_EQ(update.updatedKeys[0], expectedKey);
    EXPECT_EQ(update.updatedPaths[0], expectedPath);

    EXPECT_EQ(this->fetchData1(update.data->toThrift()), data1);
    if (publishedData2) {
      EXPECT_EQ(this->fetchData2(update.data->toThrift()), data2);
    }
  });

  // wait for initial sync
  WITH_RETRIES(EXPECT_EVENTUALLY_EQ(numUpdates, 1));

  // publish second path
  publishedData2 = true;
  this->testPublisher().publish(this->path2(), data2);

  WITH_RETRIES(EXPECT_EVENTUALLY_EQ(numUpdates, 2));

  subscriber.reset();
  WITH_RETRIES(EXPECT_EVENTUALLY_FALSE(this->isSubscribed("test")));
}

TYPED_TEST(FsdbSubManagerTest, subscribeExtended) {
  auto data1 = this->data1("foo");
  this->connectPublisherAndPublish(this->path1(), data1);
  auto subscriber =
      this->createExtendedSubscriber("test", this->extSubscriptionPaths());

  // verify initial sync
  auto boundData = subscriber->subscribeBound();
  WITH_RETRIES({
    ASSERT_EVENTUALLY_TRUE(this->isSubscribed("test"));
    ASSERT_EVENTUALLY_TRUE(*boundData.rlock());
    EXPECT_EVENTUALLY_EQ(
        this->fetchData1((*boundData.rlock())->toThrift()), data1);
  });

  // update data and verify receieved
  data1 = this->data1("bar");
  this->testPublisher().publish(this->path1(), data1);
  WITH_RETRIES(EXPECT_EVENTUALLY_EQ(
      this->fetchData1((*boundData.rlock())->toThrift()), data1));
  subscriber.reset();
  WITH_RETRIES(EXPECT_EVENTUALLY_FALSE(this->isSubscribed("test")));
}

TYPED_TEST(FsdbSubManagerTest, restartPublisher) {
  auto data1 = this->data1("foo");
  this->connectPublisherAndPublish(this->path1(), data1);
  std::optional<SubscriptionState> lastStateSeen;
  auto subscriber = this->createSubscriber("test", this->root().agent());
  auto boundData = subscriber->subscribeBound(
      [&](auto, auto newState, std::optional<bool> /*initialSyncHasData*/) {
        lastStateSeen = newState;
      });

  WITH_RETRIES({
    ASSERT_EVENTUALLY_TRUE(this->isSubscribed("test"));
    // received data
    ASSERT_EVENTUALLY_EQ(lastStateSeen, SubscriptionState::CONNECTED);
    ASSERT_EVENTUALLY_TRUE(*boundData.rlock());
    ASSERT_EVENTUALLY_EQ(
        this->fetchData1((*boundData.rlock())->toThrift()), data1);
  });
  this->testPublisher().disconnect();
  WITH_RETRIES(
      ASSERT_EVENTUALLY_EQ(lastStateSeen, SubscriptionState::DISCONNECTED));
  data1 = this->data1("bar");
  this->connectPublisherAndPublish(this->path1(), data1);
  WITH_RETRIES(ASSERT_EVENTUALLY_EQ(
                   this->fetchData1((*boundData.rlock())->toThrift()), data1););
}

TYPED_TEST(FsdbSubManagerTest, verifyGR) {
  auto data1 = this->data1("foo");
  this->connectPublisherAndPublish(this->path1(), data1);

  std::optional<SubscriptionState> lastStateSeen;
  int numUpdates = 0;
  auto subscriber =
      this->createSubscriber("test", this->root().agent(), 3 /* grHoldTimer */);
  subscriber->subscribe(
      [&](auto update) { numUpdates++; },
      [&](auto, auto newState, std::optional<bool> /*initialSyncHasData*/) {
        lastStateSeen = newState;
      });
  WITH_RETRIES({
    ASSERT_EVENTUALLY_EQ(lastStateSeen, SubscriptionState::CONNECTED);
    ASSERT_EVENTUALLY_EQ(numUpdates, 1);
  });

  this->testPublisher().disconnect();
  WITH_RETRIES(ASSERT_EVENTUALLY_EQ(
      lastStateSeen, SubscriptionState::DISCONNECTED_GR_HOLD));
  this->connectPublisherAndPublish(this->path1(), data1);

  this->testPublisher().disconnect();
  WITH_RETRIES({
    ASSERT_EVENTUALLY_EQ(
        lastStateSeen, SubscriptionState::DISCONNECTED_GR_HOLD_EXPIRED);
    ASSERT_EVENTUALLY_EQ(numUpdates, 1);
  });

  this->connectPublisherAndPublish(this->path1(), data1);
  WITH_RETRIES({
    ASSERT_EVENTUALLY_EQ(lastStateSeen, SubscriptionState::CONNECTED);
    ASSERT_EVENTUALLY_EQ(numUpdates, 2);
  });
}

template <typename SubscriberT>
class FsdbSubManagerHbTest : public FsdbSubManagerTest<SubscriberT> {
 public:
  void SetUp() override {
    FLAGS_serveHeartbeats = true;
    FLAGS_statsSubscriptionHeartbeat_s = 1;
    FLAGS_stateSubscriptionHeartbeat_s = 1;
    FsdbSubManagerTest<SubscriberT>::SetUp();
  }
};

TYPED_TEST_SUITE(FsdbSubManagerHbTest, SubscriberTypes);

TYPED_TEST(FsdbSubManagerHbTest, verifyHeartbeatCb) {
  auto data1 = this->data1("foo");
  this->connectPublisherAndPublish(this->path1(), data1);

  std::optional<SubscriptionState> lastStateSeen;
  std::optional<int64_t> lastPublishedAt;
  int numHeartbeats{0};
  auto subscriber = this->createSubscriber("test", this->root().agent());
  subscriber->subscribe(
      [&](auto update) {
        if (update.lastPublishedAt.has_value()) {
          lastPublishedAt = update.lastPublishedAt.value();
        }
      },
      [&](auto, auto newState, std::optional<bool> initialSyncHasData) {
        lastStateSeen = newState;
      },
      [&](std::optional<OperMetadata> md) { numHeartbeats++; });

  WITH_RETRIES(
      { ASSERT_EVENTUALLY_EQ(lastStateSeen, SubscriptionState::CONNECTED); });

  WITH_RETRIES({
    ASSERT_EVENTUALLY_TRUE(
        lastPublishedAt.has_value() && lastPublishedAt.value() > 0);
  });

  // Verify heartbeat callback is called
  WITH_RETRIES_N(100, { EXPECT_EVENTUALLY_GT(numHeartbeats, 0); });

  // Verify heartbeat callback continues to be called
  numHeartbeats = 0;
  WITH_RETRIES_N(100, { EXPECT_EVENTUALLY_GT(numHeartbeats, 0); });
}

} // namespace facebook::fboss::fsdb::test
