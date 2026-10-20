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

namespace facebook::fboss::platform::platform_checks {

std::vector<std::filesystem::path> PciDeviceCheck::getPciDevicePaths() const {
  return host().listDirectory("/sys/bus/pci/devices");
}

CheckResult PciDeviceCheck::run() {
  try {
    auto platformConfig = getPlatformConfig();
    const auto presentDevices = readPresentDevices();
    std::vector<PciDevice> missingDevices;

    // Get expected devices from config
    for (const auto& [_, pmUnitConfig] : *platformConfig.pmUnitConfigs()) {
      for (const auto& pciDeviceConfig : *pmUnitConfig.pciDeviceConfigs()) {
        PciDevice expectedDevice;
        expectedDevice.vendor = *pciDeviceConfig.vendorId();
        expectedDevice.device = *pciDeviceConfig.deviceId();
        expectedDevice.subsystemVendor = *pciDeviceConfig.subSystemVendorId();
        expectedDevice.subsystemDevice = *pciDeviceConfig.subSystemDeviceId();

        if (!presentDevices.contains(expectedDevice.toString())) {
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

std::set<std::string> PciDeviceCheck::readPresentDevices() const {
  std::set<std::string> present;
  for (const auto& devicePath : getPciDevicePaths()) {
    auto vendor = host().readTrimmedFile(devicePath / "vendor");
    auto device = host().readTrimmedFile(devicePath / "device");
    auto subsystemVendor =
        host().readTrimmedFile(devicePath / "subsystem_vendor");
    auto subsystemDevice =
        host().readTrimmedFile(devicePath / "subsystem_device");

    if (!(vendor && device && subsystemVendor && subsystemDevice)) {
      XLOG(WARNING) << "Failed to read PCI device info from " << devicePath;
      continue;
    }
    present.insert(
        PciDevice{*vendor, *device, *subsystemVendor, *subsystemDevice}
            .toString());
  }
  return present;
}

} // namespace facebook::fboss::platform::platform_checks
