// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/sai/switch/SaiVendorSwitchManager.h"
#include "fboss/agent/hw/sai/store/SaiStore.h"
#include "fboss/agent/platforms/sai/SaiPlatform.h"

namespace facebook::fboss {

SaiVendorSwitchManager::SaiVendorSwitchManager(
    SaiStore* saiStore,
    SaiManagerTable* managerTable,
    SaiPlatform* platform)
    : saiStore_(saiStore), managerTable_(managerTable), platform_(platform) {
#if defined(BRCM_SAI_SDK_DNX_GTE_12_0)
  if (!platform_->getAsic()->isSupported(
          HwAsic::Feature::VENDOR_SWITCH_NOTIFICATION)) {
    // Unsupported
    return;
  }
  std::vector<sai_map_t> eventIdToOptions;
  // Create VendorSwitch with all interrupts disabled
  eventIdToOptions.reserve(getInterruptEventsToBeEnabled().size());
  for (auto interruptEventId : getInterruptEventsToBeEnabled()) {
    sai_map_t mapping{};
    mapping.key = interruptEventId;
    // Keep interrupts disabled
    mapping.value = 0;
    eventIdToOptions.push_back(mapping);
  }
  auto& vendorSwitchStore = saiStore_->get<SaiVendorSwitchTraits>();
  auto vendorSwitchTraits =
      SaiVendorSwitchTraits::CreateAttributes{eventIdToOptions};
  vendorSwitch_ =
      vendorSwitchStore.setObject(std::monostate{}, vendorSwitchTraits);
#endif
}

void SaiVendorSwitchManager::setVendorSwitchEventEnableState(bool enable) {
#if defined(BRCM_SAI_SDK_DNX_GTE_12_0)
  std::vector<uint32_t> interruptsOfInterest = getInterruptEventsToBeEnabled();
  if (enable) {
    // Enable all interrupt events
    SaiApiTable::getInstance()->vendorSwitchApi().setAttribute(
        vendorSwitch_->adapterKey(),
        SaiVendorSwitchTraits::Attributes::EnableEventList{
            interruptsOfInterest});
  } else {
    // Disable all interrupt events
    SaiApiTable::getInstance()->vendorSwitchApi().setAttribute(
        vendorSwitch_->adapterKey(),
        SaiVendorSwitchTraits::Attributes::DisableEventList{
            interruptsOfInterest});
  }
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
  incrementInterruptEventCounter(eventInfo->event_id);
#endif
}

void SaiVendorSwitchManager::logCgmErrors() const {
#if defined(BRCM_SAI_SDK_DNX_GTE_12_0)
  sai_uint64_t cgmRejectBitmap =
      SaiApiTable::getInstance()->vendorSwitchApi().getAttribute(
          vendorSwitch_->adapterKey(),
          SaiVendorSwitchTraits::Attributes::CgmRejectStatusBitmap{});
  while (cgmRejectBitmap != 0) {
    // Brian Kernighan's algorithm to find set bits in the bitmap.
    uint64_t rightmostSetBit = cgmRejectBitmap & -cgmRejectBitmap;
    XLOG(WARNING) << "CGM REJECT: "
                  << getCgmDropReasonName(std::countr_zero(rightmostSetBit));
    cgmRejectBitmap -= rightmostSetBit;
  }
#endif
}

} // namespace facebook::fboss
