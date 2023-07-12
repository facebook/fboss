// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/agent/HwSwitch.h"

#include "fboss/agent/gen-cpp2/switch_config_types.h"

#include <set>
#include <vector>

namespace facebook::fboss {

class SwitchState;
class SwitchIdScopeResolver;

class TestEnsembleIf : public HwSwitchCallback {
 public:
  ~TestEnsembleIf() override {}
  virtual std::vector<PortID> masterLogicalPortIds() const = 0;
  std::vector<PortID> masterLogicalPortIds(
      const std::set<cfg::PortType>& portTypes) const;

  virtual std::shared_ptr<SwitchState> applyNewState(
      std::shared_ptr<SwitchState> state,
      bool rollbackOnHwOverflow = false) = 0;
  virtual void applyInitialConfig(const cfg::SwitchConfig& config) = 0;
  virtual std::shared_ptr<SwitchState> applyNewConfig(
      const cfg::SwitchConfig& config) = 0;
  virtual std::shared_ptr<SwitchState> getProgrammedState() const = 0;
  virtual HwSwitch* getHwSwitch() = 0;
  virtual const HwSwitch* getHwSwitch() const = 0;
  virtual const SwitchIdScopeResolver& scopeResolver() const = 0;
};

} // namespace facebook::fboss
