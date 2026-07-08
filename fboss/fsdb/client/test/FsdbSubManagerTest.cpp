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
#include <folly/logging/LoggerDB.h>
#include <folly/logging/xlog.h>
#include <gtest/gtest.h>
#include <thrift/lib/cpp2/op/Get.h>

#include <chrono>
#include <utility>

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

  template <typename PathT>
  void publishDelete(const PathT& path) {
    CHECK(publisher_);
    auto tokens = path.idTokens();
    // Extract the map key (last token) and build a MapPatch with a del child
    auto mapKey = tokens.back();
    tokens.pop_back();

    Patch patch;
    patch.basePath() = std::move(tokens);

    thrift_cow::PatchNode delChild;
    delChild.set_del();

    thrift_cow::MapPatch mapPatch;
    mapPatch.children() = {{mapKey, std::move(delChild)}};

    thrift_cow::PatchNode rootPatch;
    rootPatch.set_map_node(std::move(mapPatch));

    patch.patch() = std::move(rootPatch);
    patch.protocol() = OperProtocol::BINARY;
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

  auto mapKeyPath(std::string key) {
    return root().agent().phyStats()[std::move(key)];
  }

  phy::PhyStats mapKeyData(int val) {
    phy::PhyStats stats;
    stats.timeCollected() = val;
    return stats;
  }

  bool hasMapKey(const FsdbOperStatsRoot& root, const std::string& key) {
    return root.agent()->phyStats()->count(key) > 0;
  }

  std::map<std::string, phy::PhyStats> mapData(
      const std::map<std::string, int>& entries) {
    std::map<std::string, phy::PhyStats> stats;
    for (auto& [key, val] : entries) {
      stats[key] = phy::PhyStats();
      stats[key].timeCollected() = val;
    }
    return stats;
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

  auto mapKeyPath(std::string key) {
    return root().agent().config().defaultCommandLineArgs()[std::move(key)];
  }

  std::string mapKeyData(int val) {
    return folly::to<std::string>(val);
  }

  bool hasMapKey(const FsdbOperStateRoot& root, const std::string& key) {
    return root.agent()->config()->defaultCommandLineArgs()->count(key) > 0;
  }

  std::map<std::string, std::string> mapData(
      const std::map<std::string, int>& entries) {
    std::map<std::string, std::string> cliArgs;
    for (auto& [key, val] : entries) {
      cliArgs[key] = folly::to<std::string>(val);
    }
    return cliArgs;
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

  size_t numActivePublishers() {
    if (fsdbTestServer_) {
      return fsdbTestServer_->serviceHandler().getActivePublishers().size();
    }
    return 0;
  }

  TestAgentPublisher& testPublisher() {
    return *testPublisher_;
  }

  SubscriptionOptions getSubscriptionOptions(const std::string& clientId) {
    return SubscriptionOptions(clientId, IsStats);
  }

  SubscriptionOptions getSubscriptionOptions(
      const std::string& clientId,
      int grHoldTimer) {
    return SubscriptionOptions(clientId, IsStats, grHoldTimer);
  }

  std::unique_ptr<Subscriber> createSubscriber(SubscriptionOptions options) {
    CHECK(connectionOptions_);
    return std::make_unique<Subscriber>(
        std::move(options), *connectionOptions_);
  }

  template <typename PathT>
  std::unique_ptr<Subscriber> createSubscriber(
      SubscriptionOptions options,
      PathT path) {
    auto subscriber = createSubscriber(std::move(options));
    subscriber->addPath(std::move(path));
    return subscriber;
  }

  std::vector<ExtendedOperPath> extSubscriptionPaths() const {
    if constexpr (IsStats) {
      ExtendedOperPath path = ext_path_builder::raw(
                                  apache::thrift::op::get_field_id_v<
                                      FsdbOperStatsRoot,
                                      apache::thrift::ident::agent>)
                                  .raw(
                                      apache::thrift::op::get_field_id_v<
                                          AgentStats,
                                          apache::thrift::ident::phyStats>)
                                  .any()
                                  .raw(
                                      apache::thrift::op::get_field_id_v<
                                          phy::PhyStats,
                                          apache::thrift::ident::timeCollected>)
                                  .get();
      return {std::move(path)};
    } else {
      ExtendedOperPath path =
          ext_path_builder::raw(
              apache::thrift::op::get_field_id_v<
                  FsdbOperStateRoot,
                  apache::thrift::ident::agent>)
              .raw(
                  apache::thrift::op::
                      get_field_id_v<AgentData, apache::thrift::ident::config>)
              .raw(
                  apache::thrift::op::get_field_id_v<
                      cfg::AgentConfig,
                      apache::thrift::ident::defaultCommandLineArgs>)
              .any()
              .get();
      return {std::move(path)};
    }
  }

  std::vector<ExtendedOperPath> mapKeyExtPaths(
      const std::vector<std::string>& keys) const {
    std::vector<ExtendedOperPath> paths;
    for (const auto& key : keys) {
      if constexpr (IsStats) {
        paths.push_back(
            ext_path_builder::raw(
                apache::thrift::op::get_field_id_v<
                    FsdbOperStatsRoot,
                    apache::thrift::ident::agent>)
                .raw(
                    apache::thrift::op::get_field_id_v<
                        AgentStats,
                        apache::thrift::ident::phyStats>)
                .raw(key)
                .get());
      } else {
        paths.push_back(
            ext_path_builder::raw(
                apache::thrift::op::get_field_id_v<
                    FsdbOperStateRoot,
                    apache::thrift::ident::agent>)
                .raw(
                    apache::thrift::op::get_field_id_v<
                        AgentData,
                        apache::thrift::ident::config>)
                .raw(
                    apache::thrift::op::get_field_id_v<
                        cfg::AgentConfig,
                        apache::thrift::ident::defaultCommandLineArgs>)
                .raw(key)
                .get());
      }
    }
    return paths;
  }

  std::unique_ptr<Subscriber> createExtendedSubscriber(
      SubscriptionOptions options,
      std::vector<ExtendedOperPath> paths) {
    auto subscriber = createSubscriber(std::move(options));
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
  auto subscriber = this->createSubscriber(
      this->getSubscriptionOptions("test"), this->root().agent());
  auto boundData = subscriber->subscribeBound();
  WITH_RETRIES(EXPECT_EVENTUALLY_TRUE(this->isSubscribed("test")));
  subscriber.reset();
  WITH_RETRIES(EXPECT_EVENTUALLY_FALSE(this->isSubscribed("test")));
}

TYPED_TEST(FsdbSubManagerTest, subUnsubNoFsdb) {
  this->killFsdb();
  auto subscriber = this->createSubscriber(
      this->getSubscriptionOptions("test"), this->root().agent());
  auto boundData = subscriber->subscribeBound();
  sleep(3);
  subscriber.reset();
}

TYPED_TEST(FsdbSubManagerTest, subBeforeFsdbUp) {
  this->killFsdb();
  auto subscriber = this->createSubscriber(
      this->getSubscriptionOptions("test"), this->root().agent());
  auto boundData = subscriber->subscribeBound();
  sleep(3);
  this->createFsdbServerAndPublisher();
  sleep(3);
  subscriber.reset();
}

TYPED_TEST(FsdbSubManagerTest, subUpdateUnsub) {
  auto data1 = this->data1("foo");
  this->connectPublisherAndPublish(this->path1(), data1);
  auto subscriber = this->createSubscriber(
      this->getSubscriptionOptions("test"), this->root().agent());
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
  auto subscriber = this->createSubscriber(
      this->getSubscriptionOptions("test"), this->root().agent());
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
  auto subscriber =
      this->createSubscriber(this->getSubscriptionOptions("test"));
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

TYPED_TEST(FsdbSubManagerTest, subDeleteKeys) {
  // Publish a map with 3 keys
  auto allData = this->mapData({{"key1", 10}, {"key2", 20}, {"key3", 30}});
  this->connectPublisherAndPublish(this->path1(), allData);

  // Subscribe to only key1 and key2 using extended paths
  auto subscriber = this->createExtendedSubscriber(
      this->getSubscriptionOptions("test"),
      this->mapKeyExtPaths({"key1", "key2"}));

  auto boundData = subscriber->subscribeBound();

  // Verify initial sync delivers key1 and key2
  WITH_RETRIES({
    ASSERT_EVENTUALLY_TRUE(*boundData.rlock());
    ASSERT_EVENTUALLY_TRUE(
        this->hasMapKey((*boundData.rlock())->toThrift(), "key1"));
    ASSERT_EVENTUALLY_TRUE(
        this->hasMapKey((*boundData.rlock())->toThrift(), "key2"));
  });
  // key3 not subscribed, should not be present
  EXPECT_FALSE(this->hasMapKey((*boundData.rlock())->toThrift(), "key3"));

  // Delete key1 and key2 (leave key3)
  this->testPublisher().publishDelete(this->mapKeyPath("key1"));
  this->testPublisher().publishDelete(this->mapKeyPath("key2"));

  // Verify subscriber sees deletions
  WITH_RETRIES({
    EXPECT_EVENTUALLY_FALSE(
        this->hasMapKey((*boundData.rlock())->toThrift(), "key1"));
    EXPECT_EVENTUALLY_FALSE(
        this->hasMapKey((*boundData.rlock())->toThrift(), "key2"));
  });

  // Delete key3 (unsubscribed) - subscriber should not be affected
  this->testPublisher().publishDelete(this->mapKeyPath("key3"));

  // key3 was never in subscriber's data and deleting it should not
  // affect the subscriber's state
  EXPECT_FALSE(this->hasMapKey((*boundData.rlock())->toThrift(), "key1"));
  EXPECT_FALSE(this->hasMapKey((*boundData.rlock())->toThrift(), "key2"));
  EXPECT_FALSE(this->hasMapKey((*boundData.rlock())->toThrift(), "key3"));
}

TYPED_TEST(FsdbSubManagerTest, subscribeExtended) {
  auto data1 = this->data1("foo");
  this->connectPublisherAndPublish(this->path1(), data1);
  auto subscriber = this->createExtendedSubscriber(
      this->getSubscriptionOptions("test"), this->extSubscriptionPaths());

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
  auto subscriber = this->createSubscriber(
      this->getSubscriptionOptions("test"), this->root().agent());
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

TYPED_TEST(FsdbSubManagerTest, reconnectTriggersDisconnectAndReconnect) {
  auto data1 = this->data1("foo");
  this->connectPublisherAndPublish(this->path1(), data1);
  std::optional<SubscriptionState> lastStateSeen;
  // Record every transition the SubscriptionStateChangeCb receives so we can
  // assert the CONNECTED -> DISCONNECTED -> CONNECTED sequence is delivered,
  // not just that the final state is CONNECTED.
  folly::Synchronized<std::vector<SubscriptionState>> stateChanges;
  auto subscriber = this->createSubscriber(
      this->getSubscriptionOptions("test"), this->root().agent());
  auto boundData = subscriber->subscribeBound(
      [&](auto, auto newState, std::optional<bool> /*initialSyncHasData*/) {
        lastStateSeen = newState;
        stateChanges.wlock()->push_back(newState);
      });

  WITH_RETRIES({
    ASSERT_EVENTUALLY_TRUE(this->isSubscribed("test"));
    ASSERT_EVENTUALLY_EQ(lastStateSeen, SubscriptionState::CONNECTED);
  });
  // Snapshot the size of the transition log right before the reconnect so we
  // can scan only the post-reconnect tail for the CONNECTED -> DISCONNECTED
  // -> CONNECTED sequence.
  size_t preReconnectChanges = stateChanges.rlock()->size();

  subscriber->reconnect();

  // Wait for the full transition sequence to land in the callback log.
  WITH_RETRIES({
    auto changes = stateChanges.rlock();
    ASSERT_EVENTUALLY_GE(changes->size(), preReconnectChanges + 2);
    ASSERT_EVENTUALLY_EQ(changes->back(), SubscriptionState::CONNECTED);
  });
  // The exact transitions delivered to the SubscriptionStateChangeCb after
  // reconnect must be DISCONNECTED followed by CONNECTED.
  {
    auto changes = stateChanges.rlock();
    std::vector<SubscriptionState> postReconnect(
        changes->begin() + preReconnectChanges, changes->end());
    EXPECT_EQ(postReconnect.front(), SubscriptionState::DISCONNECTED);
    EXPECT_EQ(postReconnect.back(), SubscriptionState::CONNECTED);
  }
  // And a fresh publish after the reconnect is delivered (the subscription
  // is functionally restored, not just its state).
  auto data1New = this->data1("bar");
  this->testPublisher().publish(this->path1(), data1New);
  WITH_RETRIES(EXPECT_EVENTUALLY_EQ(
      this->fetchData1((*boundData.rlock())->toThrift()), data1New));
}

TYPED_TEST(FsdbSubManagerTest, reconnectValidationWithGR) {
  auto data1 = this->data1("foo");
  this->connectPublisherAndPublish(this->path1(), data1);
  std::optional<SubscriptionState> lastStateSeen;
  // Record every transition the SubscriptionStateChangeCb receives so we can
  // assert the CONNECTED -> DISCONNECTED_GR_HOLD -> CONNECTED sequence is
  // delivered, not just that the final state is CONNECTED.
  folly::Synchronized<std::vector<SubscriptionState>> stateChanges;
  auto subscriber = this->createSubscriber(
      this->getSubscriptionOptions("test", 5), this->root().agent());
  auto boundData = subscriber->subscribeBound(
      [&](auto, auto newState, std::optional<bool> /*initialSyncHasData*/) {
        lastStateSeen = newState;
        stateChanges.wlock()->push_back(newState);
      });

  WITH_RETRIES({
    ASSERT_EVENTUALLY_TRUE(this->isSubscribed("test"));
    ASSERT_EVENTUALLY_EQ(lastStateSeen, SubscriptionState::CONNECTED);
  });
  // Snapshot the size of the transition log right before the reconnect so we
  // can scan only the post-reconnect tail for the CONNECTED ->
  // DISCONNECTED_GR_HOLD -> CONNECTED sequence.
  size_t preReconnectChanges = stateChanges.rlock()->size();

  subscriber->reconnect();

  // Wait for the full transition sequence to land in the callback log.
  WITH_RETRIES({
    auto changes = stateChanges.rlock();
    ASSERT_EVENTUALLY_GE(changes->size(), preReconnectChanges + 2);
    ASSERT_EVENTUALLY_EQ(changes->back(), SubscriptionState::CONNECTED);
  });
  // The exact transitions delivered to the SubscriptionStateChangeCb after
  // reconnect must be DISCONNECTED_GR_HOLD followed by CONNECTED.
  {
    auto changes = stateChanges.rlock();
    std::vector<SubscriptionState> postReconnect(
        changes->begin() + preReconnectChanges, changes->end());
    EXPECT_EQ(postReconnect.front(), SubscriptionState::DISCONNECTED_GR_HOLD);
    EXPECT_EQ(postReconnect.back(), SubscriptionState::CONNECTED);
  }
  // And a fresh publish after the reconnect is delivered (the subscription
  // is functionally restored, not just its state).
  auto data1New = this->data1("bar");
  this->testPublisher().publish(this->path1(), data1New);
  WITH_RETRIES(EXPECT_EVENTUALLY_EQ(
      this->fetchData1((*boundData.rlock())->toThrift()), data1New));
}

TYPED_TEST(FsdbSubManagerTest, reconnectAfterStopIsNoop) {
  auto subscriber = this->createSubscriber(
      this->getSubscriptionOptions("test"), this->root().agent());
  auto boundData = subscriber->subscribeBound();
  WITH_RETRIES(EXPECT_EVENTUALLY_TRUE(this->isSubscribed("test")));
  subscriber->stop();
  WITH_RETRIES(EXPECT_EVENTUALLY_FALSE(this->isSubscribed("test")));
  // After stop(), the underlying subscriber is destroyed and getInfo() has
  // no value to report. Capture this baseline before invoking reconnect().
  EXPECT_FALSE(subscriber->getInfo().has_value());

  // Should be safe to call after stop -- no crash, no-op.
  subscriber->reconnect();

  // Verify reconnect was a true no-op: no resubscription happened and the
  // subscriber remains in its post-stop state.
  EXPECT_FALSE(subscriber->getInfo().has_value());
  EXPECT_FALSE(this->isSubscribed("test"));
}

TYPED_TEST(FsdbSubManagerTest, reconnectOnExtendedSubscriber) {
  auto data1 = this->data1("foo");
  this->connectPublisherAndPublish(this->path1(), data1);
  std::optional<SubscriptionState> lastStateSeen;
  bool sawDisconnected = false;
  auto subscriber = this->createExtendedSubscriber(
      this->getSubscriptionOptions("test"), this->extSubscriptionPaths());
  auto boundData = subscriber->subscribeBound(
      [&](auto, auto newState, std::optional<bool> /*initialSyncHasData*/) {
        lastStateSeen = newState;
        if (newState == SubscriptionState::DISCONNECTED) {
          sawDisconnected = true;
        }
      });

  WITH_RETRIES({
    ASSERT_EVENTUALLY_TRUE(this->isSubscribed("test"));
    ASSERT_EVENTUALLY_EQ(lastStateSeen, SubscriptionState::CONNECTED);
  });

  subscriber->reconnect();

  WITH_RETRIES({ ASSERT_EVENTUALLY_TRUE(sawDisconnected); });
  WITH_RETRIES(
      { ASSERT_EVENTUALLY_EQ(lastStateSeen, SubscriptionState::CONNECTED); });
}

TYPED_TEST(FsdbSubManagerTest, reconnectInvalidateGRSkipsGRHold) {
  auto data1 = this->data1("foo");
  this->connectPublisherAndPublish(this->path1(), data1);
  folly::Synchronized<std::vector<SubscriptionState>> stateChanges;
  std::optional<SubscriptionState> lastStateSeen;
  auto subscriber = this->createSubscriber(
      this->getSubscriptionOptions("test", 5), this->root().agent());
  auto boundData = subscriber->subscribeBound(
      [&](auto, auto newState, std::optional<bool> /*initialSyncHasData*/) {
        lastStateSeen = newState;
        stateChanges.wlock()->push_back(newState);
      });

  WITH_RETRIES({
    ASSERT_EVENTUALLY_TRUE(this->isSubscribed("test"));
    ASSERT_EVENTUALLY_EQ(lastStateSeen, SubscriptionState::CONNECTED);
  });
  size_t preReconnectChanges = stateChanges.rlock()->size();

  auto reconnectStart = std::chrono::steady_clock::now();
  subscriber->reconnect(true);

  // Wait for the post-reconnect transitions to land in the callback log.
  WITH_RETRIES({
    auto changes = stateChanges.rlock();
    ASSERT_EVENTUALLY_GE(changes->size(), preReconnectChanges + 2);
    ASSERT_EVENTUALLY_EQ(changes->back(), SubscriptionState::CONNECTED);
  });
  // The first post-reconnect transition must be DISCONNECTED_GR_HOLD_EXPIRED
  // (NOT DISCONNECTED_GR_HOLD), proving GR hold was skipped entirely.
  std::chrono::steady_clock::time_point grHoldExpiredAt;
  {
    auto changes = stateChanges.rlock();
    std::vector<SubscriptionState> postReconnect(
        changes->begin() + preReconnectChanges, changes->end());
    EXPECT_EQ(
        postReconnect.front(), SubscriptionState::DISCONNECTED_GR_HOLD_EXPIRED);
    EXPECT_EQ(postReconnect.back(), SubscriptionState::CONNECTED);
    grHoldExpiredAt = std::chrono::steady_clock::now();
  }
  // Sanity: the expired transition landed well before the 5s GR hold would
  // have elapsed, proving we did not just wait it out.
  auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
      grHoldExpiredAt - reconnectStart);
  EXPECT_LT(elapsed.count(), 5);
}

TYPED_TEST(FsdbSubManagerTest, reconnectInvalidateGRWhileInGRHold) {
  auto data1 = this->data1("foo");
  this->connectPublisherAndPublish(this->path1(), data1);
  folly::Synchronized<std::vector<SubscriptionState>> stateChanges;
  std::optional<SubscriptionState> lastStateSeen;
  // grHoldTimer=30 so the natural GR expiry will not race with the test.
  auto subscriber = this->createSubscriber(
      this->getSubscriptionOptions("test", 30), this->root().agent());
  auto boundData = subscriber->subscribeBound(
      [&](auto, auto newState, std::optional<bool> /*initialSyncHasData*/) {
        lastStateSeen = newState;
        stateChanges.wlock()->push_back(newState);
      });

  WITH_RETRIES({
    ASSERT_EVENTUALLY_TRUE(this->isSubscribed("test"));
    ASSERT_EVENTUALLY_EQ(lastStateSeen, SubscriptionState::CONNECTED);
  });

  // Drop the publisher to land the subscription in DISCONNECTED_GR_HOLD.
  this->testPublisher().disconnect();
  WITH_RETRIES(ASSERT_EVENTUALLY_EQ(
      lastStateSeen, SubscriptionState::DISCONNECTED_GR_HOLD));

  // Now invalidate GR -- subscription should immediately transition to
  // DISCONNECTED_GR_HOLD_EXPIRED via the override's "already in GR_HOLD" path,
  // not by waiting out the 30s timer.
  auto invalidateStart = std::chrono::steady_clock::now();
  subscriber->reconnect(true);
  WITH_RETRIES(ASSERT_EVENTUALLY_EQ(
      lastStateSeen, SubscriptionState::DISCONNECTED_GR_HOLD_EXPIRED));
  auto invalidateElapsed = std::chrono::duration_cast<std::chrono::seconds>(
      std::chrono::steady_clock::now() - invalidateStart);
  EXPECT_LT(invalidateElapsed.count(), 30);

  // Bring publisher back; subscription returns to CONNECTED.
  this->connectPublisherAndPublish(this->path1(), data1);
  WITH_RETRIES(
      ASSERT_EVENTUALLY_EQ(lastStateSeen, SubscriptionState::CONNECTED));
}

TYPED_TEST(FsdbSubManagerTest, reconnectInvalidateGRFlagDoesNotLeak) {
  auto data1 = this->data1("foo");
  this->connectPublisherAndPublish(this->path1(), data1);
  folly::Synchronized<std::vector<SubscriptionState>> stateChanges;
  std::optional<SubscriptionState> lastStateSeen;
  auto subscriber = this->createSubscriber(
      this->getSubscriptionOptions("test", 5), this->root().agent());
  auto boundData = subscriber->subscribeBound(
      [&](auto, auto newState, std::optional<bool> /*initialSyncHasData*/) {
        lastStateSeen = newState;
        stateChanges.wlock()->push_back(newState);
      });

  WITH_RETRIES({
    ASSERT_EVENTUALLY_TRUE(this->isSubscribed("test"));
    ASSERT_EVENTUALLY_EQ(lastStateSeen, SubscriptionState::CONNECTED);
  });

  // Round 1: reconnect(noGR=true) -- consumes the flag. Wait for
  // the full DISCONNECTED_GR_HOLD_EXPIRED -> CONNECTED cycle to land in the
  // transition log (lastStateSeen is racy because the brief GR_HOLD_EXPIRED
  // value is overwritten by CONNECTED before the retry loop wakes up).
  size_t round1Snapshot = stateChanges.rlock()->size();
  subscriber->reconnect(true);
  WITH_RETRIES({
    auto changes = stateChanges.rlock();
    ASSERT_EVENTUALLY_GE(changes->size(), round1Snapshot + 2);
    ASSERT_EVENTUALLY_EQ(changes->back(), SubscriptionState::CONNECTED);
  });
  {
    auto changes = stateChanges.rlock();
    std::vector<SubscriptionState> round1(
        changes->begin() + round1Snapshot, changes->end());
    EXPECT_EQ(round1.front(), SubscriptionState::DISCONNECTED_GR_HOLD_EXPIRED);
  }

  // Round 2: an UNRELATED publisher disconnect must follow the normal GR
  // path (DISCONNECTED_GR_HOLD), proving the flag was consumed and did not
  // leak into a subsequent unrelated flap.
  size_t round2Snapshot = stateChanges.rlock()->size();
  this->testPublisher().disconnect();
  WITH_RETRIES({
    auto changes = stateChanges.rlock();
    ASSERT_EVENTUALLY_GT(changes->size(), round2Snapshot);
    ASSERT_EVENTUALLY_EQ(
        (*changes)[round2Snapshot], SubscriptionState::DISCONNECTED_GR_HOLD);
  });
}

TYPED_TEST(
    FsdbSubManagerTest,
    reconnectInvalidateGRWhileInGRHoldExpiredDoesNotLeak) {
  // Bug regression: reconnect(noGR=true) called while subscription is
  // in DISCONNECTED_GR_HOLD_EXPIRED must not leak forceGRExpired_=true. There
  // is no upcoming CONNECTED->DISCONNECTED transition for the flag to be
  // consumed against, so without the fix the stale flag survives until the
  // next legitimate disconnect (after the publisher recovers) and incorrectly
  // skips GR hold then.
  auto data1 = this->data1("foo");
  this->connectPublisherAndPublish(this->path1(), data1);
  folly::Synchronized<std::vector<SubscriptionState>> stateChanges;
  std::optional<SubscriptionState> lastStateSeen;
  // Short GR timer so we can wait it out quickly within the test.
  auto subscriber = this->createSubscriber(
      this->getSubscriptionOptions("test", 2), this->root().agent());
  auto boundData = subscriber->subscribeBound(
      [&](auto, auto newState, std::optional<bool> /*initialSyncHasData*/) {
        lastStateSeen = newState;
        stateChanges.wlock()->push_back(newState);
      });

  WITH_RETRIES({
    ASSERT_EVENTUALLY_TRUE(this->isSubscribed("test"));
    ASSERT_EVENTUALLY_EQ(lastStateSeen, SubscriptionState::CONNECTED);
  });

  // Drop publisher and let the GR timer expire naturally so the subscription
  // settles in DISCONNECTED_GR_HOLD_EXPIRED -- the state in which the bug
  // would set forceGRExpired_ with no consumer to clear it.
  this->testPublisher().disconnect();
  WITH_RETRIES(ASSERT_EVENTUALLY_EQ(
      lastStateSeen, SubscriptionState::DISCONNECTED_GR_HOLD_EXPIRED));

  // Trigger reconnect(noGR=true) while in GR_HOLD_EXPIRED. Pre-fix
  // this leaks forceGRExpired_=true; with the fix it is a no-op because no
  // upcoming CONNECTED->DISCONNECTED edge will consume it.
  subscriber->reconnect(true);

  // Bring publisher back; subscription returns to CONNECTED.
  this->connectPublisherAndPublish(this->path1(), data1);
  WITH_RETRIES(
      ASSERT_EVENTUALLY_EQ(lastStateSeen, SubscriptionState::CONNECTED));

  // An UNRELATED publisher disconnect that follows MUST traverse the normal
  // GR path (DISCONNECTED_GR_HOLD), NOT skip to DISCONNECTED_GR_HOLD_EXPIRED.
  // With the buggy code the stale forceGRExpired_ is consumed here and this
  // assertion fires. With the fix the first post-flap transition is GR_HOLD.
  size_t snapshot = stateChanges.rlock()->size();
  this->testPublisher().disconnect();
  WITH_RETRIES({
    auto changes = stateChanges.rlock();
    ASSERT_EVENTUALLY_GT(changes->size(), snapshot);
    EXPECT_EQ((*changes)[snapshot], SubscriptionState::DISCONNECTED_GR_HOLD);
  });
}

TYPED_TEST(FsdbSubManagerTest, reconnectInvalidateGRWithoutGRHoldIsNoop) {
  auto data1 = this->data1("foo");
  this->connectPublisherAndPublish(this->path1(), data1);
  folly::Synchronized<std::vector<SubscriptionState>> stateChanges;
  std::optional<SubscriptionState> lastStateSeen;
  // grHoldTimer=0 -- noGR should be a no-op (default disconnect path).
  auto subscriber = this->createSubscriber(
      this->getSubscriptionOptions("test"), this->root().agent());
  auto boundData = subscriber->subscribeBound(
      [&](auto, auto newState, std::optional<bool> /*initialSyncHasData*/) {
        lastStateSeen = newState;
        stateChanges.wlock()->push_back(newState);
      });

  WITH_RETRIES({
    ASSERT_EVENTUALLY_TRUE(this->isSubscribed("test"));
    ASSERT_EVENTUALLY_EQ(lastStateSeen, SubscriptionState::CONNECTED);
  });
  size_t preReconnectChanges = stateChanges.rlock()->size();

  subscriber->reconnect(true);

  WITH_RETRIES({
    auto changes = stateChanges.rlock();
    ASSERT_EVENTUALLY_GE(changes->size(), preReconnectChanges + 2);
    ASSERT_EVENTUALLY_EQ(changes->back(), SubscriptionState::CONNECTED);
  });
  // First post-reconnect transition must be plain DISCONNECTED, not
  // DISCONNECTED_GR_HOLD_EXPIRED -- proves noGR is a no-op when
  // grHoldTimeSec_ == 0.
  auto changes = stateChanges.rlock();
  std::vector<SubscriptionState> postReconnect(
      changes->begin() + preReconnectChanges, changes->end());
  EXPECT_EQ(postReconnect.front(), SubscriptionState::DISCONNECTED);
  EXPECT_EQ(postReconnect.back(), SubscriptionState::CONNECTED);
}

TYPED_TEST(FsdbSubManagerTest, reconnectInvalidateGROnExtendedSubscriber) {
  auto data1 = this->data1("foo");
  this->connectPublisherAndPublish(this->path1(), data1);
  folly::Synchronized<std::vector<SubscriptionState>> stateChanges;
  std::optional<SubscriptionState> lastStateSeen;
  // Use createSubscriber to set grHoldTimer, then convert to extended by
  // adding an extended path before subscribing. createExtendedSubscriber
  // uses createSubscriber("...") which defaults grHoldTimer=0, so we build
  // it manually here to get GR.
  auto subscriber =
      this->createSubscriber(this->getSubscriptionOptions("test", 5));
  for (auto& path : this->extSubscriptionPaths()) {
    subscriber->addExtendedPath(std::move(path));
  }
  auto boundData = subscriber->subscribeBound(
      [&](auto, auto newState, std::optional<bool> /*initialSyncHasData*/) {
        lastStateSeen = newState;
        stateChanges.wlock()->push_back(newState);
      });

  WITH_RETRIES({
    ASSERT_EVENTUALLY_TRUE(this->isSubscribed("test"));
    ASSERT_EVENTUALLY_EQ(lastStateSeen, SubscriptionState::CONNECTED);
  });
  size_t preReconnectChanges = stateChanges.rlock()->size();

  subscriber->reconnect(true);

  WITH_RETRIES({
    auto changes = stateChanges.rlock();
    ASSERT_EVENTUALLY_GE(changes->size(), preReconnectChanges + 2);
    ASSERT_EVENTUALLY_EQ(changes->back(), SubscriptionState::CONNECTED);
  });
  auto changes = stateChanges.rlock();
  std::vector<SubscriptionState> postReconnect(
      changes->begin() + preReconnectChanges, changes->end());
  EXPECT_EQ(
      postReconnect.front(), SubscriptionState::DISCONNECTED_GR_HOLD_EXPIRED);
  EXPECT_EQ(postReconnect.back(), SubscriptionState::CONNECTED);
}

TYPED_TEST(FsdbSubManagerTest, verifyGR) {
  auto data1 = this->data1("foo");
  this->connectPublisherAndPublish(this->path1(), data1);

  std::optional<SubscriptionState> lastStateSeen;
  int numUpdates = 0;
  auto subscriber = this->createSubscriber(
      this->getSubscriptionOptions("test", 3), this->root().agent());
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
  auto subscriber = this->createSubscriber(
      this->getSubscriptionOptions("test"), this->root().agent());
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

TYPED_TEST(FsdbSubManagerHbTest, verifyPublisherRestartWithGR) {
  // Use a longer GR hold time so the stale state timer (which starts at
  // subscriber creation) doesn't fire before heartbeats arrive, even when
  // server setup takes a few seconds.
  int grHoldTime = 10;
  int numHeartbeatsBeforePublisherConnect = 1;
  int numHeartbeatsAfterPublisherDisconnect = 3;
  int numHeartbeats = 0;

  // Step 0: Create subscription before publisher is connected
  std::optional<SubscriptionState> lastStateSeen;
  SubscriptionOptions options =
      this->getSubscriptionOptions("test", grHoldTime);
  options.requireInitialSyncToMarkConnect_ = true;
  auto subscriber = this->createSubscriber(options, this->root().agent());
  auto boundData = subscriber->subscribeBound(
      [&](auto, auto newState, std::optional<bool> initialSyncHasData) {
        XLOG(INFO) << "DBG: SubscriptionState: "
                   << subscriptionStateToString(newState)
                   << " initialSyncHasData: "
                   << (initialSyncHasData.has_value() ? "true" : "false");
        lastStateSeen = newState;
      },
      [&](std::optional<OperMetadata> /*md*/) {
        numHeartbeats++;
        XLOG(INFO) << "DBG: numHeartbeats: " << numHeartbeats;
      });

  // Step 1: Verify that before publisher connect, heartbeats are received
  // but SubscriptionState does not transition yet to CONNECTED.
  WITH_RETRIES_N(30, {
    ASSERT_EVENTUALLY_GE(numHeartbeats, numHeartbeatsBeforePublisherConnect);
  });
  // The subscription must NOT be in CONNECTED state yet (requireInitialSync
  // prevents CONNECTED without initial data). Other states (e.g.
  // DISCONNECTED_GR_HOLD_EXPIRED) are allowed if the stale timer happened to
  // fire concurrently.
  ASSERT_FALSE(
      lastStateSeen.has_value() &&
      lastStateSeen.value() == SubscriptionState::CONNECTED);

  // Step 2: Connect publisher and publish initial data.
  // Note: the state change to CONNECTED fires before the data callback
  // (see FsdbPatchSubscriber.cpp serveStream), so we must wait separately
  // for boundData to be populated after seeing CONNECTED.
  auto data1 = this->data1("foo");
  this->connectPublisherAndPublish(this->path1(), data1);
  WITH_RETRIES(
      ASSERT_EVENTUALLY_EQ(lastStateSeen, SubscriptionState::CONNECTED));
  WITH_RETRIES({
    auto data = *boundData.rlock();
    ASSERT_EVENTUALLY_NE(data, nullptr);
    if (data) {
      ASSERT_EVENTUALLY_EQ(this->fetchData1(data->toThrift()), data1);
    }
  });

  // Step 2: Disconnect publisher, verify DISCONNECTED_GR_HOLD state
  this->testPublisher().disconnect();
  WITH_RETRIES(ASSERT_EVENTUALLY_EQ(this->numActivePublishers(), 0));
  WITH_RETRIES(ASSERT_EVENTUALLY_EQ(
      lastStateSeen, SubscriptionState::DISCONNECTED_GR_HOLD));

  // Step 3: Verify that subscriber is receiving heartbeats from connected
  // FSDB server
  int heartbeatsBefore = numHeartbeats;
  WITH_RETRIES_N(30, {
    ASSERT_EVENTUALLY_GE(
        numHeartbeats,
        heartbeatsBefore + numHeartbeatsAfterPublisherDisconnect);
  });

  // Step 4: Verify subscription eventually goes to DISCONNECTED_GR_HOLD_EXPIRED
  WITH_RETRIES(ASSERT_EVENTUALLY_EQ(
      lastStateSeen, SubscriptionState::DISCONNECTED_GR_HOLD_EXPIRED));

  // Step 5: Connect publisher and verify subscription is CONNECTED
  auto data2 = this->data1("foo");
  this->connectPublisherAndPublish(this->path1(), data2);
  WITH_RETRIES(
      ASSERT_EVENTUALLY_EQ(lastStateSeen, SubscriptionState::CONNECTED));
  // Same as Step 2: state change fires before data callback, so retry.
  WITH_RETRIES({
    auto data = *boundData.rlock();
    ASSERT_EVENTUALLY_NE(data, nullptr);
    if (data) {
      ASSERT_EVENTUALLY_EQ(this->fetchData1(data->toThrift()), data2);
    }
  });
}

} // namespace facebook::fboss::fsdb::test
