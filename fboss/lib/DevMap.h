// Copyright (c) Meta Platforms, Inc. and affiliates.

#pragma once

#include <filesystem>
#include <string>

// This file contains helper functions to resolve "/run/devmap/" symlink
// files to the corresponding target devices.

namespace facebook::fboss {

enum class FbDeviceType {
  I2C_CONTROLLER,
  GPIO_CONTROLLER,
  MDIO_CONTROLLER,
  EEPROM,
  SENSOR,
  FPGA,
  CPLD,
};

// Resolves symlink name to the target device.
std::filesystem::path resolveDevice(FbDeviceType type, const std::string& name);

// Helper to resolve I2C master to /sys/bus/i2c/devices/i2c-#/.
std::filesystem::path resolveI2cSysfs(const std::string& name);
} // namespace facebook::fboss
