// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/sai/switch/SaiTamManager.h"

#include "fboss/agent/hw/sai/api/SwitchApi.h"
#include "fboss/agent/hw/sai/store/SaiStore.h"
#include "fboss/agent/hw/sai/switch/SaiManagerTable.h"
#include "fboss/agent/hw/sai/switch/SaiSwitchManager.h"

namespace facebook::fboss {

SaiTamHandle::~SaiTamHandle() {
  managerTable->switchManager().resetTamObject();
}

SaiTamManager::SaiTamManager(
    SaiStore* saiStore,
    SaiManagerTable* managerTable,
    SaiPlatform* platform)
    : saiStore_(saiStore),
      managerTable_(managerTable),
      platform_(platform),
      tamHandle_(std::make_unique<SaiTamHandle>()) {}

} // namespace facebook::fboss
