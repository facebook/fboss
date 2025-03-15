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

  void initVendorSwitchEvents();

  void vendorSwitchEventNotificationCallback(
      sai_size_t bufferSize,
      const void* buffer,
      uint32_t eventType);

  const std::vector<uint32_t>& getAllInterruptEvents();
  const std::vector<uint32_t>& getInterruptEventsToBeEnabled();
  const std::string getVendorSwitchEventName(uint32_t eventId);

 private:
  SaiStore* saiStore_;
  SaiManagerTable* managerTable_;
  SaiPlatform* platform_;
#if defined(BRCM_SAI_SDK_DNX_GTE_12_0)
  std::shared_ptr<SaiVendorSwitch> vendorSwitch_;
#endif
};

} // namespace facebook::fboss
