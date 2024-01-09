// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.
#include "fboss/platform/config_lib/ConfigLib.h"

#include <folly/FileUtil.h>
#include <folly/logging/xlog.h>

#include "fboss/lib/platforms/PlatformMode.h"
#include "fboss/lib/platforms/PlatformProductInfo.h"

#ifndef IS_OSS
#include "fboss/platform/config_lib/GeneratedConfig.h"
#endif

DEFINE_string(
    config_file,
    "",
    "Optional configuration file. "
    "If empty we pick the platform default config");

using namespace facebook::fboss::platform;

namespace {

#ifndef IS_OSS
std::string getPlatformName(const std::optional<std::string>& platformName) {
  if (platformName) {
    return *platformName;
  }
  facebook::fboss::PlatformProductInfo productInfo(FLAGS_fruid_filepath);
  productInfo.initialize();
  auto platform = productInfo.getType();
  auto platformStr = toString(platform);
  std::transform(
      platformStr.begin(), platformStr.end(), platformStr.begin(), ::tolower);
  XLOG(INFO) << "The inferred platform is " << platformStr;
  return platformStr;
}
#endif

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
#ifndef IS_OSS
  return configs::sensor_service.at(getPlatformName(platformName));
#else
  throw std::runtime_error("config_file must be specified in OSS");
#endif
}

std::string ConfigLib::getFbdevdConfig(
    const std::optional<std::string>& platformName) const {
  if (auto configJson = getConfigFromFile()) {
    return *configJson;
  }
#ifndef IS_OSS
  return configs::fbdevd.at(getPlatformName(platformName));
#else
  throw std::runtime_error("config_file must be specified in OSS");
#endif
}

std::string ConfigLib::getFanServiceConfig(
    const std::optional<std::string>& platformName) const {
  if (auto configJson = getConfigFromFile()) {
    return *configJson;
  }
#ifndef IS_OSS
  return configs::fan_service.at(getPlatformName(platformName));
#else
  throw std::runtime_error("config_file must be specified in OSS");
#endif
}
std::string ConfigLib::getWeutilConfig(
    const std::optional<std::string>& platformName) const {
  if (auto configJson = getConfigFromFile()) {
    return *configJson;
  }
#ifndef IS_OSS
  return configs::weutil.at(getPlatformName(platformName));
#else
  throw std::runtime_error("config_file must be specified in OSS");
#endif
}
std::string ConfigLib::getFwUtilConfig(
    const std::optional<std::string>& platformName) const {
  if (auto configJson = getConfigFromFile()) {
    return *configJson;
  }
#ifndef IS_OSS
  return configs::fw_util.at(getPlatformName(platformName));
#else
  throw std::runtime_error("config_file must be specified in OSS");
#endif
}

std::string ConfigLib::getPlatformManagerConfig(
    const std::optional<std::string>& platformName) const {
  if (auto configJson = getConfigFromFile()) {
    return *configJson;
  }
#ifndef IS_OSS
  return configs::platform_manager.at(getPlatformName(platformName));
#else
  throw std::runtime_error("config_file must be specified in OSS");
#endif
}

std::string ConfigLib::getLedManagerConfig(
    const std::optional<std::string>& platformName) const {
  if (auto configJson = getConfigFromFile()) {
    return *configJson;
  }
#ifndef IS_OSS
  return configs::led_manager.at(getPlatformName(platformName));
#else
  throw std::runtime_error("config_file must be specified in OSS");
#endif
}

} // namespace facebook::fboss::platform
