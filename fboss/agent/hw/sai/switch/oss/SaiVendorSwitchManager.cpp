// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/sai/switch/SaiVendorSwitchManager.h"

namespace facebook::fboss {

SaiVendorSwitchManager::SaiVendorSwitchManager(
    SaiStore* /*saiStore*/,
    SaiManagerTable* /*managerTable*/,
    SaiPlatform* /*platform*/) {}

void SaiVendorSwitchManager::initVendorSwitchEvents() {}

void SaiVendorSwitchManager::vendorSwitchEventNotificationCallback(
    sai_size_t /*bufferSize*/,
    const void* /*buffer*/,
    uint32_t /*eventType*/) {}

const std::vector<uint32_t>& SaiVendorSwitchManager::getAllInterruptEvents() {
  static const std::vector<uint32_t> kInterruptEvents;
  return kInterruptEvents;
}

const std::vector<uint32_t>&
SaiVendorSwitchManager::getInterruptEventsToBeEnabled() {
  static const std::vector<uint32_t> kEnabledInterruptEvents;
  return kEnabledInterruptEvents;
}

const std::string SaiVendorSwitchManager::getVendorSwitchEventName(
    uint32_t /*eventId*/) {
  return std::string();
}
} // namespace facebook::fboss
