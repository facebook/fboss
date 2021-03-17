// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/sai/switch/SaiTamManager.h"

namespace facebook::fboss {
SaiTamHandle::~SaiTamHandle() {}

SaiTamManager::SaiTamManager(
    SaiStore* /* saiStore */,
    SaiManagerTable* /*managerTable*/,
    SaiPlatform* /*platform*/) {}

} // namespace facebook::fboss
