// Copyright (c) Meta Platforms, Inc. and affiliates.

#include "fboss/platform/bsp_tests/utils/KmodUtils.h"

#include <fmt/format.h>
#include <folly/Format.h>
#include <folly/logging/xlog.h>
#include <gtest/gtest.h>
#include <re2/re2.h>
#include <chrono>
#include <filesystem>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include <folly/FileUtil.h>
#include "fboss/platform/bsp_tests/BspTestEnvironment.h"
#include "fboss/platform/helpers/PlatformUtils.h"
#include "fboss/platform/platform_manager/PkgManager.h"
#include "fboss/platform/platform_manager/Utils.h"

namespace fs = std::filesystem;

namespace facebook::fboss::platform::bsp_tests {

void KmodUtils::bindDesiredDrivers(
    const platform_manager::PlatformConfig& platformConfig) {
  // Binds pci devices with a desiredDriver attribute to the driver
  for (const auto& [pmUnitName, pmUnitConfig] :
       *platformConfig.pmUnitConfigs()) {
    for (const auto& pciConf : *pmUnitConfig.pciDeviceConfigs()) {
      if (pciConf.desiredDriver()) {
        for (const auto& dirEntry :
             fs::directory_iterator("/sys/bus/pci/devices/")) {
          std::string vendor, device, subSystemVendor, subSystemDevice;
          auto deviceFilePath = dirEntry.path() / "device";
          auto vendorFilePath = dirEntry.path() / "vendor";
          auto subSystemVendorFilePath = dirEntry.path() / "subsystem_vendor";
          auto subSystemDeviceFilePath = dirEntry.path() / "subsystem_device";
          folly::readFile(vendorFilePath.c_str(), vendor);
          folly::readFile(deviceFilePath.c_str(), device);
          folly::readFile(subSystemVendorFilePath.c_str(), subSystemVendor);
          folly::readFile(subSystemDeviceFilePath.c_str(), subSystemDevice);
          if (folly::trimWhitespace(vendor).str() == *pciConf.vendorId() &&
              folly::trimWhitespace(device).str() == *pciConf.deviceId() &&
              folly::trimWhitespace(subSystemVendor).str() ==
                  *pciConf.subSystemVendorId() &&
              folly::trimWhitespace(subSystemDevice).str() ==
                  *pciConf.subSystemDeviceId()) {
            const auto& sysfsPath = dirEntry.path().string();
            auto curDriver = fmt::format("{}/driver", sysfsPath);
            if (fs::exists(curDriver)) {
              break;
            }

            fs::path desiredDriverPath =
                fs::path("/sys/bus/pci/drivers") / *pciConf.desiredDriver();
            auto pciDevId = fmt::format(
                "{} {} {} {}",
                std::string(vendor, 2, 4),
                std::string(device, 2, 4),
                std::string(subSystemVendor, 2, 4),
                std::string(subSystemDevice, 2, 4));

            auto command = fmt::format(
                "echo '{}' > /sys/bus/pci/drivers/{}/new_id",
                pciDevId,
                *pciConf.desiredDriver());
            auto result = PlatformUtils().execCommand(command);
            EXPECT_EQ(result.first, 0) << "Failed to bind a desiredDriver: "
                                       << *pciConf.desiredDriver();

            auto charDevPath = fmt::format(
                "/dev/fbiob_{}.{}.{}.{}",
                std::string(vendor, 2, 4),
                std::string(device, 2, 4),
                std::string(subSystemVendor, 2, 4),
                std::string(subSystemDevice, 2, 4));
            bool isReady = platform_manager::Utils().checkDeviceReadiness(
                [&charDevPath]() { return fs::exists(charDevPath); },
                fmt::format("Waiting for device {}", charDevPath),
                std::chrono::seconds(10));

            EXPECT_TRUE(isReady)
                << "Timed out after 10 seconds waiting for device to appear at: "
                << charDevPath;
            EXPECT_EQ(result.first, 0) << "Failed to bind a desiredDriver: "
                                       << *pciConf.desiredDriver();
            break;
          }
        }
      }
    }
  }
}

void KmodUtils::loadKmods(const BspKmodsFile& kmods) {
  // Load shared kmods first
  for (const auto& kmod : *kmods.sharedKmods()) {
    auto result = PlatformUtils().execCommand(fmt::format("modprobe {}", kmod));
    EXPECT_EQ(result.first, 0) << "Failed to load shared kmod: " << kmod;
  }

  BspTestEnvironment* env = BspTestEnvironment::GetInstance();
  const platform_manager::PlatformConfig& platformConfig =
      env->getPlatformManagerConfig();
  bindDesiredDrivers(platformConfig);

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
