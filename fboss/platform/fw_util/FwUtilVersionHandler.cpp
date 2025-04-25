#include "fboss/platform/fw_util/FwUtilVersionHandler.h"
#include <folly/String.h>
#include <folly/logging/xlog.h>
#include <glob.h>
#include <iostream>
#include "fboss/platform/fw_util/fw_util_helpers.h"
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

} // namespace facebook::fboss::platform::fw_util
