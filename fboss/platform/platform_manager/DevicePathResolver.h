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

  // Resolves a given DevicePath to the sensor sysfs path in the system.
  // Throws a runtime exception if devicePath fails to resolve to I2cDevicePath
  // or fails to resolve to sensor sysfs path.
  std::string resolveSensorPath(const std::string& devicePath);

  // Resolves a given DevicePath to the eeprom sysfs path in the system.
  // Throws a runtime exception if devicePath fails to resolve to I2cDevicePath
  // or fails to resolve to eeprom sysfs path.
  std::string resolveEepromPath(const std::string& devicePath);

  // Resolves a given DevicePath to I2cBusPath in the system.
  // Throws a runtime exception if the DevicePath cannot be resolved.
  std::string resolveI2cBusPath(const std::string& devicePath);

  // Resolves a given DevicePath to i2c sysfs path in the system.
  // Throws a runtime exception if the DevicePath cannot be resolved.
  std::string resolveI2cDevicePath(const std::string& devicePath);

  // Try resolving a given DevicePath to i2c sysfs path in the system.
  std::optional<std::string> tryResolveI2cDevicePath(
      const std::string& devicePath);

  // Resolves a given DevicePath to pci sysfs path in the system.
  // Throws a runtime exception if the DevicePath cannot be resolved.
  std::string resolvePciDevicePath(const std::string& devicePath);

  // Resolves the presenceFileName present at the given devicePath.
  std::optional<std::string> resolvePresencePath(
      const std::string& devicePath,
      const std::string& presenceFileName);

 private:
  // Get I2cDeviceConfig for a given SlotPath and DeviceName.
  // Throws a runtime exception if the I2cDeviceConfig is not found.
  I2cDeviceConfig getI2cDeviceConfig(
      const std::string& slotPath,
      const std::string& deviceName);
  // Get IdpromConfig for a given SlotPath.
  // Throws a runtime exception if the IdpromConfig is not found.
  IdpromConfig getIdpromConfig(const std::string& slotPath);
  // Get PciDeviceConfig for a given SlotPath and DeviceName.
  // Throws a runtime exception if the PciDeviceConfig is not found.
  PciDeviceConfig getPciDeviceConfig(
      const std::string& slotPath,
      const std::string& deviceName);

  PlatformConfig platformConfig_;
  const DataStore& dataStore_;
  const I2cExplorer& i2cExplorer_;
};

} // namespace facebook::fboss::platform::platform_manager
