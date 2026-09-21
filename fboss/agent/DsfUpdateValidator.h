// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <set>
#include <unordered_set>
#include "fboss/agent/types.h"

namespace facebook::fboss {
class SwitchState;

class DsfUpdateValidator {
 public:
  DsfUpdateValidator(
      const std::unordered_set<SwitchID>& localSwitchIds,
      const std::set<SwitchID>& remoteSwitchIds)
      : localSwitchIds_(localSwitchIds), remoteSwitchIds_(remoteSwitchIds) {}

  void validate(
      const std::shared_ptr<SwitchState>& oldState,
      const std::shared_ptr<SwitchState>& newState);

 private:
  std::unordered_set<SwitchID> localSwitchIds_;
  std::set<SwitchID> remoteSwitchIds_;
};
} // namespace facebook::fboss
