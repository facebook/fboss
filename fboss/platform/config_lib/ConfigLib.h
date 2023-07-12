// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <optional>
#include <string>

namespace facebook::fboss::platform {

class ConfigLib {
 public:
  virtual ~ConfigLib() = default;
  virtual std::string getSensorServiceConfig(
      const std::optional<std::string>& platformName = std::nullopt) const;

  virtual std::string getFbdevdConfig(
      const std::optional<std::string>& platformName = std::nullopt) const;

  virtual std::string getFanServiceConfig(
      const std::optional<std::string>& platformName = std::nullopt) const;

  virtual std::string getPlatformManagerConfig(
      const std::optional<std::string>& platformName = std::nullopt) const;

  virtual std::string getWeutilConfig(
      const std::optional<std::string>& platformName = std::nullopt) const;

  virtual std::string getFwUtilConfig(
      const std::optional<std::string>& platformName = std::nullopt) const;
};

} // namespace facebook::fboss::platform
