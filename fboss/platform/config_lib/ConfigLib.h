// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <optional>
#include <string>

namespace facebook::fboss::platform::config_lib {

std::string getSensorServiceConfig(
    const std::optional<std::string>& platformName = std::nullopt);

std::string getFbdevdConfig(
    const std::optional<std::string>& platformName = std::nullopt);

std::string getFanServiceConfig(
    const std::optional<std::string>& platformName = std::nullopt);

std::string getPlatformManagerConfig(
    const std::optional<std::string>& platformName = std::nullopt);

std::string getWeutilConfig(
    const std::optional<std::string>& platformName = std::nullopt);

std::string getFirmwareConfig(
    const std::optional<std::string>& platformName = std::nullopt);

} // namespace facebook::fboss::platform::config_lib
