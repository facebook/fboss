/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/normalization/TransformHandler.h"

#include <optional>

#include <folly/MapUtil.h>

namespace facebook::fboss::normalization {

std::optional<double> TransformHandler::bps(
    const std::string& portName,
    const std::string& propertyName,
    StatTimestamp propertyTimestamp,
    int64_t propertyValue,
    int32_t processIntervalSec) {
  auto maybeRate = rate(
      portName,
      propertyName,
      propertyTimestamp,
      propertyValue,
      processIntervalSec);
  if (maybeRate) {
    return handleBytesToBits(*maybeRate);
  }
  return std::nullopt;
}

std::optional<double> TransformHandler::rate(
    const std::string& portName,
    const std::string& propertyName,
    StatTimestamp propertyTimestamp,
    int64_t propertyValue,
    int32_t processIntervalSec) {
  return handleRate(
      Counter(propertyTimestamp, propertyValue),
      portName,
      propertyName,
      processIntervalSec);
}

std::optional<double> TransformHandler::handleRate(
    const Counter& counter,
    const std::string& portName,
    const std::string& propertyName,
    int32_t processIntervalSec) {
  if (auto lastPtr =
          folly::get_ptr(lastCounterCache_, portName, propertyName)) {
    auto timeDiff = counter.timestamp - lastPtr->timestamp;
    if (timeDiff < processIntervalSec) {
      // skip processing this counter
      return std::nullopt;
    }

    double rate =
        static_cast<double>(counter.value - lastPtr->value) / timeDiff;
    *lastPtr = counter;
    return rate;
  } else {
    // create a new entry
    lastCounterCache_[portName][propertyName] = counter;
    // no rate for the new counter
    return std::nullopt;
  }
}

} // namespace facebook::fboss::normalization
