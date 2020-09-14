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
  explicit BcmHwTableStatManager(
      const BcmSwitch* hw,
      bool isAlpmEnabled = false)
      : hw_(hw), isAlpmEnabled_(isAlpmEnabled) {}

  void refresh(const StateDelta& delta, HwResourceStats* stats);
  void publish(HwResourceStats stats) const;

 private:
  bool refreshHwStatusStats(HwResourceStats* stats);
  // Stats for both LPM and ALPM mode
  bool refreshLPMStats(HwResourceStats* stats);
  // Stats only supported in LPM mode
  bool refreshLPMOnlyStats(HwResourceStats* stats);
  // Stats pertaining to FP
  bool refreshFPStats(HwResourceStats* stats);

  void updateBcmStateChangeStats(
      const StateDelta& delta,
      HwResourceStats* stats);

  void decrementBcmMirrorStat(
      const std::shared_ptr<Mirror>& removedMirror,
      HwResourceStats* stats);
  void incrementBcmMirrorStat(
      const std::shared_ptr<Mirror>& addedMirror,
      HwResourceStats* stats);

  const BcmSwitch* hw_{nullptr};

  HwResourceStats stats_;
  bool isAlpmEnabled_;
};

} // namespace facebook::fboss
