// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <gtest/gtest.h>
#include <latch>

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
        return std::find_if(
                   subscribers.begin(),
                   subscribers.end(),
                   [&](const auto& subscriber) {
                     return subscriber.second.subscriberId() == subId;
                   }) != subscribers.end();
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

} // namespace facebook::fboss::fsdb::test
