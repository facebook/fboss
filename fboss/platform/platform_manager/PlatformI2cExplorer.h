// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <map>
#include <memory>
#include <optional>
#include <stdexcept>
#include <vector>

#include <fmt/format.h>
#include <folly/String.h>

#include "fboss/platform/helpers/PlatformUtils.h"

namespace facebook::fboss::platform::platform_manager {

struct I2cAddr {
 public:
  explicit I2cAddr(uint16_t addr) : addr_(addr) {}
  explicit I2cAddr(const std::string& addr)
      : addr_(std::stoi(addr, nullptr, 16 /* base */)) {
    std::string lcAddr = addr;
    folly::toLowerAscii(lcAddr);
    if (hex2Str() != lcAddr) {
      throw std::invalid_argument("Invalid I2C Address. " + addr);
    }
  }
  bool operator==(const I2cAddr& b) const {
    return addr_ == b.addr_;
  }
  // Returned string is in the format 0x0f
  std::string hex2Str() const {
    return fmt::format("{:#04x}", addr_);
  }
  // Returned string is in the format 000f
  std::string hex4Str() const {
    return fmt::format("{:04x}", addr_);
  }

 private:
  uint16_t addr_{0};
};

class PlatformI2cExplorer {
 public:
  virtual ~PlatformI2cExplorer() {}
  PlatformI2cExplorer(
      const std::shared_ptr<PlatformUtils>& platformUtils =
          std::make_shared<PlatformUtils>())
      : platformUtils_(platformUtils){};

  // This function takes as input the list of `i2cAdaptersFromCpu` defined
  // in the platform_manager_config.thrift, and provides the corresponding
  // kernel assigned bus numbers.
  std::map<std::string, int16_t> getBusNums(
      const std::vector<std::string>& i2cAdaptersFromCpu);

  // Checks if a I2c devices is present at `addr` on `busNum`.
  virtual bool isI2cDevicePresent(uint16_t busNum, const I2cAddr& addr);

  // Returns the i2c device name if present at `addr` on `busNum`.
  virtual std::optional<std::string> getI2cDeviceName(
      uint16_t busNum,
      const I2cAddr& addr);

  // Creates an i2c device for `deviceName` on `busNum` and `addr`.
  void createI2cDevice(
      const std::string& deviceName,
      uint16_t busNum,
      const I2cAddr& addr);

  // Returns the I2C Buses which were created for the channels behind the I2C
  // Mux at `busNum`@`addr`. They are listed in the ascending order of
  // channels. It reads the children of /sys/bus/i2c/devices/`busNum`-`addr`/
  // to obtain this. This function needs to be called after `createI2cMux()` for
  // the mux. Otherwise it throws an exception.
  std::vector<uint16_t> getMuxChannelI2CBuses(
      uint16_t busNum,
      const I2cAddr& addr);

  // Return sysfs path to the device at `addr` on `busNum`.
  static std::string getDeviceI2cPath(uint16_t busNum, const I2cAddr& addr);

 private:
  std::shared_ptr<PlatformUtils> platformUtils_{};
};

} // namespace facebook::fboss::platform::platform_manager
