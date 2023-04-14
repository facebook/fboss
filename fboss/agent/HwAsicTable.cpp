// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/HwAsicTable.h"
#include <optional>
#include "fboss/agent/FbossError.h"
#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/platforms/common/PlatformMappingUtils.h"
#include "fboss/lib/platforms/PlatformProductInfo.h"

namespace facebook::fboss {
HwAsicTable::HwAsicTable(
    SwSwitch* sw,
    const std::map<int64_t, cfg::SwitchInfo>& switchIdToSwitchInfo)
    : sw_(sw) {
  for (const auto& switchIdAndSwitchInfo : switchIdToSwitchInfo) {
    hwAsics_.emplace(
        SwitchID(switchIdAndSwitchInfo.first),
        HwAsic::makeAsic(
            *switchIdAndSwitchInfo.second.asicType(),
            *switchIdAndSwitchInfo.second.switchType(),
            switchIdAndSwitchInfo.first,
            std::nullopt));
  }
  // TODO - remove this once new config is rolled out everywhere
  if (switchIdToSwitchInfo.empty()) {
    int64_t switchId = sw_->getPlatform()->getAsic()->getSwitchId().has_value()
        ? *sw_->getPlatform()->getAsic()->getSwitchId()
        : 0;
    hwAsics_.emplace(
        SwitchID(switchId),
        HwAsic::makeAsic(
            sw_->getPlatform()->getAsic()->getAsicType(),
            sw_->getPlatform()->getAsic()->getSwitchType(),
            switchId,
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

} // namespace facebook::fboss
