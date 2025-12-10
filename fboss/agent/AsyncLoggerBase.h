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

DECLARE_bool(disable_async_logger);

namespace facebook::fboss {

class AsyncLoggerBase {
 public:
  enum LoggerSrcType { BCM_CINTER, SAI_REPLAYER, STATE_DELTA };
  enum BufferToWrite { BUFFER0, BUFFER1 };

  explicit AsyncLoggerBase(
      const std::string& filePath,
      uint32_t logTimeout,
      LoggerSrcType srcType,
      char* logBuf,
      char* flushBuf,
      uint32_t bufferSize);

  virtual ~AsyncLoggerBase() = default;

  void startFlushThread();
  void stopFlushThread();
  virtual void forceFlush();

  void appendLog(const char* logRecord, size_t logSize);

  static void setBootType(bool canWarmBoot);

  // Expose these variables for testing purpose
  uint32_t getFlushCount() {
    return flushCount_;
  }

 protected:
  void workerThread();
  void openLogFile(const std::string& file_path);
  void writeNewBootHeader();

  std::atomic_uint32_t flushCount_{0};
  std::atomic_bool forceFlush_{false};
  bool fullFlush_{false};
  std::atomic_bool enableLogging_{false};

  uint32_t bufferSize_;
  char* logBuffer_;
  char* flushBuffer_;
  LoggerSrcType srcType_;
  std::promise<int> promise_;
  std::future<int> future_;
  std::mutex latch_;
  std::thread* flushThread_;
  std::condition_variable cv_;
  std::chrono::milliseconds logTimeout_;
  folly::Synchronized<folly::File> logFile_;

  // Pure virtual functions to access static buffers from derived class
  virtual uint32_t getOffset() = 0;
  virtual void setOffset(uint32_t offset) = 0;
  virtual void swapCurBuf() = 0;
};

} // namespace facebook::fboss
