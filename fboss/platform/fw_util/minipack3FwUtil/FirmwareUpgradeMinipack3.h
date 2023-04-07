// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.
#pragma once

#include <string>

#include "fboss/platform/fw_util/FirmwareUpgradeInterface.h"

namespace facebook::fboss::platform::fw_util {
class FirmwareUpgradeMinipack3 : public FirmwareUpgradeInterface {
 public:
  FirmwareUpgradeMinipack3();
  virtual void upgradeFirmware(int, char**, std::string) override;
  virtual ~FirmwareUpgradeMinipack3() override = default;
};
} // namespace facebook::fboss::platform::fw_util
