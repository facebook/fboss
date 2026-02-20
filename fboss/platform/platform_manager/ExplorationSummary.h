// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "fboss/platform/platform_manager/ExplorationErrors.h"
#include "fboss/platform/platform_manager/gen-cpp2/platform_manager_service_types.h"

namespace facebook::fboss::platform::platform_manager {

class ExplorationSummary {
 public:
  // ODS counters
  constexpr static auto kTotalFailures = "platform_explorer.total_failures";
  constexpr static auto kTotalExpectedFailures =
      "platform_explorer.total_expected_failures";
  constexpr static auto kExplorationFail = "platform_explorer.exploration_fail";
  constexpr static auto kExplorationFailByType =
      "platform_explorer.exploration_fail.{}";

  explicit ExplorationSummary(const PlatformConfig& config);
  virtual ~ExplorationSummary() = default;
  void addError(
      ExplorationErrorType errorType,
      const std::string& devicePath,
      const std::string& message);
  void addError(
      ExplorationErrorType errorType,
      const std::string& slotPath,
      const std::string& deviceName,
      const std::string& errorMessage);
  // 1. Prints the exploration summary.
  // 2. Publish relevant data to ODS.
  // 3. Log status to structured logger.
  // Return final exploration status.
  ExplorationStatus summarize(
      const std::unordered_map<std::string, std::string>& firmwareVersions,
      const std::unordered_map<std::string, std::string>& hardwareVersions);
  std::map<std::string, std::vector<ExplorationError>> getFailedDevices();

 private:
  const PlatformConfig& platformConfig_;
  uint nExpectedErrs_{0}, nErrs_{0};
  std::map<std::string, std::vector<ExplorationError>> devicePathToErrors_{},
      devicePathToExpectedErrors_{};

  void print(ExplorationStatus finalStatus);
  void publishCounters(ExplorationStatus finalStatus);
};
} // namespace facebook::fboss::platform::platform_manager
