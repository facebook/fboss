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
  l3SwitchIds_ = voqSwitchIds.size() ? voqSwitchIds : npuSwitchIds;
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

std::unordered_set<SwitchID> SwitchIdScopeResolver::l3SwitchIds() const {
  return l3SwitchIds_;
}

HwSwitchMatcher SwitchIdScopeResolver::l3SwitchMatcher() const {
  CHECK(l3SwitchIds_.size())
      << " One or more l3 switchIds must be set toget l3 scope";
  return HwSwitchMatcher{l3SwitchIds_};
}

} // namespace facebook::fboss
