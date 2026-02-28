// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

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
      const HwAsicTable* asicTable,
      const SwitchIdScopeResolver* scopeResolver);

 private:
  const HwAsicTable* asicTable_;
  const SwitchIdScopeResolver* scopeResolver_;
};

} // namespace facebook::fboss
