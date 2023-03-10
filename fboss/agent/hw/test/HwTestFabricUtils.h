// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/types.h"
namespace facebook::fboss {

class HwSwitch;
void setForceTrafficOverFabric(const HwSwitch* hw, bool force);
void checkFabricReachability(const HwSwitch* hw);
void populatePortExpectedNeighbors(
    const std::vector<PortID>& ports,
    cfg::SwitchConfig& cfg);
} // namespace facebook::fboss
