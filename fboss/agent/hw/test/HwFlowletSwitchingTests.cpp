/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include <folly/IPAddress.h>

#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/hw/test/HwLinkStateDependentTest.h"
#include "fboss/agent/hw/test/HwTestAclUtils.h"
#include "fboss/agent/hw/test/HwTestFlowletSwitchingUtils.h"
#include "fboss/agent/hw/test/LoadBalancerUtils.h"
#include "fboss/agent/hw/test/ProdConfigFactory.h"
#include "fboss/agent/test/EcmpSetupHelper.h"

using namespace facebook::fboss::utility;

namespace {
static folly::IPAddressV6 kAddr1{"2803:6080:d038:3063::"};
static folly::IPAddressV6 kAddr2{"2803:6080:d038:3000::"};
folly::CIDRNetwork kAddr1Prefix{folly::IPAddress("2803:6080:d038:3063::"), 64};
folly::CIDRNetwork kAddr2Prefix{folly::IPAddress("2803:6080:d038:3000::"), 64};
constexpr auto kAddr3 = "2803:6080:d048";
constexpr auto kAddr4 = "2803:6080:e048";
const int kDefaultScalingFactor = 10;
const int kScalingFactor1 = 100;
const int kScalingFactor2 = 200;
const int kDefaultLoadWeight = 100;
const int kLoadWeight1 = 70;
const int kLoadWeight2 = 60;
const int kDefaultQueueWeight = 0;
const int kQueueWeight1 = 30;
const int kQueueWeight2 = 40;
const int kInactivityIntervalUsecs1 = 128;
const int kInactivityIntervalUsecs2 = 256;
const int kFlowletTableSize1 = 1024;
const int kFlowletTableSize2 = 2048;
const int kMinFlowletTableSize = 256;
static int kUdpProto(17);
static int kUdpDstPort(4791);
constexpr auto kAclName = "flowlet";
constexpr auto kAclCounterName = "flowletStat";
constexpr auto kDstIp = "2001::/16";
const int kMaxLinks = 4;
const int kEcmpStartId = 200000;
const int kNumEcmp = 64;
} // namespace

namespace facebook::fboss {

class HwFlowletSwitchingTest : public HwLinkStateDependentTest {
 protected:
  void SetUp() override {
    FLAGS_flowletSwitchingEnable = true;
    FLAGS_flowletStatsEnable = true;
    HwLinkStateDependentTest::SetUp();
    ecmpHelper_ = std::make_unique<utility::EcmpSetupAnyNPorts6>(
        getProgrammedState(), RouterID(0));
  }

  cfg::SwitchConfig initialConfig() const override {
    auto cfg = getDefaultConfig();
    updateFlowletConfigs(cfg);
    updatePortFlowletConfigName(cfg);
    return cfg;
  }

  void resolveNextHopsAddRoute(const int linkCount) {
    for (int i = 0; i < linkCount; ++i) {
      this->resolveNextHop(PortDescriptor(masterLogicalPortIds()[i]));
    }
    this->addRoute(kAddr1, 64);
  }

  void resolveNextHopsAddRoute(
      std::vector<PortID> masterLogicalPortsIds,
      const folly::IPAddressV6 addr) {
    std::vector<PortDescriptor> portDescriptorIds;
    for (auto& portId : masterLogicalPortsIds) {
      this->resolveNextHop(PortDescriptor(portId));
      portDescriptorIds.push_back(PortDescriptor(portId));
    }
    this->addRoute(addr, 64, portDescriptorIds);
  }

  void setupEcmpGroups(int numEcmp) {
    for (int i = 1; i <= numEcmp; i++) {
      std::vector<PortID> portIds;
      portIds.push_back(masterLogicalPortIds()[i % 64]);
      portIds.push_back(masterLogicalPortIds()[(i + 1) % 64]);
      resolveNextHopsAddRoute(
          std::move(portIds),
          folly::IPAddressV6(folly::sformat("{}:{:x}::", kAddr3, i)));
      std::vector<PortID> portIds2;
      portIds2.push_back(masterLogicalPortIds()[i % 64]);
      portIds2.push_back(masterLogicalPortIds()[(i + 2) % 64]);
      resolveNextHopsAddRoute(
          std::move(portIds2),
          folly::IPAddressV6(folly::sformat("{}:{:x}::", kAddr4, i)));
    }
  }

  void verifyEcmpGroups(const cfg::SwitchConfig& cfg, int numEcmp) {
    auto portFlowletConfig =
        getPortFlowletConfig(kScalingFactor1, kLoadWeight1, kQueueWeight1);
    for (int i = 1; i < numEcmp; i++) {
      auto currentIp1 =
          folly::IPAddress(folly::sformat("{}:{:x}::", kAddr3, i));
      folly::CIDRNetwork currentPrefix1{currentIp1, 64};
      EXPECT_TRUE(utility::verifyEcmpForFlowletSwitching(
          getHwSwitch(),
          currentPrefix1,
          *cfg.flowletSwitchingConfig(),
          portFlowletConfig,
          true /* flowletEnable */));
      auto currentIp2 =
          folly::IPAddress(folly::sformat("{}:{:x}::", kAddr4, i));
      folly::CIDRNetwork currentPrefix2{currentIp2, 64};
      EXPECT_TRUE(utility::verifyEcmpForFlowletSwitching(
          getHwSwitch(),
          currentPrefix2,
          *cfg.flowletSwitchingConfig(),
          portFlowletConfig,
          true /* flowletEnable */));
    }
  }

  void addFlowletAcl(cfg::SwitchConfig& cfg) const {
    auto* acl = utility::addAcl(&cfg, kAclName);
    acl->proto() = kUdpProto;
    acl->l4DstPort() = kUdpDstPort;
    acl->dstIp() = kDstIp;
    acl->udfGroups() = {utility::kRoceUdfFlowletGroupName};
    acl->roceBytes() = {utility::kRoceReserved};
    acl->roceMask() = {utility::kRoceReserved};
    cfg::MatchAction matchAction = cfg::MatchAction();
    matchAction.flowletAction() = cfg::FlowletAction::FORWARD;
    // presence of UDF configuration causes the FP group to be cleared out
    // completely and re-populates the table again. It has a bug where counters
    // are not cleared before the group is destroyed. The specific method is
    // clearFPGroup. This only affects fake SDK. So leaving counter creation for
    // fake SDK out until the issue is appropriately resolved.
    if (getPlatform()->getAsic()->getAsicType() !=
        cfg::AsicType::ASIC_TYPE_FAKE) {
      matchAction.counter() = kAclCounterName;
      std::vector<cfg::CounterType> counterTypes{
          cfg::CounterType::PACKETS, cfg::CounterType::BYTES};
      auto counter = cfg::TrafficCounter();
      *counter.name() = kAclCounterName;
      *counter.types() = counterTypes;
      cfg.trafficCounters()->push_back(counter);
    }
    utility::addMatcher(&cfg, kAclName, matchAction);
  }

  void updatePortFlowletConfigName(cfg::SwitchConfig& cfg) const {
    auto allPorts = masterLogicalInterfacePortIds();
    std::vector<PortID> ports(
        allPorts.begin(), allPorts.begin() + allPorts.size());
    for (auto portId : ports) {
      auto portCfg = utility::findCfgPort(cfg, portId);
      portCfg->flowletConfigName() = "default";
    }
  }

  void updateFlowletConfigs(
      cfg::SwitchConfig& cfg,
      const cfg::SwitchingMode switchingMode =
          cfg::SwitchingMode::FLOWLET_QUALITY,
      const int flowletTableSize = kFlowletTableSize1) const {
    auto flowletCfg = getFlowletSwitchingConfig(
        switchingMode, kInactivityIntervalUsecs1, flowletTableSize);
    cfg.flowletSwitchingConfig() = flowletCfg;
    updatePortFlowletConfigs(cfg, kScalingFactor1, kLoadWeight1, kQueueWeight1);
    cfg.udfConfig() = utility::addUdfFlowletAclConfig();
    addFlowletAcl(cfg);
  }

  cfg::SwitchConfig getDefaultConfig() const {
    auto cfg = utility::onePortPerInterfaceConfig(
        getHwSwitch(),
        masterLogicalPortIds(),
        getAsic()->desiredLoopbackModes());
    addLoadBalancerToConfig(cfg, getHwSwitch(), LBHash::FULL_HASH);
    return cfg;
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

  cfg::FlowletSwitchingConfig getFlowletSwitchingConfig(
      cfg::SwitchingMode switchingMode,
      uint16_t inactivityIntervalUsecs,
      int flowletTableSize) const {
    cfg::FlowletSwitchingConfig flowletCfg;
    flowletCfg.inactivityIntervalUsecs() = inactivityIntervalUsecs;
    flowletCfg.flowletTableSize() = flowletTableSize;
    flowletCfg.dynamicEgressLoadExponent() = 3;
    flowletCfg.dynamicQueueExponent() = 3;
    flowletCfg.dynamicQueueMinThresholdBytes() = 100000;
    flowletCfg.dynamicQueueMaxThresholdBytes() = 200000;
    flowletCfg.dynamicSampleRate() = 100000;
    flowletCfg.dynamicEgressMinThresholdBytes() = 1000;
    flowletCfg.dynamicEgressMaxThresholdBytes() = 10000;
    flowletCfg.dynamicPhysicalQueueExponent() = 3;
    flowletCfg.maxLinks() = kMaxLinks;
    flowletCfg.switchingMode() = switchingMode;
    return flowletCfg;
  }

  void modifyFlowletSwitchingConfig(cfg::SwitchConfig& cfg) const {
    auto flowletCfg = getFlowletSwitchingConfig(
        cfg::SwitchingMode::PER_PACKET_QUALITY,
        kInactivityIntervalUsecs2,
        kMinFlowletTableSize);
    cfg.flowletSwitchingConfig() = flowletCfg;
  }

  void modifyFlowletSwitchingMaxLinks(
      cfg::SwitchConfig& cfg,
      const int maxLinks) const {
    if (cfg.flowletSwitchingConfig().has_value()) {
      cfg.flowletSwitchingConfig()->maxLinks() = maxLinks;
    }
  }

  bool skipTest() {
    return (
        !getPlatform()->getAsic()->isSupported(HwAsic::Feature::FLOWLET) ||
        (getPlatform()->getAsic()->getAsicType() ==
         cfg::AsicType::ASIC_TYPE_FAKE));
  }

  std::vector<NextHopWeight> swSwitchWeights_ = {
      ECMP_WEIGHT,
      ECMP_WEIGHT,
      ECMP_WEIGHT,
      ECMP_WEIGHT};

  void resolveNextHop(PortDescriptor port) {
    applyNewState(ecmpHelper_->resolveNextHops(
        getProgrammedState(),
        {
            port,
        }));
  }

  void unresolveNextHop(PortDescriptor port) {
    applyNewState(ecmpHelper_->unresolveNextHops(getProgrammedState(), {port}));
  }

  void addRoute(folly::IPAddressV6 prefix, uint8_t mask) {
    auto nHops = swSwitchWeights_.size();
    ecmpHelper_->programRoutes(
        getRouteUpdater(),
        nHops,
        {RoutePrefixV6{prefix, mask}},
        std::vector<NextHopWeight>(
            swSwitchWeights_.begin(), swSwitchWeights_.begin() + nHops));
    applyNewState(ecmpHelper_->resolveNextHops(getProgrammedState(), nHops));
  }

  void addRoute(
      const folly::IPAddressV6 prefix,
      uint8_t mask,
      const std::vector<PortDescriptor>& ports) {
    ecmpHelper_->programRoutes(
        getRouteUpdater(),
        flat_set<PortDescriptor>(
            std::make_move_iterator(ports.begin()),
            std::make_move_iterator(ports.begin() + ports.size())),
        {RoutePrefixV6{prefix, mask}});
  }

  void verifyPortFlowletConfig(cfg::PortFlowletConfig& portFlowletConfig) {
    if (getHwSwitch()->getPlatform()->getAsic()->isSupported(
            HwAsic::Feature::FLOWLET_PORT_ATTRIBUTES)) {
      EXPECT_TRUE(utility::validatePortFlowletQuality(
          getHwSwitch(), masterLogicalPortIds()[0], portFlowletConfig));
    }
  }

  void verifyConfig(
      const cfg::SwitchConfig& cfg,
      bool expectFlowsetSizeZero = false) {
    auto portFlowletConfig =
        getPortFlowletConfig(kScalingFactor1, kLoadWeight1, kQueueWeight1);
    verifyPortFlowletConfig(portFlowletConfig);

    EXPECT_TRUE(utility::validateFlowletSwitchingEnabled(
        getHwSwitch(), *cfg.flowletSwitchingConfig()));
    EXPECT_TRUE(utility::verifyEcmpForFlowletSwitching(
        getHwSwitch(),
        kAddr1Prefix,
        *cfg.flowletSwitchingConfig(),
        portFlowletConfig,
        true));

    utility::checkSwHwAclMatch(getHwSwitch(), getProgrammedState(), kAclName);
    std::vector<cfg::CounterType> counterTypes{
        cfg::CounterType::PACKETS, cfg::CounterType::BYTES};
    utility::checkAclStat(
        getHwSwitch(),
        getProgrammedState(),
        {kAclName},
        kAclCounterName,
        counterTypes);
  }

  void verifyModifiedConfig() {
    auto portFlowletConfig =
        getPortFlowletConfig(kScalingFactor2, kLoadWeight2, kQueueWeight2);
    verifyPortFlowletConfig(portFlowletConfig);

    auto flowletCfg = getFlowletSwitchingConfig(
        cfg::SwitchingMode::PER_PACKET_QUALITY,
        kInactivityIntervalUsecs2,
        kMinFlowletTableSize);

    EXPECT_TRUE(
        utility::validateFlowletSwitchingEnabled(getHwSwitch(), flowletCfg));
    EXPECT_TRUE(utility::verifyEcmpForFlowletSwitching(
        getHwSwitch(), kAddr1Prefix, flowletCfg, portFlowletConfig, true));
  }

  void verifyRemoveFlowletConfig() {
    auto portFlowletConfig = getPortFlowletConfig(
        kDefaultScalingFactor, kDefaultLoadWeight, kDefaultQueueWeight);
    verifyPortFlowletConfig(portFlowletConfig);

    auto flowletCfg =
        getFlowletSwitchingConfig(cfg::SwitchingMode::FIXED_ASSIGNMENT, 0, 0);

    EXPECT_TRUE(utility::validateFlowletSwitchingDisabled(getHwSwitch()));
    EXPECT_TRUE(utility::verifyEcmpForFlowletSwitching(
        getHwSwitch(), kAddr1Prefix, flowletCfg, portFlowletConfig, false));
  }

  void validateEcmpDetails(const cfg::FlowletSwitchingConfig& flowletCfg) {
    auto ecmpDetails = getHwSwitch()->getAllEcmpDetails();
    CHECK_EQ(ecmpDetails.size(), 1);
    for (const auto& entry : ecmpDetails) {
      CHECK_GE(*(entry.ecmpId()), kEcmpStartId);
      if (*flowletCfg.flowletTableSize() > 0) {
        EXPECT_TRUE(*(entry.flowletEnabled()));
      }
    }
  }

  void verifySkipEcmpConfig(
      const cfg::SwitchConfig& cfg,
      bool expectFlowsetSizeZero = false) {
    // Port flowlet config is not present in the config
    // verify that ports are not programmed with port flowlet config in TH4
    auto portFlowletConfig = getPortFlowletConfig(
        kDefaultScalingFactor, kDefaultLoadWeight, kDefaultQueueWeight);
    verifyPortFlowletConfig(portFlowletConfig);

    EXPECT_TRUE(utility::validateFlowletSwitchingEnabled(
        getHwSwitch(), *cfg.flowletSwitchingConfig()));

    if (getHwSwitch()->getPlatform()->getAsic()->isSupported(
            HwAsic::Feature::FLOWLET_PORT_ATTRIBUTES)) {
      // verify the flowlet config is programmed in ECMP for TH4
      EXPECT_TRUE(utility::verifyEcmpForFlowletSwitching(
          getHwSwitch(),
          kAddr1Prefix,
          *cfg.flowletSwitchingConfig(),
          portFlowletConfig,
          true));
    } else {
      // verify the flowlet config is not programmed in ECMP for TH3
      auto flowletCfg =
          getFlowletSwitchingConfig(cfg::SwitchingMode::FIXED_ASSIGNMENT, 0, 0);
      EXPECT_TRUE(utility::verifyEcmpForFlowletSwitching(
          getHwSwitch(), kAddr1Prefix, flowletCfg, portFlowletConfig, false));
    }

    utility::checkSwHwAclMatch(getHwSwitch(), getProgrammedState(), kAclName);
    std::vector<cfg::CounterType> counterTypes{
        cfg::CounterType::PACKETS, cfg::CounterType::BYTES};
    utility::checkAclStat(
        getHwSwitch(),
        getProgrammedState(),
        {kAclName},
        kAclCounterName,
        counterTypes);
  }

  std::unique_ptr<utility::EcmpSetupAnyNPorts<folly::IPAddressV6>> ecmpHelper_;
};

class HwEcmpFlowletSwitchingTest : public HwFlowletSwitchingTest {
 protected:
  void SetUp() override {
    HwFlowletSwitchingTest::SetUp();
  }

  cfg::SwitchConfig initialConfig() const override {
    auto cfg = getDefaultConfig();
    return cfg;
  }

  void flowletSwitchingWBHelper(
      cfg::SwitchingMode preMode,
      int preMaxFlows,
      int preEcmpScale,
      cfg::SwitchingMode postMode,
      int postMaxFlows,
      int postEcmpScale) {
    auto setup = [this, preMode, preMaxFlows, preEcmpScale]() {
      auto cfg = initialConfig();
      updateFlowletConfigs(cfg, preMode, preMaxFlows);
      updatePortFlowletConfigName(cfg);
      applyNewConfig(cfg);
      setupEcmpGroups(preEcmpScale);
    };

    auto verify = [this, preMode, preMaxFlows, preEcmpScale]() {
      auto cfg = initialConfig();
      updateFlowletConfigs(cfg, preMode, preMaxFlows);
      updatePortFlowletConfigName(cfg);
      verifyEcmpGroups(cfg, preEcmpScale);
    };

    auto setupPostWarmboot = [this, postMode, postMaxFlows, postEcmpScale]() {
      auto cfg = initialConfig();
      updateFlowletConfigs(cfg, postMode, postMaxFlows);
      updatePortFlowletConfigName(cfg);
      applyNewConfig(cfg);
      setupEcmpGroups(postEcmpScale);
    };

    auto verifyPostWarmboot = [this, postMode, postMaxFlows, postEcmpScale]() {
      auto cfg = initialConfig();
      updateFlowletConfigs(cfg, postMode, postMaxFlows);
      updatePortFlowletConfigName(cfg);
      verifyEcmpGroups(cfg, postEcmpScale);
    };

    verifyAcrossWarmBoots(setup, verify, setupPostWarmboot, verifyPostWarmboot);
  }
};

class HwFlowletSwitchingFlowsetTests : public HwFlowletSwitchingTest {
 protected:
  void SetUp() override {
    HwFlowletSwitchingTest::SetUp();
  }

  cfg::SwitchConfig initialConfig() const override {
    auto cfg = getDefaultConfig();
    // go one higher than max allowed
    updateFlowletConfigs(
        cfg,
        cfg::SwitchingMode::FLOWLET_QUALITY,
        utility::KMaxFlowsetTableSize + 1);
    updatePortFlowletConfigName(cfg);
    return cfg;
  }
};

// Test intends to excercise the scenario where we run out of the
// flowset table because size is too big
// (1) Ensure that ECMP object is created but in non dynamic mode
// (2) Once the size if fixed through the cfg, its modified to go
// back to dynamic mode
TEST_F(HwFlowletSwitchingFlowsetTests, ValidateFlowsetExceed) {
  if (this->skipTest()) {
#if defined(GTEST_SKIP)
    GTEST_SKIP();
#endif
    return;
  }

  auto setup = [&]() { resolveNextHopsAddRoute(kMaxLinks); };

  auto verify = [&]() {
    auto cfg = initialConfig();
    auto portFlowletConfig =
        getPortFlowletConfig(kScalingFactor1, kLoadWeight1, kQueueWeight1);

    // ensure that DLB is not programmed as we started with high flowset limits
    // we expect flowset size is zero
    EXPECT_FALSE(utility::verifyEcmpForFlowletSwitching(
        getHwSwitch(),
        kAddr1Prefix,
        *cfg.flowletSwitchingConfig(),
        portFlowletConfig,
        true /* flowletEnable */));

    utility::validateFlowSetTable(
        getHwSwitch(), false /* expectFlowsetSizeZero */);

    // modify the flowlet cfg to fix the flowlet
    cfg = initialConfig();
    auto flowletCfg = getFlowletSwitchingConfig(
        cfg::SwitchingMode::FLOWLET_QUALITY,
        kInactivityIntervalUsecs2,
        KMaxFlowsetTableSize);
    cfg.flowletSwitchingConfig() = flowletCfg;
    applyNewConfig(cfg);

    EXPECT_TRUE(utility::verifyEcmpForFlowletSwitching(
        getHwSwitch(),
        kAddr1Prefix,
        *cfg.flowletSwitchingConfig(),
        portFlowletConfig,
        true /* flowletEnable */));

    utility::validateFlowSetTable(
        getHwSwitch(), true /* expectFlowsetSizeZero */);
  };
  verifyAcrossWarmBoots(setup, verify);
}

class HwFlowletSwitchingFlowsetMultipleEcmpTests
    : public HwFlowletSwitchingTest {
 protected:
  void SetUp() override {
    HwFlowletSwitchingTest::SetUp();
  }

  cfg::SwitchConfig initialConfig() const override {
    auto cfg = getDefaultConfig();
    // 2048 is current size for TH3 and TH4
    updateFlowletConfigs(
        cfg, cfg::SwitchingMode::FLOWLET_QUALITY, kFlowletTableSize2);
    updatePortFlowletConfigName(cfg);
    return cfg;
  }
};

// This is as close to real case as possible
// Ensure that when multiple ECMP objects are created and flowset table gets
// full things snap back in when first ECMP object is removed, enough space is
// created for the second object to insert itself
TEST_F(
    HwFlowletSwitchingFlowsetMultipleEcmpTests,
    ValidateFlowsetExceedForceFix) {
  if (this->skipTest()) {
#if defined(GTEST_SKIP)
    GTEST_SKIP();
#endif
    return;
  }

  auto setup = [&]() {
    // create 15 different ECMP objects
    // 32768 / 2048 = 16
    int numEcmp = int(utility::KMaxFlowsetTableSize / kFlowletTableSize2);
    int totalEcmp = 1;
    for (int i = 1; i < 8 && totalEcmp < numEcmp; i++) {
      for (int j = i + 1; j < 8 && totalEcmp < numEcmp; j++) {
        std::vector<PortID> portIds;
        portIds.push_back(masterLogicalPortIds()[0]);
        portIds.push_back(masterLogicalPortIds()[i]);
        portIds.push_back(masterLogicalPortIds()[j]);
        resolveNextHopsAddRoute(
            portIds,
            folly::IPAddressV6(
                folly::sformat("{}:{:x}::", kAddr3, totalEcmp++)));
      }
    }
    // create 2 more ECMP objects
    resolveNextHopsAddRoute(
        {masterLogicalPortIds()[1], masterLogicalPortIds()[2]}, kAddr1);
    resolveNextHopsAddRoute(
        {masterLogicalPortIds()[2], masterLogicalPortIds()[3]}, kAddr2);
  };

  auto verify = [&]() {
    auto cfg = initialConfig();
    auto portFlowletConfig =
        getPortFlowletConfig(kScalingFactor1, kLoadWeight1, kQueueWeight1);

    // ensure that DLB is not programmed for 17th route as we already have 16
    // ECMP objects with DLB. Expect flowset size is zero for the 17th object
    if (getHwSwitch()->getBootType() != BootType::WARM_BOOT) {
      EXPECT_FALSE(utility::verifyEcmpForFlowletSwitching(
          getHwSwitch(),
          kAddr2Prefix, // second route
          *cfg.flowletSwitchingConfig(),
          portFlowletConfig,
          true /* flowletEnable */));
    }

    utility::validateFlowSetTable(
        getHwSwitch(), true /* expectFlowsetSizeZero */);

    // remove ECMP object for the first route
    ecmpHelper_->unprogramRoutes(
        getRouteUpdater(), {RoutePrefixV6{kAddr1, 64}});

    // ensure that DLB is  programmed as we started with more ECMP objects
    // we expect flowset size is non zero now, that there are fewer ECMP objects
    // and they should fit in
    EXPECT_TRUE(utility::verifyEcmpForFlowletSwitching(
        getHwSwitch(),
        kAddr2Prefix,
        *cfg.flowletSwitchingConfig(),
        portFlowletConfig,
        true /* flowletEnable */));

    utility::validateFlowSetTable(
        getHwSwitch(), true /* expectFlowsetSizeZero */);
  };
  verifyAcrossWarmBoots(setup, verify);
}

// verify if all 16 ECMP groups are in DLB mode
TEST_F(HwFlowletSwitchingFlowsetMultipleEcmpTests, ValidateFlowsetTableFull) {
  if (this->skipTest()) {
#if defined(GTEST_SKIP)
    GTEST_SKIP();
#endif
    return;
  }

  auto setup = [&]() {
    // create 16 different ECMP objects
    // 32768 / 2048 = 16
    int numEcmp = int(utility::KMaxFlowsetTableSize / kFlowletTableSize2);
    int totalEcmp = 0;
    for (int i = 1; i < 8 && totalEcmp < numEcmp; i++) {
      for (int j = i + 1; j < 8 && totalEcmp < numEcmp; j++) {
        std::vector<PortID> portIds;
        portIds.push_back(masterLogicalPortIds()[0]);
        portIds.push_back(masterLogicalPortIds()[i]);
        portIds.push_back(masterLogicalPortIds()[j]);
        resolveNextHopsAddRoute(
            portIds,
            folly::IPAddressV6(
                folly::sformat("{}:{:x}::", kAddr3, totalEcmp++)));
      }
    }
  };

  auto verify = [&]() {
    auto cfg = initialConfig();
    auto portFlowletConfig =
        getPortFlowletConfig(kScalingFactor1, kLoadWeight1, kQueueWeight1);
    int numEcmp = int(utility::KMaxFlowsetTableSize / kFlowletTableSize2);

    for (int i = 0; i < numEcmp; i++) {
      auto currentIp = folly::IPAddress(folly::sformat("{}:{:x}::", kAddr3, i));
      folly::CIDRNetwork currentPrefix{currentIp, 64};
      EXPECT_TRUE(utility::verifyEcmpForFlowletSwitching(
          getHwSwitch(),
          currentPrefix, // second route
          *cfg.flowletSwitchingConfig(),
          portFlowletConfig,
          true /* flowletEnable */));
    }
    utility::validateFlowSetTable(
        getHwSwitch(), true /* expectFlowsetSizeZero */);
  };
  verifyAcrossWarmBoots(setup, verify);
}

// This test creates more than 128 ECMP groups and
// try to update the ECMP group id 200128 to be flowlet enabled and
// verify it fails.

TEST_F(
    HwFlowletSwitchingFlowsetMultipleEcmpTests,
    ValidateMaxEcmpIdFlowletUpdate) {
  if (this->skipTest() ||
      (getPlatform()->getAsic()->getAsicType() ==
       cfg::AsicType::ASIC_TYPE_FAKE)) {
#if defined(GTEST_SKIP)
    GTEST_SKIP();
#endif
    return;
  }

  auto setup = [&]() {
    // create 128 different ECMP objects
    for (int i = 1; i <= kNumEcmp; i++) {
      std::vector<PortID> portIds;
      portIds.push_back(masterLogicalPortIds()[i % 64]);
      portIds.push_back(masterLogicalPortIds()[(i + 1) % 64]);
      resolveNextHopsAddRoute(
          portIds, folly::IPAddressV6(folly::sformat("{}:{:x}::", kAddr3, i)));
      std::vector<PortID> portIds2;
      portIds2.push_back(masterLogicalPortIds()[i % 64]);
      portIds2.push_back(masterLogicalPortIds()[(i + 2) % 64]);
      resolveNextHopsAddRoute(
          portIds2, folly::IPAddressV6(folly::sformat("{}:{:x}::", kAddr4, i)));
    }

    // create 1 more ECMP object
    resolveNextHopsAddRoute(
        {masterLogicalPortIds()[1], masterLogicalPortIds()[4]}, kAddr1);

    // check to ensure we have created more than 128 ECMP objects
    auto ecmpDetails = getHwSwitch()->getAllEcmpDetails();
    CHECK_GT(ecmpDetails.size(), kNumEcmp * 2);
    // verify the ECMP Id more than Max dlb Ecmp Id
    // not enabled with flowlet config and flowset available is zero.
    utility::verifyEcmpForNonFlowlet(getHwSwitch(), kAddr1Prefix, false);
  };

  auto verify = [&]() {
    // Delete the 124 ECMP groups, so that we have enough flowset resources
    // available and try to update the ECMP group Id more than Max dlb Ecmp Id
    // not enabled with flowlet config and
    // verify flowset available is more than 2K.
    for (int i = 1; i < kNumEcmp - 1; i++) {
      ecmpHelper_->unprogramRoutes(
          getRouteUpdater(),
          {RoutePrefixV6{
              folly::IPAddressV6(folly::sformat("{}:{:x}::", kAddr3, i)), 64}});
      ecmpHelper_->unprogramRoutes(
          getRouteUpdater(),
          {RoutePrefixV6{
              folly::IPAddressV6(folly::sformat("{}:{:x}::", kAddr4, i)), 64}});
    }
    utility::verifyEcmpForNonFlowlet(getHwSwitch(), kAddr1Prefix, true);
  };
  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(HwFlowletSwitchingTest, VerifyFlowletSwitchingEnable) {
  if (this->skipTest()) {
#if defined(GTEST_SKIP)
    GTEST_SKIP();
#endif
    return;
  }

  auto setup = [&]() {
    resolveNextHopsAddRoute(kMaxLinks - 1);
    // resolve route on this port later, so we can mimic unresolved nexthop on
    // ECMP port
    this->resolveNextHop(PortDescriptor(masterLogicalPortIds()[kMaxLinks - 1]));
  };

  auto verify = [&]() {
    // verify the flowlet config
    const auto& cfg = initialConfig();
    verifyConfig(cfg);

    // Shrink egress and verify
    this->unresolveNextHop(PortDescriptor(masterLogicalPortIds()[3]));

    verifyConfig(cfg);

    // Expand egress and verify
    this->resolveNextHop(PortDescriptor(masterLogicalPortIds()[3]));

    verifyConfig(cfg);
  };

  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(HwFlowletSwitchingTest, VerifyPortFlowletConfigChange) {
  if (this->skipTest()) {
#if defined(GTEST_SKIP)
    GTEST_SKIP();
#endif
    return;
  }

  auto setup = [&]() { resolveNextHopsAddRoute(kMaxLinks); };

  auto verify = [&]() {
    auto cfg = initialConfig();
    verifyConfig(cfg);
    // Modify the port flowlet config and verify
    updatePortFlowletConfigs(cfg, kScalingFactor2, kLoadWeight2, kQueueWeight2);
    applyNewConfig(cfg);

    auto portFlowletConfig =
        getPortFlowletConfig(kScalingFactor2, kLoadWeight2, kQueueWeight2);
    verifyPortFlowletConfig(portFlowletConfig);
    // Modify to initial config to verify after warmboot
    applyNewConfig(initialConfig());
  };

  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(HwFlowletSwitchingTest, VerifyFlowletConfigChange) {
  if (this->skipTest()) {
#if defined(GTEST_SKIP)
    GTEST_SKIP();
#endif
    return;
  }

  auto setup = [&]() { resolveNextHopsAddRoute(kMaxLinks); };

  auto verify = [&]() {
    // switchingMode starts out as FLOWLET_QUALITY
    auto cfg = initialConfig();
    verifyConfig(cfg);
    // Modify the flowlet config switching mode to PER_PACKET_QUALITY
    modifyFlowletSwitchingConfig(cfg);
    // Modify the port flowlet config
    updatePortFlowletConfigs(cfg, kScalingFactor2, kLoadWeight2, kQueueWeight2);
    applyNewConfig(cfg);
    verifyModifiedConfig();
    // Modify to initial config to verify after warmboot
    // switching mode back to FLOWLET_QUALITY
    cfg = initialConfig();
    applyNewConfig(cfg);
    verifyConfig(cfg);
  };

  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(HwFlowletSwitchingTest, VerifyFlowletConfigRemoval) {
  if (this->skipTest()) {
#if defined(GTEST_SKIP)
    GTEST_SKIP();
#endif
    return;
  }

  auto setup = [&]() { resolveNextHopsAddRoute(kMaxLinks); };

  auto verify = [&]() {
    auto cfg = initialConfig();
    verifyConfig(cfg);
    // Modify the flowlet config
    modifyFlowletSwitchingConfig(cfg);
    // Modify the port flowlet config
    updatePortFlowletConfigs(cfg, kScalingFactor2, kLoadWeight2, kQueueWeight2);
    applyNewConfig(cfg);
    verifyModifiedConfig();
    // Remove the flowlet configs
    cfg = getDefaultConfig();
    applyNewConfig(cfg);
    verifyRemoveFlowletConfig();
    // Modify to initial config to verify after warmboot
    applyNewConfig(initialConfig());
  };

  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(HwFlowletSwitchingTest, VerifyGetEcmpDetails) {
  if (this->skipTest()) {
#if defined(GTEST_SKIP)
    GTEST_SKIP();
#endif
    return;
  }

  auto setup = [&]() { resolveNextHopsAddRoute(kMaxLinks); };

  auto verify = [&]() {
    auto cfg = initialConfig();
    verifyConfig(cfg);

    // start a new thread to verify the ecmp details
    std::atomic<bool> done{false};
    auto flowletCfg = *cfg.flowletSwitchingConfig();
    std::thread ecmpDetails([&flowletCfg, &done, this]() {
      while (!done) {
        validateEcmpDetails(flowletCfg);
      }
    });

    // Modify the flowlet config
    modifyFlowletSwitchingConfig(cfg);
    // Modify the port flowlet config
    updatePortFlowletConfigs(cfg, kScalingFactor2, kLoadWeight2, kQueueWeight2);
    applyNewConfig(cfg);
    verifyModifiedConfig();

    done = true;
    ecmpDetails.join();

    // Modify to initial config to verify after warmboot
    applyNewConfig(initialConfig());
  };

  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(HwEcmpFlowletSwitchingTest, VerifyEcmpFlowletSwitchingEnable) {
  if (this->skipTest()) {
#if defined(GTEST_SKIP)
    GTEST_SKIP();
#endif
    return;
  }

  // This test setup static ECMP and update the static ECMP to DLB ECMP and
  // revert the DLB ECMP to static ECMP and verify it.
  auto setup = [&]() { resolveNextHopsAddRoute(kMaxLinks); };

  auto verify = [&]() {
    auto cfg = initialConfig();
    // Modify the flowlet config to convert ECMP to DLB
    updateFlowletConfigs(cfg);
    updatePortFlowletConfigName(cfg);
    applyNewConfig(cfg);
    // verify the flowlet config
    verifyConfig(cfg);

    // modify switchingMode to per_packet
    cfg = getDefaultConfig();
    updateFlowletConfigs(
        cfg, cfg::SwitchingMode::PER_PACKET_QUALITY, kFlowletTableSize1);
    updatePortFlowletConfigName(cfg);
    applyNewConfig(cfg);

    // modify max_flows and keep mode the same
    cfg = getDefaultConfig();
    updateFlowletConfigs(
        cfg, cfg::SwitchingMode::PER_PACKET_QUALITY, kMinFlowletTableSize);
    updatePortFlowletConfigName(cfg);
    applyNewConfig(cfg);
    // verify the flowlet config
    verifyConfig(cfg);

    // Remove the flowlet configs
    cfg = getDefaultConfig();
    applyNewConfig(cfg);
    // verify removal of flowlet config
    verifyRemoveFlowletConfig();
  };

  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(HwFlowletSwitchingFlowsetMultipleEcmpTests, ValidateEcmpDetailsThread) {
  if (this->skipTest()) {
#if defined(GTEST_SKIP)
    GTEST_SKIP();
#endif
    return;
  }

  auto setup = [&]() {
    // create 2 different ECMP objects
    resolveNextHopsAddRoute(
        {masterLogicalPortIds()[0], masterLogicalPortIds()[1]}, kAddr1);
    resolveNextHopsAddRoute(
        {masterLogicalPortIds()[2], masterLogicalPortIds()[3]}, kAddr2);
  };

  auto verify = [&]() {
    auto cfg = initialConfig();
    auto portFlowletConfig =
        getPortFlowletConfig(kScalingFactor1, kLoadWeight1, kQueueWeight1);

    EXPECT_TRUE(utility::verifyEcmpForFlowletSwitching(
        getHwSwitch(),
        kAddr2Prefix, // second route
        *cfg.flowletSwitchingConfig(),
        portFlowletConfig,
        true /* flowletEnable */));

    // start a new thread to poll the ecmp details
    std::atomic<bool> done{false};
    auto hwSwitch = getHwSwitch();
    std::thread readEcmpDetails([&hwSwitch, &done]() {
      while (!done) {
        auto ecmpDetails = hwSwitch->getAllEcmpDetails();
      }
    });

    // remove ECMP object for the first route
    ecmpHelper_->unprogramRoutes(
        getRouteUpdater(), {RoutePrefixV6{kAddr1, 64}});

    EXPECT_TRUE(utility::verifyEcmpForFlowletSwitching(
        getHwSwitch(),
        kAddr2Prefix, // second route
        *cfg.flowletSwitchingConfig(),
        portFlowletConfig,
        true /* flowletEnable */));

    done = true;
    readEcmpDetails.join();
  };
  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(HwFlowletSwitchingFlowsetMultipleEcmpTests, ValidateFlowletStatsThread) {
  if (this->skipTest() ||
      (getPlatform()->getAsic()->getAsicType() ==
       cfg::AsicType::ASIC_TYPE_FAKE)) {
#if defined(GTEST_SKIP)
    GTEST_SKIP();
#endif
    return;
  }

  auto setup = [&]() {
    // create 2 different ECMP objects
    resolveNextHopsAddRoute(
        {masterLogicalPortIds()[0], masterLogicalPortIds()[1]}, kAddr1);
    resolveNextHopsAddRoute(
        {masterLogicalPortIds()[2], masterLogicalPortIds()[3]}, kAddr2);
  };

  auto verify = [&]() {
    auto cfg = initialConfig();
    auto portFlowletConfig =
        getPortFlowletConfig(kScalingFactor1, kLoadWeight1, kQueueWeight1);

    EXPECT_TRUE(utility::verifyEcmpForFlowletSwitching(
        getHwSwitch(),
        kAddr2Prefix, // second route
        *cfg.flowletSwitchingConfig(),
        portFlowletConfig,
        true /* flowletEnable */));

    // start a new thread to poll the flowlet stats
    std::atomic<bool> done{false};
    auto hwSwitch = getHwSwitch();
    std::thread readFlowletStats([&hwSwitch, &done]() {
      while (!done) {
        auto l3EcmpDlbFailPackets =
            hwSwitch->getHwFlowletStats().l3EcmpDlbFailPackets().value();
        EXPECT_EQ(l3EcmpDlbFailPackets, 0);
      }
    });

    // remove ECMP object for the first route
    ecmpHelper_->unprogramRoutes(
        getRouteUpdater(), {RoutePrefixV6{kAddr1, 64}});

    EXPECT_TRUE(utility::verifyEcmpForFlowletSwitching(
        getHwSwitch(),
        kAddr2Prefix, // second route
        *cfg.flowletSwitchingConfig(),
        portFlowletConfig,
        true /* flowletEnable */));

    done = true;
    readFlowletStats.join();
  };
  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(HwEcmpFlowletSwitchingTest, VerifySkipEcmpFlowletSwitchingEnable) {
  if (this->skipTest()) {
#if defined(GTEST_SKIP)
    GTEST_SKIP();
#endif
    return;
  }

  // This test setup static ECMP and update the static ECMP to DLB
  // without port flowlet config and verify it
  auto setup = [&]() { resolveNextHopsAddRoute(kMaxLinks); };

  auto verify = [&]() {
    auto cfg = initialConfig();
    // Modify the flowlet config to convert ECMP to DLB
    // without the flowlet port config
    updateFlowletConfigs(cfg);
    applyNewConfig(cfg);
    // verify the flowlet config is not programmed in ECMP for TH3
    // since egress is not updated with port flowlet config
    // and the flowlet config is programmed in ECMP for TH4
    verifySkipEcmpConfig(cfg);
  };

  verifyAcrossWarmBoots(setup, verify);
}

// Verify ability to create upto 128 ECMP DLB groups in spray mode
TEST_F(HwEcmpFlowletSwitchingTest, VerifyEcmpSprayModeScale) {
  if (this->skipTest()) {
#if defined(GTEST_SKIP)
    GTEST_SKIP();
#endif
    return;
  }
  int numEcmp = 63;
  // CS00012344837
  if (getPlatform()->getAsic()->getAsicType() ==
      cfg::AsicType::ASIC_TYPE_TOMAHAWK3) {
    numEcmp = 30;
  }

  auto setup = [this, numEcmp]() {
    auto cfg = initialConfig();
    updateFlowletConfigs(
        cfg, cfg::SwitchingMode::PER_PACKET_QUALITY, kMinFlowletTableSize);
    updatePortFlowletConfigName(cfg);
    applyNewConfig(cfg);
    setupEcmpGroups(numEcmp);
  };

  auto verify = [this, numEcmp]() {
    auto cfg = initialConfig();
    updateFlowletConfigs(
        cfg, cfg::SwitchingMode::PER_PACKET_QUALITY, kMinFlowletTableSize);
    updatePortFlowletConfigName(cfg);
    verifyEcmpGroups(cfg, numEcmp);
  };

  verifyAcrossWarmBoots(setup, verify);
}

// Verify warmboot from 16 ECMP (flowlet) to 96 ECMP (per-packet)
TEST_F(HwEcmpFlowletSwitchingTest, VerifyModeFlowletToSpray) {
  if (this->skipTest()) {
#if defined(GTEST_SKIP)
    GTEST_SKIP();
#endif
    return;
  }
  int numEcmp = 48;
  // CS00012344837
  if (getPlatform()->getAsic()->getAsicType() ==
      cfg::AsicType::ASIC_TYPE_TOMAHAWK3) {
    numEcmp = 30;
  }
  flowletSwitchingWBHelper(
      cfg::SwitchingMode::FLOWLET_QUALITY,
      kFlowletTableSize2,
      8,
      cfg::SwitchingMode::PER_PACKET_QUALITY,
      kMinFlowletTableSize,
      numEcmp);
}

// Verify warmboot from 16 ECMP (per-packet) to 16 ECMP (flowlet)
//
// Ideally we want to test 96 per-packet ECMP to 16 flowlet ECMP
// It becomes really hard to verify post conversion because there
// will be 16 flowlet ECMPs and 80 disabled ECMPs post-warmboot.
TEST_F(HwEcmpFlowletSwitchingTest, VerifyModeSprayToFlowlet) {
  if (this->skipTest()) {
#if defined(GTEST_SKIP)
    GTEST_SKIP();
#endif
    return;
  }

  flowletSwitchingWBHelper(
      cfg::SwitchingMode::PER_PACKET_QUALITY,
      kMinFlowletTableSize,
      8,
      cfg::SwitchingMode::FLOWLET_QUALITY,
      kFlowletTableSize2,
      8);
}

TEST_F(HwEcmpFlowletSwitchingTest, VerifyModeSprayFlowletSizeChange) {
  if (this->skipTest()) {
#if defined(GTEST_SKIP)
    GTEST_SKIP();
#endif
    return;
  }

  // Update max_flows size from 2048 -> 256
  flowletSwitchingWBHelper(
      cfg::SwitchingMode::PER_PACKET_QUALITY,
      kFlowletTableSize2,
      8,
      cfg::SwitchingMode::PER_PACKET_QUALITY,
      kMinFlowletTableSize,
      8);
}

} // namespace facebook::fboss
