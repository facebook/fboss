// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/agent/HwAsicTable.h"
#include "fboss/agent/HwSwitch.h"
#include "fboss/agent/state/StateUpdateHelpers.h"

#include "fboss/agent/gen-cpp2/switch_config_types.h"

#include <set>
#include <vector>

namespace facebook::fboss {

class SwitchState;
class SwitchIdScopeResolver;

class TestEnsembleIf : public HwSwitchCallback {
 public:
  using StateUpdateFn = FunctionStateUpdate::StateUpdateFn;
  ~TestEnsembleIf() override {}
  virtual std::vector<PortID> masterLogicalPortIds() const = 0;
  std::vector<PortID> masterLogicalPortIds(
      const std::set<cfg::PortType>& portTypes) const;

  virtual void applyNewState(
      StateUpdateFn fn,
      const std::string& name = "test-update",
      bool rollbackOnHwOverflow = false) = 0;
  virtual void applyInitialConfig(const cfg::SwitchConfig& config) = 0;
  virtual std::shared_ptr<SwitchState> applyNewConfig(
      const cfg::SwitchConfig& config) = 0;
  virtual std::shared_ptr<SwitchState> getProgrammedState() const = 0;
  virtual const std::map<int32_t, cfg::PlatformPortEntry>& getPlatformPorts()
      const = 0;
  virtual void switchRunStateChanged(SwitchRunState runState) = 0;
  virtual const SwitchIdScopeResolver& scopeResolver() const = 0;
  virtual HwAsicTable* getHwAsicTable() = 0;
  virtual std::map<PortID, FabricEndpoint> getFabricConnectivity() const = 0;
  virtual FabricReachabilityStats getFabricReachabilityStats() const = 0;
  virtual void updateStats() = 0;
};

} // namespace facebook::fboss
