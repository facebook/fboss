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

#include <memory>

#include "folly/container/F14Map.h"

#include "fboss/agent/hw/gen-cpp2/hardware_stats_types.h"
#include "fboss/agent/normalization/CounterTagManager.h"
#include "fboss/agent/normalization/StatsExporter.h"
#include "fboss/agent/normalization/TransformHandler.h"

DECLARE_string(default_device_name);

namespace facebook::fboss {

class Normalizer {
 public:
  static std::shared_ptr<Normalizer> getInstance();

  Normalizer();

  void processStats(
      const folly::F14FastMap<std::string, HwPortStats>& hwStatsMap);

  void processLinkStateChange(const std::string& portName, bool isUp);

  // delegate to CounterTagManager
  void reloadCounterTags(const cfg::SwitchConfig& curConfig);

 private:
  const std::string deviceName_;
  std::unique_ptr<normalization::TransformHandler> transformHandler_;
  std::unique_ptr<normalization::StatsExporter> statsExporter_;
  std::unique_ptr<normalization::CounterTagManager> counterTagManager_;
};

} // namespace facebook::fboss
