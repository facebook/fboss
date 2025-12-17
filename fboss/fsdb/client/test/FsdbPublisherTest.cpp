// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.
#include <folly/futures/Future.h>

#include "fboss/fsdb/client/FsdbDeltaPublisher.h"
#include "fboss/fsdb/client/FsdbPubSubManager.h"
#include "fboss/fsdb/client/FsdbStatePublisher.h"
#include "fboss/fsdb/tests/client/FsdbTestClients.h"
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
            false) {
    // Start unblocked
    serveStreamBlock_.post();
  }

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
      // Block here if requested to simulate stuck serveStream
      serveStreamBlock_.wait();
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
  void blockServeStream() {
    serveStreamBlock_.reset();
  }
  void unblockServeStream() {
    serveStreamBlock_.post();
  }

 private:
  folly::Baton<> generatorStart_;
  folly::Baton<> serveStreamBlock_;
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

TEST_F(StreamPublisherTest, pipeResetWhenServeStreamStuck) {
  auto counterPrefix = streamPublisher_->getCounterPrefix();
  EXPECT_EQ(counterPrefix, "fsdbDeltaStatePublisher_agent");
  EXPECT_EQ(
      fb303::ServiceData::get()->getCounter(counterPrefix + ".connected"), 0);

  streamPublisher_->markConnecting();
  WITH_RETRIES(
      { EXPECT_EVENTUALLY_TRUE(streamPublisher_->isConnectedToServer()); });
  EXPECT_EQ(
      fb303::ServiceData::get()->getCounter(counterPrefix + ".connected"), 1);

#if FOLLY_HAS_COROUTINES
  // Start the generator so serveStream begins consuming
  streamPublisher_->startGenerator();

  // Write one item to get serveStream started
  EXPECT_TRUE(streamPublisher_->write(OperDelta{}));

  // Give serveStream time to consume the first item
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  // Block serveStream after it has consumed from generator
  streamPublisher_->blockServeStream();

  // Fill queue to capacity while serveStream is stuck processing
  for (auto i = 0; i < streamPublisher_->queueCapacity(); ++i) {
    streamPublisher_->write(OperDelta{});
  }

  // Try to write more - this should reset the pipe since serveStream is stuck
  // and queue is full (no back pressure callback registered)
  for (auto i = 0; i < streamPublisher_->queueCapacity(); ++i) {
    streamPublisher_->write(OperDelta{});
  }

  // Verify pipe was reset (queue size should be 0)
  WITH_RETRIES({ EXPECT_EVENTUALLY_EQ(streamPublisher_->queueSize(), 0); });

  // Unblock serveStream and verify publisher disconnects
  streamPublisher_->unblockServeStream();
  WITH_RETRIES(
      { EXPECT_EVENTUALLY_FALSE(streamPublisher_->isConnectedToServer()); });
#endif
}

template <typename PublisherT>
class TestPublisher : public PublisherT {
 public:
  static constexpr size_t publishQueueSize = 4;
  TestPublisher(folly::EventBase* streamEvb, folly::EventBase* timerEvb)
      : PublisherT(
            "test_fsdb_client",
            {"agent"},
            streamEvb,
            timerEvb,
            false,
            [](State /*old*/, State /*newState*/) {},
            publishQueueSize) {}

  ~TestPublisher() override {
    this->cancel();
  }

#if FOLLY_HAS_COROUTINES
  folly::coro::Task<typename PublisherT::StreamT> setupStream() override {
    co_return typename PublisherT::StreamT();
  }

  folly::coro::Task<void> serveStream(
      typename PublisherT::StreamT&& /* stream */) override {
    auto gen = this->createGenerator();
    generatorStart_.wait();
    while (auto pubUnit = co_await gen.next()) {
      while (blockStream_.load()) {
        co_await folly::futures::sleep(std::chrono::milliseconds(100));
      }
      if (this->isCancelled()) {
        XLOG(DBG2) << " Detected cancellation";
        break;
      }
      if (!PublisherT::initialSyncComplete_) {
        PublisherT::initialSyncComplete_ = true;
      }
    }
    co_return;
  }

#endif
  void markConnecting() {
    this->setState(State::CONNECTING);
  }

  void startGenerator() {
    generatorStart_.post();
  }

  void blockStream() {
    blockStream_.store(true);
  }

  void unblockStream() {
    blockStream_.store(false);
  }

 private:
  folly::Baton<> generatorStart_;
  std::atomic<bool> blockStream_{false};
};

class PathPublisherTest : public ::testing::Test {
 public:
  void SetUp() override {
    streamEvbThread_ = std::make_unique<folly::ScopedEventBaseThread>();
    connRetryEvbThread_ = std::make_unique<folly::ScopedEventBaseThread>();
    streamPublisher_ = std::make_unique<TestPublisher<FsdbStatePublisher>>(
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
  std::unique_ptr<TestPublisher<FsdbStatePublisher>> streamPublisher_;
};

TEST_F(PathPublisherTest, coalesceOnPublishQueueBuildup) {
  auto counterPrefix = streamPublisher_->getCounterPrefix();
  EXPECT_EQ(counterPrefix, "fsdbPathStatePublisher_agent");
  EXPECT_EQ(
      fb303::ServiceData::get()->getCounter(counterPrefix + ".connected"), 0);

  streamPublisher_->markConnecting();
  WITH_RETRIES(
      { EXPECT_EVENTUALLY_TRUE(streamPublisher_->isConnectedToServer()); });
  EXPECT_EQ(
      fb303::ServiceData::get()->getCounter(counterPrefix + ".connected"), 1);

#if FOLLY_HAS_COROUTINES
  constexpr int numUpdates = 10;
  for (auto i = 0; i < numUpdates; ++i) {
    streamPublisher_->write(OperState{});
  }

  // verify updates are coalesced
  WITH_RETRIES({
    fb303::ThreadCachedServiceData::get()->publishStats();
    EXPECT_EVENTUALLY_EQ(
        fb303::ServiceData::get()->getCounter(
            counterPrefix + ".chunksWritten.sum"),
        2);
    EXPECT_EVENTUALLY_EQ(
        fb303::ServiceData::get()->getCounter(
            counterPrefix + ".coalescedUpdates.sum"),
        numUpdates - 3);
  });

  // Start the generator so serveStream begins consuming
  streamPublisher_->startGenerator();

  // verify that coalesced update is written
  WITH_RETRIES({
    fb303::ThreadCachedServiceData::get()->publishStats();
    EXPECT_EVENTUALLY_EQ(
        fb303::ServiceData::get()->getCounter(
            counterPrefix + ".chunksWritten.sum"),
        3);
  });

#endif
}

TEST_F(PathPublisherTest, verifyHeartbeatDisconnect) {
  auto counterPrefix = streamPublisher_->getCounterPrefix();
  EXPECT_EQ(
      fb303::ServiceData::get()->getCounter(counterPrefix + ".connected"), 0);

  streamPublisher_->markConnecting();
  WITH_RETRIES(
      { EXPECT_EVENTUALLY_TRUE(streamPublisher_->isConnectedToServer()); });
  EXPECT_EQ(
      fb303::ServiceData::get()->getCounter(counterPrefix + ".connected"), 1);
  EXPECT_EQ(this->streamPublisher_->isPipeClosed(), false);

#if FOLLY_HAS_COROUTINES

  // after initial sync block serveStream
  streamPublisher_->startGenerator();
  streamPublisher_->write(OperState{});
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  streamPublisher_->blockStream();

  // write some updates, which will get coalesced and should not trigger
  // queue full disconnect
  int waitIntervals{0};
  for (auto i = 0; i < TestPublisher<FsdbStatePublisher>::publishQueueSize;
       ++i) {
    streamPublisher_->write(OperState{});
    if ((i % 2) == 0) {
      // sleep for half of heartbeat intervals
      std::this_thread::sleep_for(
          std::chrono::seconds(FLAGS_fsdb_publisher_heartbeat_interval_secs));
      waitIntervals++;
    }
  }

  EXPECT_EQ(this->streamPublisher_->isPipeClosed(), false);

  // wait for heartbeat to trigger disconnect
  for (int i = waitIntervals;
       i <= TestPublisher<FsdbStatePublisher>::publishQueueSize;
       ++i) {
    std::this_thread::sleep_for(
        std::chrono::seconds(FLAGS_fsdb_publisher_heartbeat_interval_secs));
  }
  streamPublisher_->unblockStream();
  EXPECT_EQ(this->streamPublisher_->isPipeClosed(), true);

#endif
}

class DeltaPublisherTest : public ::testing::Test {
 public:
  void SetUp() override {
    FLAGS_publish_queue_full_min_updates =
        (TestPublisher<FsdbDeltaPublisher>::publishQueueSize - 4);
    FLAGS_publish_queue_memory_limit_mb = 1;
    streamEvbThread_ = std::make_unique<folly::ScopedEventBaseThread>();
    connRetryEvbThread_ = std::make_unique<folly::ScopedEventBaseThread>();
    streamPublisher_ = std::make_unique<TestPublisher<FsdbDeltaPublisher>>(
        streamEvbThread_->getEventBase(), connRetryEvbThread_->getEventBase());
  }

  void TearDown() override {
    streamPublisher_.reset();
    streamEvbThread_.reset();
    connRetryEvbThread_.reset();
  }

  OperDelta
  makeLargeUpdate(const std::string& argName, uint32_t bytes, char val = 'a') {
    return makeDelta(makeLargeAgentConfig(argName, bytes, val));
  }

 protected:
  std::unique_ptr<folly::ScopedEventBaseThread> streamEvbThread_;
  std::unique_ptr<folly::ScopedEventBaseThread> connRetryEvbThread_;
  std::unique_ptr<TestPublisher<FsdbDeltaPublisher>> streamPublisher_;
};

TEST_F(DeltaPublisherTest, publisherQueueMemoryLimit) {
  auto counterPrefix = streamPublisher_->getCounterPrefix();
  EXPECT_EQ(counterPrefix, "fsdbDeltaStatePublisher_agent");
  EXPECT_EQ(
      fb303::ServiceData::get()->getCounter(counterPrefix + ".connected"), 0);

  streamPublisher_->markConnecting();
  WITH_RETRIES(
      { EXPECT_EVENTUALLY_TRUE(streamPublisher_->isConnectedToServer()); });
  EXPECT_EQ(
      fb303::ServiceData::get()->getCounter(counterPrefix + ".connected"), 1);

#if FOLLY_HAS_COROUTINES
  // Start the generator so serveStream begins consuming
  streamPublisher_->startGenerator();

  // verify initial update is written
  streamPublisher_->write(OperDelta{});
  WITH_RETRIES({
    fb303::ThreadCachedServiceData::get()->publishStats();
    EXPECT_EVENTUALLY_EQ(
        fb303::ServiceData::get()->getCounter(
            counterPrefix + ".chunksWritten.sum"),
        1);
  });

  // Block serveStream after initial consumption to simulate stuck processing
  streamPublisher_->blockStream();

  // post large update to build up queue memory
  int updateSize = (1024 * 1024 * FLAGS_publish_queue_memory_limit_mb + 1);
  streamPublisher_->write(makeLargeUpdate("foo", updateSize));

  // validate that pipe is not reset yet
  EXPECT_EQ(this->streamPublisher_->isPipeClosed(), false);

  // trigger memory-based queue full detection
  for (int i = 0; i <= FLAGS_publish_queue_full_min_updates + 1; i++) {
    streamPublisher_->write(OperDelta{});
  }

  // validate that pipe is reset
  EXPECT_EQ(this->streamPublisher_->isPipeClosed(), true);

  streamPublisher_->unblockStream();
#endif
}

} // namespace facebook::fboss::fsdb::test
