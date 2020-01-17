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

#include "fboss/agent/gen-cpp2/switch_config_types.h"

extern "C" {
#include <bcm/cosq.h>
}

namespace facebook::fboss::utility {

bcm_cosq_control_drop_limit_alpha_value_t cfgAlphaToBcmAlpha(
    cfg::MMUScalingFactor cfgAlpha);
cfg::MMUScalingFactor bcmAlphaToCfgAlpha(
    bcm_cosq_control_drop_limit_alpha_value_t bcmAlpha);

cfg::QueueCongestionBehavior cfgQuenBehaviorFromBcmAqm(
    const bcm_cosq_gport_discard_t& discard);

bcm_cosq_gport_discard_t initBcmCosqDiscard(
    cfg::QueueCongestionBehavior behavior);

/**
 * If detection == std::nullopt, we need to set the threshold to default value
 * and no BCM_COSQ_DISCARD_ENABLE in discard->flags
 */
bcm_cosq_gport_discard_t cfgAqmToBcmAqm(
    cfg::QueueCongestionBehavior behavior,
    std::optional<cfg::QueueCongestionDetection> detection,
    const cfg::ActiveQueueManagement& defaultAqm);

/**
 * If the discard profile in BRCM is not enabled or or thresholds aren't set,
 * returns std::nullopt.
 */
std::optional<cfg::ActiveQueueManagement> bcmAqmToCfgAqm(
    const bcm_cosq_gport_discard_t& discard,
    const cfg::ActiveQueueManagement& defaultAqm);

using BcmSchedulingAndWeight = std::pair<int, int>;
using CfgSchedulingAndWeight = std::pair<cfg::QueueScheduling, int>;
BcmSchedulingAndWeight cfgSchedulingAndWeightToBcm(
    const CfgSchedulingAndWeight& pair);

CfgSchedulingAndWeight bcmSchedulingAndWeightToCfg(
    const BcmSchedulingAndWeight& pair);

} // namespace facebook::fboss::utility
