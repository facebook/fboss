// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/platform/platform_manager/PkgManager.h"

#include <fb303/ServiceData.h>
#include <folly/String.h>
#include <folly/logging/xlog.h>
#include <sys/utsname.h>

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

std::string getHostKernelVersion() {
  utsname result;
  uname(&result);
  return result.release;
}
} // namespace

PkgManager::PkgManager(const PlatformConfig& config)
    : platformConfig_(config) {}

void PkgManager::processAll() const {
  loadUpstreamKmods();

  bool newRpmInstalled{false};
  if (FLAGS_enable_pkg_mgmnt) {
    if (!FLAGS_local_rpm_path.empty()) {
      fb303::fbData->setExportedValue(
          kBspKmodsRpmName, "local_rpm: " + FLAGS_local_rpm_path);
      processLocalRpms();
      newRpmInstalled = true;
    } else {
      fb303::fbData->setExportedValue(kBspKmodsRpmName, getKmodsRpmName());
      fb303::fbData->setCounter(
          fmt::format(
              kBspKmodsRpmVersionCounter,
              *platformConfig_.bspKmodsRpmVersion()),
          1);
      newRpmInstalled = processRpms();
    }
  } else {
    fb303::fbData->setExportedValue(
        kBspKmodsRpmName, "Not managing BSP package");
  }

  if (FLAGS_reload_kmods || newRpmInstalled) {
    processKmods();
  } else {
    loadBSPKmods();
  }
}

bool PkgManager::processRpms() const {
  auto bspKmodsRpmName = getKmodsRpmName();
  if (!isRpmInstalled(bspKmodsRpmName)) {
    XLOG(INFO) << fmt::format("Installing BSP Kmods {}", bspKmodsRpmName);
    removeOldRpms(getKmodsRpmBaseWithKernelName());
    installRpm(bspKmodsRpmName, 3 /* maxAttempts */);
    runDepmod();
    return true;
  } else {
    XLOG(INFO) << fmt::format(
        "BSP Kmods {} is already installed", bspKmodsRpmName);
    return false;
  }
}

void PkgManager::processLocalRpms() const {
  installLocalRpm(3 /* maxAttempts */);
}

void PkgManager::processKmods() const {
  XLOG(INFO) << fmt::format(
      "Unloading {} kernel modules",
      platformConfig_.bspKmodsToReload()->size());
  for (int i = 0; i < platformConfig_.bspKmodsToReload()->size(); ++i) {
    unloadKmod((*platformConfig_.bspKmodsToReload())[i]);
  }
  XLOG(INFO) << fmt::format(
      "Unloading {} shared kernel modules",
      platformConfig_.sharedKmodsToReload()->size());
  for (int i = 0; i < platformConfig_.sharedKmodsToReload()->size(); ++i) {
    unloadKmod((*platformConfig_.sharedKmodsToReload())[i]);
  }
  loadBSPKmods();
}

void PkgManager::loadBSPKmods() const {
  XLOG(INFO) << fmt::format(
      "Loading {} shared kernel modules",
      platformConfig_.sharedKmodsToReload()->size());
  for (int i = 0; i < platformConfig_.sharedKmodsToReload()->size(); ++i) {
    loadKmod((*platformConfig_.sharedKmodsToReload())[i]);
  }
  XLOG(INFO) << fmt::format(
      "Loading {} kernel modules", platformConfig_.bspKmodsToReload()->size());
  for (int i = 0; i < platformConfig_.bspKmodsToReload()->size(); ++i) {
    loadKmod((*platformConfig_.bspKmodsToReload())[i]);
  }
}

void PkgManager::loadUpstreamKmods() const {
  XLOG(INFO) << fmt::format(
      "Loading {} upstream kernel modules",
      platformConfig_.upstreamKmodsToLoad()->size());
  for (int i = 0; i < platformConfig_.upstreamKmodsToLoad()->size(); ++i) {
    loadKmod((*platformConfig_.upstreamKmodsToLoad())[i]);
  }
}

bool PkgManager::isRpmInstalled(const std::string& rpmFullName) const {
  XLOG(INFO) << fmt::format(
      "Checking whether BSP Kmods {} is installed", rpmFullName);
  auto cmd = fmt::format("dnf list {} --installed", rpmFullName);
  XLOG(INFO) << fmt::format("Running command ({})", cmd);
  auto [exitStatus, standardOut] = PlatformUtils().execCommand(cmd);
  return exitStatus == 0;
}

void PkgManager::installRpm(const std::string& rpmFullName, int maxAttempts)
    const {
  int exitStatus{0}, attempt{1};
  std::string standardOut{};
  auto cmd = fmt::format("dnf install {} --assumeyes", rpmFullName);
  do {
    XLOG(INFO) << fmt::format(
        "Running command ({}); Attempt: {}", cmd, attempt++);
    std::tie(exitStatus, standardOut) = PlatformUtils().execCommand(cmd);
    XLOG(INFO) << standardOut;
  } while (attempt <= maxAttempts && exitStatus != 0);
  if (exitStatus != 0) {
    XLOG(ERR) << fmt::format(
        "Command ({}) failed with exit code {}", cmd, exitStatus);
    throw std::runtime_error(fmt::format(
        "Failed to install rpm ({}) with exit code {}",
        rpmFullName,
        exitStatus));
  }
}

void PkgManager::removeOldRpms(const std::string& rpmBaseName) const {
  std::string stdOut{};
  int exitStatus{0};
  auto getInstalledRpmNamesCmd = fmt::format("rpm -qa | grep ^{}", rpmBaseName);
  std::tie(exitStatus, stdOut) =
      PlatformUtils().execCommand(getInstalledRpmNamesCmd);
  if (exitStatus) {
    XLOG(INFO) << "No old rpms to remove";
    return;
  }

  std::vector<std::string> installedRpms;
  folly::split('\n', stdOut, installedRpms);
  XLOG(INFO) << fmt::format(
      "Removing old rpms: {}", folly::join(", ", installedRpms));
  auto removeOldRpmsCmd =
      fmt::format("rpm -e --allmatches {}", folly::join(" ", installedRpms));
  std::tie(exitStatus, stdOut) = PlatformUtils().execCommand(removeOldRpmsCmd);
  if (exitStatus) {
    XLOG(ERR) << fmt::format(
        "Command ({}) failed with exit code {}", removeOldRpmsCmd, exitStatus);
    throw std::runtime_error(fmt::format(
        "Failed to remove old rpms ({}) with exit code {}",
        folly::join(" ", installedRpms),
        exitStatus));
  }
}

void PkgManager::runDepmod() const {
  int exitStatus{0};
  std::string standardOut{};
  auto depmodCmd = "depmod -a";
  XLOG(INFO) << fmt::format("Running command ({})", depmodCmd);
  std::tie(exitStatus, standardOut) = PlatformUtils().execCommand(depmodCmd);
  if (exitStatus != 0) {
    XLOG(ERR) << fmt::format(
        "Command ({}) failed with exit code {}", depmodCmd, exitStatus);
  }
}

void PkgManager::installLocalRpm(int maxAttempts) const {
  int exitStatus{0}, attempt{1};
  std::string standardOut{};
  auto cmd = fmt::format("rpm -i --force {}", FLAGS_local_rpm_path);
  do {
    XLOG(INFO) << fmt::format(
        "Running command ({}); Attempt: {}", cmd, attempt++);
    std::tie(exitStatus, standardOut) = PlatformUtils().execCommand(cmd);
    XLOG(INFO) << standardOut;
  } while (attempt <= maxAttempts && exitStatus != 0);
  if (exitStatus != 0) {
    XLOG(ERR) << fmt::format(
        "Command ({}) failed with exit code {}", cmd, exitStatus);
    throw std::runtime_error(fmt::format(
        "Failed to install rpm ({}) with exit code {}",
        FLAGS_local_rpm_path,
        exitStatus));
  }
}

void PkgManager::unloadKmod(const std::string& moduleName) const {
  int exitStatus{0};
  std::string standardOut{};
  auto unloadCmd = fmt::format("modprobe --remove {}", moduleName);
  XLOG(INFO) << fmt::format("Running command ({})", unloadCmd);
  std::tie(exitStatus, standardOut) = PlatformUtils().execCommand(unloadCmd);
  if (exitStatus != 0) {
    XLOG(ERR) << fmt::format(
        "Command ({}) failed with exit code {}", unloadCmd, exitStatus);
    throw std::runtime_error(fmt::format(
        "Failed to unload kmod ({}) with exit code {}",
        moduleName,
        exitStatus));
  }
}

void PkgManager::loadKmod(const std::string& moduleName) const {
  int exitStatus{0};
  std::string standardOut{};
  auto loadCmd = fmt::format("modprobe {}", moduleName);
  XLOG(INFO) << fmt::format("Running command ({})", loadCmd);
  std::tie(exitStatus, standardOut) = PlatformUtils().execCommand(loadCmd);
  if (exitStatus != 0) {
    XLOG(ERR) << fmt::format(
        "Command ({}) failed with exit code {}", loadCmd, exitStatus);
    throw std::runtime_error(fmt::format(
        "Failed to load kmod ({}) with exit code {}", moduleName, exitStatus));
  }
}

std::string PkgManager::getKmodsRpmBaseWithKernelName() const {
  return fmt::format(
      "{}-{}", *platformConfig_.bspKmodsRpmName(), getHostKernelVersion());
}

std::string PkgManager::getKmodsRpmName() const {
  return fmt::format(
      "{}-{}-{}",
      *platformConfig_.bspKmodsRpmName(),
      getHostKernelVersion(),
      *platformConfig_.bspKmodsRpmVersion());
}

} // namespace facebook::fboss::platform::platform_manager
