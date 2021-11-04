//  Copyright 2021-present Facebook. All Rights Reserved.

#include <folly/init/Init.h>
#include <glog/logging.h>
#include <string.h>
#include <sysexits.h>
#include <iostream>
#include <memory>
#include "fboss/platform/fw_util/FirmwareUpgradeDarwin.h"
#include "fboss/platform/fw_util/FirmwareUpgradeInterface.h"
#include "fboss/platform/helpers/Utils.h"

using namespace facebook::fboss::platform::helpers;
using namespace facebook::fboss::platform;

std::unique_ptr<FirmwareUpgradeInterface> get_plat_type(void) {
  // TODO: use new class to get platform information so that
  // we can pick the right firmware class accordingly
  return std::make_unique<FirmwareUpgradeDarwin>();
}
/*
 * This utility will perform firmware upgrade for
 * TOR BMC less platform. Firmware upgrade will
 * include cpld, fpga, and bios.
 */
int main(int argc, char* argv[]) {
  // TODO: Add file lock to prevent multiple instace of fw-util from running
  // simultaneously.
  std::unique_ptr<FirmwareUpgradeInterface> FirmwareUpgradeInstance =
      get_plat_type();
  if (FirmwareUpgradeInstance) {
    FirmwareUpgradeInstance->upgradeFirmware(argc, argv);
  } else {
    showDeviceInfo();
  }
  return EX_OK;
}
