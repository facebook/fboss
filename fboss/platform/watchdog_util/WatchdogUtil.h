// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <vector>

#include "fboss/platform/watchdog_util/WatchdogInfo.h"

namespace facebook::fboss::platform::watchdog_util {

class WatchdogUtil {
 public:
  explicit WatchdogUtil(
      const std::string& devmapDir = "/run/devmap/watchdogs",
      const std::string& configFile = "");

  std::vector<WatchdogInfo> getAllWatchdogInfo();

  static void printHuman(
      const std::vector<WatchdogInfo>& infos,
      const std::string& devmapDir);
  static void printJson(const std::vector<WatchdogInfo>& infos);

 private:
  std::string devmapDir_;
  std::string configFile_;
};

} // namespace facebook::fboss::platform::watchdog_util
