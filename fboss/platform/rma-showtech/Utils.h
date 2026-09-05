// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once
#include <bitset>

#include "fboss/platform/helpers/PlatformUtils.h"
#include "fboss/platform/rma-showtech/I2cHelper.h"
#include "fboss/platform/rma-showtech/gen-cpp2/showtech_config_types.h"

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
  void printDeviceRegistersDetails();

 private:
  const showtech_config::ShowtechConfig& config_;
  PlatformUtils platformUtils_{};
  I2cHelper i2cHelper_{};
  void runFbossCliCmd(const std::string& cmd);
  void printSysfsAttribute(const std::string& label, const std::string& path);
  void printGpio(const showtech_config::Gpio& gpio);
  void printServiceLogs(const std::string& service) const;
  std::optional<std::tuple<int, int>> getI2cInfoForDevice(
      const std::string& path,
      bool skipLog = false);
  std::pair<int, std::string> execCommandWithLimit(
      const std::string& cmd,
      int maxLines = 5000) const;
  bool readRegVal(
      int bus,
      const std::string& addr,
      const std::string& regStr,
      uint8_t& value);

  void printRegValueTable(
      const std::vector<std::tuple<std::string, int, int>>& parsedItems,
      size_t nameWidth,
      const std::optional<std::bitset<8>>& expectedValueBits = std::nullopt);

  void parseRegValue(
      const uint8_t regValue,
      const showtech_config::parseRule& rule);
};

} // namespace facebook::fboss::platform
