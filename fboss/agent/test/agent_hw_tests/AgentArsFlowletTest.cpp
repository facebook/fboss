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
    return getAgentEnsemble()->isSai() ? 1 : 500000;
  }

  int kScalingFactor1() const {
    return getAgentEnsemble()->isSai() ? 10 : 100;
  }
  int kScalingFactor2() const {
    return getAgentEnsemble()->isSai() ? 20 : 200;
  }

  int kMinSamplingRate() const {
    auto asic = checkSameAndGetAsic(getAgentEnsemble()->getL3Asics());

    bool isTH4 = asic->getAsicType() == cfg::AsicType::ASIC_TYPE_TOMAHAWK4;
    return getAgentEnsemble()->isSai() ? 1 : isTH4 ? 1953125 : 1000000;
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

  void generateTestPrefixes(
      std::vector<RoutePrefixV6>& testPrefixes,
      std::vector<flat_set<PortDescriptor>>& testNhopSets,
      int kTotalPrefixesNeeded) {
    generatePrefixes();

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
      int startIdx = 0,
      int endIdx = INT_MAX) {
    endIdx = std::min(endIdx, (int)allPrefixes.size());
    for (int i = startIdx; i < endIdx; i++) {
      const auto& prefix = allPrefixes[i];
      verifyFwdSwitchingMode(prefix, cfg::SwitchingMode::FLOWLET_QUALITY);
    }
  }

  void updatePortFlowletConfigName(cfg::SwitchConfig& cfg) const {
    auto allPorts = masterLogicalInterfacePortIds();
    std::vector<PortID> ports(
        allPorts.begin(), allPorts.begin() + allPorts.size());
    for (const auto& portId : ports) {
      auto portCfg = utility::findCfgPort(cfg, portId);
      portCfg->flowletConfigName() = "default";
    }
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
    flowletCfg.dynamicQueueExponent() = 3;
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

  cfg::PortFlowletConfig getPortFlowletConfig(
      int scalingFactor,
      int loadWeight,
      int queueWeight) const {
    cfg::PortFlowletConfig portFlowletConfig;
    portFlowletConfig.scalingFactor() = scalingFactor;
    portFlowletConfig.loadWeight() = loadWeight;
    portFlowletConfig.queueWeight() = queueWeight;
    return portFlowletConfig;
  }

  void updatePortFlowletConfigs(
      cfg::SwitchConfig& cfg,
      int scalingFactor,
      int loadWeight,
      int queueWeight) const {
    std::map<std::string, cfg::PortFlowletConfig> portFlowletCfgMap;
    auto portFlowletConfig =
        getPortFlowletConfig(scalingFactor, loadWeight, queueWeight);
    portFlowletCfgMap.insert(std::make_pair("default", portFlowletConfig));
    cfg.portFlowletConfigs() = portFlowletCfgMap;
  }

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

  bool verifyEcmpForFlowletSwitching(
      const folly::CIDRNetwork& ip,
      const cfg::FlowletSwitchingConfig& flowletCfg,
      const bool flowletEnable) {
    AgentEnsemble* ensemble = getAgentEnsemble();
    const auto port = ensemble->masterLogicalPortIds()[0];
    auto switchId = ensemble->scopeResolver().scope(port).switchId();
    auto client = ensemble->getHwAgentTestClient(switchId);
    facebook::fboss::utility::CIDRNetwork cidr;
    cidr.IPAddress() = ip.first.str();
    cidr.mask() = ip.second;
    state::SwitchSettingsFields settings;
    settings.flowletSwitchingConfig() = flowletCfg;
    return client->sync_verifyEcmpForFlowletSwitchingHandler(
        cidr, settings, flowletEnable);
  }

  bool verifyPortFlowletConfig(
      const folly::CIDRNetwork& ip,
      cfg::PortFlowletConfig& portFlowletConfig) {
    AgentEnsemble* ensemble = getAgentEnsemble();
    auto switchId = SwitchID(0);
    auto client = ensemble->getHwAgentTestClient(switchId);

    facebook::fboss::utility::CIDRNetwork cidr;
    cidr.IPAddress() = ip.first.str();
    cidr.mask() = ip.second;

    return client->sync_verifyPortFlowletConfig(cidr, portFlowletConfig, true);
  }

  void verifyConfig(
      const RoutePrefixV6& testPrefix,
      const cfg::FlowletSwitchingConfig& flowletConfig,
      bool flowletEnable = true) {
    auto portFlowletConfig =
        getPortFlowletConfig(kScalingFactor1(), kLoadWeight1, kQueueWeight1);
    EXPECT_TRUE(
        verifyPortFlowletConfig(testPrefix.toCidrNetwork(), portFlowletConfig));

    EXPECT_TRUE(verifyEcmpForFlowletSwitching(
        testPrefix.toCidrNetwork(), flowletConfig, flowletEnable));
  }

  void modifyFlowletSwitchingConfig(cfg::SwitchConfig& cfg) const {
    auto flowletCfg = getFlowletSwitchingConfig(
        cfg::SwitchingMode::PER_PACKET_QUALITY,
        kInactivityIntervalUsecs1,
        kFlowletTableSize1,
        kDefaultSamplingRate());
    flowletCfg.backupSwitchingMode() = cfg::SwitchingMode::FIXED_ASSIGNMENT;
    cfg.flowletSwitchingConfig() = flowletCfg;
  }
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
  std::vector<flat_set<PortDescriptor>> testNhopSets;
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
    EXPECT_FALSE(verifyEcmpForFlowletSwitching(
        testPrefixes[0].toCidrNetwork(), *cfg.flowletSwitchingConfig(), true));

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
        testPrefixes[0].toCidrNetwork(), *cfg.flowletSwitchingConfig(), true));
    EXPECT_TRUE(validateFlowSetTable(true));
  };
  verifyAcrossWarmBoots(setup, verify);
}

/**
 * @brief Test flowset table size limit handling. 16 is the limit.
 */

TEST_F(AgentArsFlowletTest, ValidateFlowsetTableFull) {
  std::vector<RoutePrefixV6> testPrefixes;
  std::vector<flat_set<PortDescriptor>> testNhopSets;
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
  std::vector<flat_set<PortDescriptor>> testNhopSets;
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
    verifyConfig(testPrefixes[0], *cfg.flowletSwitchingConfig());
    XLOG(INFO) << "Verified config change";
    // Modify the flowlet config switching mode to PER_PACKET_QUALITY
    modifyFlowletSwitchingConfig(cfg);
    // Modify the port flowlet config
    updatePortFlowletConfigs(
        cfg, kScalingFactor2(), kLoadWeight2, kQueueWeight2);
    applyNewConfig(cfg);
    verifyConfig(testPrefixes[0], *cfg.flowletSwitchingConfig());
    XLOG(INFO) << "Verified modified config change";
    // Modify to initial config to verify after warmboot
    // switching mode back to FLOWLET_QUALITY
    cfg = initialConfig(*getAgentEnsemble());
    applyNewConfig(cfg);
    verifyConfig(prefixes[0], *cfg.flowletSwitchingConfig());
    XLOG(INFO) << "Verified config change";
  };

  verifyAcrossWarmBoots(setup, verify);
}

/*
 * This test verifies ability to configure ECMP groups without port config
 */

TEST_F(AgentArsFlowletTest, VerifySkipEcmpFlowletSwitchingEnable) {
  std::vector<RoutePrefixV6> testPrefixes;
  std::vector<flat_set<PortDescriptor>> testNhopSets;
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
    verifyConfig(testPrefixes[0], *cfg.flowletSwitchingConfig());
  };
  verifyAcrossWarmBoots(setup, verify);
}
} // namespace facebook::fboss
