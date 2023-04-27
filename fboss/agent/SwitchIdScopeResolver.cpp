// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/SwitchIdScopeResolver.h"

namespace facebook::fboss {

SwitchIdScopeResolver::SwitchIdScopeResolver(
    const std::map<int64_t, cfg::SwitchInfo>& switchIdToSwitchInfo)
    : switchIdToSwitchInfo_(switchIdToSwitchInfo) {
  auto voqSwitchIds = getSwitchIdsOfType(cfg::SwitchType::VOQ);
  auto npuSwitchIds = getSwitchIdsOfType(cfg::SwitchType::NPU);
  CHECK(voqSwitchIds.empty() || npuSwitchIds.empty())
      << " Only one of "
         "voq, npu switch types can be present in a chassis";
  if (voqSwitchIds.size() || npuSwitchIds.size()) {
    l3SwitchMatcher_ = std::make_unique<HwSwitchMatcher>(
        voqSwitchIds.size() ? voqSwitchIds : npuSwitchIds);
  }
  std::unordered_set<SwitchID> allSwitchIds;
  for (const auto& switchIdAndInfo : switchIdToSwitchInfo_) {
    allSwitchIds.insert(SwitchID(switchIdAndInfo.first));
  }
  if (allSwitchIds.size()) {
    allSwitchMatcher_ = std::make_unique<HwSwitchMatcher>(allSwitchIds);
  }
}

std::unordered_set<SwitchID> SwitchIdScopeResolver::getSwitchIdsOfType(
    cfg::SwitchType type) const {
  std::unordered_set<SwitchID> ids;
  for (const auto& switchIdAndInfo : switchIdToSwitchInfo_) {
    if (switchIdAndInfo.second.switchType() == type) {
      ids.insert(SwitchID(switchIdAndInfo.first));
    }
  }
  return ids;
}

const HwSwitchMatcher& SwitchIdScopeResolver::l3SwitchMatcher() const {
  CHECK(l3SwitchMatcher_)
      << " One or more l3 switchIds must be set to get l3 scope";
  return *l3SwitchMatcher_;
}

const HwSwitchMatcher& SwitchIdScopeResolver::allSwitchMatcher() const {
  CHECK(allSwitchMatcher_)
      << " One or more all switchIds must be set to get allSwitch scope";
  return *allSwitchMatcher_;
}

} // namespace facebook::fboss
