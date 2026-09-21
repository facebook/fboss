// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/lib/ThriftMethodRateLimitSetup.h"

#include "fboss/lib/ThriftMethodRateLimit.h"

#include <fb303/ExportType.h>
#include <fb303/ServiceData.h>
#include <folly/logging/xlog.h>
#include <thrift/lib/cpp2/server/ThriftServer.h>

#include <memory>

namespace facebook::fboss {

void installThriftMethodRateLimit(
    apache::thrift::ThriftServer& server,
    const std::map<std::string, double>& method2QpsLimit,
    bool shadowMode) {
  if (method2QpsLimit.empty()) {
    return;
  }
  auto odsCounterUpdateFunc = [](const std::string& method,
                                 uint64_t count,
                                 uint64_t aggCount) {
    XLOG(DBG2) << "Thrift method " << method << " rate limited " << count
               << " times" << ", total number of thrift rate limit deny "
               << aggCount;
    // Update ODS counter for rate-limited thrift methods
    facebook::fb303::fbData->addStatValue(
        "thrift.method." + method + ".rate_limited", 1, facebook::fb303::SUM);
    facebook::fb303::fbData->addStatValue(
        "thrift.method.aggregate.rate_limited", 1, facebook::fb303::SUM);
  };
  auto rateLimiter = std::make_shared<ThriftMethodRateLimit>(
      method2QpsLimit, shadowMode, odsCounterUpdateFunc);
  auto preprocessFunc =
      ThriftMethodRateLimit::getThriftMethodRateLimitPreprocessFunc(
          std::move(rateLimiter));
  server.addPreprocessFunc("ThriftMethodRateLimit", std::move(preprocessFunc));
}

} // namespace facebook::fboss
