// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <map>
#include <optional>
#include <string>

namespace facebook::fboss::platform::platform_manager {
class DataStore {
 public:
  DataStore() = default;
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

  // Get Gpio chip number of the given chip name at the given SlotPath
  uint16_t getGpioChipNum(
      const std::string& slotPath,
      const std::string& gpioChipDeviceName) const;

  // Update Gpio chip number of the given chip name at the given SlotPath
  void updateGpioChipNum(
      const std::string& slotPath,
      const std::string& gpioChipDeviceName,
      uint16_t gpioChipNum);

  // Get PmUnitName at the given SlotPath
  std::string getPmUnitName(const std::string& slotPath) const;

  // Update PmUnitName at the given SlotPath
  void updatePmUnitName(
      const std::string& slotPath,
      const std::string& pmUnitName);

  // Checks if a PmUnit exists at the given SlotPath
  bool hasPmUnit(const std::string& slotPath) const;

  // Get SysfsPath for a given PciSubDevicePath
  std::string getSysfsPath(const std::string& devicePath) const;

  // Update SysfsPath for a given PciSubDevicePath
  void updateSysfsPath(
      const std::string& devicePath,
      const std::string& sysfsPath);

  bool hasSysfsPath(const std::string& devicePath) const;

  // Get InstanceId for a given PciSubDevicePath
  uint32_t getInstanceId(const std::string& devicePath);

  // Update InstanceId for a given PciSubDevicePath
  void updateInstanceId(const std::string& devicePath, uint32_t instanceId);

  // Get CharDevPath for a given PciSubDevicePath
  std::string getCharDevPath(const std::string& devicePath) const;

  // Update CharDevPath for a given PciSubDevicePath
  void updateCharDevPath(
      const std::string& devicePath,
      const std::string& charDevPath);

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

  // Map from <SlotPath, GpioChipDeviceName> to gpio chip number.
  std::map<std::pair<std::string, std::string>, uint16_t> gpioChipNums_{};

  // Stores the PmUnitName which has been discovered at each SlotPath.
  std::map<std::string, std::string> slotPathToPmUnitName_{};

  // Map from PciSubDevicePath to SysfsPath.
  std::map<std::string, std::string> pciSubDevicePathToSysfsPath_{};

  // Map from PciSubDevicePath to instanceId.
  std::map<std::string, uint32_t> pciSubDevicePathToInstanceId_{};

  // Map from PciSubDevicePath to CharDevPath
  std::map<std::string, std::string> pciSubDevicePathToCharDevPath_{};
};
} // namespace facebook::fboss::platform::platform_manager
