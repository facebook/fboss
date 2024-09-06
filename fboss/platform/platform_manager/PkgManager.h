// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/platform/platform_manager/gen-cpp2/platform_manager_config_types.h"

namespace facebook::fboss::platform::platform_manager {

class PkgManager {
 public:
  explicit PkgManager(const PlatformConfig& config);
  void processAll() const;
  void processRpms() const;
  void processKmods() const;
  void processLocalRpms() const;
  void loadUpstreamKmods() const;
  std::string getKmodsRpmName() const;

 private:
  void loadKmod(const std::string& moduleName) const;
  void unloadKmod(const std::string& moduleName) const;
  bool isRpmInstalled(const std::string& rpmFullName) const;
  void installRpm(const std::string& rpmFullName, int maxAttempts) const;
  void installLocalRpm(int maxAttempts) const;

  const PlatformConfig& platformConfig_;
};

} // namespace facebook::fboss::platform::platform_manager
