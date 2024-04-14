/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/lib/LogThriftCall.h"
#include <folly/logging/LogLevel.h>
#include <folly/logging/xlog.h>

using apache::thrift::Cpp2ConnContext;
using apache::thrift::Cpp2RequestContext;

namespace {

auto constexpr kUnknown = "unknown";

}

namespace facebook::fboss {
LogThriftCall::LogThriftCall(
    const folly::Logger& logger,
    folly::LogLevel level,
    folly::StringPiece func,
    folly::StringPiece file,
    uint32_t line,
    Cpp2RequestContext* ctx,
    std::string paramsStr,
    const char* identityEnvFallback)
    : logger_(logger),
      level_(level),
      func_(func),
      file_(file),
      line_(line),
      start_(std::chrono::steady_clock::now()),
      paramsStr_(paramsStr) {
  std::string client = kUnknown;
  std::optional<std::string> identity;
  if (ctx) {
    Cpp2ConnContext* ctx2 = ctx->getConnectionContext();
    client = ctx2->getPeerAddress()->getAddressStr();
    identity = getIdentityFromConnContext(ctx2);
  }

  if (identityEnvFallback && !identity) {
    XLOG(ERR) << "Identity fallback set: " << identityEnvFallback;
    if (auto const fallback = getenv(identityEnvFallback)) {
      identity = std::string(fallback);
    }
  }

  identity_ = identity.value_or(kUnknown);

  // this specific format is consumed by systemd-journald/rsyslogd
  if (paramsStr.empty()) {
    FB_LOG_RAW_WITH_CONTEXT(logger_, level_, file_, line_, "")
        << func_ << " thrift request received from " << client << " ("
        << identity_ << ")";
  } else {
    FB_LOG_RAW_WITH_CONTEXT(logger_, level_, file_, line_, "")
        << func_ << " thrift request received from " << client << " ("
        << identity_ << "). params: " << paramsStr;
  }
}

LogThriftCall::~LogThriftCall() {
  auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::steady_clock::now() - start_);
  auto result =
      (std::uncaught_exceptions() || failed_) ? "failed" : "succeeded";
  if (paramsStr_.empty()) {
    FB_LOG_RAW_WITH_CONTEXT(logger_, level_, file_, line_, "")
        << func_ << " thrift request " << result << " in " << ms.count()
        << "ms";
  } else {
    FB_LOG_RAW_WITH_CONTEXT(logger_, level_, file_, line_, "")
        << func_ << " thrift request " << result << " in " << ms.count()
        << "ms. " << "params: " << paramsStr_;
  }
}

} // namespace facebook::fboss
