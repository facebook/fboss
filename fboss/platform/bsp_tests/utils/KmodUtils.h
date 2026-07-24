// Copyright (c) Meta Platforms, Inc. and affiliates.

#pragma once

#include <string>
#include <vector>

#include "fboss/platform/platform_manager/gen-cpp2/platform_manager_config_types.h"

namespace facebook::fboss::platform::bsp_tests {

using platform_manager::BspKmodsFile;

class KmodUtils {
 public:
  static void bindDesiredDrivers(
      const platform_manager::PlatformConfig& platformConfig);
  static void loadKmods(const BspKmodsFile& kmods);
  static void unloadKmods(const BspKmodsFile& kmods);
  static void fbspRemove(
      const platform_manager::PlatformConfig& platformConfig);
  static std::vector<std::string> getLoadedKmods(
      const BspKmodsFile& expectedKmods);

  // Extract the vendor keyword (e.g. "fboss") from bspKmodsRpmName
  // (e.g. "fboss_bsp_kmods") in the platform config.
  static std::string getVendorKeyword(
      const platform_manager::PlatformConfig& platformConfig);

  static std::string getKernelVersion();
};

} // namespace facebook::fboss::platform::bsp_tests
