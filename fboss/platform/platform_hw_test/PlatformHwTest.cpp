/*
 *  Copyright (c) 2004-present, Meta Platforms, Inc. and affiliates.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include <folly/MacAddress.h>
#include <gtest/gtest.h>

#include <net/if.h>
#include <net/if_arp.h>
#include <sys/ioctl.h>
#include <filesystem>
#include <memory>
#include <sstream>

#include "fboss/platform/helpers/Init.h"
#include "fboss/platform/helpers/PlatformFsUtils.h"
#include "fboss/platform/platform_manager/ConfigUtils.h"
#include "fboss/platform/weutil/ConfigUtils.h"
#include "fboss/platform/weutil/FbossEepromInterface.h"

namespace facebook::fboss::platform {

class PlatformHwTest : public ::testing::Test {
 public:
  void SetUp() override {
    platformConfig_ = platform_manager::ConfigUtils().getConfig();
    fruEepromList_ = weutil::ConfigUtils().getFruEepromList();
  }

 protected:
  std::unordered_map<std::string, weutil::FruEeprom> fruEepromList_;
  platform_manager::PlatformConfig platformConfig_;

  folly::MacAddress getMacAddress(const std::string& interface) {
    auto sock_deleter = [](int* fd) {
      if (*fd >= 0) {
        close(*fd);
      }
      delete fd;
    };
    std::unique_ptr<int, decltype(sock_deleter)> sock(
        new int(socket(AF_INET, SOCK_DGRAM, 0)), sock_deleter);
    if (*sock < 0) {
      throw std::runtime_error("Failed to create socket");
    }

    struct ifreq ifr = {};
    std::strncpy(ifr.ifr_name, interface.c_str(), IFNAMSIZ - 1);
    if (ioctl(*sock, SIOCGIFHWADDR, &ifr) < 0) {
      throw std::runtime_error("Failed to get mac address");
    }
    if (ifr.ifr_hwaddr.sa_family != ARPHRD_ETHER) {
      throw std::runtime_error("Invalid mac address");
    }
    return folly::MacAddress::fromBinary(
        {reinterpret_cast<const unsigned char*>(ifr.ifr_hwaddr.sa_data), 6});
  }
};

TEST_F(PlatformHwTest, CorrectMacX86) {
  // Get eth0 mac address
  auto eth0Mac = folly::MacAddress::ZERO;
  try {
    eth0Mac = getMacAddress("eth0");
  } catch (const std::exception& e) {
    ASSERT_TRUE(false) << "Failed to get mac address: " << e.what();
  }
  EXPECT_NE(eth0Mac, folly::MacAddress::ZERO);

  // Get SCM/COME mac address
  const auto eeprom_name = fruEepromList_.contains("SCM") ? "SCM" : "COME";
  auto eeprom = fruEepromList_.at(eeprom_name);
  FbossEepromInterface eepromInterface(eeprom.path, eeprom.offset);
  std::string eepromMacStr = (*eepromInterface.getEepromContents().x86CpuMac());
  eepromMacStr = eepromMacStr.substr(0, eepromMacStr.find(','));
  auto eepromMac = folly::MacAddress::fromString(eepromMacStr);

  EXPECT_NE(eepromMac, folly::MacAddress::ZERO);
  EXPECT_EQ(eepromMac, eth0Mac);
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
