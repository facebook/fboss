// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "fboss/agent/types.h"

#include <unordered_set>

namespace facebook::fboss {

class SwitchIdScopeResolver {
 public:
  explicit SwitchIdScopeResolver(
      const std::map<int64_t, cfg::SwitchInfo>& switchIdToSwitchInfo);

 private:
  std::unordered_set<SwitchID> l3SwitchIds() const;
  std::unordered_set<SwitchID> getSwitchIdsOfType(cfg::SwitchType type) const;
  std::map<int64_t, cfg::SwitchInfo> switchIdToSwitchInfo_;
  std::unordered_set<SwitchID> l3SwitchIds_;
};
} // namespace facebook::fboss
