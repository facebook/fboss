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

#include "fboss/agent/HwSwitch.h"
#include "fboss/agent/gen-cpp2/switch_config_types.h"

namespace facebook::fboss::utility {

void addOlympicQosToConfig(cfg::SwitchConfig& config, const HwSwitch* hwSwitch);

/*
 * Used to determine whether full-hash or half-hash config should be used when
 * enabling load balancing via addLoadBalancerToConfig().
 *
 * (LBHash = Load Balancer Hash)
 */
enum class LBHash : uint8_t {
  FULL_HASH = 0,
  HALF_HASH = 1,
};

void addLoadBalancerToConfig(
    cfg::SwitchConfig& config,
    const HwSwitch* hwSwitch,
    LBHash hashType);

uint16_t uplinksCountFromSwitch(const HwSwitch* hwSwitch);

cfg::SwitchConfig createProdRswConfig(
    const HwSwitch* hwSwitch,
    const std::vector<PortID>& masterLogicalPortIds);

cfg::SwitchConfig createProdFswConfig(
    const HwSwitch* hwSwitch,
    const std::vector<PortID>& masterLogicalPortIds);

} // namespace facebook::fboss::utility
