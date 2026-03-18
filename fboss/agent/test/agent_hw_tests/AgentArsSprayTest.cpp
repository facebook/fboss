// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.
#include "fboss/agent/AsicUtils.h"

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

const int kMaxLinks = 4;
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
      std::vector<flat_set<PortDescriptor>>& testNhopSets) {
    generatePrefixes();
    testPrefixes = prefixes;
    testNhopSets = nhopSets;
  }

  void setupEcmpGroups(int numEcmp) {
    generatePrefixes();
    std::vector<RoutePrefixV6> testPrefixes = {
        prefixes.begin(), prefixes.begin() + numEcmp};
    std::vector<flat_set<PortDescriptor>> testNhopSets = {
        nhopSets.begin(), nhopSets.begin() + numEcmp};
    auto wrapper = getSw()->getRouteUpdater();
    helper_->programRoutes(&wrapper, testNhopSets, testPrefixes);
  }

  void verifyEcmpGroups(const cfg::SwitchConfig& cfg, int numEcmp) {
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

} // namespace facebook::fboss
