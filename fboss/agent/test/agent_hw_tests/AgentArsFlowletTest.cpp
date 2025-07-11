// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <vector>
#include "fboss/agent/hw/test/HwTestCoppUtils.h"
#include "fboss/agent/test/agent_hw_tests/AgentArsBase.h"

#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/test/AgentEnsemble.h"
#include "fboss/agent/test/AgentHwTest.h"
#include "fboss/agent/test/EcmpSetupHelper.h"

#include "fboss/agent/test/utils/ConfigUtils.h"

#include "fboss/agent/test/utils/LoadBalancerTestUtils.h"

using namespace facebook::fboss::utility;

namespace facebook::fboss {

using namespace ::testing;
class AgentArsFlowletTest : public AgentArsBase {
 public:
  void setCmdLineFlagOverrides() const override {
    AgentHwTest::setCmdLineFlagOverrides();
    FLAGS_flowletSwitchingEnable = true;
    FLAGS_force_init_fp = false;
  }
  std::vector<ProductionFeature> getProductionFeaturesVerified()
      const override {
    return {
        ProductionFeature::DLB,
    };
  }

 protected:
  void SetUp() override {
    AgentArsBase::SetUp();
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
        cfg::SwitchingMode::FLOWLET_QUALITY,
        ensemble.isSai() ? cfg::SwitchingMode::FIXED_ASSIGNMENT
                         : cfg::SwitchingMode::PER_PACKET_RANDOM);
    return cfg;
  }

  void resolveNextHop(const PortDescriptor& port) {
    applyNewState([this, port](const std::shared_ptr<SwitchState>& state) {
      return helper_->resolveNextHops(state, {port});
    });
  }

  void generateTestPrefixes(
      std::vector<RoutePrefixV6>& testPrefixes,
      std::vector<flat_set<PortDescriptor>>& testNhopSets,
      int kTotalPrefixesNeeded) {
    AgentArsBase::generatePrefixes();

    testPrefixes = std::vector<RoutePrefixV6>{
        prefixes.begin(), prefixes.begin() + kTotalPrefixesNeeded};
    testNhopSets = std::vector<flat_set<PortDescriptor>>{
        nhopSets.begin(), nhopSets.begin() + kTotalPrefixesNeeded};
  }

  void unprogramRoute(const std::vector<RoutePrefixV6>& prefixesToUnprogram) {
    auto wrapper = getSw()->getRouteUpdater();
    helper_->unprogramRoutes(&wrapper, prefixesToUnprogram);
  }

  void verifyPrefixes(
      const std::vector<RoutePrefixV6>& allPrefixes,
      int startIdx,
      int endIdx) {
    for (int i = startIdx; i < allPrefixes.size() && i < endIdx; i++) {
      const auto& prefix = allPrefixes[i];
      verifyFwdSwitchingMode(prefix, cfg::SwitchingMode::FLOWLET_QUALITY);
    }
  }
};

/*
This is as close to real case as possible. Ensure that when multiple ECMP
objects are created and flowset table gets full things snap back in when first
ECMP object is removed, enough space is created for the second object to insert
itself
 */
TEST_F(AgentArsFlowletTest, ValidateFlowsetExceedForceFix) {
  std::vector<RoutePrefixV6> testPrefixes;
  std::vector<flat_set<PortDescriptor>> testNhopSets;

  generateTestPrefixes(testPrefixes, testNhopSets, 17);

  auto setup = [=, this]() {
    auto wrapper = getSw()->getRouteUpdater();
    helper_->programRoutes(&wrapper, testNhopSets, testPrefixes);
    XLOG(INFO) << "Programmed " << testPrefixes.size() << " prefixes across "
               << testNhopSets.size() << " ECMP groups";
    verifyPrefixes(testPrefixes, 0, 17);
    unprogramRoute({testPrefixes[0]});
  };

  auto verify = [=, this] {
    // Verify remaining prefixes (skip the first one that was unprogrammed)
    verifyPrefixes(testPrefixes, 1, 17);
  };

  verifyAcrossWarmBoots(setup, verify);
}
} // namespace facebook::fboss
