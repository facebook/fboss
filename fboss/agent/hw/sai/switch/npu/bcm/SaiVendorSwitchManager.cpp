// Copyright 2004-present Facebook. All Rights Reserved.

// clang-format off
#include "fboss/agent/hw/sai/store/SaiStore.h"
#include "fboss/agent/hw/sai/switch/SaiVendorSwitchManager.h"
// clang-format on

namespace facebook::fboss {

SaiVendorSwitchManager::SaiVendorSwitchManager(
    SaiStore* saiStore,
    SaiManagerTable* managerTable,
    SaiPlatform* platform)
    : saiStore_(saiStore), managerTable_(managerTable), platform_(platform) {}

void SaiVendorSwitchManager::initVendorSwitchEvents() {
#if defined(BRCM_SAI_SDK_DNX_GTE_12_0)
  std::vector<sai_map_t> eventIdToOptions;
  // TODO: As of now, creating vendor switch with enabled interrupt events
  // alone. However, once the outstanding issues are addressed as part of
  // CS00012393425, we'll need to pass in all interrupts in the event list
  // during create, with enabled ones specified with EVENT_OPTION_ENABLE.
  eventIdToOptions.reserve(getInterruptEventsToBeEnabled().size());
  for (auto interruptEventId : getInterruptEventsToBeEnabled()) {
    sai_map_t mapping{};
    mapping.key = interruptEventId;
    mapping.value = SAI_VENDOR_SWITCH_EVENT_OPTION_ENABLE;
    eventIdToOptions.push_back(mapping);
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
  XLOG(WARNING) << "ERROR INTERRUPT: "
                << getVendorSwitchEventName(eventInfo->event_id);
#endif
}

} // namespace facebook::fboss
