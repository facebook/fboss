//  Copyright 2021-present Facebook. All Rights Reserved.

#include <string.h>
#include <sysexits.h>
#include <iostream>
#include <memory>

#include <folly/init/Init.h>
#include <glog/logging.h>

#include "fboss/platform/fw_util/FirmwareUpgradeInterface.h"
#include "fboss/platform/fw_util/minipack3FwUtil/FirmwareUpgradeMinipack3.h"
#include "fboss/platform/helpers/Utils.h"

using namespace facebook::fboss::platform::fw_util;
using namespace facebook::fboss::platform;

std::unique_ptr<FirmwareUpgradeInterface> get_plat_type(
    std::string& upgradable_components) {
  // TODO: will check platform information (maybe fboss whoami) to
  // to figure out which platform we are dealing with
  upgradable_components =
      "iob_fpga, dom_fpga_1, dom_fpga_2, scm_cpld, mcb_cpld, smb_cpld, nic_i210, th5_bcm7800, bios";
  return std::make_unique<FirmwareUpgradeMinipack3>();

  // TODO: add else logic for other platforms
}
/*
 * This utility will perform firmware upgrade for
 * TOR BMC Lite platform. Firmware upgrade will
 * include cpld, fpga, and bios.
 */
int main(int argc, char* argv[]) {
  // TODO: Add file lock to prevent multiple instance of fw-util from running
  // simultaneously.
  std::string upgradable_components = "";
  std::unique_ptr<FirmwareUpgradeInterface> firmwareUpgradeInstance =
      get_plat_type(upgradable_components);
  if (firmwareUpgradeInstance) {
    firmwareUpgradeInstance->upgradeFirmware(argc, argv, upgradable_components);
  } else {
    helpers::showDeviceInfo();
  }
  return EX_OK;
}
