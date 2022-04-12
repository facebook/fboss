// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.
#pragma once

#include <string>

namespace facebook::fboss::platform::fw_util {
class FirmwareUpgradeInterface {
 public:
  FirmwareUpgradeInterface() {}
  virtual void upgradeFirmware(int, char**, std::string) = 0;
  virtual ~FirmwareUpgradeInterface() = default;
};
} // namespace facebook::fboss::platform::fw_util
