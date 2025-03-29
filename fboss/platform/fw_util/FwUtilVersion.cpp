#include <glob.h>
#include <iostream>
#include "FwUtilImpl.h"
#include "fboss/platform/helpers/PlatformNameLib.h"
#include "fw_util_helpers.h"

namespace facebook::fboss::platform::fw_util {

// TODO: Both runVersionCmdDarwin and printDarwinVersion will be removed once
//  we move to PM and we get new BSP with fw_ver
std::string FwUtilImpl::runVersionCmdDarwin(const std::string& cmd) {
  auto [exitStatus, standardOut] = PlatformUtils().execCommand(cmd);
  if (exitStatus != 0) {
    throw std::runtime_error("Run " + cmd + " failed!");
  }
  return standardOut;
}
void FwUtilImpl::printDarwinVersion(const std::string& fpd) {
  if (fpd == "all") {
    for (const auto& orderedfpd : fwDeviceNamesByPrio_) {
      auto iter = fwUtilConfig_.newFwConfigs()->find(orderedfpd.first);
      std::string versionCmd = *iter->second.version()->getVersionCmd();
      std::string version = runVersionCmdDarwin(versionCmd);
      std::cout << orderedfpd.first << " : " << version;
    }
  } else {
    auto iter = fwUtilConfig_.newFwConfigs()->find(fpd);
    if (iter != fwUtilConfig_.newFwConfigs()->end()) {
      std::string versionCmd = *iter->second.version()->getVersionCmd();
      std::string version = runVersionCmdDarwin(versionCmd);
      std::cout << fpd << " : " << version;
    } else {
      XLOG(INFO)
          << fpd
          << " is not part of the firmware target_name list Please use ./fw-util --helpon=Flags for the right usage";
    }
  }
}

void FwUtilImpl::printVersion(const std::string& fpd) {
  if (fpd == "all") {
    printAllVersions();
  } else {
    std::string version = getSingleVersion(fpd);
    std::cout << fpd << " : " << version << std::endl;
  }
}

std::string FwUtilImpl::getSingleVersion(const std::string& fpd) {
  std::string version;
  // printing single version
  auto iter = fwUtilConfig_.newFwConfigs()->find(fpd);
  if (iter == fwUtilConfig_.newFwConfigs()->end()) {
    throw std::runtime_error(
        "Firmware target name not found in config: " + fpd +
        " Please use ./fw-util --helpon=Flags for the right usage");
  }

  const auto& versionConfig = *iter->second.version();
  // Check if the path is set
  if (*versionConfig.versionType() == "Not Applicable") {
    return "Not Applicable";
  } else if (
      *versionConfig.versionType() == "sysfs" &&
      versionConfig.path().has_value()) {
    // using the glob function to expand the path to the file
    // because there are cases where we might have a wild card
    // such as  /run/devmap/cplds/FAN_CPLD/hwmon/hwmon*/fw_ver
    // For those cases, it's guarantee that there will always be
    // one match. if there is no wildcard, the glob function will
    // return a single match, which is the original path itself
    glob_t globbuf;
    int ret = glob(
        versionConfig.path()->c_str(),
        GLOB_ERR | GLOB_NOCHECK,
        nullptr,
        &globbuf);
    if (ret != 0) {
      throw std::runtime_error("Failed to glob file: " + *versionConfig.path());
    }
    std::vector<std::string> cmd = {"/usr/bin/cat", globbuf.gl_pathv[0]};
    auto [exitStatus, standardOut] = PlatformUtils().runCommand(cmd);
    checkCmdStatus(cmd, exitStatus);
    version = folly::trimWhitespace(standardOut).str();
    globfree(&globbuf);
  } else {
    XLOG(ERR)
        << "Only reading Version through sysfs is support and the path is not set for "
        << fpd;
    return "";
  }
  return version;
}

void FwUtilImpl::printAllVersions() {
  std::string version;
  for (const auto& orderedfpd : fwDeviceNamesByPrio_) {
    version = getSingleVersion(orderedfpd.first);
    std::cout << orderedfpd.first << " : " << version << std::endl;
  }
}

void FwUtilImpl::doVersionAudit() {
  bool mismatch = false;
  for (const auto& [fpdName, fwConfig] : *fwUtilConfig_.newFwConfigs()) {
    std::string desiredVersion = *fwConfig.desiredVersion();
    if (desiredVersion.empty()) {
      XLOGF(
          INFO,
          "{} does not have a desired version specified in the config.",
          fpdName);
      continue;
    }
    std::string actualVersion;
    try {
      actualVersion = getSingleVersion(fpdName);
    } catch (const std::exception& e) {
      XLOG(ERR) << "Failed to get version for " << fpdName << ": " << e.what();
      continue;
    }

    if (actualVersion != desiredVersion) {
      XLOGF(
          INFO,
          "{} is at version {} which does not match config's desired version {}.",
          fpdName,
          actualVersion,
          desiredVersion);
      mismatch = true;
    }
  }
  if (mismatch) {
    XLOG(INFO, "Firmware version mismatch found.");
    exit(1);
  } else {
    XLOG(INFO, "All firmware versions match the config.");
  }
}
} // namespace facebook::fboss::platform::fw_util
