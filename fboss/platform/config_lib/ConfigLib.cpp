// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.
#include "fboss/platform/config_lib/ConfigLib.h"

#include <folly/FileUtil.h>
#include <folly/logging/xlog.h>

#include "fboss/platform/helpers/PlatformNameLib.h"

#include "fboss/platform/config_lib/GeneratedConfig.h"

DEFINE_string(
    config_file,
    "",
    "Optional configuration file. "
    "If empty we pick the platform default config");

using namespace facebook::fboss::platform;

namespace {

std::string getPlatformName(const std::optional<std::string>& platformName) {
  auto platformStr = platformName
      ? *platformName
      : *helpers::PlatformNameLib().getPlatformName();
  std::transform(
      platformStr.begin(), platformStr.end(), platformStr.begin(), ::tolower);
  XLOG(INFO) << "The inferred platform is " << platformStr;
  return platformStr;
}

std::optional<std::string> getConfigFromFile() {
  if (!FLAGS_config_file.empty()) {
    XLOG(INFO) << "Using config file: " << FLAGS_config_file;
    std::string configJson;
    if (!folly::readFile(FLAGS_config_file.c_str(), configJson)) {
      throw std::runtime_error(
          "Can not read config file: " + FLAGS_config_file);
    }
    return configJson;
  }
  return std::nullopt;
}
} // namespace

namespace facebook::fboss::platform {

std::string ConfigLib::getSensorServiceConfig(
    const std::optional<std::string>& platformName) const {
  if (auto configJson = getConfigFromFile()) {
    return *configJson;
  }
  return configs::sensor_service.at(getPlatformName(platformName));
}

std::string ConfigLib::getFanServiceConfig(
    const std::optional<std::string>& platformName) const {
  if (auto configJson = getConfigFromFile()) {
    return *configJson;
  }
  return configs::fan_service.at(getPlatformName(platformName));
}
std::string ConfigLib::getWeutilConfig(
    const std::optional<std::string>& platformName) const {
  if (auto configJson = getConfigFromFile()) {
    return *configJson;
  }
  return configs::weutil.at(getPlatformName(platformName));
}
std::string ConfigLib::getFwUtilConfig(
    const std::optional<std::string>& platformName) const {
  if (auto configJson = getConfigFromFile()) {
    return *configJson;
  }
  return configs::fw_util.at(getPlatformName(platformName));
}

std::string ConfigLib::getPlatformManagerConfig(
    const std::optional<std::string>& platformName) const {
  if (auto configJson = getConfigFromFile()) {
    return *configJson;
  }
  return configs::platform_manager.at(getPlatformName(platformName));
}

std::string ConfigLib::getLedManagerConfig(
    const std::optional<std::string>& platformName) const {
  if (auto configJson = getConfigFromFile()) {
    return *configJson;
  }
  return configs::led_manager.at(getPlatformName(platformName));
}

} // namespace facebook::fboss::platform
