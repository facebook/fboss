// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <memory>
#include <optional>
#include <string>
#include <utility>

#include "fboss/agent/gen-cpp2/switch_state_types.h"

namespace facebook::fboss {

class AgentDirectoryUtil;
class HwAsicTable;
class SwitchState;
class RoutingInformationBase;

/**
 * Constructs file path for legacy force cold boot once flag.
 */
std::string getForceColdBootOnceFlagLegacy(const std::string& warmBootDir);

/**
 * Constructs file path for legacy warmboot flag.
 */
std::string getWarmBootFlagLegacy(const AgentDirectoryUtil* directoryUtil);

/**
 * Constructs file path for warmboot thrift switch state file.
 */
std::string getWarmBootThriftSwitchStateFile(
    const std::string& warmBootDir,
    const std::string& thriftSwitchStateFile);

/**
 * Checks and clears warmboot flags, returning whether warmboot can proceed.
 * This checks:
 * - Force cold boot flag (if present, returns false)
 * - Warmboot flag (if absent, returns false)
 * - ASIC warmboot support (if not supported, returns false)
 * - Switch state file existence (must exist if warmboot flag present)
 * - Command line flags
 *
 * Returns true if all conditions for warmboot are met.
 */
bool checkAndClearWarmBootFlags(
    const AgentDirectoryUtil* directoryUtil,
    HwAsicTable* asicTable);

/**
 * Logs boot information (type, SDK version, agent version) to boot history
 * file.
 */
void logBootHistory(
    const AgentDirectoryUtil* directoryUtil,
    const std::string& bootType,
    const std::string& sdkVersion,
    const std::string& agentVersion);

/**
 * Reconstructs SwitchState and RoutingInformationBase from warmboot state.
 * If warmBootState is present, reconstructs from saved state.
 * Otherwise, creates cold boot state with optional default VRF.
 */
std::pair<std::shared_ptr<SwitchState>, std::unique_ptr<RoutingInformationBase>>
reconstructStateAndRib(std::optional<state::WarmbootState> wbState, bool hasL3);

} // namespace facebook::fboss
