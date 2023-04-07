// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include "fboss/platform/fw_util/minipack3FwUtil/FirmwareUpgradeMinipack3.h"
#include "fboss/platform/fw_util/minipack3FwUtil/UpgradeBinaryMinipack3.h"
#include "fboss/platform/fw_util/firmware_helpers/FirmwareUpgradeHelper.h"
#include "fboss/platform/helpers/Utils.h"

namespace facebook::fboss::platform::fw_util {

FirmwareUpgradeMinipack3::FirmwareUpgradeMinipack3() {
  int exitStatus = 0;
  std::string cmdOutput =
      helpers::execCommandUnchecked(flashromStrCmd_, exitStatus);
  // Command exitStatus is expected to be 1 since command is not complete, so
  // skipping checking for exitStatus. Purpose is to look for the chip name.
  if (cmdOutput.find("MT25QL128") != std::string::npos) {
    chip_ = "-c MT25QL128";
  } else if (cmdOutput.find("MT25QU256") != std::string::npos) {
    chip_ = "-c MT25QU256";
  } else if (cmdOutput.find("W25X20") != std::string::npos) {
    chip_ = "-c W25X20";
  } else if (cmdOutput.find("W25Q32.V") != std::string::npos) {
    chip_ = "-c W25Q32.V";
  }
  helpers::execCommand(createLayoutCmd_);
}

void FirmwareUpgradeMinipack3::upgradeFirmware(
    int argc,
    char* argv[],
    std::string upgradable_components) {
  auto upgradeBinaryObj = UpgradeBinaryMinipack3();
  upgradeBinaryObj.parseCommandLine(argc, argv, upgradable_components);
}

} // namespace facebook::fboss::platform::fw_util
