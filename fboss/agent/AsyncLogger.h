/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#pragma once

#include <chrono>
#include <condition_variable>
#include <future>
#include <mutex>

#include <folly/File.h>
#include <folly/Synchronized.h>

namespace facebook::fboss {

class AsyncLogger {
 public:
  explicit AsyncLogger(
      std::string file_path,
      uint32_t bufferSize,
      uint32_t logTimeout);

  ~AsyncLogger();

  void startFlushThread();
  void stopFlushThread();
  void forceFlush();

  void appendLog(const char* logRecord, size_t logSize);

  uint32_t flushCount_{0}; // For testing/debugging purpose

 private:
  void worker_thread();

  char* logBuffer_;
  char* flushBuffer_;
  bool forceFlush_{false};
  bool fullFlush_{false};
  bool enableLogging_{false};
  uint32_t bufferSize_;
  uint32_t offset_{0};

  std::promise<int> promise_;
  std::future<int> future_;
  std::mutex latch_;
  std::thread* flushThread_;
  std::condition_variable cv_;
  std::chrono::microseconds log_timeout_;

  folly::Synchronized<folly::File> logFile_;
};

} // namespace facebook::fboss
