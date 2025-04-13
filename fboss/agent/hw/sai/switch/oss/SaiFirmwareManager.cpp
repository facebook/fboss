// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/sai/switch/SaiFirmwareManager.h"

namespace facebook::fboss {

SaiFirmwareManager::SaiFirmwareManager(
    SaiStore* saiStore,
    SaiManagerTable* managerTable,
    SaiPlatform* platform)
    : saiStore_(saiStore), managerTable_(managerTable), platform_(platform) {}

std::optional<std::string> SaiFirmwareManager::getFirmwareVersion() const {
  return std::nullopt;
}

std::optional<FirmwareOpStatus> SaiFirmwareManager::getFirmwareOpStatus()
    const {
  return std::nullopt;
}

std::optional<FirmwareFuncStatus> SaiFirmwareManager::getFirmwareFuncStatus()
    const {
  return std::nullopt;
}

} // namespace facebook::fboss
