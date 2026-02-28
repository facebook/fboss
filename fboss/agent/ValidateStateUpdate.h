// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/agent/gen-cpp2/agent_config_types.h"
#include "fboss/agent/types.h"

namespace facebook::fboss {
class StateDelta;
class SwitchIdScopeResolver;
class HwAsic;
class HwAsicTable;

bool isStateUpdateValidCommon(const StateDelta& delta);
bool isStateUpdateValidMultiSwitch(
    const StateDelta& delta,
    const SwitchIdScopeResolver* resolver,
    const std::map<SwitchID, const HwAsic*>& hwAsics);

bool isStateUpdateValidMultiSwitch(
    const StateDelta& delta,
    const SwitchIdScopeResolver* resolver,
    SwitchID switchID,
    const HwAsic* asic);

class StateUpdateValidator {
 public:
  bool isValidUpdate(const StateDelta& delta) const;
  StateUpdateValidator(
      const cfg::AgentRunMode& runMode,
      const HwAsicTable* asicTable,
      const SwitchIdScopeResolver* scopeResolver);

 private:
  cfg::AgentRunMode runMode_;
  const HwAsicTable* asicTable_;
  const SwitchIdScopeResolver* scopeResolver_;
};

} // namespace facebook::fboss
