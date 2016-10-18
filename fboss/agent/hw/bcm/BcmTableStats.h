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
  explicit BcmTableStats(const BcmSwitch* hw) : hw_(hw) {}
  void refresh() {
    stats_.hw_table_stats_stale =
        !(refreshHwStatusStats() && refreshLPMStats());
  }
  void publish() const;

 private:
  bool refreshHwStatusStats();
  bool refreshLPMStats();
  const BcmSwitch* hw_{nullptr};

  BcmHwTableStats stats_;
};
}}
