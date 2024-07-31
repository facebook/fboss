// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/platform/platform_manager/DataStore.h"

#include <fmt/format.h>
#include <folly/logging/xlog.h>

namespace facebook::fboss::platform::platform_manager {

DataStore::DataStore(const PlatformConfig& config) : platformConfig_(config) {}

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

std::string DataStore::getSysfsPath(const std::string& devicePath) const {
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

void DataStore::updatePmUnitInfo(
    const std::string& slotPath,
    const std::string& pmUnitName,
    std::optional<int> productVersion) {
  XLOG(INFO) << fmt::format(
      "At SlotPath {}, updating {} to {}",
      slotPath,
      productVersion ? fmt::format("ProductVersion {}", *productVersion)
                     : "Unknown ProductVersion",
      pmUnitName);
  slotPathToPmUnitInfo[slotPath] = {pmUnitName, productVersion};
}

PmUnitConfig DataStore::resolvePmUnitConfig(const std::string& slotPath) const {
  if (slotPathToPmUnitInfo.find(slotPath) == slotPathToPmUnitInfo.end()) {
    throw std::runtime_error(
        fmt::format("Unable to resolve PmUnitInfo for {}", slotPath));
  }
  const auto [pmUnitName, productVersion] = slotPathToPmUnitInfo.at(slotPath);
  if (!productVersion) {
    XLOG(INFO) << fmt::format(
        "Resolved Unknown ProductVersion to the main PmUnitConfig for {} at {}. "
        "Slot probably doesn't have an IDPROM to read the ProductVersion",
        pmUnitName,
        slotPath);
    return platformConfig_.pmUnitConfigs()->at(pmUnitName);
  }
  if (platformConfig_.versionedPmUnitConfigs()->contains(pmUnitName)) {
    for (const auto& versionedPmUnitConfig :
         platformConfig_.versionedPmUnitConfigs()->at(pmUnitName)) {
      if (*versionedPmUnitConfig.platformVersion() == *productVersion) {
        XLOG(INFO) << fmt::format(
            "Resolved to a PmUnitConfig of version {} for {} at {}",
            *productVersion,
            pmUnitName,
            slotPath);
        return *versionedPmUnitConfig.pmUnitConfig();
      }
    }
  }
  XLOG(INFO) << fmt::format(
      "Resolved ProductVersion {} to the main PmUnitConfig for {} at {}",
      *productVersion,
      pmUnitName,
      slotPath);
  return platformConfig_.pmUnitConfigs()->at(pmUnitName);
}
} // namespace facebook::fboss::platform::platform_manager
