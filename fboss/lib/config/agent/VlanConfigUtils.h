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
#include "fboss/agent/types.h"

namespace facebook::fboss {
class HwAsic;
namespace utility {

auto constexpr kDefaultVlanId1 = 1;
auto constexpr kDefaultVlanId4094 = 4094;
auto constexpr kFbossLoopbackVlanId = 10;

cfg::Vlan createVlanConfig(VlanID id);

// Return the default VLAN ID required by the ASIC's hardware pipeline.
int32_t getDefaultVlanId(const HwAsic& asic);

// Add the non-routable default VLAN required by the NPU, using the VLAN ID
// selected by its hardware pipeline. Non-NPU switch types and ASICs without a
// hardware default VLAN receive no config object.
void addDefaultVlan(cfg::SwitchConfig& config, const HwAsic& asic);

} // namespace utility
} // namespace facebook::fboss
