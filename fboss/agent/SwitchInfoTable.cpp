// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/SwitchInfoTable.h"

#include "fboss/agent/FbossError.h"

namespace facebook::fboss {
SwitchInfoTable::SwitchInfoTable(
    const std::map<int64_t, cfg::SwitchInfo>& switchIdToSwitchInfo) {
  for (const auto& switchIdAndSwitchInfo : switchIdToSwitchInfo) {
    switchIdToSwitchInfo_.emplace(
        SwitchID(switchIdAndSwitchInfo.first), switchIdAndSwitchInfo.second);
  }
}

std::vector<SwitchID> SwitchInfoTable::getSwitchIdsOfType(
    cfg::SwitchType type) {
  std::vector<SwitchID> switchIds;
  for (const auto& switchIdAndSwitchInfo : switchIdToSwitchInfo_) {
    if (switchIdAndSwitchInfo.second.switchType() == type) {
      switchIds.emplace_back(switchIdAndSwitchInfo.first);
    }
  }
  return switchIds;
}

} // namespace facebook::fboss
