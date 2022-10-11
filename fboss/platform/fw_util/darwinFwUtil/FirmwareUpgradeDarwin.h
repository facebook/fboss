// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.
#pragma once

#include <string>

#include "fboss/platform/fw_util/FirmwareUpgradeInterface.h"

namespace facebook::fboss::platform::fw_util {
class FirmwareUpgradeDarwin : public FirmwareUpgradeInterface {
 public:
  FirmwareUpgradeDarwin();
  virtual void upgradeFirmware(int, char**, std::string) override;
  virtual ~FirmwareUpgradeDarwin() override = default;

 protected:
  const std::string layoutPath_ = "/tmp/bios_spi_layout";
  const std::string layout_ =
      "echo \"A00000:FFFFFF normal\n"
      "400000:9EFFFF fallback\n"
      "9FA000:9FEFFF aboot_conf\n"
      "000000:FFFFFF total\n"
      "001000:01EFFF prefdl\"";
  const std::string flashromStrCmd_ = "flashrom -p internal 2>&1";
  const std::string createLayoutCmd_ = layout_ + " > " + layoutPath_;
  std::string chip_;
};
} // namespace facebook::fboss::platform::fw_util
