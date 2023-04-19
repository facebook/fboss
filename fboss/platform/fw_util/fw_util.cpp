//  Copyright 2021-present Facebook. All Rights Reserved.

#include <string.h>
#include <sysexits.h>
#include <iostream>
#include <memory>

#include <folly/init/Init.h>
#include <glog/logging.h>

#include "fboss/platform/fw_util/FirmwareUpgradeInterface.h"
#include "fboss/platform/fw_util/darwinFwUtil/FirmwareUpgradeDarwin.h"
#include "fboss/platform/fw_util/firmware_helpers/Utils.h"

using namespace facebook::fboss::platform::fw_util;

std::unique_ptr<FirmwareUpgradeInterface> get_plat_type(
    std::string& upgradable_components) {
  // TODO: will check platform information (maybe fboss whoami) to
  // to figure out which platform we are dealing with
  upgradable_components =
      "cpu_cpld, sc_scd, sc_cpld, sc_sat_cpld0, sc_sat_cpld1, fan_cpld, bios";
  return std::make_unique<FirmwareUpgradeDarwin>();

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
    showDeviceInfo();
  }
  return EX_OK;
}
