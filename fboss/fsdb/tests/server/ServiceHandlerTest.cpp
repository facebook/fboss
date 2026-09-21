// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <fb303/ServiceData.h>
#include <gtest/gtest.h>
#include <latch>

#include "fboss/fsdb/common/Utils.h"
#include "fboss/fsdb/if/gen-cpp2/fsdb_common_constants.h"
#include "fboss/fsdb/tests/utils/FsdbTestServer.h"
#include "fboss/lib/CommonUtils.h"
#include "fboss/lib/thrift_service_client/ThriftServiceClient.h"

namespace facebook::fboss::fsdb::test {

class ServiceHandlerTest : public ::testing::Test {
 public:
  void SetUp() override {
    fsdb_ = std::make_unique<FsdbTestServer>();
  }

  void TearDown() override {
    fsdb_.reset();
  }

  std::unique_ptr<apache::thrift::Client<FsdbService>> createClient(
      folly::EventBase* evb) {
    return utils::createFsdbClient(
        utils::ConnectionOptions("::1", getFsdbPort()), evb);
  }

  uint16_t getFsdbPort() const {
    return fsdb_->getFsdbPort();
  }

  OperSubRequest createRequest(const std::string& subId) {
    OperPath operPath;
    operPath.raw() = {"agent"};
    OperSubRequest req;
    req.subscriberId() = subId;
    req.path() = operPath;
    return req;
  }

  SubRequest createPatchRequest(
      const std::string& subId,
      bool forceSubscribe = false) {
    SubRequest req;
    RawOperPath p;
    p.path() = {"agent"};
    req.paths() = {{0, p}};
    req.clientId()->client() = FsdbClient::AGENT;
    req.clientId()->instanceId() = subId;
    req.forceSubscribe() = forceSubscribe;
    return req;
  }

  SubRequest createExtendedPatchRequest(const std::string& subId) {
    OperPathElem elem;
    elem.raw() = "agent";
    ExtendedOperPath path;
    path.path() = {elem};
    SubRequest req;
    req.extPaths() = {{0, path}};
    req.clientId()->client() = FsdbClient::AGENT;
    req.clientId()->instanceId() = subId;
    return req;
  }

  std::unique_ptr<FsdbTestServer> fsdb_;
};

TEST_F(ServiceHandlerTest, testSubscriberRemovedOnFailure) {
  // Subscribe with many threads at once to induce trasport level errors
  std::vector<folly::ScopedEventBaseThread> threads(500);
  std::latch latch(threads.size());
  for (int i = 0; i < threads.size(); i++) {
    auto subId = fmt::format("test-sub-{}", i);
    threads[i].getEventBase()->runInEventBaseThread([this, subId, &latch]() {
      folly::EventBase evb;
      auto isSubscribed = [this, &subId]() {
        auto subscribers = fsdb_->serviceHandler().getActiveSubscriptions();
        for (const auto& it : subscribers) {
          for (const auto& subscription : it.second) {
            if (subscription.subscriberId() == subId) {
              return true;
            }
          }
        }
        return false;
      };
      try {
        auto client = createClient(&evb);
        auto result =
            client->sync_subscribeOperStatsDelta(createRequest(subId));
        // if we were able to connect, server should have the subscriber
        EXPECT_TRUE(isSubscribed());
      } catch (const std::exception&) {
        // Expect some timeout errors, server should not have registered
        // subscriber
        EXPECT_FALSE(isSubscribed());
      }
      latch.count_down();
    });
  }
  latch.wait();
  // server should still clean up all subscribers
  WITH_RETRIES(EXPECT_EVENTUALLY_EQ(
      fsdb_->serviceHandler().getActiveSubscriptions().size(), 0));
}

TEST_F(ServiceHandlerTest, testSubscriberInfo) {
  folly::EventBase evb;
  auto client = createClient(&evb);
  auto sub1 = client->sync_subscribeOperStatsDelta(createRequest("test-sub-1"));
  SubscriberIdToOperSubscriberInfos subInfos;
  {
    auto client2 = createClient(&evb);
    auto sub2 =
        client2->sync_subscribeOperStatsDelta(createRequest("test-sub-2"));
    // empty sub ids should return both subscribers
    WITH_RETRIES({
      client2->sync_getAllOperSubscriberInfos(subInfos);
      EXPECT_EVENTUALLY_EQ(subInfos.size(), 2);
    });

    client2->sync_getOperSubscriberInfos(
        subInfos, SubscriberIds({"test-sub-1"}));
    EXPECT_EQ(subInfos.size(), 1);
  }

  // subscriber 2 should be gone
  WITH_RETRIES({
    client->sync_getAllOperSubscriberInfos(subInfos);
    EXPECT_EVENTUALLY_EQ(subInfos.size(), 1);
  });
}

// Regression test for `fboss2 show fsdb subscribers` to correctly report
// isStats flag for extended State and Stats subscriptions.
TEST_F(ServiceHandlerTest, extendedSubscriptionIsStatsAttribute) {
  folly::EventBase evb;
  auto client = createClient(&evb);
  auto stateSub = client->sync_subscribeStateExtended(
      createExtendedPatchRequest("ext-state-sub"));
  auto statsSub = client->sync_subscribeStatsExtended(
      createExtendedPatchRequest("ext-stats-sub"));

  SubscriberIdToOperSubscriberInfos subInfos;
  WITH_RETRIES({
    client->sync_getAllOperSubscriberInfos(subInfos);
    EXPECT_EVENTUALLY_EQ(subInfos.size(), 2);
  });

  ASSERT_EQ(subInfos.count("ext-state-sub"), 1);
  ASSERT_EQ(subInfos.at("ext-state-sub").size(), 1);
  const auto& stateInfo = subInfos.at("ext-state-sub").at(0);
  EXPECT_FALSE(*stateInfo.isStats());
  EXPECT_TRUE(stateInfo.extendedPaths().has_value());

  ASSERT_EQ(subInfos.count("ext-stats-sub"), 1);
  ASSERT_EQ(subInfos.at("ext-stats-sub").size(), 1);
  const auto& statsInfo = subInfos.at("ext-stats-sub").at(0);
  EXPECT_TRUE(*statsInfo.isStats());
  EXPECT_TRUE(statsInfo.extendedPaths().has_value());
}

TEST_F(ServiceHandlerTest, subscribeDup) {
  folly::EventBase evb;
  auto client = createClient(&evb);
  auto sub1 = client->sync_subscribeState(createPatchRequest("test-sub-1"));
  EXPECT_THROW(
      client->sync_subscribeState(createPatchRequest("test-sub-1")),
      fsdb::FsdbException);

  auto sub2 =
      client->sync_subscribeState(createPatchRequest("test-sub-2", true));
  EXPECT_NO_THROW(
      client->sync_subscribeState(createPatchRequest("test-sub-2", true)));
}

TEST_F(ServiceHandlerTest, verifyActiveSubscriptions) {
  SubscriberIdToOperSubscriberInfos subInfos;
  SubscriberIds idsToQuery = {"test-sub-1"};
  folly::EventBase evb;

  auto client = createClient(&evb);
  auto sub1 =
      client->sync_subscribeState(createPatchRequest("test-sub-1", true));

  auto countActiveSubs = [](auto& subInfos) {
    int count{0};
    for (const auto& [_, info] : subInfos) {
      count += info.size();
    }
    return count;
  };

  // forceSubscribe, and verify activeSubscriptions
  {
    auto client2 = createClient(&evb);
    auto sub2 =
        client2->sync_subscribeState(createPatchRequest("test-sub-1", true));
    int expectedActiveSubscriptions = 2;
    WITH_RETRIES({
      client2->sync_getOperSubscriberInfos(subInfos, idsToQuery);
      EXPECT_EVENTUALLY_EQ(
          countActiveSubs(subInfos), expectedActiveSubscriptions);
    });
  }

  // after sub2 is gone, should still see sub1
  int expectedActiveSubscriptions = 1;
  WITH_RETRIES({
    client->sync_getOperSubscriberInfos(subInfos, idsToQuery);
    EXPECT_EVENTUALLY_EQ(
        countActiveSubs(subInfos), expectedActiveSubscriptions);
  });
}

TEST_F(ServiceHandlerTest, testPublisherInfoConnectedAt) {
  // Verify that getActivePublishers entries include stream state
  // with connectedAt. Full publisher lifecycle testing is done
  // in FsdbPubSubTest.
  auto publishers = fsdb_->serviceHandler().getActivePublishers();
  // No publishers connected yet
  EXPECT_EQ(publishers.size(), 0);
}

class ExpectedSubscriptionsTest : public ::testing::Test {
 protected:
  void TearDown() override {
    fsdb_.reset();
    // fb303::ServiceData is a process-global singleton; counters
    // registered by setupServer() persist across TEST_F instances and
    // would otherwise leak into notConfigured_NoCounter (which asserts
    // absence). Clear every fsdb expected_subscriptions counter.
    auto* svc = fb303::ServiceData::get();
    const auto prefix = folly::to<std::string>(
        fsdb_common_constants::kFsdbServiceHandlerNativeStatsPrefix(),
        "expected_subscriptions.");
    std::map<std::string, int64_t> counters;
    svc->getCounters(counters);
    for (const auto& [name, _] : counters) {
      if (name.starts_with(prefix)) {
        svc->clearCounter(name);
      }
    }
  }

  void setupServer(
      const std::map<std::string, ExpectedSubscriptionType>&
          subscribersWithExpected) {
    Config thriftCfg;
    auto& subs = thriftCfg.subscribers().value();
    for (const auto& [key, type] : subscribersWithExpected) {
      SubscriberConfig sc;
      sc.expectedSubscriptionType() = type;
      subs.emplace(key, sc);
    }
    auto cfg = std::make_shared<FsdbConfig>(std::move(thriftCfg));
    fsdb_ = std::make_unique<FsdbTestServer>(cfg);
  }

  std::unique_ptr<apache::thrift::Client<FsdbService>> createClient(
      folly::EventBase* evb) {
    return utils::createFsdbClient(
        utils::ConnectionOptions("::1", fsdb_->getFsdbPort()), evb);
  }

  std::string counterName(FsdbClient client) const {
    return folly::to<std::string>(
        fsdb_common_constants::kFsdbServiceHandlerNativeStatsPrefix(),
        "expected_subscriptions.",
        fsdbClient2string(client));
  }

  int64_t getCounter(FsdbClient client) const {
    return fb303::ServiceData::get()->getCounter(counterName(client));
  }

  SubRequest makeRequest(
      const std::string& instanceId,
      FsdbClient client = FsdbClient::AGENT) {
    SubRequest req;
    RawOperPath p;
    p.path() = {"agent"};
    req.paths() = {{0, p}};
    req.clientId()->client() = client;
    req.clientId()->instanceId() = instanceId;
    return req;
  }

  void waitForActiveSubscriptions(size_t expected) {
    WITH_RETRIES(
        EXPECT_EVENTUALLY_EQ(fsdb_->getActiveSubscriptions().size(), expected));
  }

  std::unique_ptr<FsdbTestServer> fsdb_;
};

TEST_F(ExpectedSubscriptionsTest, stateMode) {
  setupServer({{"agent", ExpectedSubscriptionType::STATE}});
  EXPECT_EQ(getCounter(FsdbClient::AGENT), 0);

  folly::EventBase evb;
  auto statsClient = createClient(&evb);
  auto statsSub = statsClient->sync_subscribeStats(makeRequest("agent"));
  waitForActiveSubscriptions(1);
  EXPECT_EQ(getCounter(FsdbClient::AGENT), 0);

  auto stateClient = createClient(&evb);
  auto stateSub = stateClient->sync_subscribeState(makeRequest("agent"));
  waitForActiveSubscriptions(2);
  WITH_RETRIES(EXPECT_EVENTUALLY_EQ(getCounter(FsdbClient::AGENT), 1));
}

TEST_F(ExpectedSubscriptionsTest, statsMode) {
  setupServer({{"agent", ExpectedSubscriptionType::STATS}});
  EXPECT_EQ(getCounter(FsdbClient::AGENT), 0);

  folly::EventBase evb;
  auto stateClient = createClient(&evb);
  auto stateSub = stateClient->sync_subscribeState(makeRequest("agent"));
  waitForActiveSubscriptions(1);
  EXPECT_EQ(getCounter(FsdbClient::AGENT), 0);

  auto statsClient = createClient(&evb);
  auto statsSub = statsClient->sync_subscribeStats(makeRequest("agent"));
  waitForActiveSubscriptions(2);
  WITH_RETRIES(EXPECT_EVENTUALLY_EQ(getCounter(FsdbClient::AGENT), 1));
}

TEST_F(ExpectedSubscriptionsTest, allMode_RequiresBothKinds) {
  setupServer({{"agent", ExpectedSubscriptionType::ALL}});
  EXPECT_EQ(getCounter(FsdbClient::AGENT), 0);

  folly::EventBase evb;
  auto stateClient = createClient(&evb);
  auto stateSub = stateClient->sync_subscribeState(makeRequest("agent"));
  waitForActiveSubscriptions(1);
  EXPECT_EQ(getCounter(FsdbClient::AGENT), 0);

  auto statsClient = createClient(&evb);
  auto statsSub = statsClient->sync_subscribeStats(makeRequest("agent"));
  waitForActiveSubscriptions(2);
  WITH_RETRIES(EXPECT_EVENTUALLY_EQ(getCounter(FsdbClient::AGENT), 1));
}

TEST_F(ExpectedSubscriptionsTest, allMode_DifferentInstancesDoNotCount) {
  // Wildcard config matches both "agent:A" and "agent:B"; both derive
  // FsdbClient::AGENT. ALL mode requires same full ClientId, so a state
  // sub for instance A and a stats sub for instance B must NOT flip the
  // gauge.
  setupServer({{"agent:", ExpectedSubscriptionType::ALL}});
  EXPECT_EQ(getCounter(FsdbClient::AGENT), 0);

  folly::EventBase evb;
  auto stateClient = createClient(&evb);
  auto stateA = stateClient->sync_subscribeState(makeRequest("agent:A"));
  auto statsClient = createClient(&evb);
  auto statsB = statsClient->sync_subscribeStats(makeRequest("agent:B"));
  waitForActiveSubscriptions(2);
  EXPECT_EQ(getCounter(FsdbClient::AGENT), 0);
}

TEST_F(ExpectedSubscriptionsTest, allMode_multipleSubscriberInstances) {
  // switch_agent runs core and cluster instances. ALL mode requires a
  // single subscriberId to hold both a state and a stats subscription.
  // A state sub on "switch_agent:core" and a stats sub on
  // "switch_agent:cluster" are split across two instances, so the gauge
  // stays 0. A third state sub on "switch_agent:cluster" completes the
  // state+stats pair for that instance and flips the gauge to 1.
  setupServer({{"switch_agent:", ExpectedSubscriptionType::ALL}});
  EXPECT_EQ(getCounter(FsdbClient::SWITCH_AGENT), 0);

  folly::EventBase evb;
  auto coreStateClient = createClient(&evb);
  auto coreState =
      coreStateClient->sync_subscribeState(makeRequest("switch_agent:core"));
  auto clusterStatsClient = createClient(&evb);
  auto clusterStats = clusterStatsClient->sync_subscribeStats(
      makeRequest("switch_agent:cluster"));
  waitForActiveSubscriptions(2);
  EXPECT_EQ(getCounter(FsdbClient::SWITCH_AGENT), 0);

  auto clusterStateClient = createClient(&evb);
  auto clusterState = clusterStateClient->sync_subscribeState(
      makeRequest("switch_agent:cluster"));
  waitForActiveSubscriptions(3);
  WITH_RETRIES(EXPECT_EVENTUALLY_EQ(getCounter(FsdbClient::SWITCH_AGENT), 1));
}

TEST_F(ExpectedSubscriptionsTest, allMode_UpdateOnDisconnect) {
  setupServer({{"agent", ExpectedSubscriptionType::ALL}});
  EXPECT_EQ(getCounter(FsdbClient::AGENT), 0);

  folly::EventBase evb;
  auto statsClient = createClient(&evb);
  auto statsSub = statsClient->sync_subscribeStats(makeRequest("agent"));
  {
    auto stateClient = createClient(&evb);
    auto stateSub = stateClient->sync_subscribeState(makeRequest("agent"));
    waitForActiveSubscriptions(2);
    WITH_RETRIES(EXPECT_EVENTUALLY_EQ(getCounter(FsdbClient::AGENT), 1));
  }
  // state sub destroyed: stats remains, gauge returns to 0.
  waitForActiveSubscriptions(1);
  WITH_RETRIES(EXPECT_EVENTUALLY_EQ(getCounter(FsdbClient::AGENT), 0));

  auto stateClient2 = createClient(&evb);
  auto stateSub2 = stateClient2->sync_subscribeState(makeRequest("agent"));
  waitForActiveSubscriptions(2);
  WITH_RETRIES(EXPECT_EVENTUALLY_EQ(getCounter(FsdbClient::AGENT), 1));
}

TEST_F(ExpectedSubscriptionsTest, wildcardConfigKey_RegistersCounter) {
  // Both prefix-wildcard (":agent") and suffix-wildcard ("agent:") config
  // keys must register a counter for FsdbClient::AGENT at init. Without
  // the leading/trailing-colon strip, `:agent` would parse to UNSPECIFIED
  // and the counter would never be registered.
  setupServer(
      {{":agent", ExpectedSubscriptionType::ALL},
       {"openr:", ExpectedSubscriptionType::ALL}});
  std::map<std::string, int64_t> counters;
  fb303::ServiceData::get()->getCounters(counters);
  EXPECT_EQ(counters.count(counterName(FsdbClient::AGENT)), 1);
  EXPECT_EQ(counters.at(counterName(FsdbClient::AGENT)), 0);
  EXPECT_EQ(counters.count(counterName(FsdbClient::OPENR)), 1);
  EXPECT_EQ(counters.at(counterName(FsdbClient::OPENR)), 0);
}

TEST_F(ExpectedSubscriptionsTest, prefixWildcard_RuntimeUpdate) {
  // Prefix-wildcard config ":agent" matches subscribers with non-canonical
  // ids like "node:agent". The runtime gauge update path must derive
  // FsdbClient::AGENT from the matched config key (not from the runtime
  // subscriberId, which would parse client="node" -> UNSPECIFIED).
  setupServer({{":agent", ExpectedSubscriptionType::ALL}});
  EXPECT_EQ(getCounter(FsdbClient::AGENT), 0);

  folly::EventBase evb;
  auto stateClient = createClient(&evb);
  auto stateSub = stateClient->sync_subscribeState(makeRequest("node:agent"));
  waitForActiveSubscriptions(1);
  EXPECT_EQ(getCounter(FsdbClient::AGENT), 0);

  auto statsClient = createClient(&evb);
  auto statsSub = statsClient->sync_subscribeStats(makeRequest("node:agent"));
  waitForActiveSubscriptions(2);
  WITH_RETRIES(EXPECT_EVENTUALLY_EQ(getCounter(FsdbClient::AGENT), 1));
}

TEST_F(ExpectedSubscriptionsTest, registeredSubsCacheMatchedConfigKey) {
  // Verify configKey is populated at register time so
  // updateExpectedSubscriptionCounter() can do an O(1) field compare per
  // active sub. Covers both literal ("agent") and wildcard (":agent")
  // config keys, plus an unmatched id ("foo") that must remain unset.
  setupServer(
      {{"agent", ExpectedSubscriptionType::STATE},
       {":openr", ExpectedSubscriptionType::STATE}});

  folly::EventBase evb;
  auto c1 = createClient(&evb);
  auto s1 = c1->sync_subscribeState(makeRequest("agent"));
  auto c2 = createClient(&evb);
  auto s2 = c2->sync_subscribeState(makeRequest("node:openr"));
  auto c3 = createClient(&evb);
  auto s3 = c3->sync_subscribeState(makeRequest("foo"));
  waitForActiveSubscriptions(3);

  std::map<std::string, std::optional<std::string>> matchedBySubId;
  for (const auto& [_, subs] : fsdb_->getActiveSubscriptions()) {
    for (const auto& sub : subs) {
      matchedBySubId[*sub.subscriberId()] = sub.configKey().has_value()
          ? std::optional<std::string>(*sub.configKey())
          : std::nullopt;
    }
  }
  EXPECT_EQ(matchedBySubId.at("agent"), std::optional<std::string>("agent"));
  EXPECT_EQ(
      matchedBySubId.at("node:openr"), std::optional<std::string>(":openr"));
  EXPECT_EQ(matchedBySubId.at("foo"), std::nullopt);
}

TEST_F(ExpectedSubscriptionsTest, notConfigured_NoCounter) {
  // Empty subscribers map: no expected_subscriptions counter is registered
  // and no crash should occur on subscribe/unsubscribe.
  fsdb_ = std::make_unique<FsdbTestServer>(std::make_shared<FsdbConfig>());

  folly::EventBase evb;
  auto client = createClient(&evb);
  auto sub = client->sync_subscribeState(makeRequest("agent"));
  waitForActiveSubscriptions(1);

  std::map<std::string, int64_t> counters;
  fb303::ServiceData::get()->getCounters(counters);
  EXPECT_EQ(counters.count(counterName(FsdbClient::AGENT)), 0);
}

} // namespace facebook::fboss::fsdb::test
