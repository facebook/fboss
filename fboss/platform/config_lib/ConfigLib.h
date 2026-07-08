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

  virtual std::string getRebootCauseFinderConfig(
      const std::optional<std::string>& platformName = std::nullopt) const;

  // Resolves a platform name to the canonical platform whose config it uses,
  // applying config aliases (distinct hardware that shares another platform's
  // config, e.g. WEDGE800CNHP -> WEDGE800CACT). The result is UPPERCASE. Used
  // by services that cross-check a config's declared platformName against the
  // running platform, so the check accepts an aliased platform.
  static std::string canonicalConfigPlatformName(
      const std::string& platformName);

  // Throws if the config's declared platformName does not match the running
  // platform. Comparison is case-insensitive and resolves config aliases
  // (so a platform that reuses another's config, e.g. WEDGE800CNHP, is
  // accepted). Used by services to reject a config loaded onto the wrong box.
  static void verifyPlatformNameMatches(
      const std::string& platformNameInConfig,
      const std::string& inferredPlatformName);

 protected:
  std::string configFilePath_;
  std::optional<std::string> getConfigFromFile() const;
};

} // namespace facebook::fboss::platform
