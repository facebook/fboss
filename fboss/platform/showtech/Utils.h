// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/platform/helpers/PlatformUtils.h"
#include "fboss/platform/showtech/I2cHelper.h"
#include "fboss/platform/showtech/gen-cpp2/showtech_config_types.h"

namespace facebook::fboss::platform {

class Utils {
 public:
  Utils(const showtech_config::ShowtechConfig& config) : config_(config) {};
  ~Utils() = default;

  void printHostDetails();
  void printFbossDetails();
  void printWeutilDetails();
  void printFwutilDetails();
  void printLspciDetails();
  void printPortDetails();
  void printSensorDetails();
  void printI2cDetails();
  void printI2cDumpDetails();
  void printPsuDetails();
  void printGpioDetails();
  void printPemDetails();
  void printFanDetails();
  void printFanspinnerDetails();
  void printNvmeDetails();
  void printPowerGoodDetails();
  void printLogs();

 private:
  const showtech_config::ShowtechConfig& config_;
  PlatformUtils platformUtils_{};
  I2cHelper i2cHelper_{};
  void runFbossCliCmd(const std::string& cmd);
  void printSysfsAttribute(const std::string& label, const std::string& path);
  void printGpio(const showtech_config::Gpio& gpio);
  void printServiceLogs(const std::string& service) const;
  std::optional<std::tuple<int, int>> getI2cInfoForDevice(
      const std::string& path);
  std::pair<int, std::string> execCommandWithLimit(
      const std::string& cmd,
      int maxLines = 5000) const;
};

} // namespace facebook::fboss::platform
