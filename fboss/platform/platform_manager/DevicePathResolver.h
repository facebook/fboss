// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/platform/platform_manager/DataStore.h"
#include "fboss/platform/platform_manager/I2cExplorer.h"
#include "fboss/platform/platform_manager/gen-cpp2/platform_manager_config_types.h"

namespace facebook::fboss::platform::platform_manager {

class DevicePathResolver {
 public:
  explicit DevicePathResolver(
      const PlatformConfig& config,
      const DataStore& dataStore,
      const I2cExplorer& i2cExplorer);

  // Resolves a given DevicePath to i2c sysfs path in the system.
  // Throws a runtime exception if the DevicePath cannot be resolved.
  std::string resolveI2cDevicePath(const std::string& devicePath);

 private:
  // Get I2cDeviceConfig for a given SlotPath and DeviceName.
  // Throws a runtime exception if the I2cDeviceConfig is not found.
  I2cDeviceConfig getI2cDeviceConfig(
      const std::string& slotPath,
      const std::string& deviceName);
  // Get IdpromConfig for a given SlotPath.
  // Throws a runtime exception if the IdpromConfig is not found.
  IdpromConfig getIdpromConfig(const std::string& slotPath);

  PlatformConfig platformConfig_;
  const DataStore& dataStore_;
  const I2cExplorer& i2cExplorer_;
};

} // namespace facebook::fboss::platform::platform_manager
