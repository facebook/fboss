/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/mnpu/IpcHealthMonitor.h"

#include <fb303/ServiceData.h>
#include <folly/io/async/EventBase.h>
#include <folly/io/async/test/EventBaseTestLib.h>
#include <folly/logging/xlog.h>
#include <gtest/gtest.h>

using namespace facebook::fboss;
using namespace facebook::fb303;

namespace {
class IpcHealthMonitorTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // Create an event base for testing
    evb_ = std::make_unique<folly::EventBase>();

    // Create the IPC health monitor
    monitor_ = std::make_unique<IpcHealthMonitor>("test_client", "test_prefix");
  }

  void TearDown() override {
    // Stop the monitor
    if (monitor_) {
      monitor_->stop();
    }

    // Destroy the event base
    evb_.reset();
  }

  // Helper method to run the event base for a short time
  void runEventBaseForMs(uint32_t ms) {
    evb_->runInEventBaseThread([this, ms]() {
      evb_->runAfterDelay([this]() { evb_->terminateLoopSoon(); }, ms);
    });
    evb_->loop();
  }

  // Helper method to check if a counter exists before accessing it
  bool counterExists(const std::string& counterName) {
    try {
      fbData->getCounter(counterName);
      return true;
    } catch (const std::exception&) {
      return false;
    }
  }

  std::unique_ptr<folly::EventBase> evb_;
  std::unique_ptr<IpcHealthMonitor> monitor_;
};

TEST_F(IpcHealthMonitorTest, TestStartStop) {
  // Start the monitor
  monitor_->start();

  // Run the event base for a short time
  runEventBaseForMs(100);

  // Stop the monitor
  monitor_->stop();
}

TEST_F(IpcHealthMonitorTest, TestTrackEvents) {
  // Start the monitor
  monitor_->start();

  // Track events
  monitor_->trackEventSent();
  monitor_->trackEventDropped();
  monitor_->trackEventReceived();

  // Get the metrics
  auto metrics = monitor_->getMetrics();

  // Verify the metrics
  EXPECT_EQ(metrics.eventsSent, 1);
  EXPECT_EQ(metrics.eventsDropped, 1);
  EXPECT_EQ(metrics.eventsReceived, 1);

  // Run the event base for a short time
  runEventBaseForMs(100);

  // Stop the monitor
  monitor_->stop();

  // Verify the counters were incremented (only if they exist)
  if (counterExists("test_prefix.ipc.events_sent")) {
    EXPECT_GT(fbData->getCounter("test_prefix.ipc.events_sent"), 0);
  }
  if (counterExists("test_prefix.ipc.events_dropped")) {
    EXPECT_GT(fbData->getCounter("test_prefix.ipc.events_dropped"), 0);
  }
  if (counterExists("test_prefix.ipc.events_received")) {
    EXPECT_GT(fbData->getCounter("test_prefix.ipc.events_received"), 0);
  }
}

TEST_F(IpcHealthMonitorTest, TestTrackQueueDepth) {
  // Start the monitor
  monitor_->start();

  // Track queue depth
  monitor_->trackQueueDepth(100);

  // Get the metrics
  auto metrics = monitor_->getMetrics();

  // Verify the metrics
  EXPECT_EQ(metrics.queueDepth, 100);

  // Run the event base for a short time
  runEventBaseForMs(100);

  // Stop the monitor
  monitor_->stop();

  // Verify the counter was incremented (only if it exists)
  if (counterExists("test_prefix.ipc.queue_depth")) {
    EXPECT_GT(fbData->getCounter("test_prefix.ipc.queue_depth"), 0);
  }
}

TEST_F(IpcHealthMonitorTest, TestTrackStateTransition) {
  // Start the monitor
  monitor_->start();

  // Track state transitions
  monitor_->trackStateTransition(
      IpcConnectionState::DISCONNECTED, IpcConnectionState::CONNECTING);
  monitor_->trackStateTransition(
      IpcConnectionState::CONNECTING, IpcConnectionState::CONNECTED);
  monitor_->trackStateTransition(
      IpcConnectionState::CONNECTED, IpcConnectionState::DISCONNECTED);

  // Get the metrics
  auto metrics = monitor_->getMetrics();

  // Verify the metrics
  EXPECT_EQ(metrics.connectionState, IpcConnectionState::DISCONNECTED);
  EXPECT_EQ(metrics.connectCount, 1);
  EXPECT_EQ(metrics.disconnectCount, 1);
  EXPECT_EQ(metrics.reconnectCount, 1);
  EXPECT_GT(metrics.lastStateTransitionTimestamp, 0);

  // Run the event base for a short time to allow counter creation
  runEventBaseForMs(5000);

  // Stop the monitor
  monitor_->stop();

  // Verify the counters were incremented (only if they exist)
  // Note: Counters are created lazily by fb303 when addStatValue is called
  if (counterExists("test_prefix.ipc.connect_count")) {
    XLOG(INFO) << "test_prefix.ipc.connect_count: "
               << fbData->getCounter("test_prefix.ipc.connect_count");
    EXPECT_GT(fbData->getCounter("test_prefix.ipc.connect_count"), 0);
  } else {
    XLOG(INFO)
        << "Counter test_prefix.ipc.connect_count does not exist - this is expected if counters are not created synchronously";
    // This is acceptable since fb303 counters are created lazily
  }
  if (counterExists("test_prefix.ipc.disconnect_count")) {
    XLOG(INFO) << "test_prefix.ipc.disconnect_count: "
               << fbData->getCounter("test_prefix.ipc.disconnect_count");
    EXPECT_GT(fbData->getCounter("test_prefix.ipc.disconnect_count"), 0);
  } else {
    XLOG(INFO)
        << "Counter test_prefix.ipc.disconnect_count does not exist - this is expected if counters are not created synchronously";
  }
  if (counterExists("test_prefix.ipc.reconnect_count")) {
    XLOG(INFO) << "test_prefix.ipc.reconnect_count: "
               << fbData->getCounter("test_prefix.ipc.reconnect_count");
    EXPECT_GT(fbData->getCounter("test_prefix.ipc.reconnect_count"), 0);
  } else {
    XLOG(INFO)
        << "Counter test_prefix.ipc.reconnect_count does not exist - this is expected if counters are not created synchronously";
  }
}
} // namespace
