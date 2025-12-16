// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <map>
#include <optional>
#include <string>

#include "fboss/platform/platform_manager/gen-cpp2/platform_manager_config_types.h"
#include "fboss/platform/platform_manager/gen-cpp2/platform_manager_presence_types.h"
#include "fboss/platform/weutil/FbossEepromInterface.h"

namespace facebook::fboss::platform::platform_manager {
class DataStore {
 public:
  explicit DataStore(const PlatformConfig& config);
  virtual ~DataStore() = default;

  // Get the kernel assigned I2C bus number for the given busName.
  // If the busName could be found in global scope (e.g., bus from CPU),
  // then return it directly. Otherwise, check if there is a mapping in the
  // provided slotPath.
  uint16_t getI2cBusNum(
      const std::optional<std::string>& slotPath,
      const std::string& pmUnitScopeBusName) const;

  // Update the kernel assigned I2C bus number for the given busName. If the
  // bus name is of global scope, then SlotPath is empty.
  void updateI2cBusNum(
      const std::optional<std::string>& slotPath,
      const std::string& pmUnitScopeBusName,
      uint16_t busNum);

  // Get PmUnitInfo at the given SlotPath
  PmUnitInfo getPmUnitInfo(const std::string& slotPath) const;

  // Checks if a PmUnit exists at the given SlotPath
  bool hasPmUnit(const std::string& slotPath) const;

  // Get SysfsPath for a given PciSubDevicePath
  std::string getSysfsPath(const std::string& devicePath) const;

  // Update SysfsPath for a given PciSubDevicePath
  void updateSysfsPath(
      const std::string& devicePath,
      const std::string& sysfsPath);

  bool hasSysfsPath(const std::string& devicePath) const;

  // Update SysfsPath for a given PciSubDevicePath
  void updateSysfsPath(
      const std::string& slotPath,
      const std::string& pmUnitScopedName,
      const std::string& sysfsPath);

  // Get CharDevPath for a given PciSubDevicePath
  std::string getCharDevPath(const std::string& devicePath) const;

  // Update CharDevPath for a given PciSubDevicePath
  void updateCharDevPath(
      const std::string& devicePath,
      const std::string& charDevPath);

  // Update PmUnitName for a given slotPath.
  void updatePmUnitName(const std::string& slotPath, const std::string& name);

  // Update PmUnitVersion for a given slotPath.
  void updatePmUnitVersion(
      const std::string& slotPath,
      const PmUnitVersion& version);

  void updatePmUnitSuccessfullyExplored(
      const std::string& slotPath,
      bool successfullyExplored);

  void updatePmUnitPresenceInfo(
      const std::string& slotPath,
      const PresenceInfo& presenceInfo);

  // Resolve PmUnitConfig based on the platformSubVersion from eeprom.
  // Throws if none of the VersionedPmUnitConfig matches the version.
  PmUnitConfig resolvePmUnitConfig(const std::string& slotPath) const;

  // Store eeprom contents at the given DevicePath.
  void updateEepromContents(
      const std::string& devicePath,
      const FbossEepromInterface& contents);

  // Get eeprom contents at a given SlotPath.
  FbossEepromInterface getEepromContents(const std::string& devicePath) const;

  // Returns true if eeprom contents have been stored at the given DevicePath.
  bool hasEepromContents(const std::string& devicePath) const;

  std::map<std::string, PmUnitInfo> getSlotPathToPmUnitInfo() const;

  // Store firmware version for the given device name.
  void updateFirmwareVersion(
      const std::string& deviceName,
      const std::string& firmwareVersion);

  // Get all firmware versions as a map of device name to firmware version.
  std::unordered_map<std::string, std::string> getFirmwareVersions() const;

  // Store hardware version field.
  void updateHardwareVersion(
      const std::string& fieldName,
      const std::string& hardwareVersion);

  // Get all hardware versions as a map of field name to hardware version.
  std::unordered_map<std::string, std::string> getHardwareVersions() const;

 private:
  // Map from <pmUnitPath, pmUnitScopeBusName> to kernel i2c bus name.
  // - The pmUnitPath to the rootPmUnit is /. So a bus at root PmUnit will
  // have the entry <"/", "MuxA@1"> -> i2c-54.
  // - The CPU buses are not pinned to any PmUnit, so they are stored as
  // entry <std::nullopt, "SMBus Adapter 1654"> -> i2c-7.
  // - An INCOMING @1 bus at pmUnitPath /MCB_SLOT@0/PIM_SLOT@1 will have the
  // entry <"/MCB_SLOT@0/PIM_SLOT@1", "INCOMING@1"> -> i2c-52
  std::map<std::pair<std::optional<std::string>, std::string>, uint16_t>
      i2cBusNums_{};

  // Map from PciSubDevicePath to SysfsPath.
  std::map<std::string, std::string> pciSubDevicePathToSysfsPath_{};

  // Map from PciSubDevicePath to CharDevPath
  std::map<std::string, std::string> pciSubDevicePathToCharDevPath_{};

  // Map from SlotPath to its PmUnitInfo
  std::map<std::string, PmUnitInfo> slotPathToPmUnitInfo{};

  // Map from DevicePath to its EEPROM (IDPROM) contents.
  std::unordered_map<std::string, FbossEepromInterface> eepromContents_{};

  // Map from device name to its firmware version.
  std::unordered_map<std::string, std::string> firmwareVersions_{};

  // Map from field name to its hardware version.
  std::unordered_map<std::string, std::string> hardwareVersions_{};

  const PlatformConfig& platformConfig_;
};
} // namespace facebook::fboss::platform::platform_manager
