// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.
#include "fboss/platform/weutil/Weutil.h"

#include <folly/logging/xlog.h>
#include <thrift/lib/cpp2/protocol/Serializer.h>

#include "fboss/lib/platforms/PlatformMode.h"
#include "fboss/lib/platforms/PlatformProductInfo.h"
#include "fboss/platform/config_lib/ConfigLib.h"
#include "fboss/platform/weutil/WeutilDarwin.h"
#include "fboss/platform/weutil/WeutilImpl.h"
#include "fboss/platform/weutil/if/gen-cpp2/weutil_config_types.h"

namespace facebook::fboss::platform {

namespace {

weutil_config::WeutilConfig getWeUtilConfig() {
  weutil_config::WeutilConfig thriftConfig;
  std::string weutilConfigJson = ConfigLib().getWeutilConfig();
  apache::thrift::SimpleJSONSerializer::deserialize<
      weutil_config::WeutilConfig>(weutilConfigJson, thriftConfig);
  XLOG(INFO) << apache::thrift::SimpleJSONSerializer::serialize<std::string>(
      thriftConfig);

  return thriftConfig;
}

std::vector<std::string> getEepromNames(
    const weutil_config::WeutilConfig& thriftConfig,
    std::optional<PlatformType> platform) {
  std::vector<std::string> eepromNames;
  // Darwin does not have a dedicated chassis EEPROM. Hence it is not
  // listed in the weutil json config. It is added here manually.
  if (platform == PlatformType::PLATFORM_DARWIN) {
    eepromNames.push_back("chassis");
  }
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
    const weutil_config::WeutilConfig& thriftConfig,
    PlatformType platform) {
  std::string fruName = eepromName;
  std::transform(fruName.begin(), fruName.end(), fruName.begin(), ::toupper);
  auto itr = thriftConfig.fruEepromList()->find(fruName);
  if (itr == thriftConfig.fruEepromList()->end()) {
    throw std::runtime_error(fmt::format(
        "Invalid EEPROM name {}. Valid EEPROM names are: {}",
        eepromName,
        fmt::join(getEepromNames(thriftConfig, platform), ", ")));
  }

  return itr->second;
}

// Get the path to the eeprom based on its name.
// The chassis eeprom has special handling:
// - For Darwin, let the path be determined by the WeutilDarwin class since
//   there is no dedicated eeprom device.
// - For other platforms, get the proper name of the chassis eeprom from the
//   config file and use that to determine the path.
weutil_config::FruEepromConfig getFruEepromConfig(
    const std::string& eepromName,
    const std::string& eepromPath,
    const PlatformType platform) {
  weutil_config::FruEepromConfig fruEepromConfig;

  if (!eepromPath.empty()) {
    fruEepromConfig.path() = eepromPath;
    fruEepromConfig.offset() = 0;
  } else {
    if (eepromName == "chassis" || eepromName.empty()) {
      if (platform == PlatformType::PLATFORM_DARWIN) {
        fruEepromConfig.path() = "";
        fruEepromConfig.offset() = 0;
      } else {
        auto thriftConfig = getWeUtilConfig();
        // use chassisEepromName specified in config file.
        fruEepromConfig = getFruEepromConfig(
            thriftConfig.chassisEepromName().value(), thriftConfig, platform);
      }
    } else {
      auto thriftConfig = getWeUtilConfig();
      fruEepromConfig = getFruEepromConfig(eepromName, thriftConfig, platform);
    }
  }
  return fruEepromConfig;
}

std::optional<PlatformType> getPlatformType() {
  try {
    facebook::fboss::PlatformProductInfo prodInfo{FLAGS_fruid_filepath};
    prodInfo.initialize();
    return prodInfo.getType();
  } catch (std::exception& e) {
    XLOG(ERR) << "Failed to get platform type: " << e.what();
    return std::nullopt;
  }
}
} // namespace

std::vector<std::string> getEepromNames() {
  auto config = getWeUtilConfig();

  return getEepromNames(config, getPlatformType());
}

std::unique_ptr<WeutilInterface> createWeUtilIntf(
    const std::string& eepromName,
    const std::string& eepromPath) {
  auto platform = getPlatformType();
  if (platform.has_value()) {
    weutil_config::FruEepromConfig fruEepromConfig =
        getFruEepromConfig(eepromName, eepromPath, platform.value());
    switch (platform.value()) {
      case PlatformType::PLATFORM_DARWIN:
        return std::make_unique<WeutilDarwin>(fruEepromConfig.get_path());
        break;
      default:
        return std::make_unique<WeutilImpl>(
            fruEepromConfig.get_path(), fruEepromConfig.get_offset());
        break;
    }
  } else {
    // For platform bringup, we can use the --path option without a
    // valid config.
    if (!eepromPath.empty()) {
      return std::make_unique<WeutilImpl>(eepromPath, 0);
    } else {
      throw std::runtime_error(
          "Unable to determine platform type. Use the --path option");
    }
  }
}

} // namespace facebook::fboss::platform
