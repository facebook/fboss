// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/sai/switch/SaiFirmwareManager.h"

#include "fboss/agent/hw/sai/store/SaiStore.h"
#include "fboss/agent/hw/sai/switch/SaiManagerTable.h"
#include "fboss/agent/hw/sai/switch/SaiSwitchManager.h"

namespace facebook::fboss {

SaiFirmwareManager::SaiFirmwareManager(
    SaiStore* saiStore,
    SaiManagerTable* managerTable,
    SaiPlatform* platform)
    : saiStore_(saiStore), managerTable_(managerTable), platform_(platform) {
#if defined(BRCM_SAI_SDK_DNX_GTE_12_0)
  SwitchSaiId switchId = managerTable_->switchManager().getSwitchSaiId();

  auto firmwareObjectList =
      SaiApiTable::getInstance()->switchApi().getAttribute(
          switchId, SaiSwitchTraits::Attributes::FirmwareObjectList{});

  CHECK_LE(firmwareObjectList.size(), 1);
  if (firmwareObjectList.size() == 1) {
    auto firmwareSaiId = *firmwareObjectList.begin();
    XLOG(DBG2) << "Firmware OID: " << firmwareSaiId;
    auto& store = saiStore_->get<SaiFirmwareTraits>();
    store.loadObjectOwnedByAdapter(
        SaiFirmwareTraits::AdapterKey{firmwareSaiId});
  }
#endif
}

} // namespace facebook::fboss
