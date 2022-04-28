// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include "fboss/fsdb/Flags.h"
#include "fboss/fsdb/client/FsdbDeltaPublisher.h"
#include "fboss/lib/CommonUtils.h"

#include <folly/experimental/coro/AsyncGenerator.h>
#include <folly/experimental/coro/AsyncPipe.h>
#include <folly/io/async/ScopedEventBaseThread.h>
#include <folly/logging/xlog.h>
#include <gtest/gtest.h>
#include <algorithm>
#include <atomic>

namespace facebook::fboss::fsdb::test {

namespace {
class TestFsdbStreamPublisher : public FsdbPublisher<OperDelta> {
 public:
  TestFsdbStreamPublisher(
      folly::EventBase* streamEvb,
      folly::EventBase* timerEvb)
      : FsdbPublisher(
            "test_fsdb_client",
            {"agent"},
            streamEvb,
            timerEvb,
            false) {}

  ~TestFsdbStreamPublisher() override {
    cancel();
  }
#if FOLLY_HAS_COROUTINES
  folly::coro::Task<void> serviceLoop() override {
    auto gen = createGenerator();
    generatorStart_.wait();
    while (auto pubUnit = co_await gen.next()) {
      if (isCancelled()) {
        XLOG(DBG2) << " Detected cancellation";
        break;
      }
    }
    co_return;
  }
#endif
  void markConnected() {
    setState(State::CONNECTED);
  }
  void startGenerator() {
    generatorStart_.post();
  }

 private:
  folly::Baton<> generatorStart_;
};

} // namespace
class StreamPublisherTest : public ::testing::Test {
 public:
  void SetUp() override {
    streamEvbThread_ = std::make_unique<folly::ScopedEventBaseThread>();
    connRetryEvbThread_ = std::make_unique<folly::ScopedEventBaseThread>();
    streamPublisher_ = std::make_unique<TestFsdbStreamPublisher>(
        streamEvbThread_->getEventBase(), connRetryEvbThread_->getEventBase());
  }
  void TearDown() override {
    streamPublisher_.reset();
    streamEvbThread_.reset();
    connRetryEvbThread_.reset();
  }

 protected:
  std::unique_ptr<folly::ScopedEventBaseThread> streamEvbThread_;
  std::unique_ptr<folly::ScopedEventBaseThread> connRetryEvbThread_;
  std::unique_ptr<TestFsdbStreamPublisher> streamPublisher_;
};

TEST_F(StreamPublisherTest, overflowQueue) {
  streamPublisher_->markConnected();
  EXPECT_TRUE(streamPublisher_->isConnectedToServer());

  for (auto i = 0; i < streamPublisher_->queueCapacity(); ++i) {
    streamPublisher_->write(OperDelta{});
  }
  EXPECT_EQ(streamPublisher_->queueSize(), streamPublisher_->queueCapacity());
  // Queue capacity is not precise (~10% slack is typical), try to
  // push 2Xcapacity elements
  EXPECT_THROW(
      [&]() {
        for (auto i = 0; i < streamPublisher_->queueCapacity(); ++i) {
          streamPublisher_->write(OperDelta{});
        }
      }(),
      FsdbException);

  EXPECT_EQ(streamPublisher_->queueSize(), 0);
  // Generator should break the service loop
  streamPublisher_->startGenerator();
#if FOLLY_HAS_COROUTINES
  WITH_RETRIES(
      EXPECT_EVENTUALLY_FALSE(streamPublisher_->isConnectedToServer()));
#endif
}

} // namespace facebook::fboss::fsdb::test
