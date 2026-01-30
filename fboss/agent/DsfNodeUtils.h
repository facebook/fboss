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

#include "fboss/agent/gen-cpp2/agent_config_types.h"
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/hw/switch_asics/HwAsic.h"

namespace facebook::fboss {
struct AgentConfig;
}

namespace facebook::fboss::utility {

bool isDualStage(const cfg::AgentConfig& cfg);
bool isDualStage(const cfg::SwitchConfig& cfg);
bool isDualStage(const AgentConfig& cfg);

int64_t maxDsfSwitchId(const AgentConfig& cfg);
int64_t maxDsfSwitchId(const cfg::AgentConfig& cfg);
int64_t maxDsfSwitchId(const cfg::SwitchConfig& cfg);

// Get the maximum switch ID to configure in the system based on DSF topology.
// R3/J3 asics have a HW bug, whereby we need to constrain the max switch id
// that's used in their deployment to be below the default (HW advertised)
// value. This utility determines the appropriate max switch ID.
uint32_t getDsfVoqSwitchMaxSwitchId();
uint32_t getDsfFabricSwitchMaxSwitchId(
    const AgentConfig& agentConfig,
    HwAsic::FabricNodeRole fabricNodeRole);

} // namespace facebook::fboss::utility
