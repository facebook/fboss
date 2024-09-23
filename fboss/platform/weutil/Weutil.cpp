// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.
#include "fboss/platform/weutil/Weutil.h"
#include "fboss/platform/weutil/IoctlSmbusEepromReader.h"

#include <folly/logging/xlog.h>
#include <thrift/lib/cpp2/protocol/Serializer.h>

#include "fboss/lib/platforms/PlatformMode.h"
#include "fboss/lib/platforms/PlatformProductInfo.h"
#include "fboss/platform/config_lib/ConfigLib.h"
#include "fboss/platform/helpers/PlatformNameLib.h"
#include "fboss/platform/weutil/WeutilDarwin.h"
#include "fboss/platform/weutil/WeutilImpl.h"
#include "fboss/platform/weutil/if/gen-cpp2/weutil_config_types.h"

namespace facebook::fboss::platform {

namespace {

weutil_config::WeutilConfig getWeUtilConfig() {
  weutil_config::WeutilConfig thriftConfig;
  auto platformName = helpers::PlatformNameLib().getPlatformName();
  std::string weutilConfigJson = ConfigLib().getWeutilConfig(platformName);
  apache::thrift::SimpleJSONSerializer::deserialize<
      weutil_config::WeutilConfig>(weutilConfigJson, thriftConfig);
  XLOG(DBG1) << apache::thrift::SimpleJSONSerializer::serialize<std::string>(
      thriftConfig);
  return thriftConfig;
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

std::vector<std::string> getEepromPaths() {
  auto config = getWeUtilConfig();
  std::vector<std::string> eepromPaths;

  for (const auto& [eepromName, eepromConfig] : *config.fruEepromList()) {
    std::string fruName = eepromName;
    std::transform(fruName.begin(), fruName.end(), fruName.begin(), ::toupper);
    auto& fruPath = *eepromConfig.path();
    auto fruOffset = *eepromConfig.offset();
    std::string eepromInfo =
        fmt::format("Name:{} Path:{} Offset:{}", fruName, fruPath, fruOffset);
    eepromPaths.push_back(eepromInfo);
  }
  return eepromPaths;
}

std::unique_ptr<WeutilInterface> createWeUtilIntf(
    const std::string& eepromName,
    const std::string& eepromPath,
    const int eepromOffset) {
  auto platform = getPlatformType();

  // When path is specified, read from it directly. For platform bringup, we can
  // use the --path and --offset options without a valid config.
  if (!eepromPath.empty()) {
    if (platform && platform.value() == PlatformType::PLATFORM_DARWIN) {
      return std::make_unique<WeutilDarwin>(eepromPath);
    } else {
      return std::make_unique<WeutilImpl>(eepromPath, eepromOffset);
    }
  }
  if (!platform) {
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
  if (platform.value() == PlatformType::PLATFORM_DARWIN) {
    return std::make_unique<WeutilDarwin>(*fruEepromConfig.path());
  } else {
    return std::make_unique<WeutilImpl>(
        *fruEepromConfig.path(), *fruEepromConfig.offset());
  }
}

} // namespace facebook::fboss::platform
