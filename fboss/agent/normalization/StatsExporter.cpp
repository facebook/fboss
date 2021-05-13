/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/normalization/StatsExporter.h"

#include <fmt/core.h>

#include <folly/String.h>
#include <folly/logging/xlog.h>

namespace facebook::fboss::normalization {

namespace {
constexpr std::string_view kEntitySuffix = ".FBNet";
}

void StatsExporter::publishPortStats(
    const std::string& portName,
    const std::string& propertyName,
    int64_t timestamp,
    double value,
    int32_t intervalSec,
    std::shared_ptr<std::vector<std::string>> tags) {
  OdsCounter counter;
  counter.entity = fmt::format("{}:{}{}", deviceName_, portName, kEntitySuffix);
  counter.key = fmt::format("FBNet:interface.{}", propertyName);
  counter.unixTime = timestamp;
  counter.value = value;
  counter.intervalSec = intervalSec;
  counter.tags = std::move(tags);

  counterBuffer_.push_back(std::move(counter));
}

void GlogStatsExporter::flushCounters() {
  for (auto& counter : counterBuffer_) {
    XLOGF(
        INFO,
        "Normalized counter - Entity: {}, Key: {}, Unixtime: {}, Value: {}, Interval: {}, Tags: [{}]",
        counter.entity,
        counter.key,
        counter.unixTime,
        counter.value,
        counter.intervalSec,
        counter.tags ? folly::join(", ", *counter.tags) : "");
  }
  counterBuffer_.clear();
}

} // namespace facebook::fboss::normalization
