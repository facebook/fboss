/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include <exception>
#include <fstream>
#include <iostream>

#include "fboss/agent/AsyncLogger.h"

static facebook::fboss::AsyncLoggerBase::BufferToWrite currentBuffer =
    facebook::fboss::AsyncLoggerBase::BufferToWrite::BUFFER0;
static std::string exitFilePath;
static uint32_t bufOffset = 0;
static std::array<char, facebook::fboss::AsyncLogger::kBufferSize> buffer0;
static std::array<char, facebook::fboss::AsyncLogger::kBufferSize> buffer1;

namespace {

void terminateHandler() {
  if (bufOffset > 0) {
    // Use standard library instead of folly because in unclean exit, folly
    // library could be inaccessible so there's a higher chance of writing into
    // file using standard library.
    std::ofstream logfile;
    logfile.open(exitFilePath, std::ofstream::app);

    if (currentBuffer == facebook::fboss::AsyncLoggerBase::BUFFER0) {
      logfile.write(buffer0.data(), bufOffset);
    } else {
      logfile.write(buffer1.data(), bufOffset);
    }
    std::cerr << "Async logger exit with " << bufOffset
              << " bytes written to file " << std::endl;
  }

  std::exception_ptr eptr = std::current_exception();
  if (eptr) {
    try {
      std::rethrow_exception(eptr);
    } catch (const std::exception& ex) {
      std::cerr << "Terminated due to: " << ex.what() << "\n";
    }
  }

  abort();
}

} // namespace

namespace facebook::fboss {
AsyncLogger::AsyncLogger(
    const std::string& filePath,
    uint32_t logTimeout,
    LoggerSrcType srcType)
    : AsyncLoggerBase(
          filePath,
          logTimeout,
          srcType,
          buffer0.data(),
          buffer1.data(),
          kBufferSize) {
  if (!FLAGS_disable_async_logger) {
    std::set_terminate(terminateHandler);
  }
}

uint32_t AsyncLogger::getOffset() {
  return bufOffset;
}

void AsyncLogger::setOffset(uint32_t offset) {
  bufOffset = offset;
}

void AsyncLogger::swapCurBuf() {
  currentBuffer = currentBuffer == facebook::fboss::AsyncLoggerBase::BUFFER0
      ? facebook::fboss::AsyncLoggerBase::BUFFER1
      : facebook::fboss::AsyncLoggerBase::BUFFER0;
}

} // namespace facebook::fboss
