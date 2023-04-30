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
  SwitchID switchId() const {
    CHECK_EQ(switchIds_.size(), 1)
        << "SwitchId api only applies when, switchIds.size() == 1";
    return *switchIds_.begin();
  }

  const std::string& matcherString() const {
    return matcherString_;
  }

  bool has(const SwitchID& switchId) const {
    return switchIds_.find(switchId) != switchIds_.end();
  }

  static HwSwitchMatcher defaultHwSwitchMatcher();

  static const std::string& defaultHwSwitchMatcherKey();

 private:
  std::string matcherString_;
  std::unordered_set<SwitchID> switchIds_;
};

} // namespace facebook::fboss
