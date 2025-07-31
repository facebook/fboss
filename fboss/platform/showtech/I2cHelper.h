// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/platform/helpers/PlatformUtils.h"

#include <map>

namespace facebook::fboss::platform {

class I2cHelper {
 public:
  std::map<uint16_t, std::string> findI2cBuses();

 private:
  PlatformUtils platformUtils_{};
};

} // namespace facebook::fboss::platform
