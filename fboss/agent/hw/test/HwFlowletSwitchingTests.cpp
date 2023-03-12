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
    auto cfg = utility::onePortPerInterfaceConfig(
        getHwSwitch(),
        masterLogicalPortIds(),
        getAsic()->desiredLoopbackMode());

    auto flowletCfg = getFlowletSwitchingConfig();
    cfg.flowletSwitchingConfig() = flowletCfg;
    addLoadBalancerToConfig(cfg, getHwSwitch(), LBHash::FULL_HASH);
    return cfg;
  }

  cfg::FlowletSwitchingConfig getFlowletSwitchingConfig() const {
    cfg::FlowletSwitchingConfig flowletCfg;
    flowletCfg.inactivityIntervalUsecs() = 60;
    flowletCfg.flowletTableSize() = 1024;
    flowletCfg.dynamicEgressLoadExponent() = 3;
    flowletCfg.dynamicQueueExponent() = 3;
    flowletCfg.dynamicQueueMinThresholdBytes() = 100000;
    flowletCfg.dynamicQueueMaxThresholdBytes() = 200000;
    flowletCfg.dynamicSampleRate() = 100000;
    flowletCfg.portScalingFactor() = 400;
    flowletCfg.portLoadWeight() = 50;
    flowletCfg.portQueueWeight() = 40;
    return flowletCfg;
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

  std::unique_ptr<utility::EcmpSetupAnyNPorts<folly::IPAddressV6>> ecmpHelper_;
};

TEST_F(HwFlowletSwitchingTest, VerifyFlowletSwitchingEnable) {
  if (this->skipTest()) {
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
    auto flowletCfg = this->getFlowletSwitchingConfig();
    EXPECT_TRUE(
        utility::validateFlowletSwitchingEnabled(getHwSwitch(), flowletCfg));
    EXPECT_TRUE(utility::verifyEcmpForFlowletSwitching(
        getHwSwitch(), kAddr1Prefix, flowletCfg));
  };

  setup();
  verify();
}
} // namespace facebook::fboss
