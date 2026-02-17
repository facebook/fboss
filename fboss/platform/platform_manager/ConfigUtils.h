// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <memory>
#include <optional>

#include "fboss/platform/config_lib/ConfigLib.h"
#include "fboss/platform/helpers/PlatformFsUtils.h"
#include "fboss/platform/helpers/PlatformNameLib.h"
#include "fboss/platform/platform_manager/gen-cpp2/platform_manager_config_types.h"

namespace facebook::fboss::platform::platform_manager {

class ConfigUtils {
 public:
  static constexpr auto kConfigHashFile = "/run/devmap/metadata/config_hash";
  static constexpr auto kBuildInfoFile = "/run/devmap/metadata/build_info";

  explicit ConfigUtils(
      std::shared_ptr<ConfigLib> configLib = std::make_shared<ConfigLib>(),
      std::shared_ptr<helpers::PlatformNameLib> platformNameLib =
          std::make_shared<helpers::PlatformNameLib>(),
      std::shared_ptr<PlatformFsUtils> platformFsUtils =
          std::make_shared<PlatformFsUtils>());

  PlatformConfig getConfig();

  // Returns true if config has changed since last run (or first run)
  bool hasConfigChanged();

  // Stores hash of current config - call after successful exploration
  void storeConfigHash();

 private:
  std::shared_ptr<ConfigLib> configLib_;
  std::shared_ptr<helpers::PlatformNameLib> platformNameLib_;
  std::shared_ptr<PlatformFsUtils> pFsUtils_;
  std::optional<PlatformConfig> config_;
};

} // namespace facebook::fboss::platform::platform_manager
