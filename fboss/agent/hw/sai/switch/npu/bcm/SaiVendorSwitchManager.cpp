// Copyright 2004-present Facebook. All Rights Reserved.

// clang-format off
#include "fboss/agent/hw/sai/store/SaiStore.h"
#include "fboss/agent/hw/sai/switch/SaiVendorSwitchManager.h"
// clang-format on

namespace facebook::fboss {

const std::vector<uint32_t>& SaiVendorSwitchManager::getAllInterruptEvents() {
  static const std::vector<uint32_t> kInterruptEventList;
  return kInterruptEventList;
}

SaiVendorSwitchManager::SaiVendorSwitchManager(
    SaiStore* saiStore,
    SaiManagerTable* managerTable,
    SaiPlatform* platform)
    : saiStore_(saiStore), managerTable_(managerTable), platform_(platform) {}

void SaiVendorSwitchManager::initVendorSwitchEvents() {
#if defined(BRCM_SAI_SDK_DNX_GTE_12_0)
  auto& interruptList = getAllInterruptEvents();
  std::vector<sai_map_t> eventIdToOptions(interruptList.size());
  for (int idx = 0; idx < interruptList.size(); ++idx) {
    sai_map_t mapping{};
    mapping.key = interruptList.at(idx);
    mapping.value = SAI_VENDOR_SWITCH_EVENT_OPTION_ENABLE;
    eventIdToOptions.at(idx) = mapping;
  }
  auto& vendorSwitchStore = saiStore_->get<SaiVendorSwitchTraits>();
  auto vendorSwitchTraits =
      SaiVendorSwitchTraits::CreateAttributes{eventIdToOptions};
  vendorSwitch_ =
      vendorSwitchStore.setObject(std::monostate{}, vendorSwitchTraits);
#endif
}

void SaiVendorSwitchManager::vendorSwitchEventNotificationCallback(
    sai_size_t /*bufferSize*/,
    const void* buffer,
    uint32_t /*eventType*/) {
#if defined(BRCM_SAI_SDK_DNX_GTE_12_0)
  sai_vendor_switch_event_info_t* eventInfo =
      (sai_vendor_switch_event_info_t*)(buffer);
  // TODO: Generalize and add event_id to string conversion
  XLOG(WARNING) << "Vendor switch event notification, ID: "
                << eventInfo->event_id << ", index: " << eventInfo->index
                << ", at " << eventInfo->time;
#endif
}

} // namespace facebook::fboss
