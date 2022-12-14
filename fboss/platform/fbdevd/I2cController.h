// Copyright (c) Meta Platforms, Inc. and affiliates.

#pragma once

#include <filesystem>
#include <set>
#include <string>

// This file contains APIs to create/delete I2C client devices via the
// Linux sysfs interface. For details, please refer to:
// https://www.kernel.org/doc/Documentation/i2c/instantiating-devices

namespace facebook::fboss::platform::fbdevd {

class I2cController {
 public:
  explicit I2cController(const std::string& busName);
  ~I2cController() {}

  // Test if I2C client device at given "addr" was already created.
  bool isDeviceCreated(uint16_t addr);

  // Create I2C client device at "addr" with given "name".
  void newDevice(uint16_t addr, const std::string& name);

  // Remove the I2C client device at "addr" from the bus.
  void deleteDevice(uint16_t addr);

 private:
  std::string busName_{};
  std::filesystem::path sysI2cBusDir_{};
  std::filesystem::path sysI2cNewDevice_{};
  std::filesystem::path sysI2cDelDevice_{};
  std::set<uint16_t> createdDevices_ = {};
};

} // namespace facebook::fboss::platform::fbdevd
