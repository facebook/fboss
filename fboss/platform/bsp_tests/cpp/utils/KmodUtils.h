// Copyright (c) Meta Platforms, Inc. and affiliates.

#pragma once

#include <string>
#include <vector>

#include "fboss/platform/platform_manager/gen-cpp2/platform_manager_config_types.h"

namespace facebook::fboss::platform::bsp_tests::cpp {

using platform_manager::BspKmodsFile;

class KmodUtils {
 public:
  static void loadKmods(const BspKmodsFile& kmods);
  static void unloadKmods(const BspKmodsFile& kmods);
  static void fbspRemove(
      const platform_manager::PlatformConfig& platformConfig);
  static std::vector<std::string> getLoadedKmods(
      const BspKmodsFile& expectedKmods);
};

} // namespace facebook::fboss::platform::bsp_tests::cpp
