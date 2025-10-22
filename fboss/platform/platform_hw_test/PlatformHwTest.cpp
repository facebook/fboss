/*
 *  Copyright (c) 2004-present, Meta Platforms, Inc. and affiliates.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include <gtest/gtest.h>

#include <filesystem>
#include <sstream>

#include "fboss/platform/helpers/Init.h"
#include "fboss/platform/helpers/PlatformFsUtils.h"
#include "fboss/platform/platform_checks/checks/MacAddressCheck.h"
#include "fboss/platform/platform_manager/ConfigUtils.h"

namespace facebook::fboss::platform {

class PlatformHwTest : public ::testing::Test {
 public:
  void SetUp() override {
    platformConfig_ = platform_manager::ConfigUtils().getConfig();
  }

 protected:
  platform_manager::PlatformConfig platformConfig_;
};

TEST_F(PlatformHwTest, CorrectMacX86) {
  platform_checks::MacAddressCheck macCheck;
  auto result = macCheck.run();

  if (result.status() != platform_checks::CheckStatus::OK) {
    std::string failureMsg = "MAC address check failed";
    if (result.errorMessage().has_value()) {
      failureMsg += ": " + *result.errorMessage();
    }
    FAIL() << failureMsg;
  }
}

TEST_F(PlatformHwTest, PCIDevicesPresent) {
  struct PciDevice {
    std::string vendor;
    std::string device;
    std::string subsystemVendor;
    std::string subsystemDevice;
  };

  std::vector<PciDevice> missingDevices;

  // Get expected devices from config
  for (const auto& [_, pmUnitConfig] : *platformConfig_.pmUnitConfigs()) {
    for (const auto& pciDeviceConfig : *pmUnitConfig.pciDeviceConfigs()) {
      PciDevice pciDevice;
      pciDevice.vendor = *pciDeviceConfig.vendorId();
      pciDevice.device = *pciDeviceConfig.deviceId();
      pciDevice.subsystemVendor = *pciDeviceConfig.subSystemVendorId();
      pciDevice.subsystemDevice = *pciDeviceConfig.subSystemDeviceId();

      bool deviceFound = false;
      for (const auto& entry :
           std::filesystem::directory_iterator("/sys/bus/pci/devices")) {
        auto file = PlatformFsUtils(entry.path());
        auto vendor = file.getStringFileContent("vendor");
        auto device = file.getStringFileContent("device");
        auto subsystemVendor = file.getStringFileContent("subsystem_vendor");
        auto subsystemDevice = file.getStringFileContent("subsystem_device");
        if (!(vendor && device && subsystemVendor && subsystemDevice)) {
          EXPECT_TRUE(false) << fmt::format(
              "Failed to read PCI device info for {}:{}:{}:{}",
              vendor.value_or(""),
              device.value_or(""),
              subsystemVendor.value_or(""),
              subsystemDevice.value_or(""));
          continue;
        }
        if (std::tie(*vendor, *device, *subsystemVendor, *subsystemDevice) ==
            std::tie(
                pciDevice.vendor,
                pciDevice.device,
                pciDevice.subsystemVendor,
                pciDevice.subsystemDevice)) {
          deviceFound = true;
          break;
        }
      }
      if (!deviceFound) {
        missingDevices.push_back(pciDevice);
      }
    }
  }

  EXPECT_TRUE(missingDevices.empty())
      << "Missing PCI devices:\n"
      << [&missingDevices]() {
           std::stringstream ss;
           for (const auto& device : missingDevices) {
             ss << fmt::format(
                 "\t- {}:{}:{}:{}\n",
                 device.vendor,
                 device.device,
                 device.subsystemVendor,
                 device.subsystemDevice);
           }
           return ss.str();
         }();
}

} // namespace facebook::fboss::platform

int main(int argc, char* argv[]) {
  testing::InitGoogleTest(&argc, argv);
  facebook::fboss::platform::helpers::init(&argc, &argv);
  return RUN_ALL_TESTS();
}
