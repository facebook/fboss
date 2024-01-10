// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/platform/platform_manager/DevicePathResolver.h"

#include "fboss/platform/platform_manager/Utils.h"

using namespace facebook::fboss::platform::platform_manager;

namespace {
constexpr auto kIdprom = "IDPROM";
} // namespace

DevicePathResolver::DevicePathResolver(
    const PlatformConfig& config,
    const DataStore& dataStore,
    const I2cExplorer& i2cExplorer)
    : platformConfig_(config),
      dataStore_(dataStore),
      i2cExplorer_(i2cExplorer) {}

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
