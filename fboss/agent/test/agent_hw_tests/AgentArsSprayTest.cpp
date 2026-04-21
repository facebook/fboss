// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.
#include "fboss/agent/AgentFeatures.h"
#include "fboss/agent/AsicUtils.h"

#include <cmath>
#include <vector>
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/test/AgentEnsemble.h"
#include "fboss/agent/test/AgentHwTest.h"
#include "fboss/agent/test/EcmpSetupHelper.h"
#include "fboss/agent/test/agent_hw_tests/AgentArsBase.h"
#include "fboss/agent/test/utils/ConfigUtils.h"
#include "fboss/agent/test/utils/LoadBalancerTestUtils.h"

using namespace facebook::fboss::utility;

namespace facebook::fboss {

const int kMinFlowletTableSize = 256;

const int kLoadWeight1 = 70;
const int kQueueWeight1 = 30;

const int kLoadWeight2 = 60;
const int kQueueWeight2 = 20;

using namespace ::testing;

class AgentArsSprayTest : public AgentArsBase {
 public:
  int kScalingFactor1() const {
    return getAgentEnsemble()->isSai() ? 10 : 100;
  }
  int kScalingFactor2() const {
    return getAgentEnsemble()->isSai() ? 20 : 200;
  }

  void setCmdLineFlagOverrides() const override {
    AgentHwTest::setCmdLineFlagOverrides();
    FLAGS_flowletSwitchingEnable = true;
    FLAGS_force_init_fp = false;
  }

  std::vector<ProductionFeature> getProductionFeaturesVerified()
      const override {
    return {ProductionFeature::DLB};
  }

 protected:
  void generateTestPrefixes(
      std::vector<RoutePrefixV6>& testPrefixes,
      std::vector<boost::container::flat_set<PortDescriptor>>& testNhopSets) {
    generatePrefixes();
    testPrefixes = prefixes;
    testNhopSets = nhopSets;
  }

  void verifyEcmpGroups(const cfg::SwitchConfig& cfg, int numEcmp) {
    generatePrefixes();
    CHECK_LE(numEcmp, prefixes.size());
    CHECK_LE(numEcmp, nhopSets.size());
    for (int i = 0; i < numEcmp; i++) {
      auto portFlowletConfig =
          getPortFlowletConfig(kScalingFactor1(), kLoadWeight1, kQueueWeight1);
      EXPECT_TRUE(verifyPortFlowletConfig(
          prefixes[i].toCidrNetwork(),
          portFlowletConfig,
          nhopSets[i].begin()->phyPortID()));
      EXPECT_TRUE(verifyEcmpForFlowletSwitching(
          prefixes[i].toCidrNetwork(),
          *cfg.flowletSwitchingConfig(),
          true,
          nhopSets[i].begin()->phyPortID()));
    }
  }

  cfg::SwitchConfig initialConfig(
      const AgentEnsemble& ensemble) const override {
    auto cfg = utility::onePortPerInterfaceConfig(
        ensemble.getSw(),
        ensemble.masterLogicalPortIds(),
        true /*interfaceHasSubnet*/);
    utility::addFlowletConfigs(
        cfg,
        ensemble.masterLogicalPortIds(),
        ensemble.isSai(),
        cfg::SwitchingMode::PER_PACKET_QUALITY,
        cfg::SwitchingMode::FIXED_ASSIGNMENT);
    return cfg;
  }

  // getPortFlowletConfig, updatePortFlowletConfigs,
  // updatePortFlowletConfigName, and updateFlowletConfigs are inherited from
  // AgentArsBase

  void verifyConfig(
      const cfg::SwitchConfig& cfg,
      const RoutePrefixV6& prefix,
      const PortID& port) {
    auto portFlowletConfig =
        getPortFlowletConfig(kScalingFactor1(), kLoadWeight1, kQueueWeight1);
    EXPECT_TRUE(verifyPortFlowletConfig(
        prefix.toCidrNetwork(), portFlowletConfig, port));
    EXPECT_TRUE(verifyEcmpForFlowletSwitching(
        prefix.toCidrNetwork(), *cfg.flowletSwitchingConfig(), true, port));
  }
};

TEST_F(AgentArsSprayTest, VerifyArsEnable) {
  std::vector<RoutePrefixV6> testPrefixes;
  std::vector<boost::container::flat_set<PortDescriptor>> testNhopSets;
  generateTestPrefixes(testPrefixes, testNhopSets);

  // Find a nhopSet index with size >= 2 for meaningful shrink/expand testing
  size_t testNhopSetIndex = 0;
  for (size_t i = 0; i < testNhopSets.size(); ++i) {
    if (testNhopSets[i].size() >= 2) {
      testNhopSetIndex = i;
      break;
    }
  }
  CHECK_GE(testNhopSets[testNhopSetIndex].size(), 2)
      << "Need at least 2 ports in nhopSet for shrink/expand testing";

  auto setup = [=, this]() {
    auto wrapper = getSw()->getRouteUpdater();
    helper_->programRoutes(
        &wrapper, {testNhopSets[testNhopSetIndex]}, {testPrefixes[0]});
  };

  auto verify = [=, this]() {
    auto cfg = initialConfig(*getAgentEnsemble());
    auto port = testNhopSets[testNhopSetIndex].begin()->phyPortID();
    verifyConfig(cfg, testPrefixes[0], port);

    // Shrink egress and verify - unresolve the last port in the nhopSet
    auto portDesc = *testNhopSets[testNhopSetIndex].rbegin();
    applyNewState([&](const std::shared_ptr<SwitchState>& in) {
      return helper_->unresolveNextHops(in, {portDesc});
    });
    verifyConfig(cfg, testPrefixes[0], port);

    // Expand egress and verify
    applyNewState([&](const std::shared_ptr<SwitchState>& in) {
      return helper_->resolveNextHops(in, {portDesc});
    });
    verifyConfig(cfg, testPrefixes[0], port);
  };

  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(AgentArsSprayTest, VerifyPortFlowletConfigChange) {
  std::vector<RoutePrefixV6> testPrefixes;
  std::vector<boost::container::flat_set<PortDescriptor>> testNhopSets;
  generateTestPrefixes(testPrefixes, testNhopSets);

  auto setup = [=, this]() {
    auto wrapper = getSw()->getRouteUpdater();
    helper_->programRoutes(&wrapper, {testNhopSets[0]}, {testPrefixes[0]});
  };

  auto verify = [=, this]() {
    auto cfg = initialConfig(*getAgentEnsemble());
    auto port = testNhopSets[0].begin()->phyPortID();
    verifyConfig(cfg, testPrefixes[0], port);

    // Modify the port flowlet config
    updatePortFlowletConfigs(
        cfg, kScalingFactor2(), kLoadWeight2, kQueueWeight2);
    applyNewConfig(cfg);

    // Verify modified config
    auto portFlowletConfig =
        getPortFlowletConfig(kScalingFactor2(), kLoadWeight2, kQueueWeight2);
    EXPECT_TRUE(verifyPortFlowletConfig(
        testPrefixes[0].toCidrNetwork(), portFlowletConfig, port));

    // Restore initial config
    cfg = initialConfig(*getAgentEnsemble());
    applyNewConfig(cfg);
    verifyConfig(cfg, testPrefixes[0], port);
  };

  verifyAcrossWarmBoots(setup, verify);
}

// Verify ability to create multiple ECMP DLB groups in spray mode.
// The ResourceAccountant enforces a limit of ars_resource_percentage (default
// 75%) of max DLB groups. Compute the actual enforced limit to test at scale.
TEST_F(AgentArsSprayTest, VerifySprayModeScale) {
  auto numEcmp = static_cast<int>(std::floor(
      getMaxArsGroups() * static_cast<double>(FLAGS_ars_resource_percentage) /
      100.0));

  auto setup = [this, numEcmp]() {
    auto cfg = initialConfig(*getAgentEnsemble());
    updateFlowletConfigs(
        cfg,
        cfg::SwitchingMode::PER_PACKET_QUALITY,
        kMinFlowletTableSize,
        kScalingFactor1(),
        kLoadWeight1,
        kQueueWeight1);
    updatePortFlowletConfigName(cfg);
    applyNewConfig(cfg);
    setupEcmpGroups(numEcmp);
  };

  auto verify = [this, numEcmp]() {
    auto cfg = initialConfig(*getAgentEnsemble());
    updateFlowletConfigs(
        cfg,
        cfg::SwitchingMode::PER_PACKET_QUALITY,
        kMinFlowletTableSize,
        kScalingFactor1(),
        kLoadWeight1,
        kQueueWeight1);
    updatePortFlowletConfigName(cfg);
    verifyEcmpGroups(cfg, numEcmp);
  };

  verifyAcrossWarmBoots(setup, verify);
}

// Verify that creating ECMP groups beyond the DLB limit fails when backup mode
// is also DLB (PER_PACKET_QUALITY). Unlike VerifySprayModeScale which uses
// FIXED_ASSIGNMENT as backup, this test uses PER_PACKET_QUALITY for both
// primary and backup, so there's no fallback to non-DLB IDs.
TEST_F(AgentArsSprayTest, VerifyEcmpIdAllocationForDynamicEcmp) {
  // Use the actual ResourceAccountant enforced limit to fill all DLB slots.
  // This matches VerifySprayModeScale's calculation.
  auto numEcmp = static_cast<int>(std::floor(
      getMaxArsGroups() * static_cast<double>(FLAGS_ars_resource_percentage) /
      100.0));

  auto setup = [this, numEcmp]() {
    auto cfg = initialConfig(*getAgentEnsemble());
    // Use PER_PACKET_QUALITY for both primary and backup - no fallback to
    // non-DLB
    updateFlowletConfigs(
        cfg,
        cfg::SwitchingMode::PER_PACKET_QUALITY,
        kMinFlowletTableSize,
        kScalingFactor1(),
        kLoadWeight1,
        kQueueWeight1,
        cfg::SwitchingMode::PER_PACKET_QUALITY);
    updatePortFlowletConfigName(cfg);
    applyNewConfig(cfg);
    setupEcmpGroups(numEcmp);
  };

  auto verify = [this, numEcmp]() {
    auto cfg = initialConfig(*getAgentEnsemble());
    updateFlowletConfigs(
        cfg,
        cfg::SwitchingMode::PER_PACKET_QUALITY,
        kMinFlowletTableSize,
        kScalingFactor1(),
        kLoadWeight1,
        kQueueWeight1,
        cfg::SwitchingMode::PER_PACKET_QUALITY);
    updatePortFlowletConfigName(cfg);
    verifyEcmpGroups(cfg, numEcmp);

    // Attempt to create one more ECMP group beyond the DLB limit.
    // Since backup mode is also DLB, there's no fallback to non-DLB IDs,
    // so the ResourceAccountant should reject the state update.
    generatePrefixes();
    auto wrapper = getSw()->getRouteUpdater();
    EXPECT_THROW(
        helper_->programRoutes(
            &wrapper, {nhopSets[numEcmp]}, {prefixes[numEcmp]}),
        FbossError);
  };

  verifyAcrossWarmBoots(setup, verify);
}

const int kNumNonDlbEcmpGroups = 32;

// Test class for non-DLB ECMP tests using PER_PACKET_RANDOM mode.
class AgentArsBcmTest : public AgentArsBase {
 public:
  void setCmdLineFlagOverrides() const override {
    AgentHwTest::setCmdLineFlagOverrides();
    FLAGS_flowletSwitchingEnable = true;
    FLAGS_force_init_fp = false;
    FLAGS_enable_ecmp_resource_manager = true;
  }

  std::vector<ProductionFeature> getProductionFeaturesVerified()
      const override {
    return {ProductionFeature::DLB, ProductionFeature::ECMP_RANDOM_SPRAY};
  }

 protected:
  cfg::SwitchConfig initialConfig(
      const AgentEnsemble& ensemble) const override {
    auto cfg = utility::onePortPerInterfaceConfig(
        ensemble.getSw(),
        ensemble.masterLogicalPortIds(),
        true /*interfaceHasSubnet*/);
    utility::addFlowletConfigs(
        cfg,
        ensemble.masterLogicalPortIds(),
        ensemble.isSai(),
        cfg::SwitchingMode::PER_PACKET_RANDOM,
        cfg::SwitchingMode::FIXED_ASSIGNMENT);
    return cfg;
  }
};

// Create 32 ECMP groups with PER_PACKET_RANDOM mode and verify they are all
// created as non-flowlet groups (without DLB resources).
TEST_F(AgentArsBcmTest, VerifyEcmpIdAllocationForNonDynamicEcmp) {
  auto setup = [this]() { setupEcmpGroups(kNumNonDlbEcmpGroups); };

  auto verify = [this]() {
    generatePrefixes();
    auto cfg = initialConfig(*getAgentEnsemble());
    for (int i = 0; i < kNumNonDlbEcmpGroups; i++) {
      EXPECT_TRUE(verifyEcmpForNonFlowlet(
          prefixes[i].toCidrNetwork(),
          *cfg.flowletSwitchingConfig(),
          true,
          nhopSets[i].begin()->phyPortID()));
    }
  };

  verifyAcrossWarmBoots(setup, verify);
}

} // namespace facebook::fboss
