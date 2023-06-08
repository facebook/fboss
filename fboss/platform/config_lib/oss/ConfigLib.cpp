// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/platform/config_lib/ConfigLib.h"

namespace facebook::fboss::platform {

std::string ConfigLib::getSensorServiceConfig(
    const std::optional<std::string>& /* platformName */) const {
  throw std::runtime_error(
      "Unimplemented function. Specify config_file explicitly");
}

std::string ConfigLib::getFbdevdConfig(
    const std::optional<std::string>& /* platformName */) const {
  throw std::runtime_error(
      "Unimplemented function. Specify config_file explicitly");
}

std::string ConfigLib::getFanServiceConfig(
    const std::optional<std::string>& platformName) const {
  throw std::runtime_error(
      "Unimplemented function. Specify config_file explicitly");
}

std::string ConfigLib::getWeutilConfig(
    const std::optional<std::string>& platformName) const {
  throw std::runtime_error(
      "Unimplemented function. Specify config_file explicitly");
}

std::string ConfigLib::getFwUtilConfig(
    const std::optional<std::string>& platformName) const {
  throw std::runtime_error(
      "Unimplemented function. Specify config_file explicitly");
}

std::string ConfigLib::getPlatformManagerConfig(
    const std::optional<std::string>& platformName) const {
  throw std::runtime_error(
      "Unimplemented function. Specify config_file explicitly");
}

} // namespace facebook::fboss::platform
