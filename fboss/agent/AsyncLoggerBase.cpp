/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/AsyncLoggerBase.h"

#include <fb303/ServiceData.h>
#include <folly/FileUtil.h>
#include <gflags/gflags.h>

DECLARE_bool(disable_async_logger);

namespace {} // namespace

namespace facebook::fboss {

AsyncLoggerBase::AsyncLoggerBase(
    std::string filePath,
    uint32_t logTimeout,
    LoggerSrcType srcType,
    char* logBuf,
    char* flushBuf,
    uint32_t bufferSize)
    : bufferSize_(bufferSize), srcType_(srcType) {
  openLogFile(filePath);

  if (!FLAGS_disable_async_logger) {
    logBuffer_ = logBuf;
    flushBuffer_ = flushBuf;
    logTimeout_ = std::chrono::milliseconds(logTimeout);
  }
}

} // namespace facebook::fboss
