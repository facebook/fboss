/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include <cstdlib>
#include <exception>
#include <fstream>
#include <iostream>

#include "fboss/agent/AsyncLogger.h"
#include "fboss/agent/SysError.h"

#include <folly/FileUtil.h>
#include <gflags/gflags.h>

DEFINE_bool(
    disable_async_logger,
    false,
    "Flag to indicate whether to disable async logging and directly write into the file");

enum BufferToWrite { BUFFER0, BUFFER1 };

static std::string exitFilePath;
static uint32_t offset = 0;
static BufferToWrite currentBuffer = BUFFER0;
static std::array<char, facebook::fboss::AsyncLogger::kBufferSize> buffer0;
static std::array<char, facebook::fboss::AsyncLogger::kBufferSize> buffer1;

namespace {

void terminateHandler() {
  if (offset > 0) {
    // Use standard library instead of folly because in unclean exit, folly
    // library could be inaccessible so there's a higher chance of writing into
    // file using standard library.
    std::ofstream logfile;
    logfile.open(exitFilePath, std::ofstream::app);

    if (currentBuffer == BUFFER0) {
      logfile.write(buffer0.data(), offset);
    } else {
      logfile.write(buffer1.data(), offset);
    }
    std::cerr << "Async logger exit with " << offset
              << " bytes written to file " << std::endl;
  }

  abort();
}

} // namespace

namespace facebook::fboss {
AsyncLogger::AsyncLogger(std::string filePath, uint32_t logTimeout)
    : bufferSize_(AsyncLogger::kBufferSize) {
  openLogFile(filePath);

  if (!FLAGS_disable_async_logger) {
    logBuffer_ = buffer0.data();
    flushBuffer_ = buffer1.data();

    exitFilePath = filePath;

    logTimeout_ = std::chrono::milliseconds(logTimeout);

    std::set_terminate(terminateHandler);
  }
}

AsyncLogger::~AsyncLogger() {
  fsync(logFile_.wlock()->fd());
}

void AsyncLogger::worker_thread() {
  while (enableLogging_) {
    std::unique_lock<std::mutex> lock(latch_);

    // Wait for either 1. Timeout 2. Force flush or full flush
    cv_.wait_for(lock, logTimeout_, [this] {
      return this->forceFlush_ || this->fullFlush_;
    });

    // Swap log buffer and flush buffer
    char* writeBuffer = logBuffer_;
    uint32_t currSize = offset;
    offset = 0;
    currentBuffer = ((currentBuffer == BUFFER0) ? BUFFER1 : BUFFER0);
    logBuffer_ = flushBuffer_;

    if (fullFlush_) {
      fullFlush_ = false;
    }

    lock.unlock();
    cv_.notify_one();

    // Write content in swap buffer to file
    if (currSize > 0) {
      auto bytesWritten = logFile_.withWLock([&](auto& lockedFile) {
        return folly::writeFull(lockedFile.fd(), writeBuffer, currSize);
      });

      if (bytesWritten < 0) {
        throw SysError(
            errno, "error writing ", currSize, " bytes to log file.");
      }

      flushCount_++;
    }

    memset(writeBuffer, 0, bufferSize_);
    flushBuffer_ = writeBuffer;

    // Notify force flush that write completes
    if (forceFlush_) {
      forceFlush_ = false;
      promise_.set_value(0);
      promise_ = std::promise<int>();
    }
  }
}

void AsyncLogger::startFlushThread() {
  enableLogging_ = true;
  if (!FLAGS_disable_async_logger) {
    flushThread_ = new std::thread(&AsyncLogger::worker_thread, this);
  }
}

void AsyncLogger::stopFlushThread() {
  if (!FLAGS_disable_async_logger && enableLogging_) {
    enableLogging_ = false;
    flushThread_->join();
    delete flushThread_;
  }
}

void AsyncLogger::forceFlush() {
  if (!FLAGS_disable_async_logger) {
    future_ = promise_.get_future();

    forceFlush_ = true;
    cv_.notify_one();

    // Wait for flush to complete
    future_.get();
  }
}

void AsyncLogger::appendLog(const char* logRecord, size_t logSize) {
  if (!enableLogging_) {
    return;
  }

  if (FLAGS_disable_async_logger && logSize > 0) {
    auto bytesWritten = logFile_.withWLock([&](auto& lockedFile) {
      return folly::writeFull(lockedFile.fd(), logRecord, logSize);
    });

    if (bytesWritten < 0) {
      throw SysError(errno, "error writing ", logSize, " bytes to log file.");
    }
    return;
  }

  // Acquire the lock and check if there's enough space in the buffer
  latch_.lock();

  if (logSize + offset >= bufferSize_) {
    // Release the lock and notify worker thread to flush logs
    fullFlush_ = true;
    latch_.unlock();
    cv_.notify_one();

    // Wait for worker to finish
    std::unique_lock<std::mutex> lock(latch_);
    cv_.wait(lock, [logSize] {
      return logSize + offset < AsyncLogger::kBufferSize;
    });

    // Write to buffer
    memcpy(logBuffer_ + offset, logRecord, logSize);
    offset += logSize;

    lock.unlock();
  } else {
    // Directly write to buffer
    memcpy(logBuffer_ + offset, logRecord, logSize);
    offset += logSize;

    latch_.unlock();
  }
}

void AsyncLogger::openLogFile(std::string& file_path) {
  // By default, async logger opens log file under /var/facebook/logs/fboss/
  // However, the directory /var/facebook/logs/fboss/ might not exist for test
  // switches not running chef. When that happens, we fall back to /tmp/ and
  // write generated code there.
  try {
    if (file_path.find("/var/facebook/logs/fboss/") == 0) {
      // Writes to default log location - append to log
      logFile_ = folly::File(file_path, O_RDWR | O_CREAT | O_APPEND);
    } else {
      // Testing purpose - override the old log
      logFile_ = folly::File(file_path, O_RDWR | O_CREAT | O_TRUNC);
    }
  } catch (const std::system_error& e) {
    auto last_slash = file_path.find_last_of("/");

    std::string directory = file_path.substr(0, last_slash);
    std::string file_name = file_path.substr(last_slash + 1);

    XLOG(WARN) << "Failed to create " << file_name << " under " << directory
               << ". Falling back to /tmp/" << file_name;

    logFile_ = folly::File("/tmp/" + file_name, O_RDWR | O_CREAT | O_TRUNC);
  }
}

} // namespace facebook::fboss
