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

#include <folly/MapUtil.h>

namespace facebook::fboss::normalization {

std::optional<double> TransformHandler::bps(
    const std::string& portName,
    const std::string& propertyName,
    StatTimestamp propertyTimestamp,
    int64_t propertyValue) {
  auto maybeRate =
      rate(portName, propertyName, propertyTimestamp, propertyValue);
  if (maybeRate) {
    return handleBytesToBits(*maybeRate);
  }
  return std::nullopt;
}

std::optional<double> TransformHandler::rate(
    const std::string& portName,
    const std::string& propertyName,
    StatTimestamp propertyTimestamp,
    int64_t propertyValue) {
  return handleRate(
      Counter(propertyTimestamp, propertyValue), portName, propertyName);
}

std::optional<double> TransformHandler::handleRate(
    const Counter& counter,
    const std::string& portName,
    const std::string& propertyName) {
  if (auto lastPtr =
          folly::get_ptr(lastCounterCache_, portName, propertyName)) {
    double rate = static_cast<double>(counter.value - lastPtr->value) /
        (counter.timestamp - lastPtr->timestamp);
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
