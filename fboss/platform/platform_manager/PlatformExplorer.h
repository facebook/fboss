// Copyright (c) 2004-present, Meta Platforms, Inc. and affiliates.
// All Rights Reserved.

#pragma once

#include <memory>
#include <string>

#include "fboss/platform/helpers/PlatformFsUtils.h"
#include "fboss/platform/platform_manager/DataStore.h"
#include "fboss/platform/platform_manager/DevicePathResolver.h"
#include "fboss/platform/platform_manager/ExplorationErrorMap.h"
#include "fboss/platform/platform_manager/I2cExplorer.h"
#include "fboss/platform/platform_manager/PciExplorer.h"
#include "fboss/platform/platform_manager/PresenceChecker.h"
#include "fboss/platform/platform_manager/gen-cpp2/platform_manager_config_types.h"
#include "fboss/platform/platform_manager/gen-cpp2/platform_manager_service_types.h"
#include "fboss/platform/weutil/CachedFbossEepromParser.h"

namespace facebook::fboss::platform::platform_manager {
class PlatformExplorer {
 public:
  // Regex patterns for matching fw_ver format.
  auto static constexpr kFwVerXYPatternStr = R"((\d{1,3})\.(\d{1,3}))";
  auto static constexpr kFwVerXYZPatternStr =
      R"((\d{1,3})\.(\d{1,3})\.(\d{1,3}))";

  auto static constexpr kFirmwareVersion = "{}.firmware_version";
  auto static constexpr kGroupedFirmwareVersion = "{}.firmware_version.{}";

  explicit PlatformExplorer(
      const PlatformConfig& config,
      const std::shared_ptr<PlatformFsUtils> platformFsUtils =
          std::make_shared<PlatformFsUtils>());

  virtual ~PlatformExplorer() = default;

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

  // Get the instance id base for the FPGA at the given slotPath and
  // PmUnitScopedName. The instance id base is unique for each fpga hardware
  // discovered in the platform.
  uint32_t getFpgaInstanceId(
      const std::string& slotPath,
      const std::string& pm);

  // Publish firmware versions read from /run/devmap files to ODS.
  void publishFirmwareVersions();

  // Get the last PlatformManagerStatus.
  PlatformManagerStatus getPMStatus() const;

  // Get the PmUnitInfo of the given SlotPath and PmUnitName.
  // throws if no PmUnit found at the SlotPath.
  PmUnitInfo getPmUnitInfo(const std::string& slotPath) const;

 protected:
  virtual void updatePmStatus(const PlatformManagerStatus& newStatus);
  // A thrift struct which contains the status of PM exploration.
  // This member is thread safe since callers could be on different threads
  // E.g thrift API call on `getLastPmStatus`.
  folly::Synchronized<PlatformManagerStatus> platformManagerStatus_;

 private:
  void createDeviceSymLink(
      const std::string& linkPath,
      const std::string& devicePath);
  ExplorationStatus concludeExploration();
  void reportExplorationSummary(ExplorationStatus finalStatus);
  void setupI2cDevice(
      const std::string& devicePath,
      uint16_t busNum,
      const I2cAddr& addr,
      const std::vector<I2cRegData>& initRegSettings);
  void createI2cDevice(
      const std::string& devicePath,
      const std::string& deviceName,
      uint16_t busNum,
      const I2cAddr& addr);

  PlatformConfig platformConfig_{};
  I2cExplorer i2cExplorer_{};
  PciExplorer pciExplorer_{};
  CachedFbossEepromParser eepromParser_{};
  DataStore dataStore_;
  DevicePathResolver devicePathResolver_;
  PresenceChecker presenceChecker_;
  ExplorationErrorMap explorationErrMap_;
  std::shared_ptr<PlatformFsUtils> platformFsUtils_;

  // Map from <pmUnitPath, pmUnitScopeBusName> to kernel i2c bus name.
  // - The pmUnitPath to the rootPmUnit is /. So a bus at root PmUnit will
  // have the entry <"/", "MuxA@1"> -> i2c-54.
  // - The CPU buses are not pinned to any PmUnit, so they are stored as
  // entry <std::nullopt, "SMBus Adapter 1654"> -> i2c-7.
  // - An INCOMING @1 bus at pmUnitPath /MCB_SLOT@0/PIM_SLOT@1 will have the
  // entry <"/MCB_SLOT@0/PIM_SLOT@1", "INCOMING@1"> -> i2c-52
  std::map<std::pair<std::optional<std::string>, std::string>, uint16_t>
      i2cBusNums_{};

  // Map from <slotPath, PmUnitScopedName> to instance ids for FPGAs.
  std::map<std::pair<std::string, std::string>, uint32_t> fpgaInstanceIds_{};

  // Map from <SlotPath, GpioChipDeviceName> to gpio chip number.
  std::map<std::pair<std::string, std::string>, uint16_t> gpioChipNums_{};
};

} // namespace facebook::fboss::platform::platform_manager
