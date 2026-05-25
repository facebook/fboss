// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/agent/test/AgentHwTest.h"

namespace facebook::fboss {

// Test class for fabric link monitoring on L1 (FDSW) fabric switches.
// Tests the FDSW -> SDSW direction of fabric link monitoring packets.
// Requires DUAL_STAGE_L1 fabric node role for FabricLinkMonitoringManager.
class AgentFabricSwitchFabricLinkMonitoringTest : public AgentHwTest {
 public:
  cfg::SwitchConfig initialConfig(const AgentEnsemble& ensemble) const override;

  void SetUp() override;

 protected:
  std::vector<ProductionFeature> getProductionFeaturesVerified()
      const override {
    return {ProductionFeature::FABRIC};
  }

  // Override to ensure DUAL_STAGE_L1 fabric node role is set during
  // HwAsicTable creation. Required because HwAsicTable is created before
  // initialConfig() is called.
  void overrideTestEnsembleInitInfo(
      TestEnsembleInitInfo& initInfo) const override;

 private:
  void setCmdLineFlagOverrides() const override;

 protected:
  void runCmd(const std::string& cmd);
  void runCint(const std::string& cintStr);

 private:
  // initialConfig helpers
  void addFabricPorts(cfg::SwitchConfig& config, const AgentEnsemble& ensemble)
      const;
  void addDsfNodes(
      cfg::SwitchConfig& config,
      const std::map<int64_t, cfg::SwitchInfo>& switchIdToSwitchInfo,
      const AgentEnsemble& ensemble) const;
  void configureExpectedNeighborReachability(cfg::SwitchConfig& config) const;
};

} // namespace facebook::fboss
