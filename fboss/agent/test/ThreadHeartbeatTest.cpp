/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/ThreadHeartbeat.h"
#include <folly/system/ThreadName.h>
#include <gtest/gtest.h>

using namespace facebook::fboss;

static constexpr int heartbeatInterval = 10;

TEST(ThreadHeartbeatTest, WatchDogTest) {
  folly::EventBase testEvb;
  std::thread testThread([&testEvb]() {
    folly::setThreadName("testThread");
    testEvb.loopForever();
  });
  std::atomic_int heartbeats = 0;
  auto statsFunc = [&heartbeats](int /*delay*/, int /*backLog*/) {
    heartbeats++;
  };
  // create an evb thread with heartbeat every 10ms
  std::shared_ptr<ThreadHeartbeat> testHb = std::make_shared<ThreadHeartbeat>(
      &testEvb, "testThread", heartbeatInterval, statsFunc);
  // create a watchdog to monitor heartbeat timestamp change every 100ms
  ThreadHeartbeatWatchdog testWd(
      std::chrono::milliseconds(heartbeatInterval * 10));
  testWd.startMonitoringHeartbeat(testHb);
  testWd.start();

  // monitor 1 second, should be no missed heartbeats
  sleep(1.0);
  EXPECT_TRUE(testWd.getMissedHeartbeats() == 0);
  EXPECT_TRUE(heartbeats > 0);

  // emaulate thread got stuck, and expect missed heartbeats
  testEvb.runInEventBaseThreadAndWait([]() {
    std::this_thread::sleep_for(
        std::chrono::milliseconds(heartbeatInterval * 20));
  });
  EXPECT_TRUE(testWd.getMissedHeartbeats() > 0);

  // continue monitoring another second, and expect no more missed
  // heartbeats
  auto missedHeartbeats = testWd.getMissedHeartbeats();
  sleep(1.0);
  EXPECT_TRUE(testWd.getMissedHeartbeats() == missedHeartbeats);

  testWd.stop();
  testHb.reset();
  testEvb.runInEventBaseThread([&testEvb] { testEvb.terminateLoopSoon(); });
  testThread.join();
}
