// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/platform/platform_manager/gen-cpp2/platform_manager_config_types.h"

namespace facebook::fboss::platform::platform_manager {

class PkgUtils {
 public:
  void processRpms(const PlatformConfig& config) const;
  void processKmods(const PlatformConfig& config) const;

 private:
  void reloadKmod(const std::string& moduleName) const;
  bool isRpmInstalled(const std::string& rpmFullName) const;
  void installRpm(const std::string& rpmFullName, int maxAttempts) const;
};

} // namespace facebook::fboss::platform::platform_manager
