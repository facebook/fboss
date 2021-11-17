// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.
#pragma once

#include <sstream>
#include "fboss/platform/fw_util/FirmwareUpgradeInterface.h"
// include for new class

namespace facebook::fboss::platform {
class FirmwareUpgradeDarwin : public FirmwareUpgradeInterface {
 public:
  FirmwareUpgradeDarwin();
  virtual void upgradeFirmware(int, char**) override;
  virtual ~FirmwareUpgradeDarwin() override = default;

 protected:
  const std::string layoutPath = "/etc/bios_spi_layout";
  const std::string layout =
      "echo \"A00000:FFFFFF normal\n"
      "400000:9EFFFF fallback\n"
      "9FA000:9FEFFF aboot_conf\n"
      "000000:FFFFFF total\n"
      "001000:01EFFF prefdl\"";
  const std::string flashromStrCmd = "flashrom -p internal 2>&1";
  const std::string createLayoutCmd = layout + " > " + layoutPath;
  std::string chip;
};
} // namespace facebook::fboss::platform
