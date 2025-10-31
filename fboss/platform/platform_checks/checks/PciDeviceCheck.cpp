/*
 *  Copyright (c) 2004-present, Meta Platforms, Inc. and affiliates.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/platform/platform_checks/checks/PciDeviceCheck.h"

#include <filesystem>
#include <sstream>

#include <folly/logging/xlog.h>
#include "fboss/platform/helpers/PlatformFsUtils.h"

namespace facebook::fboss::platform::platform_checks {

PciDeviceCheck::PciDeviceCheck(std::shared_ptr<PlatformFsUtils> platformFsUtils)
    : fsUtils_(std::move(platformFsUtils)) {}

std::vector<std::filesystem::path> PciDeviceCheck::getPciDevicePaths() const {
  std::vector<std::filesystem::path> paths;
  try {
    for (const auto& entry : fsUtils_->ls("/sys/bus/pci/devices")) {
      if (entry.is_directory()) {
        paths.push_back(entry.path());
      }
    }
  } catch (const std::exception& ex) {
    XLOG(ERR) << "Error listing PCI devices: " << ex.what();
  }
  return paths;
}

CheckResult PciDeviceCheck::run() {
  try {
    auto platformConfig = getPlatformConfig();
    std::vector<PciDevice> missingDevices;

    // Get expected devices from config
    for (const auto& [_, pmUnitConfig] : *platformConfig.pmUnitConfigs()) {
      for (const auto& pciDeviceConfig : *pmUnitConfig.pciDeviceConfigs()) {
        PciDevice expectedDevice;
        expectedDevice.vendor = *pciDeviceConfig.vendorId();
        expectedDevice.device = *pciDeviceConfig.deviceId();
        expectedDevice.subsystemVendor = *pciDeviceConfig.subSystemVendorId();
        expectedDevice.subsystemDevice = *pciDeviceConfig.subSystemDeviceId();

        if (!pciDeviceExists(expectedDevice)) {
          missingDevices.push_back(expectedDevice);
          XLOG(WARNING) << "Expected PCI device " << expectedDevice.toString()
                        << " not found";
        }
      }
    }

    if (missingDevices.empty()) {
      return makeOK();
    }
    std::stringstream errorMsg;
    errorMsg << "Missing " << missingDevices.size() << " PCI device(s):";
    for (const auto& device : missingDevices) {
      errorMsg << " " << device.toString();
    }
    std::string remediationMessage =
        "Reboot may recover the missing PCI device(s)";

    return makeProblem(
        errorMsg.str(), RemediationType::REBOOT_REQUIRED, remediationMessage);
  } catch (const std::exception& ex) {
    return makeError(
        "Failed to validate PCI devices: " + std::string(ex.what()));
  }
}

bool PciDeviceCheck::pciDeviceExists(const PciDevice& expectedDevice) {
  try {
    for (const auto& devicePath : getPciDevicePaths()) {
      auto vendor = fsUtils_->getStringFileContent(devicePath / "vendor");
      auto device = fsUtils_->getStringFileContent(devicePath / "device");
      auto subsystemVendor =
          fsUtils_->getStringFileContent(devicePath / "subsystem_vendor");
      auto subsystemDevice =
          fsUtils_->getStringFileContent(devicePath / "subsystem_device");

      if (!(vendor && device && subsystemVendor && subsystemDevice)) {
        XLOG(WARNING) << "Failed to read PCI device info from " << devicePath;
        continue;
      }

      if (std::tie(*vendor, *device, *subsystemVendor, *subsystemDevice) ==
          std::tie(
              expectedDevice.vendor,
              expectedDevice.device,
              expectedDevice.subsystemVendor,
              expectedDevice.subsystemDevice)) {
        return true;
      }
    }
  } catch (const std::exception& ex) {
    XLOG(ERR) << "Error checking PCI devices: " << ex.what();
    throw;
  }

  return false;
}

} // namespace facebook::fboss::platform::platform_checks
