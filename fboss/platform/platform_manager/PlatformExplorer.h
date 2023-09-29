// Copyright (c) 2004-present, Meta Platforms, Inc. and affiliates.
// All Rights Reserved.

#pragma once

#include <folly/experimental/FunctionScheduler.h>

#include "fboss/platform/platform_manager/PlatformI2cExplorer.h"
#include "fboss/platform/platform_manager/PresenceDetector.h"
#include "fboss/platform/platform_manager/gen-cpp2/platform_manager_config_types.h"

namespace facebook::fboss::platform::platform_manager {
class PlatformExplorer {
 public:
  explicit PlatformExplorer(
      std::chrono::seconds exploreInterval,
      const PlatformConfig& config);
  void explore();
  void explorePmUnit(
      const std::string& pmUnitPath,
      const std::string& pmUnitName);
  void exploreSlot(
      const std::string& pmUnitPath,
      const std::string& slotName,
      const SlotConfig& slotConfig);
  std::optional<std::string> getPmUnitNameFromSlot(
      const std::string& slotType,
      const std::string& pmUnitPath);
  void exploreI2cDevices(
      const std::string& pmUnitPath,
      const std::vector<I2cDeviceConfig>& i2cDeviceConfigs);
  uint16_t getI2cBusNum(
      const std::optional<std::string>& pmUnitPath,
      const std::string& pmUnitScopeBusName) const;
  void updateI2cBusNum(
      const std::optional<std::string>& pmUnitPath,
      const std::string& pmUnitScopeBusName,
      uint16_t busNum);

 private:
  folly::FunctionScheduler scheduler_;
  PlatformConfig platformConfig_{};
  PlatformI2cExplorer i2cExplorer_{};
  PresenceDetector presenceDetector_{};
  // Map from <pmUnitPath, pmUnitScopeBusName> to kernel i2c bus name.
  // - The pmUnitPath to the rootPmUnit is /. So a bus at root PmUnit will have
  // the entry <"/", "MuxA@1"> -> i2c-54.
  // - The CPU buses are not pinned to any PmUnit, so they are stored as
  // entry <std::nullopt, "SMBus Adapter 1654"> -> i2c-7.
  // - An INCOMING @1 bus at pmUnitPath /MCB_SLOT@0/PIM_SLOT@1 will have the
  // entry <"/MCB_SLOT@0/PIM_SLOT@1", "INCOMING@1"> -> i2c-52
  std::map<std::pair<std::optional<std::string>, std::string>, uint16_t>
      i2cBusNums_{};
};

} // namespace facebook::fboss::platform::platform_manager
