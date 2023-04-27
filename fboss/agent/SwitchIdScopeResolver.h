// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "fboss/agent/HwSwitchMatcher.h"
#include "fboss/agent/types.h"

#include <unordered_set>

namespace facebook::fboss {
namespace cfg {
class Mirror;
}
class Mirror;

class SwitchIdScopeResolver {
 public:
  explicit SwitchIdScopeResolver(
      const std::map<int64_t, cfg::SwitchInfo>& switchIdToSwitchInfo);

  const HwSwitchMatcher& scope(const cfg::Mirror& /*m*/) const {
    return l3SwitchMatcher();
  }
  const HwSwitchMatcher& scope(const std::shared_ptr<Mirror>& /*m*/) const {
    return l3SwitchMatcher();
  }

 private:
  const HwSwitchMatcher& l3SwitchMatcher() const;
  const HwSwitchMatcher& allSwitchMatcher() const;
  std::unordered_set<SwitchID> getSwitchIdsOfType(cfg::SwitchType type) const;
  std::map<int64_t, cfg::SwitchInfo> switchIdToSwitchInfo_;
  std::unique_ptr<HwSwitchMatcher> l3SwitchMatcher_;
  std::unique_ptr<HwSwitchMatcher> allSwitchMatcher_;
};
} // namespace facebook::fboss
