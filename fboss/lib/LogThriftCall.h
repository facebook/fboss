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

#include <folly/Conv.h>
#include <folly/Range.h>
#include <folly/futures/Future.h>
#include <folly/json/DynamicConverter.h>
#include <folly/json/json.h>
#include <folly/logging/LogLevel.h>
#include <folly/logging/Logger.h>
#include <folly/logging/xlog.h>
#include <thrift/lib/cpp2/server/Cpp2ConnContext.h>
#include <sstream>
#include <string>

#define _LOGTHRIFTCALL_APPEND_PARAM(a) \
  ss << #a "=" << folly::toJson(folly::toDynamic(a)) << ",";

#define LOG_THRIFT_CALL_IMPL(level, identEnvFallback, ...)        \
  ([&](folly::StringPiece func,                                   \
       folly::StringPiece file,                                   \
       uint32_t line,                                             \
       apache::thrift::Cpp2RequestContext* ctx)                   \
       -> std::unique_ptr<::facebook::fboss::LogThriftCall> {     \
    if (!XLOG_IS_ON(level)) {                                     \
      /* short circuit case to avoid unnecessary work */          \
      return nullptr;                                             \
    }                                                             \
    static folly::Logger logger(XLOG_GET_CATEGORY_NAME());        \
    std::stringstream ss;                                         \
    FOLLY_PP_FOR_EACH(_LOGTHRIFTCALL_APPEND_PARAM, ##__VA_ARGS__) \
    return std::make_unique<::facebook::fboss::LogThriftCall>(    \
        logger,                                                   \
        folly::LogLevel::level,                                   \
        func,                                                     \
        file,                                                     \
        line,                                                     \
        ctx,                                                      \
        ss.str(),                                                 \
        identEnvFallback);                                        \
  }(__func__, __FILE__, __LINE__, getRequestContext()))

/*
 * This macro returns a LogThriftCall object that prints request
 * context info and also times the function.
 *
 * ex: auto log = LOG_THRIFT_CALL(DBG1);
 *
 * Also supports printing a few parameters:
 *
 *   void dummyThrift(
 *       std::unique_ptr<std::string> a, std::vector<int> b, int c) {
 *     auto log = LOG_THRIFT_CALL(DBG1, *a, b, c);
 *     ...
 *
 * This will print a line like:
 * "received thrift call from x ... params: *a="hello",b=[1,2,3],c=99,
 *
 * Only supports parameters that can be converted in to folly dynamic
 * with folly/DynamicConverter.h. It should be possible to extend to
 * add serialization implementation for custom types. We should look in
 * to adding support for arbitrary thrift (may require reflection).
 */
#define LOG_THRIFT_CALL(level, ...) \
  LOG_THRIFT_CALL_IMPL(level, nullptr, __VA_ARGS__)

/*
 * Variant of above for logging a thrift call that might get invoked
 * inline from a command line tool, rather that always via a thrift
 * request
 */
#define LOG_THRIFT_CALL_MAYBE_INLINE(level, ...) \
  LOG_THRIFT_CALL_IMPL(level, "SUDO_USER", __VA_ARGS__)

#define FB_LOG_RAW_WITH_CONTEXT(...) \
  FB_LOG_RAW(__VA_ARGS__) << "[" << folly::RequestContext::get() << "] "

namespace facebook::fboss {
class LogThriftCall {
 public:
  explicit LogThriftCall(
      const folly::Logger& logger,
      folly::LogLevel level,
      folly::StringPiece func,
      folly::StringPiece file,
      uint32_t line,
      apache::thrift::Cpp2RequestContext* ctx,
      std::string paramsStr,
      const char* identityEnvFallback = nullptr);
  ~LogThriftCall();

  // useful for wrap(Semi)?Future
  void markFailed() {
    failed_ = true;
  }

  std::string getIdentity() const {
    return identity_;
  }

 private:
  LogThriftCall(LogThriftCall const&) = delete;
  LogThriftCall& operator=(LogThriftCall const&) = delete;

  std::optional<std::string> getIdentityFromConnContext(
      apache::thrift::Cpp2ConnContext* ctx) const;

  folly::Logger logger_;
  folly::LogLevel level_;
  folly::StringPiece func_;
  folly::StringPiece file_;
  std::string identity_;
  uint32_t line_;
  std::chrono::time_point<std::chrono::steady_clock> start_;
  bool failed_{false};
  std::string paramsStr_;
};

// inspiration for this is INSTRUMENT_THRIFT_CALL in EdenServiceHandler.
//
// TODO: add version for Coro once coro based thrift server is standardized
template <typename RT>
folly::Future<RT> wrapFuture(
    std::unique_ptr<LogThriftCall> logObject,
    folly::Future<RT>&& f) {
  return std::move(f).thenTry(
      [logger = std::move(logObject)](folly::Try<RT>&& ret) mutable {
        if (logger && ret.hasException()) {
          logger->markFailed();
        }
        return std::forward<folly::Try<RT>>(ret);
      });
}

template <typename RT>
folly::SemiFuture<RT> wrapSemiFuture(
    std::unique_ptr<LogThriftCall> logObject,
    folly::SemiFuture<RT>&& f) {
  return std::move(f).defer(
      [logger = std::move(logObject)](folly::Try<RT>&& ret) mutable {
        if (logger && ret.hasException()) {
          logger->markFailed();
        }
        return std::forward<folly::Try<RT>>(ret);
      });
}

} // namespace facebook::fboss
