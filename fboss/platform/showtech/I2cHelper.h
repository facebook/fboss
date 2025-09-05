// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/platform/helpers/PlatformUtils.h"

#include <boost/bimap.hpp>
#include <string>

namespace facebook::fboss::platform {

class I2cHelper {
 public:
  // Map of i2cBusNum to adapterName
  using I2cBusMap = boost::bimap<uint16_t, std::string>;

  I2cHelper();
  int findI2cBusForScdI2cDevice(
      std::string scdPciAddr,
      uint16_t master,
      uint16_t bus);
  const I2cBusMap& getI2cBusMap() const;

 private:
  void findI2cBuses();
  PlatformUtils platformUtils_{};
  I2cBusMap busMap_{};
};

} // namespace facebook::fboss::platform
