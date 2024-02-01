// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/platform/platform_manager/DataStore.h"

#include <fmt/format.h>
#include <folly/logging/xlog.h>

namespace facebook::fboss::platform::platform_manager {
uint16_t DataStore::getI2cBusNum(
    const std::optional<std::string>& slotPath,
    const std::string& pmUnitScopeBusName) const {
  auto it = i2cBusNums_.find(std::make_pair(std::nullopt, pmUnitScopeBusName));
  if (it != i2cBusNums_.end()) {
    return it->second;
  }
  it = i2cBusNums_.find(std::make_pair(slotPath, pmUnitScopeBusName));
  if (it != i2cBusNums_.end()) {
    return it->second;
  }
  throw std::runtime_error(fmt::format(
      "Could not find bus number for {} at {}",
      pmUnitScopeBusName,
      slotPath ? *slotPath : "CPU"));
}

void DataStore::updateI2cBusNum(
    const std::optional<std::string>& slotPath,
    const std::string& pmUnitScopeBusName,
    uint16_t busNum) {
  XLOG(INFO) << fmt::format(
      "Updating bus {} in {} to bus number {} (i2c-{})",
      pmUnitScopeBusName,
      slotPath ? fmt::format("SlotPath {}", *slotPath) : "Global Scope",
      busNum,
      busNum);
  i2cBusNums_[std::make_pair(slotPath, pmUnitScopeBusName)] = busNum;
}

uint16_t DataStore::getGpioChipNum(
    const std::string& slotPath,
    const std::string& gpioChipDeviceName) const {
  auto it = gpioChipNums_.find(std::make_pair(slotPath, gpioChipDeviceName));
  if (it != gpioChipNums_.end()) {
    return it->second;
  }
  throw std::runtime_error(fmt::format(
      "Could not find gpio chip number for {} at {}",
      gpioChipDeviceName,
      slotPath));
}

void DataStore::updateGpioChipNum(
    const std::string& slotPath,
    const std::string& gpioChipDeviceName,
    uint16_t gpioChipNum) {
  XLOG(INFO) << fmt::format(
      "Updating gpio chip {} in {} to gpio chip number {} (gpiochip{})",
      gpioChipDeviceName,
      slotPath,
      gpioChipNum,
      gpioChipNum);
  gpioChipNums_[std::make_pair(slotPath, gpioChipDeviceName)] = gpioChipNum;
}

std::string DataStore::getPmUnitName(const std::string& slotPath) const {
  if (slotPathToPmUnitName_.find(slotPath) != slotPathToPmUnitName_.end()) {
    return slotPathToPmUnitName_.at(slotPath);
  }
  throw std::runtime_error(
      fmt::format("Could not find PmUnit at {}", slotPath));
}

void DataStore::updatePmUnitName(
    const std::string& slotPath,
    const std::string& pmUnitName) {
  XLOG(INFO) << fmt::format(
      "Updating SlotPath {} to have PmUnit {}", slotPath, pmUnitName);
  slotPathToPmUnitName_[slotPath] = pmUnitName;
}

bool DataStore::hasPmUnit(const std::string& slotPath) const {
  return slotPathToPmUnitName_.find(slotPath) != slotPathToPmUnitName_.end();
}

std::string DataStore::getSysfsPath(const std::string& devicePath) {
  auto itr = pciSubDevicePathToSysfsPath_.find(devicePath);
  if (itr != pciSubDevicePathToSysfsPath_.end()) {
    return itr->second;
  }
  throw std::runtime_error(
      fmt::format("Could not find SysfsPath for {}", devicePath));
}

void DataStore::updateSysfsPath(
    const std::string& devicePath,
    const std::string& sysfsPath) {
  XLOG(INFO) << fmt::format(
      "Updating SysfsPath for {} to {}", devicePath, sysfsPath);
  pciSubDevicePathToSysfsPath_[devicePath] = sysfsPath;
}

bool DataStore::hasSysfsPath(const std::string& devicePath) const {
  return pciSubDevicePathToSysfsPath_.find(devicePath) !=
      pciSubDevicePathToSysfsPath_.end();
}

uint32_t DataStore::getInstanceId(const std::string& devicePath) {
  auto itr = pciSubDevicePathToInstanceId_.find(devicePath);
  if (itr != pciSubDevicePathToInstanceId_.end()) {
    return itr->second;
  }
  throw std::runtime_error(
      fmt::format("Could not find instanceId for {}", devicePath));
}

void DataStore::updateInstanceId(
    const std::string& devicePath,
    uint32_t instanceId) {
  XLOG(INFO) << fmt::format(
      "Updating instanceId for {} to {}", devicePath, instanceId);
  pciSubDevicePathToInstanceId_[devicePath] = instanceId;
}

std::string DataStore::getCharDevPath(const std::string& devicePath) const {
  auto itr = pciSubDevicePathToCharDevPath_.find(devicePath);
  if (itr != pciSubDevicePathToCharDevPath_.end()) {
    return itr->second;
  }
  throw std::runtime_error(
      fmt::format("Could not find CharDevPath for {}", devicePath));
}

void DataStore::updateCharDevPath(
    const std::string& devicePath,
    const std::string& charDevPath) {
  XLOG(INFO) << fmt::format(
      "Updating CharDevPath for {} to {}", devicePath, charDevPath);
  pciSubDevicePathToCharDevPath_[devicePath] = charDevPath;
}

} // namespace facebook::fboss::platform::platform_manager
