// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include <gtest/gtest.h>
#include "fboss/fsdb/client/Client.h"
#include "fboss/fsdb/client/FsdbDeltaPublisher.h"
#include "fboss/fsdb/client/FsdbStatePublisher.h"
#include "fboss/fsdb/if/FsdbModel.h"
#include "fboss/fsdb/tests/client/FsdbTestClients.h"
#include "fboss/fsdb/tests/utils/FsdbTestServer.h"
#include "fboss/lib/CommonUtils.h"

#include <fb303/ServiceData.h>
#include <folly/coro/BlockingWait.h>
#include <folly/io/async/ScopedEventBaseThread.h>

#include <thrift/lib/cpp2/protocol/Serializer.h>
namespace {
constexpr auto kPublisherId = "fsdb_test_publisher";
const std::vector<std::string> kPublishRoot{"agent"};
} // namespace
namespace facebook::fboss::fsdb::test {

template <typename TestParam>
class FsdbPublisherTest : public ::testing::Test {
  using PublisherT = typename TestParam::PublisherT;
  using PubUnitT = typename TestParam::PubUnitT;

 public:
  void SetUp() override {
    fsdbTestServer_ = std::make_unique<FsdbTestServer>();
    streamEvbThread_ = std::make_unique<folly::ScopedEventBaseThread>();
    connRetryEvbThread_ = std::make_unique<folly::ScopedEventBaseThread>();
    publisher_ = std::make_unique<PublisherT>(
        kPublisherId,
        kPublishRoot,
        streamEvbThread_->getEventBase(),
        connRetryEvbThread_->getEventBase(),
        TestParam::PubSubStats);
  }
  void TearDown() override {
    if (fsdbTestServer_) {
      publisher_->cancel();
      WITH_RETRIES(EXPECT_EVENTUALLY_FALSE(publisher_->isConnectedToServer());
                   EXPECT_EVENTUALLY_FALSE(getPublisherRootMetadata()););
      fsdbTestServer_.reset();
    }
    publisher_.reset();
    streamEvbThread_.reset();
    connRetryEvbThread_.reset();
  }

 protected:
  bool pubSubStats() const {
    return TestParam::PubSubStats;
  }
  bool isDelta() const {
    return std::is_same_v<PubUnitT, OperDelta>;
  }
  void setupConnection(bool updateServerPort = false) {
    publisher_->setServerOptions(
        FsdbStreamClient::ServerOptions("::1", fsdbTestServer_->getFsdbPort()),
        updateServerPort);
    WITH_RETRIES_N(
        kRetries, auto metadata = getPublisherRootMetadata();
        EXPECT_EVENTUALLY_TRUE(publisher_->isConnectedToServer());
        EXPECT_EVENTUALLY_TRUE(metadata && metadata->numOpenConnections == 1));
  }
  std::optional<FsdbOperTreeMetadata> getPublisherRootMetadata() const {
    return fsdbTestServer_->getPublisherRootMetadata(
        *kPublishRoot.begin(), pubSubStats());
  }

  void publishAgentConfig(const cfg::AgentConfig& config) {
    if constexpr (std::is_same_v<PubUnitT, OperDelta>) {
      publisher_->write(makeDelta(config));
    } else {
      publisher_->write(makeState(config));
    }
  }

  void publishAndVerifyConfig(
      const std::map<std::string, std::string>& cmdLinArgs) {
    auto config = makeAgentConfig(cmdLinArgs);
    publishAgentConfig(config);
    WITH_RETRIES_N(kRetries, {
      auto fsdbOperRoot = fsdbTestServer_->serviceHandler().operRootExpensive();
      auto metadata = getPublisherRootMetadata();
      EXPECT_EVENTUALLY_EQ(*fsdbOperRoot.agent()->config(), config);
      EXPECT_EVENTUALLY_TRUE(
          metadata && *metadata->operMetadata.lastConfirmedAt() > 0);
    });
  }
  void publishPortStats(
      const folly::F14FastMap<std::string, HwPortStats>& portStats) {
    if constexpr (std::is_same_v<PubUnitT, OperDelta>) {
      publisher_->write(makeDelta(portStats));
    } else {
      publisher_->write(makeState(portStats));
    }
  }
  void publishAndVerifyStats(int64_t inBytes) {
    auto portStats = makePortStats(inBytes);
    publishPortStats(portStats);
    WITH_RETRIES_N(kRetries, {
      auto fsdbOperRoot =
          fsdbTestServer_->serviceHandler().operStatsRootExpensive();
      auto metadata = getPublisherRootMetadata();
      EXPECT_EVENTUALLY_EQ(*fsdbOperRoot.agent()->hwPortStats(), portStats);
      ASSERT_EVENTUALLY_TRUE(
          metadata && *metadata->operMetadata.lastConfirmedAt() > 0);
    });
  }
  static auto constexpr kRetries = 60;
  std::unique_ptr<FsdbTestServer> fsdbTestServer_;
  std::unique_ptr<folly::ScopedEventBaseThread> streamEvbThread_;
  std::unique_ptr<folly::ScopedEventBaseThread> connRetryEvbThread_;
  std::unique_ptr<PublisherT> publisher_;
};

using TestTypes = ::testing::Types<
    DeltaPubSubForState,
    StatePubSubForState,
    DeltaPubSubForStats,
    StatePubSubForStats>;
TYPED_TEST_SUITE(FsdbPublisherTest, TestTypes);

TYPED_TEST(FsdbPublisherTest, connectAndCancel) {
  if (this->pubSubStats()) {
    EXPECT_EQ(
        this->publisher_->getCounterPrefix(),
        this->isDelta() ? "fsdbDeltaStatPublisher_agent"
                        : "fsdbPathStatPublisher_agent");
  } else {
    EXPECT_EQ(
        this->publisher_->getCounterPrefix(),
        this->isDelta() ? "fsdbDeltaStatePublisher_agent"
                        : "fsdbPathStatePublisher_agent");
  }
  this->setupConnection();
  this->publisher_->cancel();
  WITH_RETRIES_N(
      this->kRetries,
      EXPECT_EVENTUALLY_FALSE(this->publisher_->isConnectedToServer()));
}
TYPED_TEST(FsdbPublisherTest, publish) {
  this->setupConnection();
  if (this->pubSubStats()) {
    this->publishAndVerifyStats(1);
  } else {
    this->publishAndVerifyConfig({{"foo", "bar"}});
  }
}

TYPED_TEST(FsdbPublisherTest, reconnect) {
  this->setupConnection();

  auto counterPrefix = this->publisher_->getCounterPrefix();
  EXPECT_EQ(
      fb303::ServiceData::get()->getCounter(counterPrefix + ".connected"), 1);
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
    EXPECT_EVENTUALLY_FALSE(this->publisher_->isConnectedToServer());
    // In tests, we don't start the publisher threads
    fb303::ThreadCachedServiceData::get()->publishStats();
    EXPECT_EVENTUALLY_TRUE(
        fb303::ServiceData::get()->getCounter(
            counterPrefix + ".disconnects.sum.60") >= 1);
  });
  EXPECT_EQ(
      fb303::ServiceData::get()->getCounter(counterPrefix + ".connected"), 0);
  // Setup server again and assert for reconnect
  this->fsdbTestServer_ = std::make_unique<FsdbTestServer>();
  // Need to update server address since we are binding to ephemeral port
  // which will change on server recreate
  this->setupConnection(true);
  EXPECT_EQ(
      fb303::ServiceData::get()->getCounter(counterPrefix + ".connected"), 1);
  // Publish new delta after reconnect, assert that delta gets through
  if (this->pubSubStats()) {
    this->publishAndVerifyStats(10);
  } else {
    this->publishAndVerifyConfig({{"bar", "baz"}});
  }
  auto metadata = this->getPublisherRootMetadata();
  ASSERT_EQ(metadata->numOpenConnections, 1);
}

TYPED_TEST(FsdbPublisherTest, publisherNotConnected) {
  this->setupConnection();
  // Break server, assert for disconnect
  this->fsdbTestServer_.reset();
  // Need to publish something in order to detect that server has
  // gone away
  if (this->pubSubStats()) {
    this->publishPortStats(makePortStats(1));
  } else {
    this->publishAgentConfig(makeAgentConfig({{"foo", "bar"}}));
  }
  WITH_RETRIES_N(
      this->kRetries,
      EXPECT_EVENTUALLY_FALSE(this->publisher_->isConnectedToServer()));
  // Since we change state to CONNECTED immediately after creating
  // client. The following write races b/w creating the queue on
  // transition to CONNECTED and queue setup. Once we defer transition
  // to connected after stream is setup D35757975, change this to
  // always expecting a FsdbException
  try {
    if (this->pubSubStats()) {
      this->publishPortStats(makePortStats(1));
    } else {
      this->publishAgentConfig(makeAgentConfig({{"foo", "bar"}}));
    }
  } catch (const FsdbException&) {
  }
  // Even though there is no server, we should detect a disconnect and
  // clear out the publisher queue
  WITH_RETRIES(EXPECT_EVENTUALLY_EQ(this->publisher_->queueSize(), 0););
}

TYPED_TEST(FsdbPublisherTest, getState) {
  this->setupConnection();

  folly::ScopedEventBaseThread streamEvb;
  std::unique_ptr<apache::thrift::Client<FsdbService>> client;
  streamEvb.getEventBase()->runInEventBaseThreadAndWait([&]() {
    client = Client::getClient(
        folly::SocketAddress(
            "::1", this->fsdbTestServer_->getFsdbPort()) /* dstAddr */,
        std::nullopt /* srcAddr */,
        std::nullopt /* tos */,
        false /* plaintextClient */,
        streamEvb.getEventBase());
  });

  auto getThrift = [&](auto& req) {
    if (this->pubSubStats()) {
      folly::coro::blockingWait(client->co_getOperStats(req));
    } else {
      folly::coro::blockingWait(client->co_getOperState(req));
    }
  };
  OperGetRequest req;
  // Empty path
  EXPECT_THROW(getThrift(req), FsdbException);
  // Data not ready
  req.path()->raw() = kPublishRoot;
  EXPECT_THROW(getThrift(req), FsdbException);
  if (this->pubSubStats()) {
    this->publishAndVerifyStats(10);
  } else {
    this->publishAndVerifyConfig({{"bar", "baz"}});
  }
  // Success
  EXPECT_NO_THROW(getThrift(req));
  // Reset in the correct evb before those evbs go away
  streamEvb.getEventBase()->runInEventBaseThreadAndWait(
      [&] { client.reset(); });
}

template <typename TestParam>
class FsdbPublisherGRTest : public FsdbPublisherTest<TestParam> {};

using GRTestTypes = ::testing::Types<
    DeltaPubSubForState,
    StatePubSubForState,
    DeltaPubSubForStats,
    StatePubSubForStats>;
TYPED_TEST_SUITE(FsdbPublisherGRTest, GRTestTypes);

TYPED_TEST(FsdbPublisherGRTest, grDisconnect) {
  this->setupConnection();
  WITH_RETRIES_N(
      this->kRetries,
      EXPECT_EVENTUALLY_TRUE(this->publisher_->isConnectedToServer()));
  auto status = this->publisher_->disconnectForGR();
  EXPECT_TRUE(status);
  WITH_RETRIES_N(
      this->kRetries,
      EXPECT_EVENTUALLY_FALSE(this->publisher_->isConnectedToServer()));
}

} // namespace facebook::fboss::fsdb::test
