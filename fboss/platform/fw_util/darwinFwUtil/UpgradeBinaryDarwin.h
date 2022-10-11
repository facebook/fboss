// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.
#pragma once

#include <sysexits.h>
#include <algorithm>
#include <unordered_set>

#include <folly/init/Init.h>

#include "fboss/platform/fw_util/darwinFwUtil/FirmwareUpgradeDarwin.h"
#include "fboss/platform/fw_util/firmware_helpers/FirmwareUpgradeHelper.h"

#define DARWIN_CPLD_ADDR 0x23
#define DARWIN_FAN_ADDR 0x60
#define DARWIN_CPLD_JTAG_REG_ADDR 0x0c
namespace facebook::fboss::platform::fw_util {

class UpgradeBinaryDarwin : public FirmwareUpgradeDarwin {
 public:
  UpgradeBinaryDarwin();
  int parseCommandLine(int, char**, std::string);
  virtual ~UpgradeBinaryDarwin() override = default;

 protected:
  std::unordered_set<std::string> jamUpgradableBinaries_ = {
      "cpu_cpld",
      "sc_cpld",
      "sc_sat_cpld0",
      "sc_sat_cpld1"};
  std::string scBus_;
  std::string fanBus_;
  bool failedPath_ = false;
  const std::string darwinScSatPath_ = "/sys/bus/pci/devices/0000:07:00.0/";
  const std::string darwinFanCpldPath_ =
      "/sys/bus/i2c/drivers/rook-fan-cpld/*/*/*/";
  const std::string darwinScCpldPath_ = "/sys/bus/i2c/drivers/blackhawk-cpld/";
  const std::string blackhawkRegister_ = "0023";
  const std::string darwinCpuCpldPath_ =
      "/sys/bus/pci/drivers/scd/0000\\:ff\\:0b.3/";
  void upgradeThroughJam(std::string, std::string, std::string);
  void upgradeThroughXapp(std::string, std::string, std::string);
  void setJtagMux(std::string);
  void printVersion(std::string, std::string);
  void printAllVersion();
  void constructCpldPath(std::string, std::string, std::string);
  std::string getBusNumber(std::string, std::string);
  std::string getSysfsCpldVersion(std::string, std::string, std::string);
  std::string getFanCpldVersion(void);
  std::string getRegValue(std::string);
  std::string getBiosVersion(void);
  std::string getFullScCpldPath(void);
  void cleanPath(std::string);
};

} // namespace facebook::fboss::platform::fw_util
