// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.
#pragma once

#include <folly/init/Init.h>
#include <sysexits.h>
#include <algorithm>
#include <unordered_set>
#include "fboss/platform/fw_util/FirmwareUpgradeDarwin.h"
#include "fboss/platform/helpers/FirmwareUpgradeHelper.h"
#include "fboss/platform/helpers/Utils.h"

#define DARWIN_CPLD_ADDR 0x23
#define DARWIN_CPLD_JTAG_REG_ADDR 0x0c
namespace facebook::fboss::platform {

class UpgradeBinaryDarwin : public FirmwareUpgradeDarwin {
 public:
  UpgradeBinaryDarwin();
  int parseCommandLine(int, char**);
  void printUsage(void);
  virtual ~UpgradeBinaryDarwin() override = default;

 protected:
  std::unordered_set<std::string> jamUpgradableBinaries = {
      "cpu_cpld",
      "sc_cpld",
      "sc_sat_cpld0",
      "sc_sat_cpld1"};
  std::string bus;
  void upgradeThroughJam(std::string, std::string, std::string);
  void upgradeThroughXapp(std::string, std::string, std::string);
  void setJtagMux(std::string);
};

} // namespace facebook::fboss::platform
