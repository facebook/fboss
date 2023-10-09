// Copyright (c) 2004-present, Meta Platforms, Inc. and affiliates.
// All Rights Reserved.

#pragma once

#include <folly/experimental/FunctionScheduler.h>

#include "fboss/platform/platform_manager/PciExplorer.h"
#include "fboss/platform/platform_manager/PlatformI2cExplorer.h"
#include "fboss/platform/platform_manager/PresenceDetector.h"
#include "fboss/platform/platform_manager/gen-cpp2/platform_manager_config_types.h"

namespace facebook::fboss::platform::platform_manager {
class PlatformExplorer {
 public:
  explicit PlatformExplorer(
      std::chrono::seconds exploreInterval,
      const PlatformConfig& config);

  // Explore the platform.
  void explore();

  // Explore the PmUnit present at the given slotPath.
  void explorePmUnit(
      const std::string& slotPath,
      const std::string& pmUnitName);

  // Explore the slotName which is located at a PmUnit in the given
  // parentSlotPath.
  void exploreSlot(
      const std::string& parentSlotPath,
      const std::string& slotName,
      const SlotConfig& slotConfig);

  // Get the PmUnit name which has been plugged in at the given slotPath,
  // and the slot is of given slotType.
  std::optional<std::string> getPmUnitNameFromSlot(
      const std::string& slotType,
      const std::string& slotPath);

  // Explore the I2C devices in the PmUnit at the given SlotPath.
  void exploreI2cDevices(
      const std::string& slotPath,
      const std::vector<I2cDeviceConfig>& i2cDeviceConfigs);

  // Explore the PCI devices in the PmUnit at the given SlotPath.
  void explorePciDevices(
      const std::string& slotPath,
      const std::vector<PciDeviceConfig>& pciDeviceConfigs);

  // Get the kernel assigned I2C bus number for the given busName.
  // If the busName could be found in global scope (e.g., bus from CPU),
  // then return it directly. Otherwise, check if there is a mapping in the
  // provided slotPath.
  uint16_t getI2cBusNum(
      const std::optional<std::string>& slotPath,
      const std::string& pmUnitScopeBusName) const;

  // Update the kernel assigned I2C bus number for the given busName. If the
  // bus name is of global scope, then slotPath is empty.
  void updateI2cBusNum(
      const std::optional<std::string>& slotPath,
      const std::string& pmUnitScopeBusName,
      uint16_t busNum);

 private:
  folly::FunctionScheduler scheduler_;
  PlatformConfig platformConfig_{};
  PlatformI2cExplorer i2cExplorer_{};
  PciExplorer pciExplorer_{};
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
