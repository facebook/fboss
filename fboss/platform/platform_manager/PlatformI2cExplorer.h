// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <map>
#include <memory>
#include <stdexcept>
#include <vector>

#include "fboss/platform/helpers/PlatformUtils.h"

namespace facebook::fboss::platform::platform_manager {

class PlatformI2cExplorer {
 public:
  virtual ~PlatformI2cExplorer() {}
  PlatformI2cExplorer(
      const std::shared_ptr<PlatformUtils>& platformUtils =
          std::make_shared<PlatformUtils>())
      : platformUtils_(platformUtils){};

  // This function takes as input the list of `i2cBussesFromMainBoard` defined
  // in the platform_manager_config.thrift, and outputs a map from
  // `i2cBussesFromMainBoard` to the corresponding i2c bus names assigned on the
  // board by the kernel.
  std::map<std::string, std::string> getBusesfromBsp(
      const std::vector<std::string>& i2cBussesFromMainBoard);

  // Returns the FRU Type name based on the contents read from the EEPROM
  std::string getFruTypeName(const std::string& eepromPath);

  // Checks if a I2c devices is present at `addr` on `busName`.
  virtual bool isI2cDevicePresent(const std::string& busName, uint8_t addr);

  // Creates an i2c device for `deviceName` on `busName` and `addr`.
  void createI2cDevice(
      const std::string& deviceName,
      const std::string& busName,
      uint8_t addr);

  bool createI2cMux(
      const std::string& deviceName,
      const std::string& busName,
      uint8_t addr,
      uint8_t numChannels);

  // Returns the I2C Buses whih were created for the channels behind the I2C Mux
  // at `busName`@`addr`. They are listed in the ascending order of channels. It
  // reads the children of /sys/bus/i2c/devices/`busName`-`addr`/ to obtain
  // this. This function needs to be called after `createI2cMux()` for the mux.
  // Otherwise it throws an exception.
  std::vector<std::string> getMuxChannelI2CBuses(
      const std::string& busName,
      uint8_t addr);

  // Return sysfs path to the device at `addr` on `i2cBusName`.
  static std::string getDeviceI2cPath(
      const std::string& i2cBusName,
      uint8_t addr);

 private:
  std::shared_ptr<PlatformUtils> platformUtils_{};
};

} // namespace facebook::fboss::platform::platform_manager
