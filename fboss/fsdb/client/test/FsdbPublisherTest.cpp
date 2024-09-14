// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include "fboss/fsdb/client/FsdbDeltaPublisher.h"
#include "fboss/fsdb/common/Flags.h"
#include "fboss/lib/CommonUtils.h"

#include <folly/coro/AsyncGenerator.h>
#include <folly/coro/AsyncPipe.h>
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
  folly::coro::Task<StreamT> setupStream() override {
    co_return StreamT();
  }

  folly::coro::Task<void> serveStream(StreamT&& /* stream */) override {
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
  void markConnecting() {
    setState(State::CONNECTING);
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
  auto counterPrefix = streamPublisher_->getCounterPrefix();
  EXPECT_EQ(counterPrefix, "fsdbDeltaStatePublisher_agent");
  EXPECT_EQ(
      fb303::ServiceData::get()->getCounter(counterPrefix + ".connected"), 0);
  streamPublisher_->markConnecting();
  WITH_RETRIES(
      { EXPECT_EVENTUALLY_TRUE(streamPublisher_->isConnectedToServer()); });
  EXPECT_EQ(
      fb303::ServiceData::get()->getCounter(counterPrefix + ".connected"), 1);

  for (auto i = 0; i < streamPublisher_->queueCapacity(); ++i) {
    streamPublisher_->write(OperDelta{});
  }
  EXPECT_EQ(streamPublisher_->queueSize(), streamPublisher_->queueCapacity());
#if FOLLY_HAS_COROUTINES
  // Queue capacity is not precise (~10% slack is typical), try to
  // push 2Xcapacity elements
  bool writeFailed = false;
  for (auto i = 0; i < streamPublisher_->queueCapacity() && !writeFailed; ++i) {
    writeFailed = !streamPublisher_->write(OperDelta{});
  }
  EXPECT_TRUE(writeFailed);
  WITH_RETRIES({
    fb303::ThreadCachedServiceData::get()->publishStats();
    EXPECT_EVENTUALLY_EQ(
        fb303::ServiceData::get()->getCounter(
            counterPrefix + ".writeErrors.sum.60"),
        1);
  });

  EXPECT_EQ(streamPublisher_->queueSize(), 0);
  // Generator should break the service loop
  streamPublisher_->startGenerator();
  WITH_RETRIES({
    EXPECT_EVENTUALLY_FALSE(streamPublisher_->isConnectedToServer());
    fb303::ThreadCachedServiceData::get()->publishStats();
    EXPECT_EVENTUALLY_EQ(
        fb303::ServiceData::get()->getCounter(
            counterPrefix + ".disconnects.sum.60"),
        1);
  });
  EXPECT_EQ(
      fb303::ServiceData::get()->getCounter(counterPrefix + ".connected"), 0);
#endif
}

} // namespace facebook::fboss::fsdb::test
