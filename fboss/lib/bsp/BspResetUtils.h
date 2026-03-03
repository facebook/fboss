// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <optional>
#include <string>

namespace facebook::fboss {

/**
 * Common utility functions for hardware reset operations via sysfs.
 *
 * These functions provide a standardized way to perform hardware resets
 * for components like MDIO controllers and PHY retimers that expose
 * reset control through sysfs paths.
 *
 * Reset convention:
 *   - Write "1" to sysfs path: Hold component in reset state
 *   - Write "0" to sysfs path: Release component from reset state
 */

/**
 * Hold a hardware component in reset state by writing "1" to its reset path.
 *
 * @param resetPath The sysfs path for reset control
 * @param componentName Human-readable name for logging (e.g., "MDIO controller
 * 5")
 * @return true if reset was successfully applied, false otherwise
 */
bool holdResetViaSysfs(
    const std::optional<std::string>& resetPath,
    const std::string& componentName);

/**
 * Release a hardware component from reset state by writing "0" to its reset
 * path.
 *
 * @param resetPath The sysfs path for reset control
 * @param componentName Human-readable name for logging (e.g., "PHY 7")
 * @return true if reset was successfully released, false otherwise
 */
bool releaseResetViaSysfs(
    const std::optional<std::string>& resetPath,
    const std::string& componentName);

} // namespace facebook::fboss
