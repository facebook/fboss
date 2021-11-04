// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include "fboss/platform/fw_util/FirmwareUpgradeDarwin.h"
#include "fboss/platform/fw_util/upgradeBinary/upgradeBinaryDarwin.h"
#include "fboss/platform/helpers/FirmwareUpgradeHelper.h"
#include "fboss/platform/helpers/Utils.h"

using namespace facebook::fboss::platform::helpers;

namespace facebook::fboss::platform {

FirmwareUpgradeDarwin::FirmwareUpgradeDarwin() {
  int exitStatus = 0;
  std::string cmdOutput = execCommandUnchecked(flashromStrCmd, exitStatus);
  /*cmd exit status is expected to be 1 since cmd
   is not complete, so skipping checking for out
   value. Purpose is to look for the chip name.
  */
  if (cmdOutput.find("MX25L12805D") != std::string::npos) {
    chip = "-c MX25L12805D";
  } else if (cmdOutput.find("N25Q128..3E") != std::string::npos) {
    chip = "-c N25Q128..3E";
  }

  execCommand(createLayoutCmd);
}

void FirmwareUpgradeDarwin::upgradeFirmware(int argc, char* argv[]) {
  auto upgradeBinaryObj = UpgradeBinaryDarwin();
  upgradeBinaryObj.parseCommandLine(argc, argv);
}

} // namespace facebook::fboss::platform
