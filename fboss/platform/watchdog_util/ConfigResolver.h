// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <memory>
#include <string>
#include <vector>

#include "fboss/platform/platform_manager/gen-cpp2/platform_manager_config_types.h"
#include "fboss/platform/watchdog_util/WatchdogInfo.h"

namespace facebook::fboss::platform::watchdog_util {

class ConfigResolver {
 public:
  explicit ConfigResolver(
      const std::string& devmapDir = "/run/devmap/watchdogs",
      const std::string& configFile = "");

  ~ConfigResolver();

  // Resolve all watchdogs found in devmapDir using platform_manager.json
  std::vector<ResolvedWatchdog> resolveAll();

 private:
  std::string devmapDir_;
  std::string configFile_;

  // Helpers
  std::vector<ResolvedWatchdog> enumerateDevmap();
  void loadPlatformConfig();
  void resolveDevicePath(ResolvedWatchdog& rw);
  void resolveHardwareDetails(ResolvedWatchdog& rw);

  // Platform config (deserialized)
  struct PlatformConfigHolder {
    facebook::fboss::platform::platform_manager::PlatformConfig config;
    bool loaded{false};
  };
  std::unique_ptr<PlatformConfigHolder> configHolder_;
};

} // namespace facebook::fboss::platform::watchdog_util
