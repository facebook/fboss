// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include <gtest/gtest.h>

#include "fboss/fsdb/client/FsdbStreamClient.h"
#include "fboss/fsdb/common/Flags.h"
#include "fboss/lib/CommonUtils.h"

#include <fb303/ServiceData.h>
#include <folly/coro/AsyncGenerator.h>
#include <folly/coro/AsyncPipe.h>
#include <folly/io/async/ScopedEventBaseThread.h>
#include <folly/logging/xlog.h>
#include <algorithm>
#include <atomic>

namespace facebook::fboss::fsdb {

class TestFsdbStreamClient : public FsdbStreamClient {
 public:
  TestFsdbStreamClient(folly::EventBase* streamEvb, folly::EventBase* timerEvb)
      : FsdbStreamClient(
            "test_fsdb_client",
            streamEvb,
            timerEvb,
            "test_fsdb_client",
            false,
            [this](auto oldState, auto newState) {
              EXPECT_NE(oldState, newState);
              lastStateUpdateSeen_ = newState;
            }) {}

  ~TestFsdbStreamClient() {
    cancel();
  }
#if FOLLY_HAS_COROUTINES
  folly::coro::Task<StreamT> setupStream() override {
    co_return StreamT();
  }

  folly::coro::Task<void> serveStream(StreamT&& /* stream */) override {
    auto [gen, pipe] = folly::coro::AsyncPipe<int>::create();
    pipe.write(1);
    while (auto intgen = co_await gen.next()) {
      if (isCancelled()) {
        XLOG(DBG2) << " Detected cancellation";
        break;
      }
    }
    co_return;
  }
#endif
  std::optional<FsdbStreamClient::State> lastStateUpdateSeen() const {
    return lastStateUpdateSeen_.load();
  }
  void markConnecting() {
    setState(State::CONNECTING);
  }

 private:
  std::atomic<std::optional<FsdbStreamClient::State>> lastStateUpdateSeen_{
      std::nullopt};
};

class StreamClientTest : public ::testing::Test {
 public:
  void SetUp() override {
    streamEvbThread_ = std::make_unique<folly::ScopedEventBaseThread>();
    connRetryEvbThread_ = std::make_unique<folly::ScopedEventBaseThread>();
    streamClient_ = std::make_unique<TestFsdbStreamClient>(
        streamEvbThread_->getEventBase(), connRetryEvbThread_->getEventBase());
  }
  void TearDown() override {
    streamClient_.reset();
    streamEvbThread_.reset();
    connRetryEvbThread_.reset();
  }
  void verifyServiceLoopRunning(
      bool expectRunning,
      const std::vector<TestFsdbStreamClient*>& clients) const {
#if FOLLY_HAS_COROUTINES
    WITH_RETRIES_N(kRetries, {
      EXPECT_EVENTUALLY_TRUE(std::all_of(
          clients.begin(),
          clients.end(),
          [expectRunning](const auto& streamClient) {
            return streamClient->serviceLoopRunning() == expectRunning;
          }));
    });
#endif
  }
  void verifyServiceLoopRunning(bool expectRunning) const {
    verifyServiceLoopRunning(expectRunning, {streamClient_.get()});
  }

 protected:
  static auto constexpr kRetries = 60;
  std::unique_ptr<folly::ScopedEventBaseThread> streamEvbThread_;
  std::unique_ptr<folly::ScopedEventBaseThread> connRetryEvbThread_;
  std::unique_ptr<TestFsdbStreamClient> streamClient_;
};

TEST_F(StreamClientTest, connectAndCancel) {
  streamClient_->setServerOptions(
      FsdbStreamClient::ServerOptions("::1", FLAGS_fsdbPort));
  auto counterPrefix = streamClient_->getCounterPrefix();
  EXPECT_EQ(counterPrefix, "test_fsdb_client");
  EXPECT_EQ(
      fb303::ServiceData::get()->getCounter(counterPrefix + ".connected"), 0);
  streamClient_->markConnecting();
  WITH_RETRIES(
      { EXPECT_EVENTUALLY_TRUE(streamClient_->isConnectedToServer()); });
  EXPECT_EQ(
      fb303::ServiceData::get()->getCounter(counterPrefix + ".connected"), 1);
  EXPECT_EQ(
      *streamClient_->lastStateUpdateSeen(),
      FsdbStreamClient::State::CONNECTED);
  verifyServiceLoopRunning(true);
  streamClient_->cancel();
  EXPECT_EQ(
      *streamClient_->lastStateUpdateSeen(),
      FsdbStreamClient::State::CANCELLED);
  verifyServiceLoopRunning(false);
  EXPECT_EQ(
      fb303::ServiceData::get()->getCounter(counterPrefix + ".connected"), 0);
}

TEST_F(StreamClientTest, multipleStreamClientsOnSameEvb) {
  auto streamClient2 = std::make_unique<TestFsdbStreamClient>(
      streamEvbThread_->getEventBase(), connRetryEvbThread_->getEventBase());
  streamClient_->markConnecting();
  streamClient2->markConnecting();
  WITH_RETRIES(
      { EXPECT_EVENTUALLY_TRUE(streamClient_->isConnectedToServer()); });
  WITH_RETRIES(
      { EXPECT_EVENTUALLY_TRUE(streamClient2->isConnectedToServer()); });
  verifyServiceLoopRunning(true, {streamClient_.get(), streamClient2.get()});
  streamClient_->cancel();
  streamClient2->cancel();
  verifyServiceLoopRunning(false, {streamClient_.get(), streamClient2.get()});
}
} // namespace facebook::fboss::fsdb
