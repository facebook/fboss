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

#include "fboss/agent/hw/gen-cpp2/hardware_stats_types.h"

#include "folly/container/F14Map.h"

namespace facebook::fboss {

class Normalizer {
 public:
  static std::shared_ptr<Normalizer> getInstance();

  void processStats(
      const folly::F14FastMap<std::string, HwPortStats>& hwStatsMap);

  void processLinkStateChange(const std::string& portName, bool isUp);
};

} // namespace facebook::fboss
