// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "fboss/agent/hw/test/HwLinkStateDependentTest.h"
#include "fboss/agent/hw/test/HwTestFlowletSwitchingUtils.h"
#include "fboss/agent/test/utils/ConfigUtils.h"
#include "fboss/agent/test/utils/EcmpDataPlaneTestUtil.h"
#include "fboss/agent/test/utils/LoadBalancerTestUtils.h"

#include "fboss/agent/hw/test/LoadBalancerUtils.h"

#include "fboss/agent/test/utils/LoadBalancerTestRunner.h"

namespace facebook::fboss {

template <typename EcmpTestHelperT, bool kFlowLetSwitching = false>
class HwLoadBalancerTest
    : public HwLinkStateDependentTest,
      public utility::
          HwLoadBalancerTestRunner<EcmpTestHelperT, kFlowLetSwitching> {
 public:
  using Runner =
      utility::HwLoadBalancerTestRunner<EcmpTestHelperT, kFlowLetSwitching>;
  using Test = HwLinkStateDependentTest;

  using SETUP_FN = typename Runner::SETUP_FN;
  using VERIFY_FN = typename Runner::VERIFY_FN;
  using SETUP_POSTWB_FN = typename Runner::SETUP_POSTWB_FN;
  using VERIFY_POSTWB_FN = typename Runner::VERIFY_POSTWB_FN;

  cfg::SwitchConfig initialConfig() const override {
    auto cfg = utility::onePortPerInterfaceConfig(
        getHwSwitchEnsemble(), masterLogicalPortIds());
    return cfg;
  }

  void SetUp() override {
    Test::SetUp();
    Runner::setEcmpHelper();
  }

  void runLoadBalanceTest(
      unsigned int ecmpWidth,
      const cfg::LoadBalancer& loadBalancer,
      const std::vector<NextHopWeight>& weights,
      bool loopThroughFrontPanel = false,
      bool loadBalanceExpected = true,
      uint8_t deviation = 25) {
    /* TODO: remove this hack.
     * apply thrift config relies on sdk version in switch config to determine
     * how to compute load balancer seeds. however sdk version is unavailable in
     * hw tests unlike agent tests. remove this hack when hw tests can inject
     * sdk version in thrift config.
     */
    auto lbConfig = loadBalancer;
    auto seed = getHwSwitch()->generateDeterministicSeed(*lbConfig.id());
    lbConfig.seed() = seed;
    Runner::runLoadBalanceTest(
        ecmpWidth,
        lbConfig,
        weights,
        loopThroughFrontPanel,
        loadBalanceExpected,
        deviation);
  }

  void runDynamicLoadBalanceTest(
      unsigned int ecmpWidth,
      const cfg::LoadBalancer& loadBalancer,
      const std::vector<NextHopWeight>& weights,
      bool loopThroughFrontPanel = false,
      bool loadBalanceExpected = true,
      cfg::SwitchingMode preWBMode = cfg::SwitchingMode::FIXED_ASSIGNMENT,
      cfg::SwitchingMode postWBMode = cfg::SwitchingMode::FLOWLET_QUALITY,
      uint8_t deviation = 25) {
    auto lbConfig = loadBalancer;
    auto seed = getHwSwitch()->generateDeterministicSeed(*lbConfig.id());
    lbConfig.seed() = seed;
    Runner::runDynamicLoadBalanceTest(
        ecmpWidth,
        lbConfig,
        weights,
        loopThroughFrontPanel,
        loadBalanceExpected,
        preWBMode,
        postWBMode,
        deviation);
  }

  void runEcmpShrinkExpandLoadBalanceTest(
      unsigned int ecmpWidth,
      const cfg::LoadBalancer& loadBalancer,
      uint8_t deviation = 25) {
    Runner::runEcmpShrinkExpandLoadBalanceTest(
        ecmpWidth, loadBalancer, deviation);
  }

  void setEcmpMemberStatus(const TestEnsembleIf* ensemble) override {
    utility::setEcmpMemberStatus(ensemble);
  }

  int getL3EcmpDlbFailPackets(const TestEnsembleIf* ensemble) const override {
    return utility::getL3EcmpDlbFailPackets(ensemble);
  }

 private:
  void runTestAcrossWarmBoots(
      SETUP_FN setup,
      VERIFY_FN verify,
      SETUP_POSTWB_FN setupPostWarmboot,
      VERIFY_POSTWB_FN verifyPostWarmboot) override {
    auto verifyFn = [&]() {
      verify();
      EXPECT_TRUE(
          utility::isHwDeterministicSeed(
              getHwSwitch(), getProgrammedState(), LoadBalancerID::ECMP));
    };
    Test::verifyAcrossWarmBoots(
        setup, verifyFn, setupPostWarmboot, verifyPostWarmboot);
  }

  TestEnsembleIf* getEnsemble() override {
    return getHwSwitchEnsemble();
  }

  const TestEnsembleIf* getEnsemble() const override {
    return getHwSwitchEnsemble();
  }
};
} // namespace facebook::fboss
