// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/agent/gen-cpp2/agent_config_types.h"
#include "fboss/agent/types.h"

namespace facebook::fboss {
class StateDelta;
class SwitchIdScopeResolver;
class HwAsic;
class HwAsicTable;
class HwSwitchHandler;
class SwitchStats;
class Port;
class ResourceAccountant;
class SwitchState;

bool hasValidPortQueues(
    const std::shared_ptr<Port>& port,
    bool isEcnProbabilisticMarkingSupported);

bool isStateUpdateValidCommon(
    const StateDelta& delta,
    const HwAsicTable* hwAsicTable);
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
  bool isValidUpdate(const StateDelta& delta, SwitchStats* stats = nullptr)
      const;
  StateUpdateValidator(
      const cfg::AgentRunMode& runMode,
      const HwSwitchHandler* hwSwitchHandler,
      const HwAsicTable* asicTable,
      const SwitchIdScopeResolver* scopeResolver);

  ResourceAccountant* getResourceAccountant() {
    return resourceAccountant_.get();
  }
  const ResourceAccountant* getResourceAccountant() const {
    return resourceAccountant_.get();
  }
  void resetResourceAccountant(const std::shared_ptr<SwitchState>& oldState);

 private:
  cfg::AgentRunMode runMode_;
  const HwSwitchHandler* hwSwitchHandler_;
  const HwAsicTable* asicTable_;
  const SwitchIdScopeResolver* scopeResolver_;
  std::unique_ptr<ResourceAccountant> resourceAccountant_;
};

} // namespace facebook::fboss
