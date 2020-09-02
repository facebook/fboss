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
  explicit AsyncLogger(std::string filePath, uint32_t logTimeout);

  ~AsyncLogger();

  /*
   * To handle unclean exit, we use terminate handler to write out the logs that
   * are still in the buffer. However, terminate handler is invoked after the
   * program exits, meaning variables could be already destructed when we enter
   * terminate handler. Therefore, we use static variables for the async buffer
   * and related data structures. They are not garbage collected until terminate
   * handler is finished.
   * However, the downside of this approach is that the buffer size needs to be
   * hardcoded (instead of declaring a google flag and pass in as command line
   * argument). The buffer size should be known at compile time.
   * Therefore, we chose the buffer size of 409600, which is an appropriate
   * number that's not introducing too much memory overhead (roughly 0.035% of
   * current prod usage), but still perform well in frequent updates and
   * benchmark tests.
   */
  static auto constexpr kBufferSize = 409600;

  void startFlushThread();
  void stopFlushThread();
  void forceFlush();

  void appendLog(const char* logRecord, size_t logSize);

  // Expose these variables for testing purpose
  uint32_t flushCount_{0};

 private:
  void worker_thread();
  void openLogFile(std::string& file_path);

  bool forceFlush_{false};
  bool fullFlush_{false};
  bool enableLogging_{false};

  uint32_t bufferSize_;

  char* logBuffer_;
  char* flushBuffer_;

  std::promise<int> promise_;
  std::future<int> future_;
  std::mutex latch_;
  std::thread* flushThread_;
  std::condition_variable cv_;
  std::chrono::microseconds logTimeout_;

  folly::Synchronized<folly::File> logFile_;
};

} // namespace facebook::fboss
