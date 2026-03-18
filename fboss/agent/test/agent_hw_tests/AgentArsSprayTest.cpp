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
};

} // namespace facebook::fboss
