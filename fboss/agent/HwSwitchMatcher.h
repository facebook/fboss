// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "fboss/agent/types.h"

#include <unordered_set>

namespace facebook::fboss {

class HwSwitchMatcher {
 public:
  explicit HwSwitchMatcher(const std::string& matcherString);
  explicit HwSwitchMatcher(const std::unordered_set<SwitchID>& switchIds);
  HwSwitchMatcher() : HwSwitchMatcher(defaultHwSwitchMatcherKey()) {}

  const std::unordered_set<SwitchID> switchIds() const {
    return switchIds_;
  }
  /*
   * Get switchId - only applies when switchIds.size() == 1
   */
  SwitchID switchId() const;

  const std::string& matcherString() const {
    return matcherString_;
  }

  bool has(const SwitchID& switchId) const {
    return switchIds_.find(switchId) != switchIds_.end();
  }

  bool operator==(const HwSwitchMatcher& r) const {
    return std::tie(matcherString_, switchIds_) ==
        std::tie(r.matcherString_, r.switchIds_);
  }
  bool operator!=(const HwSwitchMatcher& r) const {
    return !(*this == r);
  }
  static HwSwitchMatcher defaultHwSwitchMatcher();

  static const std::string& defaultHwSwitchMatcherKey();

 private:
  std::string matcherString_;
  std::unordered_set<SwitchID> switchIds_;
};

} // namespace facebook::fboss
