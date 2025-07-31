// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/platform/helpers/PlatformUtils.h"
#include "fboss/platform/showtech/I2cHelper.h"
#include "fboss/platform/showtech/gen-cpp2/showtech_config_types.h"

using namespace facebook::fboss::platform::showtech_config;

namespace facebook::fboss::platform {

class Utils {
 public:
  Utils(ShowtechConfig config) : config_(config){};
  ~Utils() = default;

  void printHostDetails();
  void printFbossDetails();
  void printWeutilDetails();
  void printFwutilDetails();
  void printLspciDetails();
  void printPortDetails();
  void printSensorDetails();
  void printI2cDetails();
  void runFbossCliCmd(const std::string& cmd);

 private:
  ShowtechConfig config_;
  PlatformUtils platformUtils_{};
  I2cHelper i2cHelper_{};
};

} // namespace facebook::fboss::platform
