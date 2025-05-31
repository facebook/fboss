/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/test/agent_hw_tests/AgentArsBase.h"

#include "fboss/agent/test/EcmpSetupHelper.h"
#include "fboss/agent/test/utils/ConfigUtils.h"
#include "fboss/agent/test/utils/LoadBalancerTestUtils.h"
#include "fboss/agent/test/utils/ScaleTestUtils.h"

namespace facebook::fboss {

class AgentEcmpSpilloverTest : public AgentArsBase {
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
        cfg::SwitchingMode::PER_PACKET_QUALITY);
    return cfg;
  }

  void setCmdLineFlagOverrides() const override {
    AgentArsBase::setCmdLineFlagOverrides();
    FLAGS_flowletSwitchingEnable = true;
    FLAGS_dlbResourceCheckEnable = false;
    FLAGS_enable_ecmp_resource_manager = true;
    FLAGS_ecmp_resource_percentage = 100;
    FLAGS_ecmp_resource_manager_make_before_break_buffer = 0;
  }

  std::vector<production_features::ProductionFeature>
  getProductionFeaturesVerified() const override {
    return {
        production_features::ProductionFeature::DLB,
        production_features::ProductionFeature::ACL_COUNTER,
        production_features::ProductionFeature::SINGLE_ACL_TABLE};
  }

  void generatePrefixes() override {
    AgentArsBase::generatePrefixes();
    const auto kMaxDlbEcmpGroup =
        utility::getMaxDlbEcmpGroups(getAgentEnsemble()->getL3Asics());
    // Create two test prefix vectors
    dynamicPrefixes = {prefixes.begin(), prefixes.begin() + kMaxDlbEcmpGroup};
    dynamicNhopSets = {nhopSets.begin(), nhopSets.begin() + kMaxDlbEcmpGroup};
    spilloverPrefixes = {
        prefixes.begin() + kMaxDlbEcmpGroup,
        prefixes.begin() + kMaxDlbEcmpGroup + 32};
    spilloverNhopSets = {
        nhopSets.begin() + kMaxDlbEcmpGroup,
        nhopSets.begin() + kMaxDlbEcmpGroup + 32};
  }

  void programDynamicPrefixes() {
    auto wrapper = getSw()->getRouteUpdater();
    helper_->programRoutes(&wrapper, dynamicNhopSets, dynamicPrefixes);
  }
  void programSpilloverPrefixes() {
    auto wrapper = getSw()->getRouteUpdater();
    helper_->programRoutes(&wrapper, spilloverNhopSets, spilloverPrefixes);
  }
  void verifyDynamicPrefixes() {
    for (const auto& prefix : dynamicPrefixes) {
      verifyFwdSwitchingMode(prefix, cfg::SwitchingMode::PER_PACKET_QUALITY);
    }
  }
  void verifySpilloverPrefixes() {
    for (const auto& prefix : spilloverPrefixes) {
      verifyFwdSwitchingMode(prefix, cfg::SwitchingMode::PER_PACKET_RANDOM);
    }
  }

  std::vector<flat_set<PortDescriptor>> dynamicNhopSets;
  std::vector<RoutePrefixV6> dynamicPrefixes;
  std::vector<flat_set<PortDescriptor>> spilloverNhopSets;
  std::vector<RoutePrefixV6> spilloverPrefixes;
};

// Basic test to verify ecmp spillover
TEST_F(AgentEcmpSpilloverTest, VerifyEcmpSpillover) {
  generatePrefixes();

  auto setup = [=, this]() {
    programDynamicPrefixes();
    programSpilloverPrefixes();
  };

  auto verify = [=, this]() {
    verifyDynamicPrefixes();
    verifySpilloverPrefixes();
  };

  verifyAcrossWarmBoots(setup, verify);
}

// Basic test to verify ecmp decompression
TEST_F(AgentEcmpSpilloverTest, VerifyEcmpDecompress) {
  generatePrefixes();

  auto setup = [=, this]() {
    programDynamicPrefixes();
    programSpilloverPrefixes();

    verifyDynamicPrefixes();
    verifySpilloverPrefixes();

    // delete prefixes from dynamic range
    {
      std::vector<RoutePrefixV6> delPrefixes = {
          dynamicPrefixes.begin(), dynamicPrefixes.begin() + 32};
      auto wrapper = getSw()->getRouteUpdater();
      helper_->unprogramRoutes(&wrapper, delPrefixes);
    }
  };

  auto verify = [=, this]() {
    // verify previously spilled over routes are decompressed
    for (const auto& prefix : spilloverPrefixes) {
      verifyFwdSwitchingMode(prefix, cfg::SwitchingMode::PER_PACKET_QUALITY);
    }
  };

  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(AgentEcmpSpilloverTest, VerifyEcmpReplay) {
  generatePrefixes();

  auto setup = [=, this]() {
    programDynamicPrefixes();
    programSpilloverPrefixes();
  };

  auto verify = [=, this]() {
    verifyDynamicPrefixes();
    verifySpilloverPrefixes();

    // replay the same prefixes again and verify they are still dynamic
    programDynamicPrefixes();
    verifyDynamicPrefixes();

    // replay the same spilled over prefixes again and verify no change
    programSpilloverPrefixes();
    verifySpilloverPrefixes();
  };

  verifyAcrossWarmBoots(setup, verify);
}

} // namespace facebook::fboss
