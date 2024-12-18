// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/platform/platform_manager/PkgManager.h"

#include <fb303/ServiceData.h>
#include <folly/FileUtil.h>
#include <folly/String.h>
#include <folly/logging/xlog.h>
#include <re2/re2.h>
#include <sys/utsname.h>
#include <thrift/lib/cpp2/protocol/Serializer.h>

#include "fboss/platform/helpers/PlatformUtils.h"

DEFINE_bool(
    enable_pkg_mgmnt,
    true,
    "Enable download and installation of the BSP rpm");

DEFINE_bool(
    reload_kmods,
    false,
    "The kmods are usually reloaded only when the BSP RPM changes. "
    "But if this flag is set, the kmods will be reloaded everytime.");

DEFINE_string(
    local_rpm_path,
    "",
    "Path to the local rpm file that needs to be installed on the system.");

namespace facebook::fboss::platform::platform_manager {
namespace {
constexpr auto kBspKmodsRpmName = "fboss_bsp_kmods_rpm";
constexpr auto kBspKmodsRpmVersionCounter = "bsp_kmods_rpm_version.{}";
constexpr auto kBspKmodsFilePath = "/usr/local/{}_bsp/{}/kmods.json";
const re2::RE2 kBspRpmNameRe = "(?P<KEYWORD>[a-z]+)_bsp_kmods";
} // namespace

namespace package_manager {
SystemInterface::SystemInterface() {}

bool SystemInterface::loadKmod(const std::string& moduleName) const {
  int exitStatus{0};
  std::string standardOut{};
  auto unloadCmd = fmt::format("modprobe {}", moduleName);
  VLOG(1) << fmt::format("Running command ({})", unloadCmd);
  std::tie(exitStatus, standardOut) = PlatformUtils().execCommand(unloadCmd);
  return exitStatus == 0;
}

bool SystemInterface::unloadKmod(const std::string& moduleName) const {
  int exitStatus{0};
  std::string standardOut{};
  auto unloadCmd = fmt::format("rmmod {}", moduleName);
  VLOG(1) << fmt::format("Running command ({})", unloadCmd);
  std::tie(exitStatus, standardOut) = PlatformUtils().execCommand(unloadCmd);
  return exitStatus == 0;
}

int SystemInterface::installRpm(const std::string& rpmFullName) const {
  int exitStatus{0};
  std::string standardOut{};
  auto cmd = fmt::format("dnf install {} --assumeyes", rpmFullName);
  VLOG(1) << fmt::format("Running command ({})", cmd);
  std::tie(exitStatus, standardOut) = PlatformUtils().execCommand(cmd);
  return exitStatus;
}

int SystemInterface::installLocalRpm() const {
  int exitStatus{0};
  std::string standardOut{};
  auto cmd = fmt::format("rpm -i --force {}", FLAGS_local_rpm_path);
  VLOG(1) << fmt::format("Running command ({})", cmd);
  std::tie(exitStatus, standardOut) = PlatformUtils().execCommand(cmd);
  return exitStatus;
}

int SystemInterface::depmod() const {
  int exitStatus{0};
  std::string standardOut{};
  auto depmodCmd = "depmod -a";
  VLOG(1) << fmt::format("Running command ({})", depmodCmd);
  std::tie(exitStatus, standardOut) = PlatformUtils().execCommand(depmodCmd);
  return exitStatus;
}

std::vector<std::string> SystemInterface::getInstalledRpms(
    const std::string& rpmBaseName) const {
  std::string standardOut{};
  int exitStatus{0};
  auto cmd = fmt::format("rpm -qa | grep ^{}", rpmBaseName);
  VLOG(1) << fmt::format("Running command ({})", cmd);
  std::tie(exitStatus, standardOut) = PlatformUtils().execCommand(cmd);
  if (exitStatus) {
    return {};
  }
  std::vector<std::string> installedRpms;
  folly::split('\n', standardOut, installedRpms);
  return installedRpms;
}

int SystemInterface::removeRpms(
    const std::vector<std::string>& installedRpms) const {
  int exitStatus{0};
  std::string standardOut;
  auto removeOldRpmsCmd =
      fmt::format("rpm -e --allmatches {}", folly::join(" ", installedRpms));
  VLOG(1) << fmt::format("Running command ({})", removeOldRpmsCmd);
  std::tie(exitStatus, standardOut) =
      PlatformUtils().execCommand(removeOldRpmsCmd);
  return exitStatus;
}

std::string SystemInterface::lsmod() const {
  VLOG(1) << "Running command (lsmod)";
  auto result = PlatformUtils().execCommand("lsmod");
  return result.second;
}

std::string SystemInterface::getHostKernelVersion() const {
  VLOG(1) << "Using system name structure for host kernel version";
  utsname result;
  uname(&result);
  return result.release;
}

bool SystemInterface::isRpmInstalled(const std::string& rpmFullName) const {
  auto cmd = fmt::format("dnf list {} --installed", rpmFullName);
  VLOG(1) << fmt::format("Running command ({})", cmd);
  auto [exitStatus, standardOut] = PlatformUtils().execCommand(cmd);
  return exitStatus == 0;
}
} // namespace package_manager

PkgManager::PkgManager(
    const PlatformConfig& config,
    const std::shared_ptr<package_manager::SystemInterface>& systemInterface,
    const std::shared_ptr<PlatformFsUtils>& platformFsUtils)
    : platformConfig_(config),
      systemInterface_(systemInterface),
      platformFsUtils_(platformFsUtils) {}

void PkgManager::processAll() const {
  SCOPE_SUCCESS {
    fb303::fbData->setCounter(kProcessAllFailure, 0);
  };
  SCOPE_FAIL {
    fb303::fbData->setCounter(kProcessAllFailure, 1);
  };

  if (FLAGS_local_rpm_path.size()) {
    fb303::fbData->setExportedValue(
        kBspKmodsRpmName, "local_rpm: " + FLAGS_local_rpm_path);
  } else if (FLAGS_enable_pkg_mgmnt) {
    fb303::fbData->setExportedValue(kBspKmodsRpmName, getKmodsRpmName());
    fb303::fbData->setCounter(
        fmt::format(
            kBspKmodsRpmVersionCounter, *platformConfig_.bspKmodsRpmVersion()),
        1);
  } else {
    fb303::fbData->setExportedValue(
        kBspKmodsRpmName, "Not managing BSP package");
  }

  // BSP management
  // If new rpm is installed, PkgManager unload first to prevent kmods leak.
  // After install, PkgManager loads kmods.
  if (FLAGS_local_rpm_path.size()) {
    unloadBspKmods();
    processLocalRpms();
    loadRequiredKmods();
    return;
  }
  if (FLAGS_enable_pkg_mgmnt) {
    auto bspKmodsRpmName = getKmodsRpmName();
    if (!systemInterface_->isRpmInstalled(bspKmodsRpmName)) {
      XLOG(INFO) << fmt::format(
          "BSP Kmods {} is not installed", bspKmodsRpmName);
      // Unload old kmods from previous BSP installation to prevent old kmods
      // from binding to devices. This relies of old kmods being listed in
      // kmods.json file.
      unloadBspKmods();
      // Install desired rpm
      processRpms();
      // In cases where kmods.json from previous BSP installation is absent
      // (like provisioning, where this is the first run of PM), the kmods might
      // be present in the initramfs
      unloadBspKmods();
      // Load required kmods from PM config.
      loadRequiredKmods();
      return;
    }
    XLOG(INFO) << fmt::format(
        "BSP Kmods {} is already installed", bspKmodsRpmName);
  }
  // Kmods management.
  // Only when no BSP management happened.
  if (FLAGS_reload_kmods) {
    unloadBspKmods();
  }
  loadRequiredKmods();
}

void PkgManager::processRpms() const {
  SCOPE_SUCCESS {
    fb303::fbData->setCounter(kProcessRpmFailure, 0);
  };
  SCOPE_FAIL {
    fb303::fbData->setCounter(kProcessRpmFailure, 1);
  };

  int exitStatus{0};
  if (auto installedRpms =
          systemInterface_->getInstalledRpms(getKmodsRpmBaseWithKernelName());
      !installedRpms.empty()) {
    XLOG(INFO) << fmt::format(
        "Removing old rpms: {}", folly::join(", ", installedRpms));
    if (exitStatus = systemInterface_->removeRpms(installedRpms);
        exitStatus != 0) {
      throw std::runtime_error(fmt::format(
          "Failed to remove old rpms ({}) with exit code {}",
          folly::join(" ", installedRpms),
          exitStatus));
    }
  } else {
    XLOG(INFO) << "Skipping removing old rpms. Reason: No rpms installed.";
  }
  auto bspKmodsRpmName = getKmodsRpmName();
  for (auto [success, attempt] = std::pair{false, 0}; attempt < 3 && !success;
       attempt++) {
    XLOG(INFO) << fmt::format(
        "Installing BSP {}, Attempt #{}", bspKmodsRpmName, attempt);
    exitStatus = systemInterface_->installRpm(bspKmodsRpmName);
    success = exitStatus == 0;
  }
  if (exitStatus != 0) {
    throw std::runtime_error(fmt::format(
        "Failed to install rpm ({}) with exit code {}",
        FLAGS_local_rpm_path,
        exitStatus));
  }
  XLOG(INFO) << "Caching kernel modules dependencies";
  if (exitStatus = systemInterface_->depmod(); exitStatus != 0) {
    XLOG(ERR) << fmt::format("Failed to depmod with exit code {}", exitStatus);
  }
}

void PkgManager::processLocalRpms() const {
  uint exitStatus{0};
  for (auto [success, attempt] = std::pair{false, 0}; attempt < 3 && !success;
       attempt++) {
    XLOG(INFO) << fmt::format(
        "Installing local rpm at {}, Attempt #{}",
        FLAGS_local_rpm_path,
        attempt);
    exitStatus = systemInterface_->installLocalRpm();
    success = exitStatus == 0;
  }
  if (exitStatus != 0) {
    throw std::runtime_error(fmt::format(
        "Failed to install rpm ({}) with exit code {}",
        FLAGS_local_rpm_path,
        exitStatus));
  }
}

void PkgManager::unloadBspKmods() const {
  SCOPE_SUCCESS {
    fb303::fbData->setCounter(kUnloadKmodsFailure, 0);
  };
  SCOPE_FAIL {
    fb303::fbData->setCounter(kUnloadKmodsFailure, 1);
  };

  std::string keyword{};
  re2::RE2::FullMatch(
      *platformConfig_.bspKmodsRpmName(), kBspRpmNameRe, &keyword);
  std::string bspKmodsFilePath = fmt::format(
      kBspKmodsFilePath, keyword, systemInterface_->getHostKernelVersion());
  auto jsonBspKmodsFile =
      platformFsUtils_->getStringFileContent(bspKmodsFilePath);
  if (!jsonBspKmodsFile) {
    // No old rpms, most likely first ever rpm installation.
    if (auto installedRpms =
            systemInterface_->getInstalledRpms(getKmodsRpmBaseWithKernelName());
        installedRpms.empty()) {
      XLOG(WARNING) << fmt::format(
          "Skipping BSP kmods unloading. Reason: {} doesn't exist; "
          "most likely because this is the first RPM installation.",
          bspKmodsFilePath);
      return;
    }
    // Unexpected read failures. Can't proceed forward.
    // This shouldn't happen in production.
    throw std::runtime_error(fmt::format(
        "Failed to unload kmods. Reason: Failed to read {}; "
        "most likely because RPM is incompatible. A possible remediation: {}",
        bspKmodsFilePath,
        fmt::format("dnf install {} --assumeyes", getKmodsRpmName())));
  }
  BspKmodsFile bspKmodsFile;
  apache::thrift::SimpleJSONSerializer::deserialize<BspKmodsFile>(
      *jsonBspKmodsFile, bspKmodsFile);
  XLOG(INFO) << "Unloading kernel modules based on kmods.json";
  const auto loadedKmods = systemInterface_->lsmod();
  for (const auto& kmod : *bspKmodsFile.bspKmods()) {
    if (loadedKmods.find(kmod) != std::string::npos) {
      XLOG(INFO) << fmt::format("Unloading {}", kmod);
      if (!systemInterface_->unloadKmod(kmod)) {
        throw std::runtime_error(fmt::format("Failed to unload ({})", kmod));
      }
    } else {
      XLOG(INFO) << fmt::format(
          "Skipping to unload {}. Reason: Already unloaded", kmod);
    }
  }
  XLOG(INFO) << "Unloading shared kernel modules";
  for (const auto& kmod : *bspKmodsFile.sharedKmods()) {
    if (loadedKmods.find(kmod) != std::string::npos) {
      XLOG(INFO) << fmt::format("Unloading {}", kmod);
      if (!systemInterface_->unloadKmod(kmod)) {
        throw std::runtime_error(fmt::format("Failed to unload ({})", kmod));
      }
    } else {
      XLOG(INFO) << fmt::format(
          "Skipping to unload {}. Reason: Already unloaded", kmod);
    }
  }
}

void PkgManager::loadRequiredKmods() const {
  XLOG(INFO) << fmt::format(
      "Loading {} required kernel modules",
      platformConfig_.requiredKmodsToLoad()->size());
  for (const auto& requiredKmod : *platformConfig_.requiredKmodsToLoad()) {
    XLOG(INFO) << fmt::format("Loading {}", requiredKmod);
    if (!systemInterface_->loadKmod(requiredKmod)) {
      fb303::fbData->setCounter(kLoadKmodsFailure, 1);
      throw std::runtime_error(
          fmt::format("Failed to load ({})", requiredKmod));
    }
  }
  fb303::fbData->setCounter(kLoadKmodsFailure, 0);
}

std::string PkgManager::getKmodsRpmName() const {
  return fmt::format(
      "{}-{}-{}",
      *platformConfig_.bspKmodsRpmName(),
      systemInterface_->getHostKernelVersion(),
      *platformConfig_.bspKmodsRpmVersion());
}

std::string PkgManager::getKmodsRpmBaseWithKernelName() const {
  return fmt::format(
      "{}-{}",
      *platformConfig_.bspKmodsRpmName(),
      systemInterface_->getHostKernelVersion());
}
} // namespace facebook::fboss::platform::platform_manager
