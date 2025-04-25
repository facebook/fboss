#include "fboss/platform/fw_util/FwUtilVersionHandler.h"
#include <folly/String.h>
#include <folly/logging/xlog.h>
#include <glob.h>
#include <iostream>
#include "fboss/platform/fw_util/fw_util_helpers.h"
#include "fboss/platform/helpers/PlatformFsUtils.h"
#include "fboss/platform/helpers/PlatformUtils.h"

namespace facebook::fboss::platform::fw_util {

void FwUtilVersionHandler::printDarwinVersion(const std::string& fpd) {
  if (fpd == "all") {
    for (const auto& orderedfpd : fwDeviceNamesByPrio_) {
      auto iter = fwUtilConfig_.newFwConfigs()->find(orderedfpd.first);
      std::string versionCmd = *iter->second.version()->getVersionCmd();
      auto [exitStatus, version] = PlatformUtils().execCommand(versionCmd);
      if (exitStatus != 0) {
        throw std::runtime_error("Run " + versionCmd + " failed!");
      }
      std::cout << orderedfpd.first << " : " << version;
    }
  } else {
    auto iter = fwUtilConfig_.newFwConfigs()->find(fpd);
    if (iter != fwUtilConfig_.newFwConfigs()->end()) {
      std::string versionCmd = *iter->second.version()->getVersionCmd();
      auto [exitStatus, version] = PlatformUtils().execCommand(versionCmd);
      if (exitStatus != 0) {
        throw std::runtime_error("Run " + versionCmd + " failed!");
      }
      std::cout << fpd << " : " << version;
    } else {
      XLOG(INFO)
          << fpd
          << " is not part of the firmware target_name list. Please use ./fw-util --helpon=Flags for the right usage";
    }
  }
}

std::string FwUtilVersionHandler::getSingleVersion(const std::string& fpd) {
  std::string version;
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
    // Rewrote to use library function instead of executing cat
    auto content = PlatformFsUtils().getStringFileContent(globbuf.gl_pathv[0]);
    if (!content.has_value()) {
      globfree(&globbuf);
      throw std::runtime_error(
          "Failed to read file: " + std::string(globbuf.gl_pathv[0]));
    }
    version = content.value();
    globfree(&globbuf);
  } else {
    XLOG(ERR)
        << "Only reading Version through sysfs is support and the path is not set for "
        << fpd;
    return "";
  }
  return version;
}
} // namespace facebook::fboss::platform::fw_util
