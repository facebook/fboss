// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.
#pragma once

#include <sysexits.h>
#include <algorithm>
#include <unordered_set>

#include <folly/init/Init.h>

#include "fboss/platform/fw_util/minipack3FwUtil/FirmwareUpgradeMinipack3.h"
#include "fboss/platform/fw_util/firmware_helpers/FirmwareUpgradeHelper.h"

#define MINIPACK3_CPLD_ADDR 0x23
#define MINIPACK3_FAN_ADDR 0x60
#define MINIPACK3_CPLD_JTAG_REG_ADDR 0x0c
#define IOB_FPGA_SPI_CONTROLLER_ID 0
#define DOM_FPGA_SPI_CONTROLLER_ID 1
#define MCB_CPLD_SPI_CONTROLLER_ID 3
#define SMB_CPLD_SPI_CONTROLLER_ID 4
#define ASIC_FW_SPI_CONTROLLER_ID 5
#define SCM_CPLD_SPICONTROLLER_ID 6

#define FBIOB_SPI_REG_SPI_CONTROLLER_BASE 0x10000

#define FBIOB_SPI_REG_SPI_FUNCTION_CTRL(channel_id)       FBIOB_SPI_REG_SPI_CONTROLLER_BASE + 0x80*channel_id + 0x04
#define FBIOB_SPI_REG_SPI_CMD(channel_id)                 FBIOB_SPI_REG_SPI_CONTROLLER_BASE + 0x80*channel_id + 0x08
#define FBIOB_SPI_REG_SPI_DESCRIPTOR_STATUS(channel_id)   FBIOB_SPI_REG_SPI_CONTROLLER_BASE + 0x80*channel_id + 0x0C


namespace facebook::fboss::platform::fw_util {

class UpgradeBinaryMinipack3 : public FirmwareUpgradeMinipack3 {
 public:
  UpgradeBinaryMinipack3();
  int parseCommandLine(int, char**, std::string);
  virtual ~UpgradeBinaryMinipack3() override = default;

 protected:
  std::unordered_set<std::string> flashromUpgradableBinaries_ = {
      "iob_fpga",
      "dom_fpga_1",
      "dom_fpga_2",
      "scm_cpld",
      "mcb_cpld",
      "smb_cpld",
      "nic_i210",
      "th5_bcm7800",
      "bios"};
  bool failedPath_ = false;
  void upgradeThroughFlashrom(std::string, std::string, std::string);
  void setSpiMux(std::string);
  void printVersion(std::string, std::string);
  void printAllVersion();
  void cleanPath(std::string);
};

} // namespace facebook::fboss::platform::fw_util
