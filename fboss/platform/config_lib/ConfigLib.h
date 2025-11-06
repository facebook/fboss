// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <optional>
#include <string>

#include <gflags/gflags.h>

DECLARE_string(config_file);
DECLARE_bool(run_in_netos);

namespace facebook::fboss::platform {

class ConfigLib {
 public:
  virtual ~ConfigLib() = default;
  explicit ConfigLib(const std::string& configFilePath = "")
      : configFilePath_(configFilePath) {}
  virtual std::string getSensorServiceConfig(
      const std::optional<std::string>& platformName = std::nullopt) const;

  virtual std::string getFanServiceConfig(
      const std::optional<std::string>& platformName = std::nullopt) const;

  virtual std::string getPlatformManagerConfig(
      const std::optional<std::string>& platformName = std::nullopt) const;

  virtual std::string getFwUtilConfig(
      const std::optional<std::string>& platformName = std::nullopt) const;

  virtual std::string getLedManagerConfig(
      const std::optional<std::string>& platformName = std::nullopt) const;

  virtual std::string getBspTestConfig(
      const std::optional<std::string>& platformName = std::nullopt) const;

  virtual std::string getShowtechConfig(
      const std::optional<std::string>& platformName = std::nullopt) const;

 protected:
  std::string configFilePath_;
  std::optional<std::string> getConfigFromFile() const;
};

} // namespace facebook::fboss::platform
