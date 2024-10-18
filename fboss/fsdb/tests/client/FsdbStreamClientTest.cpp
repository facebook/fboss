// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include "fboss/fsdb/client/FsdbStreamClient.h"
#include <gtest/gtest.h>
#include "fboss/fsdb/tests/client/FsdbTestClients.h"
#include "fboss/fsdb/tests/utils/FsdbTestServer.h"
#include "fboss/lib/CommonUtils.h"
#include "fboss/lib/thrift_service_client/ConnectionOptions.h"

#include <folly/io/async/ScopedEventBaseThread.h>

namespace facebook::fboss::fsdb::test {

class FsdbStreamClientTest : public ::testing::Test {
 public:
  void SetUp() override {
    fsdbTestServer_ = std::make_unique<FsdbTestServer>();
    streamEvbThread_ = std::make_unique<folly::ScopedEventBaseThread>();
    connRetryEvbThread_ = std::make_unique<folly::ScopedEventBaseThread>();
    streamClient_ = std::make_unique<TestFsdbStreamClient>(
        streamEvbThread_->getEventBase(), connRetryEvbThread_->getEventBase());
  }
  void TearDown() override {
    fsdbTestServer_.reset();
    streamClient_.reset();
    streamEvbThread_.reset();
    connRetryEvbThread_.reset();
  }

 protected:
  static auto constexpr kRetryInterval = 60;
  std::unique_ptr<FsdbTestServer> fsdbTestServer_;
  std::unique_ptr<folly::ScopedEventBaseThread> streamEvbThread_;
  std::unique_ptr<folly::ScopedEventBaseThread> connRetryEvbThread_;
  std::unique_ptr<TestFsdbStreamClient> streamClient_;
};

TEST_F(FsdbStreamClientTest, connectAndCancel) {
  streamClient_->setConnectionOptions(
      utils::ConnectionOptions("::1", fsdbTestServer_->getFsdbPort()));
  WITH_RETRIES(ASSERT_EVENTUALLY_TRUE(streamClient_->isConnectedToServer()));
  EXPECT_EQ(
      *streamClient_->lastStateUpdateSeen(),
      FsdbStreamClient::State::CONNECTED);
  streamClient_->cancel();
  WITH_RETRIES(ASSERT_EVENTUALLY_FALSE(streamClient_->isConnectedToServer()));
  EXPECT_EQ(
      *streamClient_->lastStateUpdateSeen(),
      FsdbStreamClient::State::CANCELLED);
}

TEST_F(FsdbStreamClientTest, multipleStreamClientsOnSameEvb) {
  streamClient_->setConnectionOptions(
      utils::ConnectionOptions("::1", fsdbTestServer_->getFsdbPort()));
  auto streamClient2 = std::make_unique<TestFsdbStreamClient>(
      streamEvbThread_->getEventBase(), connRetryEvbThread_->getEventBase());
  streamClient2->setConnectionOptions(
      utils::ConnectionOptions("::1", fsdbTestServer_->getFsdbPort()));
  WITH_RETRIES(ASSERT_EVENTUALLY_TRUE(streamClient_->isConnectedToServer()));
  WITH_RETRIES(ASSERT_EVENTUALLY_TRUE(streamClient2->isConnectedToServer()));
  EXPECT_EQ(
      *streamClient_->lastStateUpdateSeen(),
      FsdbStreamClient::State::CONNECTED);
  EXPECT_EQ(
      *streamClient2->lastStateUpdateSeen(),
      FsdbStreamClient::State::CONNECTED);
  streamClient_->cancel();
  WITH_RETRIES(ASSERT_EVENTUALLY_FALSE(streamClient_->isConnectedToServer()));
  EXPECT_EQ(
      *streamClient_->lastStateUpdateSeen(),
      FsdbStreamClient::State::CANCELLED);
  streamClient2->cancel();
  WITH_RETRIES(ASSERT_EVENTUALLY_FALSE(streamClient2->isConnectedToServer()));
  EXPECT_EQ(
      *streamClient2->lastStateUpdateSeen(),
      FsdbStreamClient::State::CANCELLED);
}

TEST_F(FsdbStreamClientTest, reconnect) {
  streamClient_->setConnectionOptions(
      utils::ConnectionOptions("::1", fsdbTestServer_->getFsdbPort()));
  WITH_RETRIES(ASSERT_EVENTUALLY_TRUE(streamClient_->isConnectedToServer()));
  EXPECT_EQ(
      *streamClient_->lastStateUpdateSeen(),
      FsdbStreamClient::State::CONNECTED);
  // Break server, assert for disconnect
  fsdbTestServer_.reset();
  WITH_RETRIES(ASSERT_EVENTUALLY_FALSE(streamClient_->isConnectedToServer()));
  EXPECT_EQ(
      *streamClient_->lastStateUpdateSeen(),
      FsdbStreamClient::State::DISCONNECTED);
  // Setup server again and assert for reconnect
  fsdbTestServer_ = std::make_unique<FsdbTestServer>();
  // Need to update server address since we are binding to ephemeral port
  // which will change on server recreate
  streamClient_->setConnectionOptions(
      utils::ConnectionOptions("::1", fsdbTestServer_->getFsdbPort()), true);
  WITH_RETRIES(ASSERT_EVENTUALLY_TRUE(streamClient_->isConnectedToServer()));
  EXPECT_EQ(
      *streamClient_->lastStateUpdateSeen(),
      FsdbStreamClient::State::CONNECTED);
}
} // namespace facebook::fboss::fsdb::test
