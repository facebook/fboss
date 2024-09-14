// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include <gtest/gtest.h>
#include "fboss/fsdb/oper/ExtendedPathBuilder.h"
#include "fboss/fsdb/tests/client/FsdbTestClients.h"
#include "fboss/fsdb/tests/utils/FsdbTestServer.h"
#include "fboss/lib/CommonUtils.h"

#include <folly/coro/BlockingWait.h>
#include <folly/io/async/ScopedEventBaseThread.h>
#include <folly/logging/LogLevel.h>
#include <folly/logging/LoggerDB.h>
#include <thrift/lib/cpp2/protocol/Serializer.h>
#include <memory>

namespace {

auto constexpr kSubscriberId = "fsdb_test_subscriber";
auto constexpr kPublisherId = "fsdb_test_publisher";
auto constexpr kUnknownPublisherId = "publisher_unknown";
const std::vector<std::string> kPublishRoot{"agent"};
const std::string rawConfig = R"(
    {"publishers":{"fsdb_test_publisher":{"paths":[{"path":{"raw":["agent"]},"isExpected":false}]}},"subscribers":{":ExtSub_disallowed":{"trackReconnect":true,"allowExtendedSubscriptions":false},":ExtSub_allowed":{"trackReconnect":true,"allowExtendedSubscriptions":true}}}
)";
auto constexpr kUnknownPublisherPathConfig = R"(
    {"publishers":{"fsdb_test_publisher":{"paths":[{"path":{"raw":["unknown_path"]},"isExpected":false}]}},"subscribers":{":ExtSub_disallowed":{"trackReconnect":true,"allowExtendedSubscriptions":false},":ExtSub_allowed":{"trackReconnect":true,"allowExtendedSubscriptions":true}}}
)";
} // namespace

namespace facebook::fboss::fsdb::test {

template <typename TestParam>
class FsdbPubSubTest : public ::testing::Test {
 public:
  using PublisherT = typename TestParam::PublisherT;
  using PubUnitT = typename TestParam::PubUnitT;
  using SubscriberT = typename TestParam::SubscriberT;
  using SubUnitT = typename TestParam::SubUnitT;

  void SetUp() override {
    folly::LoggerDB::get().setLevel("fboss.thrift_cow", folly::LogLevel::DBG4);
    folly::LoggerDB::get().setLevel("fboss.fsdb", folly::LogLevel::DBG4);
    auto config = getFsdbConfig();
    fsdbTestServer_ = std::make_unique<FsdbTestServer>(std::move(config));
    publisherStreamEvbThread_ =
        std::make_unique<folly::ScopedEventBaseThread>();
    subscriberStreamEvbThread_ =
        std::make_unique<folly::ScopedEventBaseThread>();
    connRetryEvbThread_ = std::make_unique<folly::ScopedEventBaseThread>();
    std::vector<std::string> root;
    subscriber_ = createSubscriber(kSubscriberId);
    publisher_ = createPublisher(kPublisherId);
  }
  void TearDown() override {
    cancelConnections();
    fsdbTestServer_.reset();
    subscriber_.reset();
    publisher_.reset();
    subscriberStreamEvbThread_.reset();
    publisherStreamEvbThread_.reset();
    connRetryEvbThread_.reset();
  }

 protected:
  virtual std::shared_ptr<FsdbConfig> getFsdbConfig() {
    return FsdbConfig::fromRaw(rawConfig);
  }
  template <typename SubsT>
  std::unique_ptr<SubsT> createSubscriberImpl(const std::string& id) const {
    return std::make_unique<SubsT>(
        id,
        getSubscribePath<SubsT>(),
        subscriberStreamEvbThread_->getEventBase(),
        connRetryEvbThread_->getEventBase(),
        TestParam::PubSubStats);
  }
  template <typename SubsT>
  auto getSubscribePath() const {
    if constexpr (std::is_same_v<typename SubsT::SubUnitT, SubscriberChunk>) {
      RawOperPath p;
      p.path() = kPublishRoot;
      return std::map<SubscriptionKey, RawOperPath>{{0, std::move(p)}};
    } else {
      return kPublishRoot;
    }
  }
  std::unique_ptr<SubscriberT> createSubscriber(const std::string& id) const {
    return createSubscriberImpl<SubscriberT>(id);
  }
  std::unique_ptr<PublisherT> createPublisherImpl(
      const std::string& id,
      folly::EventBase* publisherEvb,
      const std::vector<std::string>& path = kPublishRoot) const {
    return std::make_unique<PublisherT>(
        id,
        path,
        publisherEvb,
        connRetryEvbThread_->getEventBase(),
        TestParam::PubSubStats);
  }
  std::unique_ptr<PublisherT> createPublisher(
      const std::string& id,
      const std::vector<std::string>& path = kPublishRoot) const {
    return createPublisherImpl(
        id, publisherStreamEvbThread_->getEventBase(), path);
  }
  void setupConnection(
      FsdbStreamClient& pubSub,
      bool updateServerPort = false) {
    pubSub.setServerOptions(
        FsdbStreamClient::ServerOptions("::1", fsdbTestServer_->getFsdbPort()),
        updateServerPort);
    WITH_RETRIES_N(kRetries, {
      ASSERT_EVENTUALLY_TRUE(pubSub.isConnectedToServer())
          << pubSub.clientId() << " did not connected";
    });
  }
  void setupConnections(bool updateServerPort = false) {
    setupConnection(*publisher_, updateServerPort);
    checkPublishing({publisher_->clientId()});
    setupConnection(*subscriber_, updateServerPort);
    checkSubscribed({subscriber_->clientId()});
  }
  void checkSubscribed(
      const SubscriberIds& subscribers,
      int numSubscriptions = 1) {
    WITH_RETRIES({
      auto subscriberToInfo = folly::coro::blockingWait(
          fsdbTestServer_->getClient()->co_getOperSubscriberInfos(subscribers));
      for (auto subscriber : subscribers) {
        auto sitr = subscriberToInfo.find(subscriber);
        if (numSubscriptions) {
          ASSERT_EVENTUALLY_NE(sitr, subscriberToInfo.end());
          ASSERT_EVENTUALLY_EQ(sitr->second.size(), numSubscriptions);
        } else {
          ASSERT_EVENTUALLY_EQ(sitr, subscriberToInfo.end());
        }
      }
    });
  }
  void checkPublishing(const PublisherIds& publishers, int numActive = 1) {
    WITH_RETRIES({
      auto publisherToInfo = folly::coro::blockingWait(
          fsdbTestServer_->getClient()->co_getOperPublisherInfos(publishers));
      for (auto publisher : publishers) {
        auto pitr = publisherToInfo.find(publisher);
        auto metadata = fsdbTestServer_->getPublisherRootMetadata(
            *kPublishRoot.begin(), pubSubStats());
        if (numActive) {
          ASSERT_EVENTUALLY_NE(pitr, publisherToInfo.end());
          ASSERT_EVENTUALLY_EQ(pitr->second.size(), 1);
          ASSERT_EVENTUALLY_TRUE(metadata);
          ASSERT_EVENTUALLY_EQ(metadata->numOpenConnections, numActive);
        } else {
          ASSERT_EVENTUALLY_EQ(pitr, publisherToInfo.end());
          ASSERT_EVENTUALLY_FALSE(metadata);
        }
      }
    });
  }
  void cancelConnection(FsdbStreamClient& pubSub) {
    pubSub.cancel();
    WITH_RETRIES_N(kRetries, {
      ASSERT_EVENTUALLY_TRUE(!pubSub.isConnectedToServer());
      ASSERT_EVENTUALLY_TRUE(!pubSub.serviceLoopRunning());
    });
  }
  void cancelConnections() {
    if (subscriber_) {
      cancelConnection(*subscriber_);
    }
    if (publisher_) {
      cancelConnection(*publisher_);
    }
  }

  void publishAgentConfig(const cfg::AgentConfig& config) {
    if constexpr (std::is_same_v<typename TestParam::PubUnitT, OperDelta>) {
      publisher_->write(makeDelta(config));
    } else if constexpr (std::is_same_v<PubUnitT, OperState>) {
      publisher_->write(makeState(config));
    } else if constexpr (std::is_same_v<PubUnitT, Patch>) {
      publisher_->write(makePatch(config));
    }
  }
  void publishPortStats(
      const folly::F14FastMap<std::string, HwPortStats>& portStats) {
    if constexpr (std::is_same_v<PubUnitT, OperDelta>) {
      publisher_->write(makeDelta(portStats));
    } else if constexpr (std::is_same_v<PubUnitT, OperState>) {
      publisher_->write(makeState(portStats));
    } else if constexpr (std::is_same_v<PubUnitT, Patch>) {
      publisher_->write(makePatch(portStats));
    }
  }
  bool pubSubStats() const {
    return TestParam::PubSubStats;
  }
  bool isDelta() const {
    return std::is_same_v<PubUnitT, OperDelta>;
  }
  bool isPatch() const {
    return std::is_same_v<PubUnitT, Patch>;
  }
  template <typename SubRequestT>
  auto subscribe(const SubRequestT& reqIn) {
    auto req = std::make_unique<SubRequestT>(reqIn);
    if constexpr (std::is_same_v<TestParam, DeltaPubSubForState>) {
      return folly::coro::blockingWait(
          this->fsdbTestServer_->serviceHandler().co_subscribeOperStateDelta(
              std::move(req)));
    } else if constexpr (std::is_same_v<TestParam, StatePubSubForState>) {
      return folly::coro::blockingWait(
          this->fsdbTestServer_->serviceHandler().co_subscribeOperStatePath(
              std::move(req)));
    } else if constexpr (std::is_same_v<TestParam, DeltaPubSubForStats>) {
      return folly::coro::blockingWait(
          this->fsdbTestServer_->serviceHandler().co_subscribeOperStatsDelta(
              std::move(req)));
    } else if constexpr (std::is_same_v<TestParam, StatePubSubForStats>) {
      return folly::coro::blockingWait(
          this->fsdbTestServer_->serviceHandler().co_subscribeOperStatsPath(
              std::move(req)));
    } else if constexpr (std::is_same_v<TestParam, PatchPubSubForState>) {
      return folly::coro::blockingWait(
          this->fsdbTestServer_->serviceHandler().co_subscribeState(
              std::move(req)));
    } else if constexpr (std::is_same_v<TestParam, PatchPubSubForStats>) {
      return folly::coro::blockingWait(
          this->fsdbTestServer_->serviceHandler().co_subscribeStats(
              std::move(req)));
    }
  }
  auto subscribeExtended(const OperSubRequestExtended& reqIn) {
    if constexpr (std::is_same_v<TestParam, DeltaPubSubForState>) {
      return folly::coro::blockingWait(
          this->fsdbTestServer_->getClient()
              ->co_subscribeOperStateDeltaExtended(reqIn));
    } else if constexpr (std::is_same_v<TestParam, StatePubSubForState>) {
      return folly::coro::blockingWait(
          this->fsdbTestServer_->getClient()->co_subscribeOperStatePathExtended(
              reqIn));
    } else if constexpr (std::is_same_v<TestParam, DeltaPubSubForStats>) {
      return folly::coro::blockingWait(
          this->fsdbTestServer_->getClient()
              ->co_subscribeOperStatsDeltaExtended(reqIn));

    } else if constexpr (std::is_same_v<TestParam, StatePubSubForStats>) {
      return folly::coro::blockingWait(
          this->fsdbTestServer_->getClient()->co_subscribeOperStatsPathExtended(
              reqIn));
    }
  }

  auto makePubRequest(std::string id, std::vector<std::string> path) {
    if constexpr (std::is_same_v<PubUnitT, Patch>) {
      PubRequest req;
      req.clientId()->client() = FsdbClient::ADHOC;
      req.clientId()->instanceId() = std::move(id);
      req.path()->path() = std::move(path);
      return req;
    } else {
      OperPubRequest req;
      req.publisherId() = std::move(id);
      req.path()->raw() = std::move(path);
      return req;
    }
  }

  auto makeSubRequest(std::string id, std::vector<std::string> path) {
    if constexpr (std::is_same_v<PubUnitT, Patch>) {
      SubRequest req;
      req.clientId()->client() = FsdbClient::ADHOC;
      req.clientId()->instanceId() = std::move(id);
      RawOperPath p;
      p.path() = std::move(path);
      req.paths() = {{0, std::move(p)}};
      return req;
    } else {
      OperSubRequest req;
      req.subscriberId() = std::move(id);
      req.path()->raw() = std::move(path);
      return req;
    }
  }

  template <typename Req>
  auto setupPublisher(const Req& reqIn) {
    auto req = std::make_unique<Req>(reqIn);
    if constexpr (std::is_same_v<TestParam, DeltaPubSubForState>) {
      return folly::coro::blockingWait(
          this->fsdbTestServer_->serviceHandler().co_publishOperStateDelta(
              std::move(req)));
    } else if constexpr (std::is_same_v<TestParam, StatePubSubForState>) {
      return folly::coro::blockingWait(
          this->fsdbTestServer_->serviceHandler().co_publishOperStatePath(
              std::move(req)));
    } else if constexpr (std::is_same_v<TestParam, DeltaPubSubForStats>) {
      return folly::coro::blockingWait(
          this->fsdbTestServer_->serviceHandler().co_publishOperStatsDelta(
              std::move(req)));
    } else if constexpr (std::is_same_v<TestParam, StatePubSubForStats>) {
      return folly::coro::blockingWait(
          this->fsdbTestServer_->serviceHandler().co_publishOperStatsPath(
              std::move(req)));
    } else if constexpr (std::is_same_v<TestParam, PatchPubSubForState>) {
      return folly::coro::blockingWait(
          this->fsdbTestServer_->serviceHandler().co_publishState(
              std::move(req)));
    } else if constexpr (std::is_same_v<TestParam, PatchPubSubForStats>) {
      return folly::coro::blockingWait(
          this->fsdbTestServer_->serviceHandler().co_publishStats(
              std::move(req)));
    }
  }

  static auto constexpr kRetries = 10;
  std::unique_ptr<FsdbTestServer> fsdbTestServer_;
  std::unique_ptr<folly::ScopedEventBaseThread> subscriberStreamEvbThread_;
  std::unique_ptr<folly::ScopedEventBaseThread> publisherStreamEvbThread_;
  std::unique_ptr<folly::ScopedEventBaseThread> connRetryEvbThread_;
  std::unique_ptr<SubscriberT> subscriber_;
  std::unique_ptr<PublisherT> publisher_;
};

using TestTypes = ::testing::Types<
    DeltaPubSubForState,
    StatePubSubForState,
    PatchPubSubForState,
    DeltaPubSubForStats,
    StatePubSubForStats,
    PatchPubSubForStats>;

TYPED_TEST_SUITE(FsdbPubSubTest, TestTypes);

TYPED_TEST(FsdbPubSubTest, connectAndCancel) {
  if (this->pubSubStats()) {
    EXPECT_EQ(
        this->publisher_->getCounterPrefix(),
        this->isDelta()       ? "fsdbDeltaStatPublisher_agent"
            : this->isPatch() ? "fsdbPatchStatPublisher_agent"
                              : "fsdbPathStatPublisher_agent");
    EXPECT_EQ(
        this->subscriber_->getCounterPrefix(),
        this->isDelta()       ? "fsdbDeltaStatSubscriber_agent"
            : this->isPatch() ? "fsdbPatchStatSubscriber_agent"
                              : "fsdbPathStatSubscriber_agent");
  } else {
    EXPECT_EQ(
        this->publisher_->getCounterPrefix(),
        this->isDelta()       ? "fsdbDeltaStatePublisher_agent"
            : this->isPatch() ? "fsdbPatchStatePublisher_agent"
                              : "fsdbPathStatePublisher_agent");
    EXPECT_EQ(
        this->subscriber_->getCounterPrefix(),
        this->isDelta()       ? "fsdbDeltaStateSubscriber_agent"
            : this->isPatch() ? "fsdbPatchStateSubscriber_agent"
                              : "fsdbPathStateSubscriber_agent");
  }
  this->setupConnections();
  this->cancelConnections();

  // In tests, we don't start the publisher threads
  fb303::ThreadCachedServiceData::get()->publishStats();
  EXPECT_EQ(
      fb303::ServiceData::get()->getCounter("watchdog_thread_heartbeat_missed"),
      0);
}

TYPED_TEST(FsdbPubSubTest, rePublishSubscribe) {
  // Cancel and resubscribe
  this->setupConnections();
  this->cancelConnections();
  // Setup same connections again, should go through, since the prev
  // ones were cancelled and thus state on server should be cleared.
  auto publisher = this->createPublisher(kPublisherId);
  this->setupConnection(*publisher);
  auto subscriber = this->createSubscriber(kSubscriberId);
  this->setupConnection(*subscriber);
}

TYPED_TEST(FsdbPubSubTest, dupSubscriber) {
  auto req = this->makeSubRequest("test", {"agent"});
  auto res = this->subscribe(req);
  // we've had errors sneak through this in the past because the subscription
  // key depended on the current timestamp. Introducing a sleep To make sure we
  // validate subscriber unique-ness even if timing is a factor
  // @lint-ignore CLANGTIDY
  std::this_thread::sleep_for(std::chrono::seconds(3));
  EXPECT_THROW({ auto res2 = this->subscribe(req); }, FsdbException);
}

TYPED_TEST(FsdbPubSubTest, multiplePublishers) {
  this->setupConnections();
  folly::ScopedEventBaseThread pub2Thread;
  // Same path different publisher Id
  auto publisher2 =
      this->createPublisherImpl("publisher2", pub2Thread.getEventBase());
  this->setupConnection(*publisher2);
  // Same publisherId, different path
  folly::ScopedEventBaseThread pub3Thread;
  auto publisher3 =
      this->createPublisherImpl("publisher3", pub3Thread.getEventBase());
  this->setupConnection(*publisher3);
  // Check publishing status
  this->checkPublishing({publisher2->clientId()}, 3);
  this->checkPublishing({kPublisherId}, 3 /*paths*/);
  // Cancel connections before evb thread are stopped
  this->cancelConnection(*publisher2);
  // Publisher 2 should be pruned
  this->checkPublishing({"publisher3"}, 2);
  this->cancelConnection(*publisher3);
  // Back to 1
  this->checkPublishing({kPublisherId});
}

TYPED_TEST(FsdbPubSubTest, publish) {
  this->setupConnections();
  if (this->pubSubStats()) {
    this->publishPortStats(makePortStats(1));
  } else {
    this->publishAgentConfig(makeAgentConfig({{"foo", "bar"}}));
  }
  // initial sync after first publish
  this->subscriber_->assertQueue(1, this->kRetries);
}

TYPED_TEST(FsdbPubSubTest, dupPublisher) {
  auto req = this->makePubRequest("test", {"agent"});
  auto res = this->setupPublisher(req);
  // we've had errors sneak through this in the past because the publisher
  // key depended on the current timestamp. Introducing a sleep To make sure we
  // validate publisher unique-ness even if timing is a factor
  // @lint-ignore CLANGTIDY
  std::this_thread::sleep_for(std::chrono::seconds(3));
  EXPECT_THROW(this->setupPublisher(req), FsdbException);
}

TYPED_TEST(FsdbPubSubTest, reconnect) {
  this->setupConnections();
  std::array<std::string, 2> counterPrefixes{
      this->publisher_->getCounterPrefix(),
      this->subscriber_->getCounterPrefix()};

  for (const auto& counterPrefix : counterPrefixes) {
    EXPECT_EQ(
        fb303::ServiceData::get()->getCounter(counterPrefix + ".connected"), 1);
  }
  // Break server, assert for disconnect
  this->fsdbTestServer_.reset();
  // Need to publish something in order to detect that server has
  // gone away
  if (this->pubSubStats()) {
    this->publishPortStats(makePortStats(1));
  } else {
    this->publishAgentConfig(makeAgentConfig({{"foo", "bar"}}));
  }
  WITH_RETRIES_N(this->kRetries, {
    ASSERT_EVENTUALLY_TRUE(!this->publisher_->isConnectedToServer());
    ASSERT_EVENTUALLY_TRUE(!this->subscriber_->isConnectedToServer());
    // In tests, we don't start the publisher threads
    fb303::ThreadCachedServiceData::get()->publishStats();
    for (const auto& counterPrefix : counterPrefixes) {
      EXPECT_EVENTUALLY_TRUE(
          fb303::ServiceData::get()->getCounter(
              counterPrefix + ".disconnects.sum.60") >= 1);
      EXPECT_EQ(
          fb303::ServiceData::get()->getCounter(counterPrefix + ".connected"),
          0);
    }
    // check aggregate counter
    EXPECT_EVENTUALLY_TRUE(
        fb303::ServiceData::get()->getCounter(
            "fsdb_streams.disconnects.sum.60") >= 1);
  });
  // Setup server again and assert for reconnect
  this->fsdbTestServer_ = std::make_unique<FsdbTestServer>();
  // Need to update server address since we are binding to ephemeral port
  // which will change on server recreate
  this->setupConnections(true);
  // Publish new delta after reconnect, assert that delta gets through
  if (this->pubSubStats()) {
    this->publishPortStats(makePortStats(2));
  } else {
    this->publishAgentConfig(makeAgentConfig({{"foo", "bar"}, {"bar", "baz"}}));
  }
  // Initial sync on first publish
  this->subscriber_->assertQueue(1, this->kRetries);
}

TYPED_TEST(FsdbPubSubTest, publishToMultipleSubscribers) {
  this->setupConnections();
  auto subscriber2 = this->createSubscriber("fsdb_test_subscriber2");
  this->setupConnection(*subscriber2);
  this->checkSubscribed(
      {this->subscriber_->clientId(), subscriber2->clientId()});
  if (this->pubSubStats()) {
    this->publishPortStats(makePortStats(1));
  } else {
    this->publishAgentConfig(makeAgentConfig({{"foo", "bar"}}));
  }
  // Initial sync only after first publish
  this->subscriber_->assertQueue(1, this->kRetries);
  subscriber2->assertQueue(1, this->kRetries);
}

TYPED_TEST(FsdbPubSubTest, publisherDropCausesSubscribersReset) {
  this->setupConnections();
  auto subscriber2 = this->createSubscriber("fsdb_test_subscriber2");
  this->setupConnection(*subscriber2);
  if (this->pubSubStats()) {
    this->publishPortStats(makePortStats(1));
  } else {
    this->publishAgentConfig(makeAgentConfig({{"foo", "bar"}}));
  }
  // initial sync after first publish
  WITH_RETRIES_N(this->kRetries, {
    ASSERT_EVENTUALLY_TRUE(this->subscriber_->initialSyncDone());
    ASSERT_EVENTUALLY_TRUE(subscriber2->initialSyncDone());
  });
  this->cancelConnection(*this->publisher_);
  // Subscriber should reset after publisher goes away
  WITH_RETRIES_N(this->kRetries, {
    ASSERT_EVENTUALLY_FALSE(this->subscriber_->initialSyncDone());
    ASSERT_EVENTUALLY_FALSE(subscriber2->initialSyncDone());
  });
  // Subscriber should auto reconnect after resceiving a reset
  WITH_RETRIES_N(this->kRetries, {
    ASSERT_EVENTUALLY_TRUE(this->subscriber_->isConnectedToServer());
    ASSERT_EVENTUALLY_TRUE(subscriber2->isConnectedToServer());
  });
}

TYPED_TEST(FsdbPubSubTest, publishToMultipleSubscribersCancelOne) {
  this->setupConnections();
  auto subscriber2 = this->createSubscriber("fsdb_test_subscriber2");
  this->setupConnection(*subscriber2);
  this->checkSubscribed(
      {this->subscriber_->clientId(), subscriber2->clientId()});
  if (this->pubSubStats()) {
    this->publishPortStats(makePortStats(1));
  } else {
    this->publishAgentConfig(makeAgentConfig({{"foo", "bar"}}));
  }
  // Initial sync only after first publish
  this->subscriber_->assertQueue(1, this->kRetries);
  subscriber2->assertQueue(1, this->kRetries);
  this->cancelConnection(*this->subscriber_);
  this->checkSubscribed({this->subscriber_->clientId()}, 0 /* not subscribed*/);
  this->checkSubscribed({subscriber2->clientId()});
  if (this->pubSubStats()) {
    this->publishPortStats(makePortStats(2));
  } else {
    this->publishAgentConfig(makeAgentConfig({{"foo", "bar"}, {"bar", "baz"}}));
  }
  this->subscriber_->assertQueue(1, this->kRetries);
  subscriber2->assertQueue(2, this->kRetries);
}

TYPED_TEST(FsdbPubSubTest, publishToMultipleSubscribersOfDifferentTypes) {
  this->setupConnections();
  auto subscriber2 =
      this->template createSubscriberImpl<TestFsdbStateSubscriber>(
          "fsdb_test_subscriber2");
  auto subscriber3 =
      this->template createSubscriberImpl<TestFsdbDeltaSubscriber>(
          "fsdb_test_subscriber3");
  this->setupConnection(*subscriber2);
  this->checkSubscribed({subscriber2->clientId()});
  this->setupConnection(*subscriber3);
  this->checkSubscribed({subscriber3->clientId()});
  if (this->pubSubStats()) {
    this->publishPortStats(makePortStats(1));
  } else {
    this->publishAgentConfig(makeAgentConfig({{"foo", "bar"}}));
  }
  // Initial sync only after first publish
  this->subscriber_->assertQueue(1, this->kRetries);
  subscriber2->assertQueue(1, this->kRetries);
  subscriber3->assertQueue(1, this->kRetries);
}

TYPED_TEST(FsdbPubSubTest, subscriberReadyBeforePublisher) {
  auto publisherId = this->publisher_->clientId();
  this->publisher_.reset();
  this->checkPublishing({publisherId}, 0);
  this->subscriber_.reset();
  this->subscriber_ = this->createSubscriber(kSubscriberId);
  this->setupConnection(*this->subscriber_);
  // No publisher sync ever. Subscriber just waits
  EXPECT_THROW(
      checkWithRetry([this]() { return this->subscriber_->queueSize() == 1; }),
      FbossError);
}

TYPED_TEST(FsdbPubSubTest, verifyExtendedSubRestrictionNotEnforced) {
  FLAGS_checkSubscriberConfig = true;
  FLAGS_enforceSubscriberConfig = false;
  std::vector<ExtendedOperPath> paths;
  paths.emplace_back(ext_path_builder::raw("agent").get());
  auto req = OperSubRequestExtended();
  req.paths() = paths;
  req.subscriberId() = "TestSub:ExtSub_disallowed";
  EXPECT_NO_THROW(this->subscribeExtended(req));
}

TYPED_TEST(FsdbPubSubTest, verifyExtendedSubAllowed) {
  FLAGS_checkSubscriberConfig = true;
  FLAGS_enforceSubscriberConfig = true;
  std::vector<ExtendedOperPath> paths;
  paths.emplace_back(ext_path_builder::raw("agent").get());
  auto req = OperSubRequestExtended();
  req.paths() = paths;
  req.subscriberId() = "TestSub:ExtSub_allowed";
  EXPECT_NO_THROW(this->subscribeExtended(req));
}

TYPED_TEST(FsdbPubSubTest, verifyWildcardSubDenied) {
  if (this->isPatch()) {
    return; // patch extended subs not implemented yet
  }
  FLAGS_checkSubscriberConfig = true;
  FLAGS_enforceSubscriberConfig = true;
  std::vector<ExtendedOperPath> paths;
  paths.emplace_back(
      ext_path_builder::raw("agent").raw("switchState").regex("port.*").get());
  auto req = OperSubRequestExtended();
  req.paths() = paths;
  req.subscriberId() = "TestSub:ExtSub_allowed";
  EXPECT_THROW(this->subscribeExtended(req), FsdbException);
}

TYPED_TEST(FsdbPubSubTest, verifyExtendedSubRestricted) {
  if (this->isPatch()) {
    return; // patch extended subs not implemented yet
  }
  FLAGS_checkSubscriberConfig = true;
  FLAGS_enforceSubscriberConfig = true;
  std::vector<ExtendedOperPath> paths;
  paths.emplace_back(ext_path_builder::raw("agent").get());
  auto req = OperSubRequestExtended();
  req.paths() = paths;
  req.subscriberId() = "TestSub:ExtSub_disallowed";
  EXPECT_THROW(this->subscribeExtended(req), FsdbException);
}

TYPED_TEST(FsdbPubSubTest, verifyUnknownPublisherNotEnforced) {
  FLAGS_checkOperOwnership = true;
  FLAGS_enforcePublisherConfig = false;
  auto publisher = this->createPublisher(kUnknownPublisherId);
  this->setupConnection(*publisher);
  this->checkPublishing({publisher->clientId()});
}

TYPED_TEST(FsdbPubSubTest, verifyUnknownPublisherRejected) {
  FLAGS_checkOperOwnership = true;
  FLAGS_enforcePublisherConfig = true;
  auto publisher = this->createPublisher(kUnknownPublisherId);
  publisher->setServerOptions(
      FsdbStreamClient::ServerOptions(
          "::1", this->fsdbTestServer_->getFsdbPort()),
      false);
  WITH_RETRIES_N(
      this->kRetries, ASSERT_FALSE(publisher->isConnectedToServer()));
}

template <typename TestParam>
class FsdbUnknownPublisherPathTest : public FsdbPubSubTest<TestParam> {
 protected:
  std::shared_ptr<FsdbConfig> getFsdbConfig() override {
    return FsdbConfig::fromRaw(kUnknownPublisherPathConfig);
  }
};

TYPED_TEST_SUITE(FsdbUnknownPublisherPathTest, TestTypes);

TYPED_TEST(
    FsdbUnknownPublisherPathTest,
    verifyUnknownPublisherPathNotEnforced) {
  FLAGS_checkOperOwnership = true;
  FLAGS_enforcePublisherConfig = false;
  auto publisher = this->createPublisher(kPublisherId);
  this->setupConnection(*publisher);
  this->checkPublishing({publisher->clientId()});
}

TYPED_TEST(FsdbUnknownPublisherPathTest, verifyUnknownPublisherPathRejected) {
  FLAGS_checkOperOwnership = true;
  FLAGS_enforcePublisherConfig = true;
  auto publisher = this->createPublisher(kPublisherId);
  publisher->setServerOptions(
      FsdbStreamClient::ServerOptions(
          "::1", this->fsdbTestServer_->getFsdbPort()),
      false);
  WITH_RETRIES_N(
      this->kRetries, ASSERT_FALSE(publisher->isConnectedToServer()));
}

class DeltaStatePubSubTest : public FsdbPubSubTest<DeltaPubSubForState> {
 protected:
  void publishRawJson(const std::string& json1, const std::string& json2) {
    OperPath deltaPath;
    deltaPath.raw() = {"agent"};
    OperDeltaUnit deltaUnit;
    deltaUnit.path() = deltaPath;
    deltaUnit.newState() = json1;
    OperDelta delta;
    delta.changes()->push_back(deltaUnit);
    deltaUnit.newState() = json2;
    delta.changes()->push_back(deltaUnit);
    delta.protocol() = OperProtocol::SIMPLE_JSON;
    publisher_->write(std::move(delta));
  }
};

TEST_F(DeltaStatePubSubTest, dropsInvalidChange) {
  this->setupConnections();
  cfg::AgentConfig config;
  config.defaultCommandLineArgs() = {{"foo", "bar"}};

  // Craft delta with a valid change and an invalid change
  OperPath validPath;
  validPath.raw() = {"agent", "config"};
  OperDeltaUnit validUnit;
  validUnit.path() = validPath;
  validUnit.newState() =
      apache::thrift::SimpleJSONSerializer::serialize<std::string>(config);

  OperPath nonExistentPath;
  OperDeltaUnit nonExistentUnit;
  nonExistentPath.raw() = {"agent", "nonExistentPath"};
  nonExistentUnit.path() = nonExistentPath;
  nonExistentUnit.newState() = "some value";

  OperPath invalidPath;
  OperDeltaUnit typeMistmatchUnit;
  invalidPath.raw() = {"agent", "switchState"};
  typeMistmatchUnit.path() = nonExistentPath;
  typeMistmatchUnit.newState() = "some value"; // should be a struct

  OperDelta delta;
  delta.changes() = {validUnit, nonExistentUnit, typeMistmatchUnit};
  delta.protocol() =
      OperProtocol::BINARY; // TODO: extend test to cover all protocols

  // Publisher should not be rejected, and subscriber should still get served
  this->subscriber_->assertQueue(0, this->kRetries);
  publisher_->write(std::move(delta));
  this->subscriber_->assertQueue(1, this->kRetries);

  auto counterPrefix = this->publisher_->getCounterPrefix();
  WITH_RETRIES_N(this->kRetries, {
    EXPECT_EVENTUALLY_TRUE(this->publisher_->isConnectedToServer());
    // In tests, we don't start the publisher threads
    fb303::ThreadCachedServiceData::get()->publishStats();
    EXPECT_EVENTUALLY_EQ(
        fb303::ServiceData::get()->getCounter(
            counterPrefix + ".disconnects.sum.60"),
        0);
  });
}
} // namespace facebook::fboss::fsdb::test
