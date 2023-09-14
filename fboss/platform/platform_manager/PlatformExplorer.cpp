// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.
#include <chrono>
#include <exception>
#include <stdexcept>

#include <folly/logging/xlog.h>

#include "fboss/platform/platform_manager/PlatformExplorer.h"

namespace facebook::fboss::platform::platform_manager {

PlatformExplorer::PlatformExplorer(
    std::chrono::seconds exploreInterval,
    const PlatformConfig& config)
    : platformConfig_(config) {
  scheduler_.addFunction([this]() { explore(); }, exploreInterval);
  scheduler_.start();
}

void PlatformExplorer::explore() {
  XLOG(INFO) << "Exploring the platform";
  for (const auto& [busName, busNum] :
       i2cExplorer_.getBusNums(*platformConfig_.i2cAdaptersFromCpu())) {
    updateI2cBusNum("", busName, busNum);
  }
  const PmUnitConfig& rootPmUnitConfig =
      platformConfig_.pmUnitConfigs()->at(*platformConfig_.rootPmUnitName());
  SlotConfig rootSlotConfig{};
  rootSlotConfig.slotType_ref() = *rootPmUnitConfig.pluggedInSlotType_ref();
  exploreSlot("", "", rootSlotConfig);
}

void PlatformExplorer::explorePmUnit(
    const std::string& parentPmUnitPath,
    const std::string& parentSlotName,
    const SlotConfig& parentSlotConfig,
    const std::string& pmUnitName) {
  auto pmUnitPath =
      fmt::format("{}::{}/{}", parentPmUnitPath, parentSlotName, pmUnitName);
  auto pmUnitConfig = platformConfig_.pmUnitConfigs()->at(pmUnitName);
  XLOG(INFO) << fmt::format("Exploring PmUnit {}", pmUnitPath);

  int i = 0;
  for (const auto& busName : *parentSlotConfig.outgoingI2cBusNames()) {
    auto busNum = getI2cBusNum(parentPmUnitPath, busName);
    updateI2cBusNum(pmUnitPath, fmt::format("INCOMING@{}", i++), busNum);
  }

  XLOG(INFO) << fmt::format(
      "Exploring I2C Devices for PmUnit {}. Count {}",
      pmUnitPath,
      pmUnitConfig.i2cDeviceConfigs_ref()->size());
  exploreI2cDevices(pmUnitPath, *pmUnitConfig.i2cDeviceConfigs());

  XLOG(INFO) << fmt::format(
      "Exploring Slots for PmUnit {}. Count {}",
      pmUnitPath,
      pmUnitConfig.outgoingSlotConfigs_ref()->size());
  for (const auto& [slotName, slotConfig] :
       *pmUnitConfig.outgoingSlotConfigs()) {
    exploreSlot(pmUnitPath, slotName, slotConfig);
  }
}

void PlatformExplorer::exploreSlot(
    const std::string& pmUnitPath,
    const std::string& slotName,
    const SlotConfig& slotConfig) {
  XLOG(INFO) << fmt::format("Exploring Slot {}::{}", pmUnitPath, slotName);
  auto pluggedInPmUnitName = getPmUnitNameFromSlot(slotConfig, pmUnitPath);

  if (!pluggedInPmUnitName) {
    XLOG(INFO) << fmt::format(
        "No device could be read in Slot {}::{}", pmUnitPath, slotName);
    return;
  }

  explorePmUnit(pmUnitPath, slotName, slotConfig, *pluggedInPmUnitName);
}

std::optional<std::string> PlatformExplorer::getPmUnitNameFromSlot(
    const SlotConfig& slotConfig,
    const std::string& pmUnitPath) {
  bool isChildPmUnitPlugged =
      presenceDetector_.isPresent(*slotConfig.presenceDetection());
  if (!isChildPmUnitPlugged) {
    XLOG(INFO) << "No device detected";
    return std::nullopt;
  }
  auto slotTypeConfig =
      platformConfig_.slotTypeConfigs_ref()->at(*slotConfig.slotType());
  std::optional<std::string> pmUnitNameInEeprom{std::nullopt};
  if (slotTypeConfig.idpromConfig_ref()) {
    auto idpromConfig = *slotTypeConfig.idpromConfig_ref();
    auto eepromI2cBusNum = getI2cBusNum(pmUnitPath, *idpromConfig.busName());
    i2cExplorer_.createI2cDevice(
        *idpromConfig.kernelDeviceName(),
        eepromI2cBusNum,
        I2cAddr(*idpromConfig.address()));
    auto eepromPath = i2cExplorer_.getDeviceI2cPath(
        eepromI2cBusNum, I2cAddr(*idpromConfig.address()));
    try {
      pmUnitNameInEeprom = i2cExplorer_.getPmUnitName(eepromPath);
    } catch (const std::exception& e) {
      XLOG(ERR) << fmt::format(
          "Could not fetch contents of IDPROM {}. {}", eepromPath, e.what());
    }
  }
  if (slotTypeConfig.pmUnitName()) {
    if (pmUnitNameInEeprom &&
        *pmUnitNameInEeprom != *slotTypeConfig.pmUnitName()) {
      XLOG(WARNING) << fmt::format(
          "The PmUnit type in eeprom `{}` is different from the one in config `{}`",
          *pmUnitNameInEeprom,
          *slotTypeConfig.pmUnitName());
    }
    return *slotTypeConfig.pmUnitName();
  }
  return pmUnitNameInEeprom;
}

void PlatformExplorer::exploreI2cDevices(
    const std::string& pmUnitPath,
    const std::vector<I2cDeviceConfig>& i2cDeviceConfigs) {
  for (const auto& i2cDeviceConfig : i2cDeviceConfigs) {
    i2cExplorer_.createI2cDevice(
        *i2cDeviceConfig.kernelDeviceName(),
        getI2cBusNum(pmUnitPath, *i2cDeviceConfig.busName()),
        I2cAddr(*i2cDeviceConfig.address()));
    if (i2cDeviceConfig.numOutgoingChannels()) {
      auto channelBusNums = i2cExplorer_.getMuxChannelI2CBuses(
          getI2cBusNum(pmUnitPath, *i2cDeviceConfig.busName()),
          I2cAddr(*i2cDeviceConfig.address()));
      assert(channelBusNums.size() == i2cDeviceConfig.numOutgoingChannels());
      for (int i = 0; i < i2cDeviceConfig.numOutgoingChannels(); ++i) {
        updateI2cBusNum(
            pmUnitPath,
            fmt::format("{}@{}", *i2cDeviceConfig.pmUnitScopedName(), i),
            channelBusNums[i]);
      }
    }
  }
}

uint16_t PlatformExplorer::getI2cBusNum(
    const std::string& pmUnitPath,
    const std::string& pmUnitScopeBusName) const {
  return i2cBusNamesToNums_.at(std::make_pair(pmUnitPath, pmUnitScopeBusName));
}

void PlatformExplorer::updateI2cBusNum(
    const std::string& pmUnitPath,
    const std::string& pmUnitScopeBusName,
    uint16_t busNum) {
  XLOG(INFO) << fmt::format(
      "Updating bus `{}` in PmUnit `{}` to bus number {} (i2c-{})",
      pmUnitScopeBusName,
      pmUnitPath,
      busNum,
      busNum);
  i2cBusNamesToNums_[std::make_pair(pmUnitPath, pmUnitScopeBusName)] = busNum;
}

} // namespace facebook::fboss::platform::platform_manager
