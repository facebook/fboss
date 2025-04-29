// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "fboss/agent/hw/sai/api/VendorSwitchApi.h"
#include "fboss/agent/hw/sai/store/SaiObject.h"

#include <cstdint>
#include <vector>

namespace facebook::fboss {

class SaiManagerTable;
class SaiPlatform;
class SaiStore;

#if defined(BRCM_SAI_SDK_DNX_GTE_12_0)
using SaiVendorSwitch = SaiObject<SaiVendorSwitchTraits>;
#endif

class SaiVendorSwitchManager {
 public:
  SaiVendorSwitchManager(
      SaiStore* saiStore,
      SaiManagerTable* managerTable,
      SaiPlatform* platform);

  void setVendorSwitchEventEnableState(bool enable);

  void vendorSwitchEventNotificationCallback(
      sai_size_t bufferSize,
      const void* buffer,
      uint32_t eventType);

  void logCgmErrors() const;

  const std::vector<uint32_t>& getAllInterruptEvents();
  const std::vector<uint32_t>& getInterruptEventsToBeEnabled();
  const std::string getVendorSwitchEventName(uint32_t eventId);
  void incrementInterruptEventCounter(uint32_t eventId);
  const std::string getCgmDropReasonName(int reason) const;
  void initWarningInterruptEvents();

 private:
  SaiStore* saiStore_;
  SaiManagerTable* managerTable_;
  SaiPlatform* platform_;
#if defined(BRCM_SAI_SDK_DNX_GTE_12_0)
  std::shared_ptr<SaiVendorSwitch> vendorSwitch_;
#endif
  // Identify interrupts which are warning interrupts to
  // differentiate it from critical error interrupts.
  std::vector<uint32_t> warningInterrupts_;
};

} // namespace facebook::fboss
