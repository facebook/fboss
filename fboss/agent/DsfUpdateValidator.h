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
      const std::set<SwitchID>& remoteSwitchIds)
      : sw_(sw), remoteSwitchIds_(remoteSwitchIds) {}

  ~DsfUpdateValidator();
  std::shared_ptr<SwitchState> validateAndGetUpdate(
      const std::shared_ptr<SwitchState>& in,
      const std::map<SwitchID, std::shared_ptr<SystemPortMap>>&
          switchId2SystemPorts,
      const std::map<SwitchID, std::shared_ptr<InterfaceMap>>& switchId2Intfs);

 private:
  const SwSwitch* sw_;
  std::set<SwitchID> remoteSwitchIds_;
};
} // namespace facebook::fboss
