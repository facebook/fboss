// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/platform/platform_manager/DevicePathResolver.h"

#include <filesystem>
#include <stdexcept>

#include "fboss/platform/platform_manager/PciExplorer.h"
#include "fboss/platform/platform_manager/Utils.h"

using namespace facebook::fboss::platform::platform_manager;
namespace fs = std::filesystem;

namespace {
constexpr auto kIdprom = "IDPROM";
const re2::RE2 kHwmonRe{"hwmon\\d+"};
const re2::RE2 kIioDeviceRe{"iio:device\\d+"};
} // namespace

DevicePathResolver::DevicePathResolver(
    const PlatformConfig& config,
    const DataStore& dataStore,
    const I2cExplorer& i2cExplorer)
    : platformConfig_(config),
      dataStore_(dataStore),
      i2cExplorer_(i2cExplorer) {}

std::string DevicePathResolver::resolveSensorPath(
    const std::string& devicePath) {
  auto i2cDevicePath = resolveI2cDevicePath(devicePath);
  if (fs::exists(fmt::format("{}/hwmon", i2cDevicePath))) {
    i2cDevicePath = fmt::format("{}/hwmon", i2cDevicePath);
  }
  std::string sensorPath;
  for (const auto& dirEntry : fs::directory_iterator(i2cDevicePath)) {
    auto dirName = dirEntry.path().filename();
    if (re2::RE2::FullMatch(dirName.string(), kHwmonRe) ||
        re2::RE2::FullMatch(dirName.string(), kIioDeviceRe)) {
      sensorPath = dirEntry.path();
      break;
    }
  }
  if (sensorPath.empty()) {
    throw std::runtime_error(fmt::format(
        "Couldn't find hwmon[num] nor iio:device[num] folder under {} for {}",
        i2cDevicePath,
        devicePath));
  }
  return sensorPath;
}

std::string DevicePathResolver::resolveEepromPath(
    const std::string& devicePath) {
  auto i2cDevicPath = resolveI2cDevicePath(devicePath);
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

std::string DevicePathResolver::resolveI2cDevicePath(
    const std::string& devicePath) {
  const auto [slotPath, deviceName] = Utils().parseDevicePath(devicePath);
  uint16_t busNum;
  std::optional<I2cAddr> i2cAddr;
  if (deviceName == kIdprom) {
    auto idpromConfig = getIdpromConfig(slotPath);
    busNum = dataStore_.getI2cBusNum(slotPath, *idpromConfig.busName());
    i2cAddr = I2cAddr(*idpromConfig.address());
  } else {
    auto i2cDeviceConfig = getI2cDeviceConfig(slotPath, deviceName);
    busNum = dataStore_.getI2cBusNum(slotPath, *i2cDeviceConfig.busName());
    i2cAddr = I2cAddr(*i2cDeviceConfig.address());
  }
  if (!i2cExplorer_.isI2cDevicePresent(busNum, *i2cAddr)) {
    throw std::runtime_error(
        fmt::format("{} is not plugged-in to the platform", deviceName));
  }
  return i2cExplorer_.getDeviceI2cPath(busNum, *i2cAddr);
}

std::optional<std::string> DevicePathResolver::tryResolveI2cDevicePath(
    const std::string& devicePath) {
  try {
    return resolveI2cDevicePath(devicePath);
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
    const std::string& presenceFileName) {
  const auto [slotPath, deviceName] = Utils().parseDevicePath(devicePath);
  if (!dataStore_.hasPmUnit(slotPath)) {
    XLOG(ERR) << fmt::format("No PmUnit exists at {}", slotPath);
    return std::nullopt;
  }
  auto pmUnitName = dataStore_.getPmUnitName(slotPath);
  auto pmUnitConfig = platformConfig_.pmUnitConfigs()->at(pmUnitName);
  auto i2cDeviceConfig = std::find_if(
      pmUnitConfig.i2cDeviceConfigs()->begin(),
      pmUnitConfig.i2cDeviceConfigs()->end(),
      [deviceNameCopy = deviceName](auto i2cDeviceConfig) {
        return *i2cDeviceConfig.pmUnitScopedName() == deviceNameCopy;
      });
  if (i2cDeviceConfig != pmUnitConfig.i2cDeviceConfigs()->end()) {
    auto busNum =
        dataStore_.getI2cBusNum(slotPath, *i2cDeviceConfig->busName());
    auto i2cAddr = I2cAddr(*i2cDeviceConfig->address());
    if (!i2cExplorer_.isI2cDevicePresent(busNum, i2cAddr)) {
      XLOG(ERR) << fmt::format(
          "{} is not plugged-in to the platform", deviceName);
      return std::nullopt;
    }
    auto targetPath =
        std::filesystem::path(i2cExplorer_.getDeviceI2cPath(busNum, i2cAddr)) /
        "hwmon";
    std::string hwmonSubDir = "";
    for (const auto& dirEntry :
         std::filesystem::directory_iterator(targetPath)) {
      auto dirName = dirEntry.path().filename();
      if (re2::RE2::FullMatch(dirName.string(), kHwmonRe)) {
        hwmonSubDir = dirName.string();
        break;
      }
    }
    if (hwmonSubDir.empty()) {
      XLOG(ERR) << fmt::format(
          "Couldn't find hwmon[num] folder within ({})", targetPath.string());
      return std::nullopt;
    }
    return targetPath / hwmonSubDir / presenceFileName;
  }
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

I2cDeviceConfig DevicePathResolver::getI2cDeviceConfig(
    const std::string& slotPath,
    const std::string& deviceName) {
  auto pmUnitName = dataStore_.getPmUnitName(slotPath);
  auto pmUnitConfig = platformConfig_.pmUnitConfigs()->at(pmUnitName);
  auto i2cDeviceConfig = std::find_if(
      pmUnitConfig.i2cDeviceConfigs()->begin(),
      pmUnitConfig.i2cDeviceConfigs()->end(),
      [&](auto i2cDeviceConfig) {
        return *i2cDeviceConfig.pmUnitScopedName() == deviceName;
      });
  if (i2cDeviceConfig == pmUnitConfig.i2cDeviceConfigs()->end()) {
    throw std::runtime_error(fmt::format(
        "Couldn't find i2c device config for {} at {}", deviceName, slotPath));
  }
  return *i2cDeviceConfig;
}

IdpromConfig DevicePathResolver::getIdpromConfig(const std::string& slotPath) {
  auto pmUnitName = dataStore_.getPmUnitName(slotPath);
  auto pmUnitConfig = platformConfig_.pmUnitConfigs()->at(pmUnitName);
  auto idpromConfig = platformConfig_.slotTypeConfigs()
                          ->at(*pmUnitConfig.pluggedInSlotType())
                          .idpromConfig();
  if (!idpromConfig) {
    throw std::runtime_error(
        fmt::format("Couldn't find idprom config at {}", slotPath));
  }
  return *idpromConfig;
}

PciDeviceConfig DevicePathResolver::getPciDeviceConfig(
    const std::string& slotPath,
    const std::string& deviceName) {
  auto pmUnitName = dataStore_.getPmUnitName(slotPath);
  auto pmUnitConfig = platformConfig_.pmUnitConfigs()->at(pmUnitName);
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
