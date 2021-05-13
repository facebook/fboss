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

#include <string>
#include <vector>

#include "folly/container/F14Map.h"

#include "fboss/agent/normalization/Types.h"

namespace facebook::fboss::normalization {

// where the actual transform is performed. It should be stateful to be
// able to process transforms such as RATE and BPS
class TransformHandler {
 public:
  // (cur_bytes - prev_bytes) / interval * 8
  // return nullopt if bps calculation failed due to missing prev_bytes data
  std::optional<double> bps(
      const std::string& portName,
      const std::string& propertyName,
      StatTimestamp propertyTimestamp,
      int64_t propertyValue,
      int32_t processIntervalSec);

  // (cur_value - prev_value) / interval
  std::optional<double> rate(
      const std::string& portName,
      const std::string& propertyName,
      StatTimestamp propertyTimestamp,
      int64_t propertyValue,
      int32_t processIntervalSec);

 private:
  std::optional<double> handleRate(
      const Counter& counter,
      const std::string& portName,
      const std::string& propertyName,
      int32_t processIntervalSec);

  static constexpr double handleBytesToBits(double bytesValue) {
    return bytesValue * 8;
  }

  // {portName -> {propertyName, lastCounter}}
  folly::F14FastMap<std::string, folly::F14FastMap<std::string, Counter>>
      lastCounterCache_;
};

} // namespace facebook::fboss::normalization
