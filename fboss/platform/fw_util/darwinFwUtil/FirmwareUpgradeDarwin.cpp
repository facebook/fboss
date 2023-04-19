// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include "fboss/platform/fw_util/darwinFwUtil/FirmwareUpgradeDarwin.h"
#include "fboss/platform/fw_util/darwinFwUtil/UpgradeBinaryDarwin.h"
#include "fboss/platform/fw_util/firmware_helpers/FirmwareUpgradeHelper.h"
#include "fboss/platform/fw_util/firmware_helpers/Utils.h"

namespace facebook::fboss::platform::fw_util {

FirmwareUpgradeDarwin::FirmwareUpgradeDarwin() {
  int exitStatus = 0;
  std::string cmdOutput = execCommandUnchecked(flashromStrCmd_, exitStatus);
  // Command exitStatus is expected to be 1 since command is not complete, so
  // skipping checking for exitStatus. Purpose is to look for the chip name.
  if (cmdOutput.find("MX25L12805D") != std::string::npos) {
    chip_ = "-c MX25L12805D";
  } else if (cmdOutput.find("N25Q128..3E") != std::string::npos) {
    chip_ = "-c N25Q128..3E";
  }
  execCommand(createLayoutCmd_);
}

void FirmwareUpgradeDarwin::upgradeFirmware(
    int argc,
    char* argv[],
    std::string upgradable_components) {
  auto upgradeBinaryObj = UpgradeBinaryDarwin();
  upgradeBinaryObj.parseCommandLine(argc, argv, upgradable_components);
}

} // namespace facebook::fboss::platform::fw_util
