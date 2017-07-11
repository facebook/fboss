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

namespace facebook { namespace fboss {

class BcmSwitch;

class BcmTableStats {
 public:
  explicit BcmTableStats(const BcmSwitch* hw, bool isAlpmEnabled=false) : hw_(hw),
    isAlpmEnabled_(isAlpmEnabled) {}
  void refresh() {
    stats_.hw_table_stats_stale =
        !(refreshHwStatusStats() && refreshLPMStats());
    if (!isAlpmEnabled_) {
      stats_.hw_table_stats_stale |= refreshLPMOnlyStats();
    }
  }
  void publish() const;

 private:
  bool refreshHwStatusStats();
  // Stats for both LPM and ALPM mode
  bool refreshLPMStats();
  // Stats only supported in LPM mode
  bool refreshLPMOnlyStats();
  const BcmSwitch* hw_{nullptr};

  BcmHwTableStats stats_;
  bool isAlpmEnabled_;
};
}}
