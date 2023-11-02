// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/fsdb/common/Utils.h"
#include <fmt/format.h>
#include <folly/String.h>
#include <folly/logging/xlog.h>
#include <folly/system/ThreadName.h>
#include <sstream>

namespace facebook::fboss::fsdb {
// static
int64_t Utils::getNowSec() {
  return std::chrono::duration_cast<std::chrono::seconds>(
             std::chrono::system_clock::now().time_since_epoch())
      .count();
}

// static
std::string Utils::stringifyTimestampSec(int64_t timestampSec) {
  // A sample value of timestampSec is "1615705308" i.e. 10 chars. We use 11
  // chars and pad with 0 on the left to ensure numerical order is same as
  // lexicographic order.
  // TODO: quantify when this will overflow
  // NOTE: DO NOT MODIFY ACROSS RELEASES
  return fmt::format("{:011}", timestampSec);
}

// static
std::tuple<Metric, int64_t> Utils::decomposeKey(const std::string& key) {
  // TODO: optimization - need only last token instead of all of them

  std::vector<std::string_view> tokens;
  folly::split('.', key, tokens);
  CHECK_GE(tokens.size(), 2);

  const auto metric = folly::join(".", tokens.begin(), tokens.end() - 1);
  const auto timestampSec = folly::to<int64_t>(tokens.at(tokens.size() - 1));
  return {metric, timestampSec};
}

// static
FqMetric Utils::getFqMetric(
    const PublisherId& publisherId,
    const Metric& metric) {
  FqMetric ret;
  ret.publisherId() = publisherId;
  ret.metric() = metric;
  return ret;
}

OperDelta createDelta(std::vector<OperDeltaUnit>&& deltaUnits) {
  OperDelta delta;
  delta.changes() = deltaUnits;
  delta.protocol() = OperProtocol::BINARY;
  return delta;
}
} // namespace facebook::fboss::fsdb
