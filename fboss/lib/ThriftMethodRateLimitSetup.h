// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <map>
#include <string>

namespace apache::thrift {
class ThriftServer;
}

namespace facebook::fboss {

// Installs a per-thrift-method rate limiter as a preprocess function on the
// given ThriftServer. method2QpsLimit maps a thrift method name to its allowed
// queries per second; methods not present in the map are never rate limited.
// When shadowMode is true, requests that exceed their limit are logged and
// counted but still served. Deny counts are exported to fb303/ODS as
// thrift.method.<method>.rate_limited and thrift.method.aggregate.rate_limited.
// No-op when method2QpsLimit is empty.
void installThriftMethodRateLimit(
    apache::thrift::ThriftServer& server,
    const std::map<std::string, double>& method2QpsLimit,
    bool shadowMode);

} // namespace facebook::fboss
