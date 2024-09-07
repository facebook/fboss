/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/HwBasePortFb303Stats.h"

namespace facebook::fboss {

void HwBasePortFb303Stats::updateQueueWatermarkStats(
    const std::map<int16_t, int64_t>&
    /*queueWatermarks*/) const {}
void HwBasePortFb303Stats::updateEgressGvoqWatermarkStats(
    const std::map<int16_t, int64_t>& /*gvoqWatermarks*/) const {}
} // namespace facebook::fboss
