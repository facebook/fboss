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

#include "fboss/agent/hw/bcm/gen-cpp2/bcmswitch_types.h"
#include "fboss/agent/state/StateDelta.h"

namespace facebook { namespace fboss {

class BcmSwitch;

class BcmHwTableStatManager {
 public:
  explicit BcmHwTableStatManager(
      const BcmSwitch* hw,
      bool isAlpmEnabled = false)
      : hw_(hw), isAlpmEnabled_(isAlpmEnabled) {}

  void refresh(const StateDelta& delta, BcmHwTableStats* stats);
  void publish(BcmHwTableStats stats) const;

 private:
  bool refreshHwStatusStats(BcmHwTableStats* stats);
  // Stats for both LPM and ALPM mode
  bool refreshLPMStats(BcmHwTableStats* stats);
  // Stats only supported in LPM mode
  bool refreshLPMOnlyStats(BcmHwTableStats* stats);
  // Stats pertaining to FP
  bool refreshFPStats(BcmHwTableStats* stats);

  void updateBcmStateChangeStats(
      const StateDelta& delta,
      BcmHwTableStats* stats);

  void decrementBcmMirrorStat(
      const std::shared_ptr<Mirror>& removedMirror,
      BcmHwTableStats* stats);
  void incrementBcmMirrorStat(
      const std::shared_ptr<Mirror>& addedMirror,
      BcmHwTableStats* stats);

  const BcmSwitch* hw_{nullptr};

  BcmHwTableStats stats_;
  bool isAlpmEnabled_;
};
}}
