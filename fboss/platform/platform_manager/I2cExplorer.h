// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <filesystem>
#include <map>
#include <memory>
#include <optional>
#include <vector>

#include <fmt/format.h>
#include <folly/String.h>
#include <folly/logging/xlog.h>
#include <re2/re2.h>

#include "fboss/platform/helpers/PlatformUtils.h"
#include "fboss/platform/platform_manager/I2cAddr.h"
#include "fboss/platform/platform_manager/gen-cpp2/platform_manager_config_types.h"

namespace facebook::fboss::platform::platform_manager {

class I2cExplorer {
 public:
  virtual ~I2cExplorer() = default;
  I2cExplorer(
      const std::shared_ptr<PlatformUtils>& platformUtils =
          std::make_shared<PlatformUtils>())
      : platformUtils_(platformUtils) {}

  const re2::RE2 kI2cBusNameRegex{"i2c-\\d+"};

  // Given a filepath to an i2c bus, this function returns the bus number.
  // The i2c filepath should end in kI2cBusNameRegex, otherwise it throws
  // an exception.
  uint16_t extractBusNumFromPath(const std::filesystem::path& busPath);

  // This function takes as input the list of `i2cAdaptersFromCpu` defined
  // in the platform_manager_config.thrift, and provides the corresponding
  // kernel assigned bus numbers.
  std::map<std::string, uint16_t> getBusNums(
      const std::vector<std::string>& i2cAdaptersFromCpu);

  // Checks if a I2c devices is present at `addr` on `busNum`.
  virtual bool isI2cDevicePresent(uint16_t busNum, const I2cAddr& addr) const;

  // Returns the i2c device name if present at `addr` on `busNum`.
  virtual std::optional<std::string> getI2cDeviceName(
      uint16_t busNum,
      const I2cAddr& addr);

  // Creates an i2c device for `deviceName` on `busNum` and `addr`.
  void createI2cDevice(
      const std::string& pmUnitScopedName,
      const std::string& deviceName,
      uint16_t busNum,
      const I2cAddr& addr);

  // Setup/Initialize an I2C device by updating device registers with
  // the supplied values. Called before createI2cDevice().
  void setupI2cDevice(
      uint16_t busNum,
      const I2cAddr& addr,
      const std::vector<I2cRegData>& initRegSettings);

  // Returns the I2C Buses which were created for the channels behind the I2C
  // Mux at `busNum`@`addr`. It reads the children of
  // /sys/bus/i2c/devices/`busNum`-`addr`/ to obtain this. This function needs
  // to be called after `createI2cMux()` for the mux. Otherwise it throws an
  // exception. The key is channel number and value is the bus number.
  std::map<uint16_t, uint16_t> getMuxChannelI2CBuses(
      uint16_t busNum,
      const I2cAddr& addr);

  // Return sysfs path to the device at `addr` on `busNum`.
  static std::string getDeviceI2cPath(uint16_t busNum, const I2cAddr& addr);

  // Return character device path (/dev/i2c-#) of the given i2c bus.
  static std::string getI2cBusCharDevPath(uint16_t busNum);

 private:
  std::shared_ptr<PlatformUtils> platformUtils_{};
};

} // namespace facebook::fboss::platform::platform_manager
