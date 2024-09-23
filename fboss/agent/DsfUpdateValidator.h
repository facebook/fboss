// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <set>
#include <unordered_set>
#include "fboss/agent/types.h"

namespace facebook::fboss {
class SwitchState;
class SwSwitch;
class SystemPortMap;
class InterfaceMap;

class DsfUpdateValidator {
 public:
  DsfUpdateValidator(
      const SwSwitch* sw,
      const std::unordered_set<SwitchID>& localSwitchIds,
      const std::set<SwitchID>& remoteSwitchIds)
      : sw_(sw),
        localSwitchIds_(localSwitchIds),
        remoteSwitchIds_(remoteSwitchIds) {}

  ~DsfUpdateValidator();
  std::shared_ptr<SwitchState> validateAndGetUpdate(
      const std::shared_ptr<SwitchState>& in,
      const std::map<SwitchID, std::shared_ptr<SystemPortMap>>&
          switchId2SystemPorts,
      const std::map<SwitchID, std::shared_ptr<InterfaceMap>>& switchId2Intfs);

 private:
  void validate(
      const std::shared_ptr<SwitchState>& oldState,
      const std::shared_ptr<SwitchState>& newState);
  const SwSwitch* sw_;
  std::unordered_set<SwitchID> localSwitchIds_;
  std::set<SwitchID> remoteSwitchIds_;
};
} // namespace facebook::fboss
