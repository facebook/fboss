// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <string>
#include <vector>

#include "fboss/platform/platform_manager/gen-cpp2/platform_manager_config_types.h"

namespace facebook::fboss::platform::platform_manager {

enum class DiffType {
  ADDED,
  REMOVED,
  MODIFIED,
};

struct ConfigDifference {
  DiffType type;
  std::string pmUnitName;
  std::string versionInfo; // e.g., "default → v10"
  std::string deviceName;
  std::string fieldName;
  std::optional<std::string> oldValue;
  std::optional<std::string> newValue;
};

class ConfigDiffer {
 public:
  // Compare two PmUnitConfig objects and return a list of differences
  void comparePmUnitConfigs(
      const PmUnitConfig& config1,
      const PmUnitConfig& config2,
      const std::string& pmUnitName,
      const std::string& versionInfo);

  // Compare versioned configs for all PmUnits
  void compareVersionedConfigs(const PlatformConfig& config);

  // Compare versioned configs for a specific PmUnit
  void compareVersionedConfigs(
      const PlatformConfig& config,
      const std::string& pmUnitName);

  // Print differences to stdout
  void printDifferences();

  // Get accumulated differences
  const std::vector<ConfigDifference>& getDiffs() const;

 private:
  // Helper methods for comparing specific config components
  void compareI2cDeviceConfigs(
      const std::vector<I2cDeviceConfig>& configs1,
      const std::vector<I2cDeviceConfig>& configs2,
      const std::string& pmUnitName,
      const std::string& versionInfo);

  // Helper to add a difference
  void addDifference(
      const std::string& pmUnitName,
      const std::string& versionInfo,
      DiffType type,
      const std::string& deviceName,
      const std::string& fieldName,
      const std::optional<std::string>& oldValue = std::nullopt,
      const std::optional<std::string>& newValue = std::nullopt);

  // Member to accumulate differences during comparison
  std::vector<ConfigDifference> diffs_;
};

} // namespace facebook::fboss::platform::platform_manager
