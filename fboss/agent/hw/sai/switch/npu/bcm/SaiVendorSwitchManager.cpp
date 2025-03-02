// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/sai/switch/SaiVendorSwitchManager.h"

namespace facebook::fboss {

SaiVendorSwitchManager::SaiVendorSwitchManager(
    SaiStore* saiStore,
    SaiManagerTable* managerTable,
    SaiPlatform* platform)
    : saiStore_(saiStore), managerTable_(managerTable), platform_(platform) {
  // TODO: Create the vendor switch object
}

} // namespace facebook::fboss
