// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "fboss/agent/types.h"

namespace facebook::fboss {

class HwSwitchMatcher {
 public:
  explicit HwSwitchMatcher(const std::string& matcherString);
  explicit HwSwitchMatcher(const std::set<SwitchID>& switchIds);
  HwSwitchMatcher() : HwSwitchMatcher("id=0") {}

  const std::set<SwitchID> npus() const {
    return switchIds_;
  }

  const std::string& matcherString() const {
    return matcherString_;
  }

  bool has(const SwitchID& switchId) const {
    return switchIds_.find(switchId) != switchIds_.end();
  }

  static HwSwitchMatcher defaultHwSwitchMatcher();

 private:
  std::string matcherString_;
  std::set<SwitchID> switchIds_;
};

} // namespace facebook::fboss
