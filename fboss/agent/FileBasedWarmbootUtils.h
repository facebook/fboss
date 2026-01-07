// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <string>

namespace facebook::fboss {

class AgentDirectoryUtil;
class HwAsicTable;

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

} // namespace facebook::fboss
