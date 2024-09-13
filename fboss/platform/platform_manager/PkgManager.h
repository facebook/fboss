// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/platform/platform_manager/gen-cpp2/platform_manager_config_types.h"

DECLARE_bool(enable_pkg_mgmnt);
DECLARE_bool(reload_kmods);
DECLARE_string(local_rpm_path);

namespace facebook::fboss::platform::platform_manager {

class PkgManager {
 public:
  explicit PkgManager(const PlatformConfig& config);
  virtual ~PkgManager() = default;
  void processAll() const;
  // Returns true if the rpm is newly installed, false otherwise.
  virtual bool processRpms() const;
  virtual void processKmods() const;
  virtual void processLocalRpms() const;
  virtual void loadBSPKmods() const;
  virtual void loadUpstreamKmods() const;
  std::string getKmodsRpmName() const;
  std::string getKmodsRpmBaseWithKernelName() const;

 private:
  void loadKmod(const std::string& moduleName) const;
  void unloadKmod(const std::string& moduleName) const;
  bool isRpmInstalled(const std::string& rpmFullName) const;
  void installRpm(const std::string& rpmFullName, int maxAttempts) const;
  void removeOldRpms(const std::string& rpmBaseName) const;
  void runDepmod() const;
  void installLocalRpm(int maxAttempts) const;

  const PlatformConfig& platformConfig_;
};

} // namespace facebook::fboss::platform::platform_manager
