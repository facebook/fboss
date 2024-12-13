// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/platform/helpers/PlatformFsUtils.h"
#include "fboss/platform/platform_manager/gen-cpp2/platform_manager_config_types.h"

DECLARE_bool(enable_pkg_mgmnt);
DECLARE_bool(reload_kmods);
DECLARE_string(local_rpm_path);

namespace facebook::fboss::platform::platform_manager {
namespace package_manager {
class SystemInterface {
 public:
  explicit SystemInterface();
  virtual ~SystemInterface() = default;
  virtual bool loadKmod(const std::string& moduleName) const;
  virtual bool unloadKmod(const std::string& moduleName) const;
  virtual int installRpm(const std::string& rpmFullName) const;
  virtual int depmod() const;
  virtual std::vector<std::string> getInstalledRpms(
      const std::string& rpmBaseName) const;
  virtual int removeRpms(const std::vector<std::string>& installedRpms) const;
  virtual std::string lsmod() const;
  virtual bool isRpmInstalled(const std::string& rpmFullName) const;
  std::string getHostKernelVersion() const;
  int installLocalRpm() const;
};
} // namespace package_manager

class PkgManager {
 public:
  explicit PkgManager(
      const PlatformConfig& config,
      const std::shared_ptr<package_manager::SystemInterface>& systemInterface =
          std::make_shared<package_manager::SystemInterface>(),
      const std::shared_ptr<PlatformFsUtils>& platformFsUtils =
          std::make_shared<PlatformFsUtils>());
  virtual ~PkgManager() = default;
  virtual void processAll() const;
  virtual void processRpms() const;
  void processLocalRpms() const;
  virtual void unloadBspKmods() const;
  virtual void loadRequiredKmods() const;

 private:
  std::string getKmodsRpmName() const;
  std::string getKmodsRpmBaseWithKernelName() const;

  const PlatformConfig& platformConfig_;
  const std::shared_ptr<package_manager::SystemInterface> systemInterface_;
  const std::shared_ptr<PlatformFsUtils> platformFsUtils_;
};

} // namespace facebook::fboss::platform::platform_manager
