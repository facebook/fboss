// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/platform/helpers/PlatformUtils.h"

namespace facebook::fboss::platform {

class Utils {
 public:
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
  PlatformUtils platformUtils_{};
};

} // namespace facebook::fboss::platform
