// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/platform/config_lib/ConfigLib.h"

namespace facebook::fboss::platform::config_lib {

std::string getSensorServiceConfig(
    const std::optional<std::string>& /* platformName */) {
  throw std::runtime_error(
      "Unimplemented function. Specify config_file explicitly");
}

std::string getFbdevdConfig(
    const std::optional<std::string>& /* platformName */) {
  throw std::runtime_error(
      "Unimplemented function. Specify config_file explicitly");
}

std::string getFanServiceConfig(
    const std::optional<std::string>& platformName) {
  throw std::runtime_error(
      "Unimplemented function. Specify config_file explicitly");
}

std::string getWeutilConfig(const std::optional<std::string>& platformName) {
  throw std::runtime_error(
      "Unimplemented function. Specify config_file explicitly");
}

std::string getFwUtilConfig(const std::optional<std::string>& platformName) {
  throw std::runtime_error(
      "Unimplemented function. Specify config_file explicitly");
}
} // namespace facebook::fboss::platform::config_lib
