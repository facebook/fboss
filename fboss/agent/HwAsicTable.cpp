// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/HwAsicTable.h"
#include <optional>
#include "fboss/agent/FbossError.h"
#include "fboss/agent/platforms/common/PlatformMappingUtils.h"
#include "fboss/lib/platforms/PlatformProductInfo.h"

namespace facebook::fboss {
HwAsicTable::HwAsicTable(
    const std::map<int64_t, cfg::SwitchInfo>& switchIdToSwitchInfo) {
  for (const auto& switchIdAndSwitchInfo : switchIdToSwitchInfo) {
    hwAsics_.emplace(
        SwitchID(switchIdAndSwitchInfo.first),
        HwAsic::makeAsic(
            *switchIdAndSwitchInfo.second.asicType(),
            *switchIdAndSwitchInfo.second.switchType(),
            switchIdAndSwitchInfo.first,
            std::nullopt));
  }
}

const HwAsic* HwAsicTable::getHwAsicIf(SwitchID switchID) const {
  auto iter = hwAsics_.find(switchID);
  if (iter != hwAsics_.end()) {
    return iter->second.get();
  }
  return nullptr;
}

bool HwAsicTable::isFeatureSupported(SwitchID switchId, HwAsic::Feature feature)
    const {
  auto asic = getHwAsicIf(switchId);
  CHECK(asic);
  return asic->isSupported(feature);
}

bool HwAsicTable::isFeatureSupportedOnAnyAsic(HwAsic::Feature feature) const {
  for (const auto& entry : hwAsics_) {
    if (entry.second->isSupported(feature)) {
      return true;
    }
  }
  return false;
}

} // namespace facebook::fboss
