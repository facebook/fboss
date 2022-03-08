// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include "fboss/fsdb/client/FsdbStreamClient.h"
#include "fboss/fsdb/Flags.h"
#include "fboss/lib/CommonUtils.h"

#include <folly/experimental/coro/AsyncGenerator.h>
#include <folly/experimental/coro/AsyncPipe.h>
#include <folly/io/async/ScopedEventBaseThread.h>
#include <folly/logging/xlog.h>
#include <gtest/gtest.h>
#include <algorithm>
#include <atomic>

namespace facebook::fboss::fsdb::test {

class TestFsdbStreamClient : public FsdbStreamClient {
 public:
  TestFsdbStreamClient(folly::EventBase* streamEvb, folly::EventBase* timerEvb)
      : FsdbStreamClient(
            "test_fsdb_client",
            streamEvb,
            timerEvb,
            [this](auto oldState, auto newState) {
              EXPECT_NE(oldState, newState);
              lastStateUpdateSeen_ = newState;
            }) {}

  ~TestFsdbStreamClient() {
    cancel();
  }
#if FOLLY_HAS_COROUTINES
  folly::coro::Task<void> serviceLoop() override {
    try {
      auto [gen, pipe] = folly::coro::AsyncPipe<int>::create();
      pipe.write(1);
      while (auto intgen = co_await gen.next()) {
        if (isCancelled()) {
          XLOG(DBG2) << " Detected cancellation";
          break;
        }
      }
    } catch (const folly::OperationCancelled&) {
      XLOG(DBG2) << "Packet Stream Operation cancelled";
    } catch (const std::exception& ex) {
      XLOG(ERR) << clientId() << " Server error: " << folly::exceptionStr(ex);
      setState(State::DISCONNECTED);
    }
    XLOG(DBG2) << "Client Cancellation Completed";
    co_return;
  }
#endif
  std::optional<FsdbStreamClient::State> lastStateUpdateSeen() const {
    return lastStateUpdateSeen_.load();
  }
  void markConnected() {
    setState(State::CONNECTED);
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
    checkWithRetry(
        [&]() {
          return std::all_of(
              clients.begin(),
              clients.end(),
              [expectRunning](const auto& streamClient) {
                return streamClient->serviceLoopRunning() == expectRunning;
              });
        },
        kRetries);
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
  streamClient_->setServerToConnect("::1", FLAGS_fsdbPort);
  streamClient_->markConnected();
  EXPECT_EQ(
      *streamClient_->lastStateUpdateSeen(),
      FsdbStreamClient::State::CONNECTED);
  verifyServiceLoopRunning(true);
  streamClient_->cancel();
  EXPECT_EQ(
      *streamClient_->lastStateUpdateSeen(),
      FsdbStreamClient::State::CANCELLED);
  verifyServiceLoopRunning(false);
}
TEST_F(StreamClientTest, multipleStreamClientsOnSameEvb) {
  auto streamClient2 = std::make_unique<TestFsdbStreamClient>(
      streamEvbThread_->getEventBase(), connRetryEvbThread_->getEventBase());
  streamClient_->markConnected();
  streamClient2->markConnected();
  verifyServiceLoopRunning(true, {streamClient_.get(), streamClient2.get()});
  streamClient_->cancel();
  streamClient2->cancel();
  verifyServiceLoopRunning(false, {streamClient_.get(), streamClient2.get()});
}
} // namespace facebook::fboss::fsdb::test
