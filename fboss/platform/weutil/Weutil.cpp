// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.
#include "fboss/platform/weutil/Weutil.h"

#include <folly/logging/xlog.h>
#include <thrift/lib/cpp2/protocol/Serializer.h>

#include "fboss/platform/helpers/PlatformNameLib.h"
#include "fboss/platform/weutil/ConfigUtils.h"
#include "fboss/platform/weutil/WeutilDarwin.h"
#include "fboss/platform/weutil/WeutilImpl.h"

namespace facebook::fboss::platform {

std::unique_ptr<WeutilInterface> createWeUtilIntf(
    const std::string& eepromName,
    const std::string& eepromPath,
    const int eepromOffset) {
  auto platformName = helpers::PlatformNameLib().getPlatformName();
  bool isDarwin = platformName && *platformName == "DARWIN";
  // When path is specified, read from it directly. For platform bringup, we can
  // use the --path and --offset options without a valid config.
  if (!eepromPath.empty()) {
    if (isDarwin) {
      return std::make_unique<WeutilDarwin>(eepromPath);
    } else {
      return std::make_unique<WeutilImpl>(eepromPath, eepromOffset);
    }
  }
  if (!platformName) {
    throw std::runtime_error(
        "Unable to determine platform type. Use the --path option");
  }
  weutil::ConfigUtils configUtils(platformName);
  weutil::FruEeprom fruEeprom;
  std::string upperEepromName = eepromName;
  std::transform(
      upperEepromName.begin(),
      upperEepromName.end(),
      upperEepromName.begin(),
      ::toupper);
  if (upperEepromName == "CHASSIS" || upperEepromName.empty()) {
    fruEeprom = configUtils.getFruEeprom(configUtils.getChassisEepromName());
  } else {
    fruEeprom = configUtils.getFruEeprom(upperEepromName);
  }
  if (isDarwin) {
    return std::make_unique<WeutilDarwin>(fruEeprom.path);
  } else {
    return std::make_unique<WeutilImpl>(fruEeprom.path, fruEeprom.offset);
  }
}

} // namespace facebook::fboss::platform
