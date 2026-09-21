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
#include "fboss/agent/state/StateDelta.h"

namespace facebook::fboss {

class BcmSwitch;

class BcmHwTableStatManager {
 public:
  explicit BcmHwTableStatManager(const BcmSwitch* hw);

  void refresh(const StateDelta& delta, HwResourceStats* stats) const;

 private:
  bool refreshHwStatusStats(HwResourceStats* stats) const;
  // Stats for both LPM and ALPM mode
  bool refreshLPMStats(HwResourceStats* stats) const;
  // Stats only supported in LPM mode
  bool refreshLPMOnlyStats(HwResourceStats* stats) const;
  // Stats pertaining to FP
  bool refreshFPStats(HwResourceStats* stats) const;
  // Free route counts in ALPM mode
  bool refreshAlpmFreeRouteCounts(HwResourceStats* stats) const;
  // Stats pertaining to EM
  bool refreshEMStats(HwResourceStats* stats) const;
  void updateBcmStateChangeStats(
      const StateDelta& delta,
      HwResourceStats* stats) const;

  void decrementBcmMirrorStat(
      const std::shared_ptr<Mirror>& removedMirror,
      HwResourceStats* stats) const;
  void incrementBcmMirrorStat(
      const std::shared_ptr<Mirror>& addedMirror,
      HwResourceStats* stats) const;
  int getAlpmMaxRouteCount(int& count, bool isV6) const;
  int getAlpmUsedRouteCount(int& count, bool isV6) const;

  using AlpmUsedRouteCounts = std::array<int, 3>;
  static const auto kIpv4AlpmUsedIndex = 0;
  static const auto kIpv6Mask64AlpmUsedIndex = 1;
  static const auto kIpv6Mask128AlpmUsedIndex = 2;
  int getAlpmUsedRouteCounts(AlpmUsedRouteCounts& counts) const;

  const BcmSwitch* hw_{nullptr};

  const bool isAlpmEnabled_;
  const bool is128ByteIpv6Enabled_;
};

} // namespace facebook::fboss
