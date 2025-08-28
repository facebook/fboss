// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.
#include "fboss/platform/weutil/Weutil.h"

#include <folly/String.h>
#include <folly/logging/xlog.h>
#include <re2/re2.h>
#include <thrift/lib/cpp2/protocol/Serializer.h>
#include <filesystem>

#include "fboss/platform/helpers/PlatformNameLib.h"
#include "fboss/platform/platform_manager/ConfigUtils.h"
#include "fboss/platform/weutil/WeutilDarwin.h"
#include "fboss/platform/weutil/WeutilImpl.h"

namespace facebook::fboss::platform {

namespace {

// Extract EEPROM name from symbolicPath
std::string getEepromName(const std::string& symbolicPath) {
  // Extracts the eeprom device name from the symbolic path.
  // Example: /run/devmap/eeproms/CHASSIS_EEPROM -> CHASSIS
  // Example: /run/devmap/eeproms/SCM_EEPROM -> SCM
  // Example: /run/devmap/eeproms/PDB_L_EEPROM -> PDB_L
  // Edge case: /run/devmap/eeproms/COME_FRU -> COME_FRU
  // Edge case: /run/devmap/eeproms/MERU800BFA_SCM_EEPROM -> SCM

  static const re2::RE2 re(
      "^/run/devmap/eeproms/([A-Za-z0-9_]+?)(?:_EEPROM)?$");
  re2::StringPiece match;
  if (!re2::RE2::FullMatch(symbolicPath, re, &match)) {
    throw std::runtime_error(fmt::format(
        "Unable to find eeprom name from symbolic path: {}", symbolicPath));
  }

  std::string eepromName = std::string(match);
  // Special handling for MERU platforms: extract text after underscore
  folly::StringPiece sp(eepromName);
  if (sp.removePrefix("MERU800BFA_") || sp.removePrefix("MERU_") ||
      sp.removePrefix("MERU800BIA_")) {
    eepromName = sp.str();
  }

  return eepromName;
}

// Extract EEPROM symbolicPath from devicePath
std::string getEepromPath(
    const platform_manager::PlatformConfig& platformConfig,
    const std::string& devicePath) {
  std::string eepromPath;
  for (const auto& [symbolicLink, path] :
       *platformConfig.symbolicLinkToDevicePath()) {
    if (path == devicePath) {
      eepromPath = symbolicLink;
      break;
    }
  }
  if (eepromPath.empty()) {
    throw std::runtime_error(fmt::format(
        "Unable to find symbolic link for device path: {}", devicePath));
  }
  return eepromPath;
}

int getEepromOffset(
    const platform_manager::PlatformConfig& platformConfig,
    const std::string& eepromName) {
  int eepromOffset = 0;
  for (const auto& [__, slotConfig] : *platformConfig.slotTypeConfigs()) {
    if (slotConfig.pmUnitName() && *slotConfig.pmUnitName() == eepromName) {
      if (slotConfig.idpromConfig()) {
        eepromOffset = *slotConfig.idpromConfig()->offset();
      }
      break;
    }
  }
  return eepromOffset;
}

std::vector<std::string> getEepromNames(
    const weutil_config::WeutilConfig& thriftConfig) {
  std::vector<std::string> eepromNames;
  for (const auto& [eepromName, eepromConfig] : *thriftConfig.fruEepromList()) {
    std::string fruName = eepromName;
    std::transform(fruName.begin(), fruName.end(), fruName.begin(), ::tolower);
    eepromNames.push_back(fruName);
  }
  return eepromNames;
}

// Gets the FruEepromConfig based on the eeprom name specified.
weutil_config::FruEepromConfig getFruEepromConfig(
    const std::string& eepromName,
    const weutil_config::WeutilConfig& thriftConfig) {
  std::string fruName = eepromName;
  std::transform(fruName.begin(), fruName.end(), fruName.begin(), ::toupper);
  auto itr = thriftConfig.fruEepromList()->find(fruName);
  if (itr == thriftConfig.fruEepromList()->end()) {
    throw std::runtime_error(fmt::format(
        "Invalid EEPROM name {}. Valid EEPROM names are: {}",
        eepromName,
        folly::join(", ", getEepromNames(thriftConfig))));
  }
  return itr->second;
}

} // namespace

weutil_config::WeutilConfig getWeUtilConfig() {
  // Create a WeutilConfig from the platform manager config
  weutil_config::WeutilConfig thriftConfig;
  platform_manager::PlatformConfig platformConfig =
      platform::platform_manager::ConfigUtils().getConfig();

  // Get Chassis Eeprom Device Name
  // Darwin Platform special case that doesn't have a chassis EEPROM path
  if (platformConfig.platformName().value() == "darwin") {
    thriftConfig.chassisEepromName() = "CHASSIS";
  } else if (!platformConfig.chassisEepromDevicePath()->empty()) {
    std::string chassisEepromPath = getEepromPath(
        platformConfig, *platformConfig.chassisEepromDevicePath());
    std::string chassisEepromName = getEepromName(chassisEepromPath);
    thriftConfig.chassisEepromName() = chassisEepromName;
  } else {
    throw std::runtime_error("Chassis EEPROM device path not found");
  }

  // Create FruEepromList
  std::map<std::string, weutil_config::FruEepromConfig> fruEepromList;
  for (const auto& [symbolicLink, devicePath] :
       *platformConfig.symbolicLinkToDevicePath()) {
    weutil_config::FruEepromConfig fruEepromConfig;
    if (symbolicLink.starts_with("/run/devmap/eeproms/")) {
      std::string eepromName = getEepromName(symbolicLink);
      fruEepromConfig.path() = symbolicLink;
      fruEepromConfig.offset() = getEepromOffset(platformConfig, eepromName);
      fruEepromList[eepromName] = fruEepromConfig;
    }
  }

  /*
  Because of upstream kernel issues, we have to manually read the
  SCM EEPROM for the Meru800BFA/BIA platforms. It is read directly
  with ioctl and written to the /run/devmap file.
  See: https://github.com/facebookexternal/fboss.bsp.arista/pull/31/files
  */
  if (platformConfig.platformName().value() == "meru800bfa" ||
      platformConfig.platformName().value() == "meru800bia") {
    if (std::filesystem::exists("/run/devmap/eeproms/MERU_SCM_EEPROM")) {
      std::string eepromName = "SCM";
      weutil_config::FruEepromConfig fruEepromConfig;
      fruEepromConfig.path() = "/run/devmap/eeproms/MERU_SCM_EEPROM";
      fruEepromConfig.offset() = getEepromOffset(platformConfig, eepromName);
      fruEepromList[eepromName] = fruEepromConfig;
    }
  } else if (platformConfig.platformName().value() == "darwin") {
    // Darwin Platform special case that doesn't have a chassis EEPROM path
    std::string eepromName = "CHASSIS";
    weutil_config::FruEepromConfig fruEepromConfig;
    fruEepromConfig.path() = "";
    fruEepromConfig.offset() = 0;
    fruEepromList[eepromName] = fruEepromConfig;
  }

  thriftConfig.fruEepromList() = fruEepromList;
  return thriftConfig;
}

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
  auto thriftConfig = getWeUtilConfig();
  weutil_config::FruEepromConfig fruEepromConfig;
  if (eepromName == "chassis" || eepromName.empty()) {
    fruEepromConfig =
        getFruEepromConfig(*thriftConfig.chassisEepromName(), thriftConfig);
  } else {
    fruEepromConfig = getFruEepromConfig(eepromName, thriftConfig);
  }
  if (isDarwin) {
    return std::make_unique<WeutilDarwin>(*fruEepromConfig.path());
  } else {
    return std::make_unique<WeutilImpl>(
        *fruEepromConfig.path(), *fruEepromConfig.offset());
  }
}

} // namespace facebook::fboss::platform
