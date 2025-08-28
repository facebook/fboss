// Copyright (c) Meta Platforms, Inc. and affiliates.

#include "fboss/platform/bsp_tests/utils/KmodUtils.h"

#include <fmt/format.h>
#include <folly/Format.h>
#include <folly/logging/xlog.h>
#include <gtest/gtest.h>
#include <re2/re2.h>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include "fboss/platform/helpers/PlatformUtils.h"
#include "fboss/platform/platform_manager/PkgManager.h"

namespace facebook::fboss::platform::bsp_tests {

void KmodUtils::loadKmods(const BspKmodsFile& kmods) {
  // Load shared kmods first
  for (const auto& kmod : *kmods.sharedKmods()) {
    auto result = PlatformUtils().execCommand(fmt::format("modprobe {}", kmod));
    EXPECT_EQ(result.first, 0) << "Failed to load shared kmod: " << kmod;
  }

  // Then load BSP kmods
  for (const auto& kmod : *kmods.bspKmods()) {
    auto result = PlatformUtils().execCommand(fmt::format("modprobe {}", kmod));
    EXPECT_EQ(result.first, 0) << "Failed to load BSP kmod: " << kmod;
  }
}

void KmodUtils::unloadKmods(const BspKmodsFile& kmods) {
  // Unload non-shared kmods first
  for (const auto& kmod : *kmods.bspKmods()) {
    auto result =
        PlatformUtils().execCommand(fmt::format("modprobe -r {}", kmod));
    EXPECT_EQ(result.first, 0) << "Failed to unload BSP kmod: " << kmod;
  }

  // Then unload shared kmods
  for (const auto& kmod : *kmods.sharedKmods()) {
    auto result =
        PlatformUtils().execCommand(fmt::format("modprobe -r {}", kmod));
    EXPECT_EQ(result.first, 0) << "Failed to unload shared kmod: " << kmod;
  }
}

// Run fbsp-remove.sh script
void KmodUtils::fbspRemove(
    const platform_manager::PlatformConfig& platformConfig) {
  // Find fbsp-remove.sh script path
  constexpr auto kremoveScriptPath = "/usr/local/{}_bsp/{}/fbsp-remove.sh";
  const re2::RE2 kBspRpmNameRe = "(?P<KEYWORD>[a-z]+)_bsp_kmods";
  std::string keyword{};
  re2::RE2::FullMatch(
      *platformConfig.bspKmodsRpmName(), kBspRpmNameRe, &keyword);
  std::string fbspPath = fmt::format(
      kremoveScriptPath,
      keyword,
      platform_manager::package_manager::SystemInterface()
          .getHostKernelVersion());

  XLOG(DBG) << "Running fbsp-remove: " << fbspPath;
  auto result = PlatformUtils().execCommand(fmt::format("{} -f", fbspPath));
  EXPECT_EQ(result.first, 0) << "Failed to run fbsp-remove.sh";
}

// Get list of loaded kernel modules from `lsmod` that match expected modules
std::vector<std::string> KmodUtils::getLoadedKmods(
    const BspKmodsFile& expectedKmods) {
  auto lsmodResult = PlatformUtils().execCommand("lsmod");
  EXPECT_EQ(lsmodResult.first, 0) << "Failed to run lsmod";
  const auto& lsmodOutput = lsmodResult.second;

  std::set<std::string> expectedKmodSet;
  for (const auto& kmod : *expectedKmods.bspKmods()) {
    expectedKmodSet.insert(kmod);
  }
  for (const auto& kmod : *expectedKmods.sharedKmods()) {
    expectedKmodSet.insert(kmod);
  }

  // Parse lsmod output and find matching kmods
  std::vector<std::string> loadedKmods;
  std::istringstream iss(lsmodOutput);
  std::string line;

  // Skip header line
  std::getline(iss, line);

  while (std::getline(iss, line)) {
    std::istringstream lineStream(line);
    std::string kmod;
    lineStream >> kmod;

    if (!kmod.empty() && expectedKmodSet.find(kmod) != expectedKmodSet.end()) {
      loadedKmods.push_back(kmod);
    }
  }

  return loadedKmods;
}

} // namespace facebook::fboss::platform::bsp_tests
