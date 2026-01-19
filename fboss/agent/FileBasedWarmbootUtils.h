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
std::string getForceColdBootOnceFlagLegacy(
    const AgentDirectoryUtil* directoryUtil);

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
 * Checks if the force cold boot flag exists.
 * If the flag exists, it will be removed.
 * Returns true if force cold boot is requested.
 */
bool checkForceColdBootFlag(const AgentDirectoryUtil* directoryUtil);

/**
 * Checks if the warmboot flag exists.
 * If the flag exists, it will be removed.
 * Returns true if warmboot flag was present.
 */
bool checkCanWarmBootFlag(const AgentDirectoryUtil* directoryUtil);

/**
 * Checks if the ASIC supports warmboot.
 * Returns true if warmboot is supported.
 */
bool checkAsicSupportsWarmboot(HwAsicTable* asicTable);

/**
 * Checks if the warmboot state file exists.
 * Returns true if the file exists.
 */
bool checkWarmbootStateFileExists(
    const std::string& warmBootDir,
    const std::string& thriftSwitchStateFile);

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
