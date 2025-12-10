// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/sai/switch/SaiFirmwareManager.h"

#include "fboss/agent/hw/HwSwitchFb303Stats.h"
#include "fboss/agent/hw/sai/store/SaiStore.h"
#include "fboss/agent/hw/sai/switch/SaiManagerTable.h"
#include "fboss/agent/hw/sai/switch/SaiSwitchManager.h"
#include "fboss/agent/platforms/sai/SaiPlatform.h"

#include <re2/re2.h>

namespace {
using namespace facebook::fboss;

#if defined(BRCM_SAI_SDK_DNX_GTE_12_0)
int64_t getIsolationFirmwareIntForString(const std::string& firmwareVersion) {
  // Firmware Version is of the form 2.4.0-EA9
  static const re2::RE2 pattern("^([0-9]+)\\.([0-9]+)\\.([0-9]+)-EA([0-9]+)$");

  int major, minor, patch, release;
  if (RE2::FullMatch(
          firmwareVersion, pattern, &major, &minor, &patch, &release)) {
    int64_t versionInt =
        (major * 10000000) + (minor * 100000) + (patch * 1000) + release;
    return versionInt;
  } else {
    XLOG(WARNING) << "Failed to match Firmware version to Int: "
                  << firmwareVersion;
    return 0;
  }
}

FirmwareOpStatus saiFirmwareOpStatusToFirmwareOpStatus(
    sai_firmware_op_status_t opStatus) {
  switch (opStatus) {
    case SAI_FIRMWARE_OP_STATUS_UNKNOWN:
      return FirmwareOpStatus::UNKNOWN;
    case SAI_FIRMWARE_OP_STATUS_LOADED:
      return FirmwareOpStatus::LOADED;
    case SAI_FIRMWARE_OP_STATUS_NOT_LOADED:
      return FirmwareOpStatus::NOT_LOADED;
    case SAI_FIRMWARE_OP_STATUS_RUNNING:
      return FirmwareOpStatus::RUNNING;
    case SAI_FIRMWARE_OP_STATUS_STOPPED:
      return FirmwareOpStatus::STOPPED;
    case SAI_FIRMWARE_OP_STATUS_ERROR:
      return FirmwareOpStatus::ERROR;
  }
}

FirmwareFuncStatus saiFirmwareFuncStatusToFirmwareFuncStatus(
    sai_firmware_func_status_t funcStatus) {
  switch (funcStatus) {
    case SAI_FIRMWARE_FUNC_STATUS_UNKNOWN:
      return FirmwareFuncStatus::UNKNOWN;
    case SAI_FIRMWARE_FUNC_STATUS_ISOLATED:
      return FirmwareFuncStatus::ISOLATED;
    case SAI_FIRMWARE_FUNC_STATUS_MONITORING:
      return FirmwareFuncStatus::MONITORING;
  }
}
#endif

} // namespace

namespace facebook::fboss {

SaiFirmwareManager::SaiFirmwareManager(
    SaiStore* saiStore,
    SaiManagerTable* managerTable,
    SaiPlatform* platform)
    : saiStore_(saiStore), managerTable_(managerTable), platform_(platform) {
#if defined(BRCM_SAI_SDK_DNX_GTE_12_0)
  auto firmwareHandle = std::make_unique<SaiFirmwareHandle>();
  SwitchSaiId switchId = managerTable_->switchManager().getSwitchSaiId();

  auto firmwareObjectList =
      SaiApiTable::getInstance()->switchApi().getAttribute(
          switchId, SaiSwitchTraits::Attributes::FirmwareObjectList{});

  // If Firmware is not configured,
  //   - firmwareObjectList.size() will be 0.
  // If Firmware is configured,
  //   - firmwareObjectList.size() will be 1.
  //   - This is because the current API supports only a single Firmware.
  //   - In future, when multiple Firmwares are supported, SAI impls
  //     will expose additional create APIs, and get attrs (e.g. PATH)
  //     to determine which Firmware OID belongs to which Firmware.
  // In the current programming model, the Firmware is configured during switch
  // create and cannot change later. Thus, we can cache Firmware OID post
  // switch create.
  CHECK_LE(firmwareObjectList.size(), 1);
  if (firmwareObjectList.size() == 1) {
    auto firmwareSaiId = *firmwareObjectList.begin();
    auto& store = saiStore_->get<SaiFirmwareTraits>();
    firmwareHandle->firmware = store.loadObjectOwnedByAdapter(
        SaiFirmwareTraits::AdapterKey{firmwareSaiId});

    handles_.emplace(
        std::make_pair(
            SaiFirmwareManager::kFirmwareName, std::move(firmwareHandle)));

    auto firmwareVersion = getFirmwareVersion();
    CHECK(firmwareVersion.has_value());
    auto firmwareVersionInt =
        getIsolationFirmwareIntForString(firmwareVersion.value());
    platform_->getHwSwitch()->getSwitchStats()->isolationFirmwareVersion(
        firmwareVersionInt);
    XLOG(DBG2) << "Firmware OID: " << firmwareSaiId
               << " Firmware Version: " << firmwareVersion.value()
               << " Firmware Version Int: " << firmwareVersionInt;
  }
#endif
}

std::optional<std::string> SaiFirmwareManager::getFirmwareVersion() const {
#if defined(BRCM_SAI_SDK_DNX_GTE_12_0)
  auto firmwareHandle = getFirmwareHandle(SaiFirmwareManager::kFirmwareName);
  if (firmwareHandle) {
    auto version =
        GET_ATTR(Firmware, Version, firmwareHandle->firmware->attributes());
    return std::string(version.begin(), version.end());
  }
#endif

  return std::nullopt;
}

std::optional<FirmwareOpStatus> SaiFirmwareManager::getFirmwareOpStatus()
    const {
#if defined(BRCM_SAI_SDK_DNX_GTE_12_0)
  auto firmwareHandle = getFirmwareHandle(SaiFirmwareManager::kFirmwareName);
  if (firmwareHandle) {
    auto opStatus = SaiApiTable::getInstance()->firmwareApi().getAttribute(
        firmwareHandle->firmware->adapterKey(),
        SaiFirmwareTraits::Attributes::OpStatus{});
    firmwareHandle->firmware->setAttribute(
        SaiFirmwareTraits::Attributes::OpStatus{opStatus},
        true /* skipHwWrite */);

    return saiFirmwareOpStatusToFirmwareOpStatus(
        static_cast<sai_firmware_op_status_t>(opStatus));
  }
#endif

  return std::nullopt;
}

std::optional<FirmwareFuncStatus> SaiFirmwareManager::getFirmwareFuncStatus()
    const {
#if defined(BRCM_SAI_SDK_DNX_GTE_12_0)
  auto firmwareHandle = getFirmwareHandle(SaiFirmwareManager::kFirmwareName);
  if (firmwareHandle) {
    auto funcStatus = SaiApiTable::getInstance()->firmwareApi().getAttribute(
        firmwareHandle->firmware->adapterKey(),
        SaiFirmwareTraits::Attributes::FunctionalStatus{});
    firmwareHandle->firmware->setAttribute(
        SaiFirmwareTraits::Attributes::FunctionalStatus{funcStatus},
        true /* skipHwWrite */);

    return saiFirmwareFuncStatusToFirmwareFuncStatus(
        static_cast<sai_firmware_func_status_t>(funcStatus));
  }
#endif

  return std::nullopt;
}

} // namespace facebook::fboss
