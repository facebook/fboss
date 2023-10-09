// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.
#include <chrono>
#include <exception>
#include <stdexcept>

#include <folly/logging/xlog.h>

#include "fboss/platform/platform_manager/PlatformExplorer.h"

namespace {
constexpr auto kRootSlotPath = "/";
}

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
    updateI2cBusNum(std::nullopt, busName, busNum);
  }
  const PmUnitConfig& rootPmUnitConfig =
      platformConfig_.pmUnitConfigs()->at(*platformConfig_.rootPmUnitName());
  auto pmUnitName = getPmUnitNameFromSlot(
      *rootPmUnitConfig.pluggedInSlotType(), kRootSlotPath);
  CHECK(pmUnitName == *platformConfig_.rootPmUnitName());
  explorePmUnit(kRootSlotPath, *platformConfig_.rootPmUnitName());
}

void PlatformExplorer::explorePmUnit(
    const std::string& slotPath,
    const std::string& pmUnitName) {
  auto pmUnitConfig = platformConfig_.pmUnitConfigs()->at(pmUnitName);
  XLOG(INFO) << fmt::format("Exploring PmUnit {} at {}", pmUnitName, slotPath);

  XLOG(INFO) << fmt::format(
      "Exploring PCI Devices for PmUnit {} at SlotPath {}. Count {}",
      pmUnitName,
      slotPath,
      pmUnitConfig.pciDeviceConfigs()->size());
  explorePciDevices(slotPath, *pmUnitConfig.pciDeviceConfigs());

  XLOG(INFO) << fmt::format(
      "Exploring I2C Devices for PmUnit {} at SlotPath {}. Count {}",
      pmUnitName,
      slotPath,
      pmUnitConfig.i2cDeviceConfigs()->size());
  exploreI2cDevices(slotPath, *pmUnitConfig.i2cDeviceConfigs());

  XLOG(INFO) << fmt::format(
      "Exploring Slots for PmUnit {} at SlotPath {}. Count {}",
      pmUnitName,
      slotPath,
      pmUnitConfig.outgoingSlotConfigs_ref()->size());
  for (const auto& [slotName, slotConfig] :
       *pmUnitConfig.outgoingSlotConfigs()) {
    exploreSlot(slotPath, slotName, slotConfig);
  }
}

void PlatformExplorer::exploreSlot(
    const std::string& parentSlotPath,
    const std::string& slotName,
    const SlotConfig& slotConfig) {
  std::string childSlotPath = fmt::format("{}/{}", parentSlotPath, slotName);
  XLOG(INFO) << fmt::format("Exploring SlotPath {}", childSlotPath);

  if (slotConfig.presenceDetection() &&
      presenceDetector_.isPresent(*slotConfig.presenceDetection())) {
    XLOG(INFO) << fmt::format(
        "No device could be detected in SlotPath {}", childSlotPath);
  }

  int i = 0;
  for (const auto& busName : *slotConfig.outgoingI2cBusNames()) {
    auto busNum = getI2cBusNum(parentSlotPath, busName);
    updateI2cBusNum(childSlotPath, fmt::format("INCOMING@{}", i++), busNum);
  }
  auto childPmUnitName =
      getPmUnitNameFromSlot(*slotConfig.slotType(), childSlotPath);

  if (!childPmUnitName) {
    XLOG(INFO) << fmt::format(
        "No device could be read in Slot {}", childSlotPath);
    return;
  }

  explorePmUnit(childSlotPath, *childPmUnitName);
}

std::optional<std::string> PlatformExplorer::getPmUnitNameFromSlot(
    const std::string& slotType,
    const std::string& slotPath) {
  auto slotTypeConfig = platformConfig_.slotTypeConfigs_ref()->at(slotType);
  CHECK(slotTypeConfig.idpromConfig() || slotTypeConfig.pmUnitName());
  std::optional<std::string> pmUnitNameInEeprom{std::nullopt};
  if (slotTypeConfig.idpromConfig_ref()) {
    auto idpromConfig = *slotTypeConfig.idpromConfig_ref();
    auto eepromI2cBusNum = getI2cBusNum(slotPath, *idpromConfig.busName());
    i2cExplorer_.createI2cDevice(
        *idpromConfig.kernelDeviceName(),
        eepromI2cBusNum,
        I2cAddr(*idpromConfig.address()));
    auto eepromPath = i2cExplorer_.getDeviceI2cPath(
        eepromI2cBusNum, I2cAddr(*idpromConfig.address()));
    try {
      // TODO: One eeprom parsing library is implemented, get the
      // Product Name from eeprom contents of eepromPath and use it here.
      pmUnitNameInEeprom = std::nullopt;
    } catch (const std::exception& e) {
      XLOG(ERR) << fmt::format(
          "Could not fetch contents of IDPROM {}. {}", eepromPath, e.what());
    }
  }
  if (slotTypeConfig.pmUnitName()) {
    if (pmUnitNameInEeprom &&
        *pmUnitNameInEeprom != *slotTypeConfig.pmUnitName()) {
      XLOG(WARNING) << fmt::format(
          "The PmUnit name in eeprom `{}` is different from the one in config `{}`",
          *pmUnitNameInEeprom,
          *slotTypeConfig.pmUnitName());
    }
    return *slotTypeConfig.pmUnitName();
  }
  return pmUnitNameInEeprom;
}

void PlatformExplorer::exploreI2cDevices(
    const std::string& slotPath,
    const std::vector<I2cDeviceConfig>& i2cDeviceConfigs) {
  for (const auto& i2cDeviceConfig : i2cDeviceConfigs) {
    i2cExplorer_.createI2cDevice(
        *i2cDeviceConfig.kernelDeviceName(),
        getI2cBusNum(slotPath, *i2cDeviceConfig.busName()),
        I2cAddr(*i2cDeviceConfig.address()));
    if (i2cDeviceConfig.numOutgoingChannels()) {
      auto channelBusNums = i2cExplorer_.getMuxChannelI2CBuses(
          getI2cBusNum(slotPath, *i2cDeviceConfig.busName()),
          I2cAddr(*i2cDeviceConfig.address()));
      assert(channelBusNums.size() == i2cDeviceConfig.numOutgoingChannels());
      for (int i = 0; i < i2cDeviceConfig.numOutgoingChannels(); ++i) {
        updateI2cBusNum(
            slotPath,
            fmt::format("{}@{}", *i2cDeviceConfig.pmUnitScopedName(), i),
            channelBusNums[i]);
      }
    }
  }
}

void PlatformExplorer::explorePciDevices(
    const std::string& slotPath,
    const std::vector<PciDeviceConfig>& pciDeviceConfigs) {
  for (const auto& pciDeviceConfig : pciDeviceConfigs) {
    PciDevice device{
        *pciDeviceConfig.pmUnitScopedName(),
        *pciDeviceConfig.vendorId(),
        *pciDeviceConfig.deviceId(),
        *pciDeviceConfig.subSystemVendorId(),
        *pciDeviceConfig.subSystemDeviceId()};
    for (const auto& [i2cAdapterName, i2cAdapterConfig] :
         *pciDeviceConfig.i2cAdapterConfigs()) {
      pciExplorer_.createI2cAdapter(device, i2cAdapterConfig);
    }
    for (const auto& [spiMasterName, spiMasterConfig] :
         *pciDeviceConfig.spiMasterConfigs()) {
      pciExplorer_.createSpiMaster(device, spiMasterConfig);
    }
    for (const auto& [name, fpgaIpBlockConfig] :
         *pciDeviceConfig.gpioChipConfigs()) {
      pciExplorer_.create(device, fpgaIpBlockConfig);
    }
    for (const auto& [name, fpgaIpBlockConfig] :
         *pciDeviceConfig.watchdogConfigs()) {
      pciExplorer_.create(device, fpgaIpBlockConfig);
    }
    for (const auto& [name, fpgaIpBlockConfig] :
         *pciDeviceConfig.fanTachoPwmConfigs()) {
      pciExplorer_.create(device, fpgaIpBlockConfig);
    }
    for (const auto& [name, fpgaIpBlockConfig] :
         *pciDeviceConfig.ledCtrlConfigs()) {
      pciExplorer_.createLedCtrl(device, fpgaIpBlockConfig);
    }
    for (const auto& [name, xcvrCtrlConfig] :
         *pciDeviceConfig.xcvrCtrlConfigs()) {
      pciExplorer_.createXcvrCtrl(device, xcvrCtrlConfig);
    }
  }
}

uint16_t PlatformExplorer::getI2cBusNum(
    const std::optional<std::string>& slotPath,
    const std::string& pmUnitScopeBusName) const {
  auto it = i2cBusNums_.find(std::make_pair(std::nullopt, pmUnitScopeBusName));
  if (it != i2cBusNums_.end()) {
    return it->second;
  } else {
    return i2cBusNums_.at(std::make_pair(slotPath, pmUnitScopeBusName));
  }
}

void PlatformExplorer::updateI2cBusNum(
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

} // namespace facebook::fboss::platform::platform_manager
