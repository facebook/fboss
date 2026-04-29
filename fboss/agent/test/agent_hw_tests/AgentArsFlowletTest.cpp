// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.
#include "fboss/agent/AsicUtils.h"
#include "fboss/agent/test/TestUtils.h"

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

const int kLoadWeight2 = 60;
const int kQueueWeight2 = 20;

const int kFlowletTableSize1 = 1024;
const int kInactivityIntervalUsecs1 = 128;
const int kLoadWeight1 = 70;
const int kQueueWeight1 = 30;
const int KMaxFlowsetTableSize = 32768;

using namespace ::testing;
class AgentArsFlowletTest : public AgentArsBase {
 public:
  int kDefaultSamplingRate() const {
    return getAgentEnsemble()->isSai() ? 1000 : 500000;
  }

  int kScalingFactor1() const {
    return getAgentEnsemble()->isSai() ? 10 : 100;
  }
  int kScalingFactor2() const {
    return getAgentEnsemble()->isSai() ? 20 : 200;
  }

  int kMinSamplingRate() const {
    auto asic = checkSameAndGetAsicForTesting(getAgentEnsemble()->getL3Asics());

    bool isTH3 = asic->getAsicType() == cfg::AsicType::ASIC_TYPE_TOMAHAWK3;
    return getAgentEnsemble()->isSai() ? (isTH3 ? 1000 : 512)
                                       : (isTH3 ? 1000000 : 1953125);
  }

  void setCmdLineFlagOverrides() const override {
    AgentHwTest::setCmdLineFlagOverrides();
    FLAGS_flowletSwitchingEnable = true;
    FLAGS_force_init_fp = false;
  }
  std::vector<ProductionFeature> getProductionFeaturesVerified()
      const override {
    return {
        ProductionFeature::DLB,
        ProductionFeature::ARS_FLOWLET,
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
    auto hwAsic = checkSameAndGetAsicForTesting(ensemble.getL3Asics());
    utility::addFlowletConfigs(
        cfg,
        ensemble.masterLogicalPortIds(),
        ensemble.isSai(),
        cfg::SwitchingMode::FLOWLET_QUALITY,
        ensemble.isSai() ? cfg::SwitchingMode::FIXED_ASSIGNMENT
                         : cfg::SwitchingMode::PER_PACKET_RANDOM,
        hwAsic->isSupported(HwAsic::Feature::ARS_FUTURE_PORT_LOAD));
    return cfg;
  }

  void generateTestPrefixes(
      std::vector<RoutePrefixV6>& testPrefixes,
      std::vector<boost::container::flat_set<PortDescriptor>>& testNhopSets,
      int kTotalPrefixesNeeded) {
    generatePrefixes();

    testPrefixes = std::vector<RoutePrefixV6>{
        prefixes.begin(), prefixes.begin() + kTotalPrefixesNeeded};
    testNhopSets = std::vector<boost::container::flat_set<PortDescriptor>>{
        nhopSets.begin(), nhopSets.begin() + kTotalPrefixesNeeded};
  }

  void unprogramRoute(const std::vector<RoutePrefixV6>& prefixesToUnprogram) {
    auto wrapper = getSw()->getRouteUpdater();
    helper_->unprogramRoutes(&wrapper, prefixesToUnprogram);
  }

  void verifyPrefixes(
      const std::vector<RoutePrefixV6>& allPrefixes,
      int startIdx = 0,
      int endIdx = INT_MAX) {
    endIdx = std::min(endIdx, (int)allPrefixes.size());
    for (int i = startIdx; i < endIdx; i++) {
      const auto& prefix = allPrefixes[i];
      verifyFwdSwitchingMode(prefix, cfg::SwitchingMode::FLOWLET_QUALITY);
    }
  }

  void updatePortFlowletConfigName(cfg::SwitchConfig& cfg) const {
    AgentArsBase::updatePortFlowletConfigName(cfg);
  }
  bool supportsFuturePortLoad() const {
    auto hwAsic =
        checkSameAndGetAsicForTesting(getAgentEnsemble()->getL3Asics());
    return hwAsic->isSupported(HwAsic::Feature::ARS_FUTURE_PORT_LOAD);
  }

  cfg::FlowletSwitchingConfig getFlowletSwitchingConfig(
      cfg::SwitchingMode switchingMode,
      uint16_t inactivityIntervalUsecs,
      int flowletTableSize,
      int samplingRate) const {
    cfg::FlowletSwitchingConfig flowletCfg;
    flowletCfg.inactivityIntervalUsecs() = inactivityIntervalUsecs;
    flowletCfg.flowletTableSize() = flowletTableSize;
    flowletCfg.dynamicEgressLoadExponent() = 3;
    if (supportsFuturePortLoad()) {
      flowletCfg.dynamicQueueExponent() = 3;
    }
    flowletCfg.dynamicQueueMinThresholdBytes() = 100000;
    flowletCfg.dynamicQueueMaxThresholdBytes() = 200000;
    flowletCfg.dynamicSampleRate() = samplingRate;
    flowletCfg.dynamicEgressMinThresholdBytes() = 1000;
    flowletCfg.dynamicEgressMaxThresholdBytes() = 10000;
    flowletCfg.dynamicPhysicalQueueExponent() = 4;
    flowletCfg.maxLinks() = kMaxLinks;
    flowletCfg.switchingMode() = switchingMode;
    return flowletCfg;
  }

  // getPortFlowletConfig and updatePortFlowletConfigs are now inherited from
  // AgentArsBase

  void updateFlowletConfigs(
      cfg::SwitchConfig& cfg,
      const cfg::SwitchingMode switchingMode =
          cfg::SwitchingMode::FLOWLET_QUALITY,
      const int flowletTableSize = kFlowletTableSize1,
      const cfg::SwitchingMode backupSwitchingMode =
          cfg::SwitchingMode::FIXED_ASSIGNMENT) const {
    auto flowletCfg = getFlowletSwitchingConfig(
        switchingMode,
        kInactivityIntervalUsecs1,
        flowletTableSize,
        kDefaultSamplingRate());
    flowletCfg.backupSwitchingMode() = backupSwitchingMode;
    cfg.flowletSwitchingConfig() = flowletCfg;
    updatePortFlowletConfigs(
        cfg, kScalingFactor1(), kLoadWeight1, kQueueWeight1);
  }

  bool validateFlowSetTable(const bool expectFlowsetSizeZero) {
    AgentEnsemble* ensemble = getAgentEnsemble();
    auto switchId = SwitchID(0);
    auto client = ensemble->getHwAgentTestClient(switchId);
    return client->sync_validateFlowSetTable(expectFlowsetSizeZero);
  }

  // verifyEcmpForFlowletSwitching and verifyPortFlowletConfig are now inherited
  // from AgentArsBase

  void verifyConfig(
      const RoutePrefixV6& testPrefix,
      const cfg::FlowletSwitchingConfig& flowletConfig,
      const PortID& port,
      bool flowletEnable = true) {
    auto portFlowletConfig =
        getPortFlowletConfig(kScalingFactor1(), kLoadWeight1, kQueueWeight1);
    EXPECT_TRUE(verifyPortFlowletConfig(
        testPrefix.toCidrNetwork(), portFlowletConfig, port));

    EXPECT_TRUE(verifyEcmpForFlowletSwitching(
        testPrefix.toCidrNetwork(), flowletConfig, flowletEnable, port));
  }

  void modifyFlowletSwitchingConfig(cfg::SwitchConfig& cfg) const {
    auto flowletCfg = getFlowletSwitchingConfig(
        cfg::SwitchingMode::PER_PACKET_QUALITY,
        kInactivityIntervalUsecs1,
        kFlowletTableSize1,
        kMinSamplingRate());
    flowletCfg.backupSwitchingMode() = cfg::SwitchingMode::FIXED_ASSIGNMENT;
    cfg.flowletSwitchingConfig() = flowletCfg;
  }

  void verifyEcmpGroups(
      std::vector<RoutePrefixV6> testPrefixes,
      std::vector<boost::container::flat_set<PortDescriptor>> testNhopSets,
      const cfg::SwitchConfig& cfg,
      int numEcmp) {
    for (int i = 0; i < numEcmp; i++) {
      const auto& currentIp = testPrefixes[i];
      EXPECT_TRUE(verifyEcmpForFlowletSwitching(
          currentIp.toCidrNetwork(),
          *cfg.flowletSwitchingConfig(),
          true /* flowletEnable */,
          testNhopSets[i].begin()->phyPortID()));
    }
  }

  void setupEcmpGroups(
      std::vector<RoutePrefixV6>& testPrefixes,
      std::vector<boost::container::flat_set<PortDescriptor>>& testNhopSets,
      int numEcmp) {
    auto wrapper = getSw()->getRouteUpdater();
    generateTestPrefixes(testPrefixes, testNhopSets, numEcmp);
    helper_->programRoutes(&wrapper, testNhopSets, testPrefixes);
  }
  void flowletSwitchingWBHelper(
      std::vector<RoutePrefixV6>& testPrefixes,
      std::vector<boost::container::flat_set<PortDescriptor>>& testNhopSets,
      const cfg::SwitchingMode preMode,
      int preMaxFlows,
      int preEcmpScale,
      cfg::SwitchingMode postMode,
      int postMaxFlows,
      int postEcmpScale) {
    auto setup = [this,
                  &testPrefixes,
                  &testNhopSets,
                  preMode,
                  preMaxFlows,
                  preEcmpScale]() {
      auto cfg = AgentArsBase::initialConfig(*getAgentEnsemble());
      updateFlowletConfigs(cfg, preMode, preMaxFlows);
      updatePortFlowletConfigName(cfg);
      applyNewConfig(cfg);
      setupEcmpGroups(testPrefixes, testNhopSets, preEcmpScale);
    };

    auto verify = [this,
                   &testPrefixes,
                   &testNhopSets,
                   preMode,
                   preMaxFlows,
                   preEcmpScale]() {
      auto cfg = AgentArsBase::initialConfig(*getAgentEnsemble());
      updateFlowletConfigs(cfg, preMode, preMaxFlows);
      updatePortFlowletConfigName(cfg);
      verifyEcmpGroups(testPrefixes, testNhopSets, cfg, preEcmpScale);
    };

    auto setupPostWarmboot = [this,
                              &testPrefixes,
                              &testNhopSets,
                              postMode,
                              postMaxFlows,
                              postEcmpScale]() {
      auto cfg = AgentArsBase::initialConfig(*getAgentEnsemble());
      updateFlowletConfigs(cfg, postMode, postMaxFlows);
      updatePortFlowletConfigName(cfg);
      applyNewConfig(cfg);
      setupEcmpGroups(testPrefixes, testNhopSets, postEcmpScale);
    };

    auto verifyPostWarmboot = [this,
                               &testPrefixes,
                               &testNhopSets,
                               postMode,
                               postMaxFlows,
                               postEcmpScale]() {
      auto cfg = AgentArsBase::initialConfig(*getAgentEnsemble());
      updateFlowletConfigs(cfg, postMode, postMaxFlows);
      updatePortFlowletConfigName(cfg);
      verifyEcmpGroups(testPrefixes, testNhopSets, cfg, postEcmpScale);
    };

    verifyAcrossWarmBoots(setup, verify, setupPostWarmboot, verifyPostWarmboot);
  }

  std::unique_ptr<utility::EcmpSetupAnyNPorts<folly::IPAddressV6>> ecmpHelper_;
};

/**
 * @brief Test flowset table recovery when ECMP objects are removed
 *
 * This test simulates a real-world scenario where multiple ECMP objects are
 * created and the flowset table becomes full. It ensures that when the first
 * ECMP object is removed, sufficient space is created for subsequent objects
 * to insert themselves properly.
 *
 * @details The test verifies the system's ability to recover from flowset
 * table exhaustion by removing entries and allowing new ones to be inserted.
 */
TEST_F(AgentArsFlowletTest, ValidateFlowsetExceedForceFix) {
  std::vector<RoutePrefixV6> testPrefixes;
  std::vector<boost::container::flat_set<PortDescriptor>> testNhopSets;

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

/**
 * @brief Test flowset table size limit handling and recovery
 *
 * This test exercises the scenario where the flowset table runs out of space
 * due to excessive size requirements.
 *
 * @details Test procedure:
 * 1. Ensure that ECMP object is created but operates in non-dynamic mode
 *    when flowset table is exhausted
 * 2. Once the size is corrected through configuration changes, verify that
 *    the system transitions back to dynamic mode
 *
 * @note This validates the system's graceful degradation and recovery
 * capabilities when hardware resources are constrained.
 */
TEST_F(AgentArsFlowletTest, ValidateFlowsetExceed) {
  std::vector<RoutePrefixV6> testPrefixes;
  std::vector<boost::container::flat_set<PortDescriptor>> testNhopSets;
  generateTestPrefixes(testPrefixes, testNhopSets, kMaxLinks);

  auto setup = [=, this]() {
    auto cfg = initialConfig(*getAgentEnsemble());
    applyNewConfig(cfg);
    // Program the prefixes
    auto wrapper = getSw()->getRouteUpdater();
    helper_->programRoutes(&wrapper, testNhopSets, testPrefixes);
    XLOG(INFO) << "Programmed " << testPrefixes.size() << " prefixes across "
               << testNhopSets.size() << " ECMP groups";
  };
  auto verify = [=, this] {
    auto cfg = initialConfig(*getAgentEnsemble());
    updateFlowletConfigs(
        cfg,
        cfg::SwitchingMode::FLOWLET_QUALITY,
        KMaxFlowsetTableSize + 1,
        cfg::SwitchingMode::FIXED_ASSIGNMENT);
    applyNewConfig(cfg);
    initialConfig(*getAgentEnsemble());
    auto port = testNhopSets[0].begin()->phyPortID();
    EXPECT_FALSE(verifyEcmpForFlowletSwitching(
        testPrefixes[0].toCidrNetwork(),
        *cfg.flowletSwitchingConfig(),
        true,
        port));

    // Now modify the config to go back to dynamic mode
    updateFlowletConfigs(
        cfg,
        cfg::SwitchingMode::FLOWLET_QUALITY,
        KMaxFlowsetTableSize,
        cfg::SwitchingMode::FIXED_ASSIGNMENT);
    updatePortFlowletConfigName(cfg);
    applyNewConfig(cfg);

    // Verify that the prefix is now in dynamic mode
    EXPECT_TRUE(verifyEcmpForFlowletSwitching(
        testPrefixes[0].toCidrNetwork(),
        *cfg.flowletSwitchingConfig(),
        true,
        port));
    EXPECT_TRUE(validateFlowSetTable(true));
  };
  verifyAcrossWarmBoots(setup, verify);
}

/**
 * @brief Test flowset table size limit handling. 16 is the limit.
 */

TEST_F(AgentArsFlowletTest, ValidateFlowsetTableFull) {
  std::vector<RoutePrefixV6> testPrefixes;
  std::vector<boost::container::flat_set<PortDescriptor>> testNhopSets;
  generateTestPrefixes(testPrefixes, testNhopSets, 16);

  auto setup = [=, this]() {
    auto wrapper = getSw()->getRouteUpdater();
    helper_->programRoutes(&wrapper, testNhopSets, testPrefixes);
    XLOG(INFO) << "Programmed " << testPrefixes.size() << " prefixes across "
               << testNhopSets.size() << " ECMP groups";
  };
  auto verify = [=, this] {
    // Verify remaining prefixes (skip the first one that was unprogrammed)
    verifyPrefixes(testPrefixes);
    EXPECT_TRUE(validateFlowSetTable(true));
  };
  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(AgentArsFlowletTest, VerifyFlowletConfigChange) {
  std::vector<RoutePrefixV6> testPrefixes;
  std::vector<boost::container::flat_set<PortDescriptor>> testNhopSets;
  generateTestPrefixes(testPrefixes, testNhopSets, kMaxLinks);

  auto setup = [=, this]() {
    auto wrapper = getSw()->getRouteUpdater();
    helper_->programRoutes(&wrapper, testNhopSets, testPrefixes);
    XLOG(INFO) << "Programmed " << testPrefixes.size() << " prefixes across "
               << testNhopSets.size() << " ECMP groups";
  };

  auto verify = [&]() {
    // switchingMode starts out as FLOWLET_QUALITY
    auto cfg = initialConfig(*getAgentEnsemble());
    auto port = testNhopSets[0].begin()->phyPortID();
    verifyConfig(testPrefixes[0], *cfg.flowletSwitchingConfig(), port);
    XLOG(INFO) << "Verified config change";
    // Modify the flowlet config switching mode to PER_PACKET_QUALITY
    modifyFlowletSwitchingConfig(cfg);
    // Modify the port flowlet config
    updatePortFlowletConfigs(
        cfg, kScalingFactor2(), kLoadWeight2, kQueueWeight2);
    applyNewConfig(cfg);
    verifyConfig(testPrefixes[0], *cfg.flowletSwitchingConfig(), port);
    XLOG(INFO) << "Verified modified config change";
    cfg = initialConfig(*getAgentEnsemble());
    applyNewConfig(cfg);
    verifyConfig(prefixes[0], *cfg.flowletSwitchingConfig(), port);
    XLOG(INFO) << "Verified config change";
  };

  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(AgentArsFlowletTest, VerifyFlowletConfigRemoval) {
  std::vector<RoutePrefixV6> testPrefixes;
  std::vector<boost::container::flat_set<PortDescriptor>> testNhopSets;
  generateTestPrefixes(testPrefixes, testNhopSets, kMaxLinks);

  auto setup = [=, this]() {
    auto wrapper = getSw()->getRouteUpdater();
    helper_->programRoutes(&wrapper, testNhopSets, testPrefixes);
    XLOG(INFO) << "Programmed " << testPrefixes.size() << " prefixes across "
               << testNhopSets.size() << " ECMP groups";
  };

  auto verify = [&]() {
    // switchingMode starts out as FLOWLET_QUALITY
    auto cfg = initialConfig(*getAgentEnsemble());
    auto port = testNhopSets[0].begin()->phyPortID();
    verifyConfig(testPrefixes[0], *cfg.flowletSwitchingConfig(), port);
    XLOG(INFO) << "Verified inital config";
    // Modify the flowlet config
    modifyFlowletSwitchingConfig(cfg);
    // Modify the port flowlet config
    updatePortFlowletConfigs(
        cfg, kScalingFactor2(), kLoadWeight2, kQueueWeight2);
    applyNewConfig(cfg);
    verifyConfig(testPrefixes[0], *cfg.flowletSwitchingConfig(), port);
    XLOG(INFO) << "Verified modified config";
    // Remove the flowlet configs
    auto removedCfg = AgentArsBase::initialConfig(*getAgentEnsemble());
    auto expectedFlowletSetting = getFlowletSwitchingConfig(
        cfg::SwitchingMode::FIXED_ASSIGNMENT, 0, 0, 0);
    applyNewConfig(removedCfg);
    XLOG(INFO) << "Applied removed config";
    verifyConfig(testPrefixes[0], expectedFlowletSetting, port, false);
    XLOG(INFO) << "Verified config removal";
    // Modify to initial config to verify after warmboot applyNewConfig(
    applyNewConfig(initialConfig(*getAgentEnsemble()));
  };

  verifyAcrossWarmBoots(setup, verify);
}

/*
 * This test setup static ECMP and update the static ECMP to DLB ECMP and revert
 * the DLB ECMP to static ECMP and verify it.
 */
TEST_F(AgentArsFlowletTest, VerifyEcmpFlowletSwitchingEnable) {
  std::vector<RoutePrefixV6> testPrefixes;
  std::vector<boost::container::flat_set<PortDescriptor>> testNhopSets;
  generateTestPrefixes(testPrefixes, testNhopSets, kMaxLinks);

  auto setup = [=, this]() {
    auto wrapper = getSw()->getRouteUpdater();
    helper_->programRoutes(&wrapper, testNhopSets, testPrefixes);

    XLOG(INFO) << "Programmed " << testPrefixes.size() << " prefixes across "
               << testNhopSets.size() << " ECMP groups";
  };

  auto verify = [&]() {
    auto expectedFlowletSetting = getFlowletSwitchingConfig(
        cfg::SwitchingMode::FIXED_ASSIGNMENT, 0, 0, 0);
    auto port = testNhopSets[0].begin()->phyPortID();
    // 1. setup static ECMP
    auto cfg = AgentArsBase::initialConfig(*getAgentEnsemble());
    applyNewConfig(cfg);
    verifyConfig(testPrefixes[0], expectedFlowletSetting, port, false);
    XLOG(INFO) << "Verified inital config";
    // 2. update the static ECMP to DLB ECMP
    updateFlowletConfigs(cfg);
    updatePortFlowletConfigName(cfg);
    applyNewConfig(cfg);
    verifyConfig(
        testPrefixes[1],
        *cfg.flowletSwitchingConfig(),
        testNhopSets[1].begin()->phyPortID());
    XLOG(INFO) << "Verified modified config";
    // 3. revert the DLB ECMP to static ECMP
    auto defaultCfg = AgentArsBase::initialConfig(*getAgentEnsemble());

    applyNewConfig(defaultCfg);
    verifyConfig(testPrefixes[0], expectedFlowletSetting, port, false);
    XLOG(INFO) << "Verified config removal";
  };
  verifyAcrossWarmBoots(setup, verify);
}

/*
 * This test verifies ability to configure ECMP groups without port config
 */

TEST_F(AgentArsFlowletTest, VerifySkipEcmpFlowletSwitchingEnable) {
  std::vector<RoutePrefixV6> testPrefixes;
  std::vector<boost::container::flat_set<PortDescriptor>> testNhopSets;
  generateTestPrefixes(testPrefixes, testNhopSets, kMaxLinks);

  auto setup = [=, this]() {
    auto wrapper = getSw()->getRouteUpdater();
    helper_->programRoutes(&wrapper, testNhopSets, testPrefixes);

    XLOG(INFO) << "Programmed " << testPrefixes.size() << " prefixes across "
               << testNhopSets.size() << " ECMP groups";

    auto cfg = AgentArsBase::initialConfig(*getAgentEnsemble());
    updateFlowletConfigs(cfg);
    applyNewConfig(cfg);
  };

  auto verify = [&]() {
    auto cfg = AgentArsBase::initialConfig(*getAgentEnsemble());
    updateFlowletConfigs(cfg);
    applyNewConfig(cfg);
    verifyConfig(
        testPrefixes[0],
        *cfg.flowletSwitchingConfig(),
        testNhopSets[0].begin()->phyPortID());
  };
  verifyAcrossWarmBoots(setup, verify);
}
} // namespace facebook::fboss
