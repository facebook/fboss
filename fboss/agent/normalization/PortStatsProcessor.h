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
#include "fboss/agent/normalization/Types.h"
#include "fboss/agent/types.h"

namespace facebook::fboss::normalization {

class TransformHandler;
class StatsExporter;

class PortStatsProcessor {
 public:
  PortStatsProcessor(
      TransformHandler* transformHandler,
      StatsExporter* statsExporter);

  void processStats(
      const folly::F14FastMap<std::string, HwPortStats>& hwStatsMap);

 private:
  void process(
      const std::string& portName,
      const std::string& normalizedPropertyName,
      StatTimestamp propertyTimestamp,
      int64_t propertyValue,
      TransformType type);

  TransformHandler* transformHandler_;
  StatsExporter* statsExporter_;
};

} // namespace facebook::fboss::normalization
