// Copyright (c) Meta Platforms, Inc. and affiliates.

#include <fmt/core.h>

#include "fboss/lib/CommonFileUtils.h"
#include "fboss/lib/DevMap.h"
#include "fboss/platform/fbdevd/I2cController.h"

namespace fs = std::filesystem;

namespace facebook::fboss::platform::fbdevd {

bool I2cController::isDeviceCreated(uint16_t addr) {
  return createdDevices_.find(addr) != createdDevices_.end();
}

void I2cController::newDevice(uint16_t addr, const std::string& name) {
  if (isDeviceCreated(addr)) {
    return;
  }

  // Send "name address" string to the sysfs file to initiate device
  // creation.
  std::string inStr = name + " " + std::to_string(addr);
  if (!facebook::fboss::writeSysfs(sysI2cNewDevice_.string(), inStr)) {
    throw std::runtime_error(fmt::format(
        "I2C bus {}: failed to create device at address {}", busName_, addr));
  }

  createdDevices_.insert(addr);
}

void I2cController::deleteDevice(uint16_t addr) {
  if (!isDeviceCreated(addr)) {
    return;
  }

  // Send "address" string to the sysfs file to delete the I2C device.
  std::string addrStr = std::to_string(addr);
  if (!facebook::fboss::writeSysfs(sysI2cDelDevice_.string(), addrStr)) {
    throw std::runtime_error(fmt::format(
        "I2C bus {}: failed to delete device at address {}", busName_, addr));
  }

  createdDevices_.erase(addr);
}

I2cController::I2cController(const std::string& busName) {
  busName_ = busName;
  sysI2cBusDir_ = facebook::fboss::resolveI2cSysfs(busName);
  if (!fs::exists(sysI2cBusDir_)) {
    throw std::runtime_error(busName + " sysfs/i2c path does not exist");
  }

  // Initialize "createdDevices_" set by looking for device entries in
  // "sysI2cBusDir_" whose name starts with "<busNum>-".
  //   - "sysI2cBusDir" format: /sys/bus/i2c/devices/i2c-<bus-num>
  //   - device entries under "sysI2cBusDir": <bus-num>-<4-byte-hex-addr>
  std::string busId = sysI2cBusDir_.filename().string();
  std::string busPrefix = "i2c-";
  std::string devPrefix = busId.substr(busPrefix.length()) + "-";
  for (auto const& dirEntry : fs::directory_iterator(sysI2cBusDir_)) {
    std::string fileName = dirEntry.path().filename().string();
    if (fileName.find(devPrefix) == 0) {
      std::string addrStr = fileName.substr(devPrefix.length());
      uint16_t addr = std::stoul(addrStr, nullptr, 16);
      createdDevices_.insert(addr);
    }
  }

  // Store below 2 files for easier reference in new|deleteDevice().
  sysI2cNewDevice_ = sysI2cBusDir_ / "new_device";
  sysI2cDelDevice_ = sysI2cBusDir_ / "delete_device";
}
} // namespace facebook::fboss::platform::fbdevd
