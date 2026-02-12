// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <memory>
#include <optional>

#include "fboss/platform/config_lib/ConfigLib.h"
#include "fboss/platform/helpers/PlatformNameLib.h"
#include "fboss/platform/platform_manager/gen-cpp2/platform_manager_config_types.h"

namespace facebook::fboss::platform::platform_manager {

class ConfigUtils {
 public:
  explicit ConfigUtils(
      std::shared_ptr<ConfigLib> configLib = std::make_shared<ConfigLib>(),
      std::shared_ptr<helpers::PlatformNameLib> platformNameLib =
          std::make_shared<helpers::PlatformNameLib>());

  PlatformConfig getConfig();

 private:
  std::shared_ptr<ConfigLib> configLib_;
  std::shared_ptr<helpers::PlatformNameLib> platformNameLib_;
  std::optional<PlatformConfig> config_;
};

} // namespace facebook::fboss::platform::platform_manager
