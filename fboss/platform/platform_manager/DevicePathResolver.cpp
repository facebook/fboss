// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/platform/platform_manager/DevicePathResolver.h"

#include <filesystem>
#include <stdexcept>

#include "fboss/platform/platform_manager/PciExplorer.h"
#include "fboss/platform/platform_manager/Utils.h"

namespace fs = std::filesystem;

namespace {
const re2::RE2 kHwmonRe{"hwmon\\d+"};
const re2::RE2 kIioDeviceRe{"iio:device\\d+"};
} // namespace

namespace facebook::fboss::platform::platform_manager {

DevicePathResolver::DevicePathResolver(const DataStore& dataStore)
    : dataStore_(dataStore) {}

std::string DevicePathResolver::resolveSensorPath(
    const std::string& devicePath) const {
  auto sensorPath = dataStore_.getSysfsPath(devicePath);
  if (fs::exists(fmt::format("{}/hwmon", sensorPath))) {
    sensorPath = fmt::format("{}/hwmon", sensorPath);
  }
  for (const auto& dirEntry : fs::directory_iterator(sensorPath)) {
    auto dirName = dirEntry.path().filename();
    if (re2::RE2::FullMatch(dirName.string(), kHwmonRe) ||
        re2::RE2::FullMatch(dirName.string(), kIioDeviceRe)) {
      return dirEntry.path();
    }
  }
  throw std::runtime_error(fmt::format(
      "Couldn't find hwmon[num] nor iio:device[num] folder under {} for {}",
      sensorPath,
      devicePath));
}

std::string DevicePathResolver::resolveEepromPath(
    const std::string& devicePath) {
  auto i2cDevicPath = dataStore_.getSysfsPath(devicePath);
  auto eepromPath = i2cDevicPath + "/eeprom";
  if (!fs::exists(eepromPath)) {
    throw std::runtime_error(fmt::format(
        "Fail to resolve EepromPath for {} - {} doesn't exist",
        devicePath,
        eepromPath));
  }
  return eepromPath;
}

std::string DevicePathResolver::resolveI2cBusPath(
    const std::string& devicePath) {
  const auto [slotPath, deviceName] = Utils().parseDevicePath(devicePath);
  return fmt::format(
      "/dev/i2c-{}", dataStore_.getI2cBusNum(slotPath, deviceName));
}

std::string DevicePathResolver::resolvePciSubDevSysfsPath(
    const std::string& devicePath) {
  auto sysfsPath = dataStore_.getSysfsPath(devicePath);
  if (!fs::exists(sysfsPath)) {
    throw std::runtime_error(fmt::format(
        "Fail to resolve PciSubDevice's SysfsPath for {} - {} no longer exists",
        devicePath,
        sysfsPath));
  }
  return sysfsPath;
}

std::string DevicePathResolver::resolvePciSubDevCharDevPath(
    const std::string& devicePath) const {
  auto charDevPath = dataStore_.getCharDevPath(devicePath);
  if (!fs::exists(charDevPath)) {
    throw std::runtime_error(fmt::format(
        "Fail to resolve PciSubDevice's CharDevPath for {} - {} no longer exists",
        devicePath,
        charDevPath));
  }
  return charDevPath;
}

std::optional<std::string> DevicePathResolver::tryResolveI2cDevicePath(
    const std::string& devicePath) {
  try {
    return dataStore_.getSysfsPath(devicePath);
  } catch (const std::exception& ex) {
    XLOG(ERR) << fmt::format(
        "Failed to resolve I2cDevicePath from {}. Reason: {}",
        devicePath,
        ex.what());
  }
  return std::nullopt;
}

std::string DevicePathResolver::resolvePciDevicePath(
    const std::string& devicePath) {
  // Check if sysfs path is stored in DataStore (e.g info-rom)
  // Otherwise, try to construct PciDevice sysfs path via config.
  // TODO: rely on dataStore_ as part of efforts in D52785459
  if (dataStore_.hasSysfsPath(devicePath)) {
    return dataStore_.getSysfsPath(devicePath);
  }
  const auto [slotPath, deviceName] = Utils().parseDevicePath(devicePath);
  auto pciDeviceConfig = getPciDeviceConfig(slotPath, deviceName);
  auto pciDevice = PciDevice(
      *pciDeviceConfig.pmUnitScopedName(),
      *pciDeviceConfig.vendorId(),
      *pciDeviceConfig.deviceId(),
      *pciDeviceConfig.subSystemVendorId(),
      *pciDeviceConfig.subSystemDeviceId());
  if (!fs::exists(pciDevice.sysfsPath())) {
    throw std::runtime_error(
        fmt::format("{} is not plugged-in to the platform", deviceName));
  }
  return pciDevice.sysfsPath();
}

std::optional<std::string> DevicePathResolver::resolvePresencePath(
    const std::string& devicePath,
    const std::string& presenceFileName) const {
  try {
    XLOG(INFO) << "Resolving PresencePath with SensorPath";
    auto sensorPath = fs::path(resolveSensorPath(devicePath));
    return sensorPath / presenceFileName;
  } catch (const std::exception& e) {
    XLOG(ERR) << fmt::format(
        "Failed to resolve PresencePath with SensorPath. Reason: {}", e.what());
  }
  XLOG(INFO) << "Resolving PresencePath with PciDevicePath";
  const auto [slotPath, deviceName] = Utils().parseDevicePath(devicePath);
  if (!dataStore_.hasPmUnit(slotPath)) {
    XLOG(ERR) << fmt::format(
        "Failed to resolve PresencePath with PciDevicePath. "
        "Reason: No PmUnit exists at {}",
        slotPath);
    return std::nullopt;
  }
  auto pmUnitConfig = dataStore_.resolvePmUnitConfig(slotPath);
  auto pciDeviceConfig = std::find_if(
      pmUnitConfig.pciDeviceConfigs()->begin(),
      pmUnitConfig.pciDeviceConfigs()->end(),
      [deviceNameCopy = deviceName](auto pciDeviceConfig) {
        return *pciDeviceConfig.pmUnitScopedName() == deviceNameCopy;
      });
  if (pciDeviceConfig != pmUnitConfig.pciDeviceConfigs()->end()) {
    auto pciDevice = PciDevice(
        *pciDeviceConfig->pmUnitScopedName(),
        *pciDeviceConfig->vendorId(),
        *pciDeviceConfig->deviceId(),
        *pciDeviceConfig->subSystemVendorId(),
        *pciDeviceConfig->subSystemDeviceId());
    auto targetPath = std::filesystem::path(pciDevice.sysfsPath());
    return targetPath / presenceFileName;
  }
  return std::nullopt;
}

PciDeviceConfig DevicePathResolver::getPciDeviceConfig(
    const std::string& slotPath,
    const std::string& deviceName) {
  auto pmUnitConfig = dataStore_.resolvePmUnitConfig(slotPath);
  auto pciDeviceConfig = std::find_if(
      pmUnitConfig.pciDeviceConfigs()->begin(),
      pmUnitConfig.pciDeviceConfigs()->end(),
      [&](auto pciDeviceConfig) {
        return *pciDeviceConfig.pmUnitScopedName() == deviceName;
      });
  if (pciDeviceConfig == pmUnitConfig.pciDeviceConfigs()->end()) {
    throw std::runtime_error(fmt::format(
        "Couldn't find PciDeviceConfig for {} at {}", deviceName, slotPath));
  }
  return *pciDeviceConfig;
}
} // namespace facebook::fboss::platform::platform_manager
