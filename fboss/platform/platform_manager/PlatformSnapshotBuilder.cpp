// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/platform/platform_manager/PlatformSnapshotBuilder.h"

#include <fmt/format.h>
#include <folly/logging/xlog.h>

#include "fboss/platform/platform_manager/I2cAddr.h"
#include "fboss/platform/platform_manager/Utils.h"

namespace facebook::fboss::platform::platform_manager {

PlatformSnapshotBuilder::PlatformSnapshotBuilder(
    const PlatformConfig& config,
    const DataStore& dataStore)
    : platformConfig_(config), dataStore_(dataStore) {}

PlatformSnapshot PlatformSnapshotBuilder::build() const {
  PlatformSnapshot snapshot;
  snapshot.platformName() = *platformConfig_.platformName();
  snapshot.i2cAdaptersFromCpu() = *platformConfig_.i2cAdaptersFromCpu();

  Slot rootSlot;
  rootSlot.slotType() = *platformConfig_.rootSlotType();

  if (dataStore_.hasPmUnit("/")) {
    auto pmUnitInfo = dataStore_.getPmUnitInfo("/");
    rootSlot.pluggedInPMUnit() = buildPMUnit("/", *pmUnitInfo.name());
  }

  snapshot.rootSlot() = std::move(rootSlot);

  // Build reverse map: kernel bus name (i2c-N) -> human-friendly config name
  for (const auto& [key, busNum] : dataStore_.getI2cBusNums()) {
    const auto& [slotPath, configBusName] = key;
    std::string kernelBusName = fmt::format("i2c-{}", busNum);

    std::string humanName =
        Utils().createDevicePath(slotPath.value_or("/"), configBusName);
    snapshot.i2cPathToHumanFriendlyName()[kernelBusName] = std::move(humanName);
  }

  return snapshot;
}

PMUnit PlatformSnapshotBuilder::buildPMUnit(
    const std::string& slotPath,
    const std::string& pmUnitName) const {
  PMUnit pmUnit;
  pmUnit.name() = pmUnitName;

  PmUnitConfig pmUnitConfig;
  try {
    pmUnitConfig = dataStore_.resolvePmUnitConfig(slotPath);
  } catch (const std::exception& ex) {
    XLOG(ERR) << fmt::format(
        "Failed to resolve PmUnitConfig for snapshot at {}: {}",
        slotPath,
        ex.what());
    return pmUnit;
  }
  pmUnit.pluggedInSlotType() = *pmUnitConfig.pluggedInSlotType();

  for (const auto& i2cDeviceConfig : *pmUnitConfig.i2cDeviceConfigs()) {
    I2cDevice device;
    try {
      auto busNum =
          dataStore_.getI2cBusNum(slotPath, *i2cDeviceConfig.busName());
      device.busName() = fmt::format("i2c-{}", busNum);
    } catch (const std::exception&) {
      device.busName() = *i2cDeviceConfig.busName();
    }
    try {
      device.addr() =
          static_cast<int32_t>(I2cAddr(*i2cDeviceConfig.address()).raw());
    } catch (const std::exception& ex) {
      XLOG(ERR) << fmt::format(
          "Failed to resolve I2cAddr for {}: {}",
          *i2cDeviceConfig.address(),
          ex.what());
      continue;
    }
    device.kernelDeviceName() = *i2cDeviceConfig.kernelDeviceName();
    device.pmUnitScopedName() = *i2cDeviceConfig.pmUnitScopedName();
    if (i2cDeviceConfig.numOutgoingChannels().has_value()) {
      device.numOutgoingChannels() = *i2cDeviceConfig.numOutgoingChannels();
    }
    pmUnit.i2cDevices()->push_back(std::move(device));
  }

  for (const auto& [slotName, slotConfig] :
       *pmUnitConfig.outgoingSlotConfigs()) {
    pmUnit.outgoingSlots()[slotName] =
        buildSlot(slotPath, slotName, slotConfig);
  }

  return pmUnit;
}

Slot PlatformSnapshotBuilder::buildSlot(
    const std::string& parentSlotPath,
    const std::string& slotName,
    const SlotConfig& slotConfig) const {
  Slot slot;
  slot.slotType() = *slotConfig.slotType();

  std::string childSlotPath = parentSlotPath == "/"
      ? fmt::format("/{}", slotName)
      : fmt::format("{}/{}", parentSlotPath, slotName);

  for (const auto& busName : *slotConfig.outgoingI2cBusNames()) {
    try {
      auto busNum = dataStore_.getI2cBusNum(parentSlotPath, busName);
      slot.outgoingI2cBusNames()->push_back(fmt::format("i2c-{}", busNum));
    } catch (const std::exception&) {
      slot.outgoingI2cBusNames()->push_back(busName);
    }
  }

  if (dataStore_.hasPmUnit(childSlotPath)) {
    auto pmUnitInfo = dataStore_.getPmUnitInfo(childSlotPath);
    slot.pluggedInPMUnit() = buildPMUnit(childSlotPath, *pmUnitInfo.name());
  }

  return slot;
}

} // namespace facebook::fboss::platform::platform_manager
