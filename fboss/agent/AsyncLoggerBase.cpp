/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include <iomanip>

#include "fboss/agent/AsyncLoggerBase.h"

#include <fb303/ServiceData.h>
#include <folly/FileUtil.h>
#include <gflags/gflags.h>

DECLARE_bool(disable_async_logger);

namespace {

constexpr auto kBuildRevision = "build_revision";
constexpr auto kVerboseSdkVersion = "Verbose SDK Version";

bool isWarmBoot;
std::mutex bootTypeLatch_;

} // namespace

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

void AsyncLoggerBase::setBootType(bool canWarmBoot) {
  bootTypeLatch_.lock();
  isWarmBoot = canWarmBoot;
  bootTypeLatch_.unlock();
}

void AsyncLoggerBase::startFlushThread() {
  enableLogging_ = true;
  if (!FLAGS_disable_async_logger) {
    flushThread_ = new std::thread(&AsyncLoggerBase::workerThread, this);
  }
  writeNewBootHeader();
}

void AsyncLoggerBase::stopFlushThread() {
  if (!FLAGS_disable_async_logger && enableLogging_) {
    enableLogging_ = false;
    flushThread_->join();
    delete flushThread_;
  }
}
void AsyncLoggerBase::forceFlush() {
  if (!FLAGS_disable_async_logger) {
    future_ = promise_.get_future();
    forceFlush_ = true;
    cv_.notify_one();
    future_.get();
  }
}

void AsyncLoggerBase::writeNewBootHeader() {
  auto now = std::chrono::system_clock::now();
  auto timer = std::chrono::system_clock::to_time_t(now);
  std::tm tm;
  localtime_r(&timer, &tm);
  auto bootType = isWarmBoot ? "warmboot" : "coldboot";
  std::ostringstream oss;
  oss << "// Start of a " << bootType << " "
      << std::put_time(&tm, "%Y-%m-%d %T") << std::endl;
  auto ossString = oss.str();
  appendLog(ossString.c_str(), ossString.size());
  std::string commitId;
  fb303::fbData->getExportedValue(commitId, kBuildRevision);
  std::string verboseSdkVersion;
  fb303::fbData->getExportedValue(verboseSdkVersion, kVerboseSdkVersion);
  verboseSdkVersion.erase(
      std::remove(verboseSdkVersion.begin(), verboseSdkVersion.end(), '\n'),
      verboseSdkVersion.end());
  std::string agentVersion = folly::to<std::string>(
      "// Commit id : ",
      commitId,
      "\n",
      "// SDK version : ",
      verboseSdkVersion,
      "\n");
  appendLog(agentVersion.c_str(), agentVersion.size());
}

} // namespace facebook::fboss
