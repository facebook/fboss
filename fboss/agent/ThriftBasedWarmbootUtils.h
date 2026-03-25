// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/agent/gen-cpp2/switch_state_types.h"

namespace facebook::fboss {

class HwAsicTable;
class HwSwitchThriftClientTable;

/**
 * Checks if warmboot can be performed from HwSwitch.
 * This checks:
 * - Multi-switch mode is enabled
 * - HwSwitch thrift client table is available
 * - Only single switch is configured (multi-switch support TODO)
 * - HwAgent is in CONFIGURED state
 *
 * If warmboot is possible, returns the warmboot state.
 * Else return nullopt.
 */
std::optional<state::WarmbootState> checkAndGetWarmbootStateFromHwSwitch(
    bool isRunModeMultiSwitch,
    const HwAsicTable* asicTable,
    HwSwitchThriftClientTable* hwSwitchThriftClientTable);

} // namespace facebook::fboss
