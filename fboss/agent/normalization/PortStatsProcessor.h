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

#include "folly/container/F14Map.h"

#include "fboss/agent/hw/gen-cpp2/hardware_stats_types.h"
#include "fboss/agent/normalization/CounterTagManager.h"
#include "fboss/agent/normalization/Types.h"
#include "fboss/agent/types.h"

namespace facebook::fboss::normalization {

class TransformHandler;
class StatsExporter;

class PortStatsProcessor {
 public:
  PortStatsProcessor(
      TransformHandler* transformHandler,
      StatsExporter* statsExporter,
      CounterTagManager* counterTagManager);

  void processStats(
      const folly::F14FastMap<std::string, HwPortStats>& hwStatsMap);

 private:
  // @param processIntervalSec: ODS has a minimal interval to accept a data
  // point which is 15 seconds. Also, existing collections have different data
  // interval for different counters. It's very important to support that to not
  // break existing use cases during onbox migration
  void process(
      const std::string& portName,
      const std::string& normalizedPropertyName,
      StatTimestamp propertyTimestamp,
      int64_t propertyValue,
      TransformType type,
      int32_t processIntervalSec);

  TransformHandler* transformHandler_;
  StatsExporter* statsExporter_;
  CounterTagManager* counterTagManager_;
};

} // namespace facebook::fboss::normalization
