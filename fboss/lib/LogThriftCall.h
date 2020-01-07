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

#include <folly/Range.h>
#include <folly/logging/LogLevel.h>
#include <folly/logging/Logger.h>
#include <folly/logging/xlog.h>
#include <thrift/lib/cpp2/server/Cpp2ConnContext.h>
#include <string>

namespace facebook::fboss {
class LogThriftCall {
 public:
  explicit LogThriftCall(
      const folly::Logger& logger,
      folly::LogLevel level,
      folly::StringPiece func,
      folly::StringPiece file,
      uint32_t line,
      apache::thrift::Cpp2RequestContext* ctx);
  ~LogThriftCall();

 private:
  folly::Logger logger_;
  folly::LogLevel level_;
  folly::StringPiece func_;
  folly::StringPiece file_;
  uint32_t line_;
  std::chrono::time_point<std::chrono::steady_clock> start_;
};

} // namespace facebook::fboss

/*
 * This macro returns a LogThriftCall object that prints request
 * context info and also times the function.
 *
 * ex: auto log = LOG_THRIFT_CALL(DBG1);
 *
 * TODO: add ability to log arguments/return values as well
 */
#define LOG_THRIFT_CALL(level)                                  \
  ([&](folly::StringPiece func,                                 \
       folly::StringPiece file,                                 \
       uint32_t line,                                           \
       apache::thrift::Cpp2RequestContext* ctx) {               \
    static folly::Logger logger(XLOG_GET_CATEGORY_NAME());      \
    return ::facebook::fboss::LogThriftCall(                    \
        logger, folly::LogLevel::level, func, file, line, ctx); \
  }(__func__, __FILE__, __LINE__, getConnectionContext()))
