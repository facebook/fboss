// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/platform/platform_manager/DevicePathResolver.h"
#include <stdexcept>

#include "fboss/platform/platform_manager/PciExplorer.h"
#include "fboss/platform/platform_manager/Utils.h"

using namespace facebook::fboss::platform::platform_manager;
namespace fs = std::filesystem;

namespace {
constexpr auto kIdprom = "IDPROM";
const re2::RE2 kValidHwmonDirName{"hwmon[0-9]+"};
} // namespace

DevicePathResolver::DevicePathResolver(
    const PlatformConfig& config,
    const DataStore& dataStore,
    const I2cExplorer& i2cExplorer)
    : platformConfig_(config),
      dataStore_(dataStore),
      i2cExplorer_(i2cExplorer) {}

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
      if (re2::RE2::FullMatch(dirName.string(), kValidHwmonDirName)) {
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
