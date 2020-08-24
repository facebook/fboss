/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/AsyncLogger.h"

#include <gtest/gtest.h>
#include <stdio.h>

#define TEST_LOG "/tmp/sai_logger_test"

using namespace facebook::fboss;

class AsyncLoggerTest : public ::testing::Test {
 public:
  void SetUp() override {
    asyncLogger =
        std::make_unique<AsyncLogger>(TEST_LOG, bufferSize, logTimeout);
    asyncLogger->startFlushThread();
  }

  void TearDown() override {
    asyncLogger->stopFlushThread();
    std::remove(TEST_LOG);
  }

  std::unique_ptr<AsyncLogger> asyncLogger;
  std::condition_variable cv;
  std::mutex latch;
  uint32_t bufferSize = 20;
  uint32_t logTimeout = 50;
};

TEST_F(AsyncLoggerTest, logTimeoutTest) {
  std::string str = "TestString";
  asyncLogger->appendLog(str.c_str(), str.size());
  EXPECT_EQ(asyncLogger->flushCount_, 0);

  // Wait for one log timeout and then check
  // Add 5ms of boundry to let background thread flush data
  std::unique_lock<std::mutex> lock(latch);
  cv.wait_for(lock, std::chrono::milliseconds(logTimeout + 5));
  EXPECT_EQ(asyncLogger->flushCount_, 1);

  // Wait for one more log timeout but there's no logging happening
  // So flush count should still be the same
  cv.wait_for(lock, std::chrono::milliseconds(logTimeout + 5));
  EXPECT_EQ(asyncLogger->flushCount_, 1);
}

TEST_F(AsyncLoggerTest, fullflushTest) {
  std::string str1 = "TestString1";
  asyncLogger->appendLog(str1.c_str(), str1.size());
  EXPECT_EQ(asyncLogger->flushCount_, 0);

  std::string str2 = "TestString2";
  asyncLogger->appendLog(str2.c_str(), str2.size());

  // Log buffer is swapped, but it's non-blocking and flush does not take place
  // immediately. Wait for 5ms to let that happen
  std::unique_lock<std::mutex> lock(latch);
  cv.wait_for(lock, std::chrono::milliseconds(5));
  EXPECT_EQ(asyncLogger->flushCount_, 1);
}

TEST_F(AsyncLoggerTest, forceflushTest) {
  std::string str = "TestString";
  asyncLogger->appendLog(str.c_str(), str.size());
  EXPECT_EQ(asyncLogger->flushCount_, 0);

  // Force flush will block until the flush happens
  asyncLogger->forceFlush();
  EXPECT_EQ(asyncLogger->flushCount_, 1);
}

TEST_F(AsyncLoggerTest, emptyBufferTest) {
  std::string str = "";
  asyncLogger->appendLog(str.c_str(), str.size());
  EXPECT_EQ(asyncLogger->flushCount_, 0);

  // Wait for one log timeout and then check empty buffer
  // Add 5ms of boundry to let background thread flush data
  std::unique_lock<std::mutex> lock(latch);
  cv.wait_for(lock, std::chrono::milliseconds(logTimeout + 5));
  EXPECT_EQ(asyncLogger->flushCount_, 0);

  // Force flush and check
  asyncLogger->forceFlush();
  EXPECT_EQ(asyncLogger->flushCount_, 0);
}

TEST_F(AsyncLoggerTest, concurrentWaitTest) {
  std::string str0 = "FirstTestString";
  asyncLogger->appendLog(str0.c_str(), str0.size());

  // The first string will fill up the empty buffer, causing the next two
  // appends to wait for buffer to be swapped. When they finish waiting, logger
  // should realize that the buffer can only accept one. So it will immediately
  // flush one more time, rather than waiting for the next timeout to flush the
  // other string.
  std::thread t([&]() {
    std::string str1 = "SecondTestString";
    asyncLogger->appendLog(str1.c_str(), str1.size());
  });

  std::string str2 = "ThirdTestString";
  asyncLogger->appendLog(str2.c_str(), str2.size());

  t.join();

  // When the third append finished, the logger should have swapped the buffer
  // and potentially still writing into the buffer. Therefore, we wait for a bit
  // here to let the flush happen and increase the flush count.
  std::unique_lock<std::mutex> lock(latch);
  cv.wait_for(lock, std::chrono::milliseconds(5));

  // Without triggering a timeout for 50ms, logger should flush str0, and one of
  // str1 or str2. The other unflushed string should be in the current buffer.
  // Therefore, the flush count is two.
  EXPECT_EQ(asyncLogger->flushCount_, 2);
}
