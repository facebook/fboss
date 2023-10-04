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
#include "fboss/agent/hw/test/HwTestFlowletSwitchingUtils.h"
#include "fboss/agent/hw/test/ProdConfigFactory.h"
#include "fboss/agent/test/EcmpSetupHelper.h"

using namespace facebook::fboss::utility;

namespace {
static folly::IPAddressV6 kAddr1{"2803:6080:d038:3063::"};
folly::CIDRNetwork kAddr1Prefix{folly::IPAddress("2803:6080:d038:3063::"), 64};
const int kScalingFactor1 = 100;
const int kScalingFactor2 = 200;
const int kLoadWeight1 = 70;
const int kLoadWeight2 = 60;
const int kQueueWeight1 = 30;
const int kQueueWeight2 = 40;
const int kInactivityIntervalUsecs1 = 128;
const int kInactivityIntervalUsecs2 = 256;
const int kFlowletTableSize1 = 1024;
const int kFlowletTableSize2 = 2048;
} // namespace

namespace facebook::fboss {

class HwFlowletSwitchingTest : public HwLinkStateDependentTest {
 protected:
  void SetUp() override {
    FLAGS_flowletSwitchingEnable = true;
    HwLinkStateDependentTest::SetUp();
    ecmpHelper_ = std::make_unique<utility::EcmpSetupAnyNPorts6>(
        getProgrammedState(), RouterID(0));
  }

  cfg::SwitchConfig initialConfig() const override {
    auto cfg = getDefaultConfig();
    auto flowletCfg = getFlowletSwitchingConfig(
        kInactivityIntervalUsecs1, kFlowletTableSize1);
    cfg.flowletSwitchingConfig() = flowletCfg;
    updatePortFlowletConfigs(cfg, kScalingFactor1, kLoadWeight1, kQueueWeight1);

    auto allPorts = masterLogicalInterfacePortIds();
    std::vector<PortID> ports(
        allPorts.begin(), allPorts.begin() + allPorts.size());
    for (auto portId : ports) {
      auto portCfg = utility::findCfgPort(cfg, portId);
      portCfg->flowletConfigName() = "default";
    }

    return cfg;
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
      uint16_t inactivityIntervalUsecs,
      uint16_t flowletTableSize) const {
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
    return flowletCfg;
  }

  void modifyFlowletSwitchingConfig(cfg::SwitchConfig& cfg) const {
    auto flowletCfg = getFlowletSwitchingConfig(
        kInactivityIntervalUsecs2, kFlowletTableSize2);
    cfg.flowletSwitchingConfig() = flowletCfg;
  }

  bool skipTest() {
    return !getPlatform()->getAsic()->isSupported(HwAsic::Feature::DLB);
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

  void
  verifyPortFlowletConfig(int scalingFactor, int loadWeight, int queueWeight) {
    if (getHwSwitch()->getPlatform()->getAsic()->isSupported(
            HwAsic::Feature::FLOWLET_PORT_ATTRIBUTES)) {
      auto portFlowletConfig =
          getPortFlowletConfig(scalingFactor, loadWeight, queueWeight);
      EXPECT_TRUE(utility::validatePortFlowletQuality(
          getHwSwitch(), masterLogicalPortIds()[0], portFlowletConfig));
    }
  }

  void verifyInitialConfig() {
    verifyPortFlowletConfig(kScalingFactor1, kLoadWeight1, kQueueWeight1);

    auto flowletCfg = getFlowletSwitchingConfig(
        kInactivityIntervalUsecs1, kFlowletTableSize1);

    EXPECT_TRUE(
        utility::validateFlowletSwitchingEnabled(getHwSwitch(), flowletCfg));
    EXPECT_TRUE(utility::verifyEcmpForFlowletSwitching(
        getHwSwitch(), kAddr1Prefix, flowletCfg, true));
  }

  void verifyModifiedConfig() {
    verifyPortFlowletConfig(kScalingFactor2, kLoadWeight2, kQueueWeight2);

    auto flowletCfg = getFlowletSwitchingConfig(
        kInactivityIntervalUsecs2, kFlowletTableSize2);

    EXPECT_TRUE(
        utility::validateFlowletSwitchingEnabled(getHwSwitch(), flowletCfg));
    EXPECT_TRUE(utility::verifyEcmpForFlowletSwitching(
        getHwSwitch(), kAddr1Prefix, flowletCfg, true));
  }

  void verifyRemoveFlowletConfig() {
    verifyPortFlowletConfig(10, 100, 0);

    auto flowletCfg = getFlowletSwitchingConfig(0, 0);

    EXPECT_TRUE(utility::validateFlowletSwitchingDisabled(getHwSwitch()));
    EXPECT_TRUE(utility::verifyEcmpForFlowletSwitching(
        getHwSwitch(), kAddr1Prefix, flowletCfg, false));
  }

  std::unique_ptr<utility::EcmpSetupAnyNPorts<folly::IPAddressV6>> ecmpHelper_;
};

TEST_F(HwFlowletSwitchingTest, VerifyFlowletSwitchingEnable) {
  if (this->skipTest()) {
#if defined(GTEST_SKIP)
    GTEST_SKIP();
#endif
    return;
  }

  auto setup = [&]() {
    this->resolveNextHop(PortDescriptor(masterLogicalPortIds()[0]));
    this->resolveNextHop(PortDescriptor(masterLogicalPortIds()[1]));
    this->resolveNextHop(PortDescriptor(masterLogicalPortIds()[2]));
    this->addRoute(kAddr1, 64);
    this->resolveNextHop(PortDescriptor(masterLogicalPortIds()[3]));
  };

  auto verify = [&]() {
    verifyInitialConfig();

    // Shrink egress and verify
    this->unresolveNextHop(PortDescriptor(masterLogicalPortIds()[3]));

    verifyInitialConfig();

    // Expand egress and verify
    this->resolveNextHop(PortDescriptor(masterLogicalPortIds()[3]));

    verifyInitialConfig();
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

  auto setup = [&]() {
    this->resolveNextHop(PortDescriptor(masterLogicalPortIds()[0]));
    this->resolveNextHop(PortDescriptor(masterLogicalPortIds()[1]));
    this->resolveNextHop(PortDescriptor(masterLogicalPortIds()[2]));
    this->resolveNextHop(PortDescriptor(masterLogicalPortIds()[3]));
    this->addRoute(kAddr1, 64);
  };

  auto verify = [&]() {
    verifyInitialConfig();
    auto cfg = initialConfig();
    // Modify the port flowlet config and verify
    updatePortFlowletConfigs(cfg, kScalingFactor2, kLoadWeight2, kQueueWeight2);
    applyNewConfig(cfg);
    verifyPortFlowletConfig(kScalingFactor2, kLoadWeight2, kQueueWeight2);
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

  auto setup = [&]() {
    this->resolveNextHop(PortDescriptor(masterLogicalPortIds()[0]));
    this->resolveNextHop(PortDescriptor(masterLogicalPortIds()[1]));
    this->resolveNextHop(PortDescriptor(masterLogicalPortIds()[2]));
    this->resolveNextHop(PortDescriptor(masterLogicalPortIds()[3]));
    this->addRoute(kAddr1, 64);
  };

  auto verify = [&]() {
    verifyInitialConfig();
    auto cfg = initialConfig();
    // Modify the flowlet config
    modifyFlowletSwitchingConfig(cfg);
    // Modify the port flowlet config
    updatePortFlowletConfigs(cfg, kScalingFactor2, kLoadWeight2, kQueueWeight2);
    applyNewConfig(cfg);
    verifyModifiedConfig();
    // Modify to initial config to verify after warmboot
    applyNewConfig(initialConfig());
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

  auto setup = [&]() {
    this->resolveNextHop(PortDescriptor(masterLogicalPortIds()[0]));
    this->resolveNextHop(PortDescriptor(masterLogicalPortIds()[1]));
    this->resolveNextHop(PortDescriptor(masterLogicalPortIds()[2]));
    this->resolveNextHop(PortDescriptor(masterLogicalPortIds()[3]));
    this->addRoute(kAddr1, 64);
  };

  auto verify = [&]() {
    verifyInitialConfig();
    auto cfg = initialConfig();
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

} // namespace facebook::fboss
