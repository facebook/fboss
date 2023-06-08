// Copyright (c) 2004-present, Meta Platforms, Inc. and affiliates.
// All Rights Reserved.

#pragma once

#include <folly/experimental/FunctionScheduler.h>

#include "fboss/platform/config_lib/ConfigLib.h"
#include "fboss/platform/platform_manager/gen-cpp2/platform_manager_config_types.h"

namespace facebook::fboss::platform::platform_manager {
class PlatformExplorer {
 public:
  explicit PlatformExplorer(
      std::chrono::seconds exploreInterval,
      const std::string& configFile,
      const ConfigLib& configLib = ConfigLib());
  void explore();

 private:
  folly::FunctionScheduler scheduler_;
  PlatformConfig platformConfig_{};
};

} // namespace facebook::fboss::platform::platform_manager
