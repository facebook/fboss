// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/hw/test/HwLinkStateDependentTest.h"
#include "fboss/agent/hw/test/LoadBalancerUtils.h"
#include "fboss/agent/hw/test/dataplane_tests/HwEcmpDataPlaneTestUtil.h"

namespace facebook {
namespace fboss {

template <typename EcmpTestHelperT>
class HwLoadBalancerTest : public HwLinkStateDependentTest {
 private:
  cfg::SwitchConfig initialConfig() const override {
    auto cfg = utility::onePortPerVlanConfig(
        getHwSwitch(), masterLogicalPortIds(), cfg::PortLoopbackMode::MAC);
    return cfg;
  }

  void SetUp() override {
    HwLinkStateDependentTest::SetUp();
    helper_ = getECMPHelper();
  }

  void setupECMPForwarding(
      unsigned int ecmpWidth,
      const cfg::LoadBalancer& loadBalancer,
      const std::vector<NextHopWeight>& weights) {
    setupECMPHelper(ecmpWidth, weights);
    applyNewState(utility::addLoadBalancer(
        getPlatform(), getProgrammedState(), loadBalancer));
  }

  virtual std::unique_ptr<EcmpTestHelperT> getECMPHelper() = 0;

  virtual void setupECMPHelper(
      unsigned int ecmpWidth,
      const std::vector<NextHopWeight>& weights) = 0;

  void pumpTrafficPortAndVerifyLoadBalanced(
      unsigned int ecmpWidth,
      const std::vector<NextHopWeight>& weights,
      bool loopThroughFrontPanel,
      uint8_t deviation) {
    helper_->pumpTraffic(ecmpWidth, loopThroughFrontPanel);
    // Don't tolerate a deviation of > 25%
    EXPECT_TRUE(helper_->isLoadBalanced(ecmpWidth, weights, deviation));
  }

  void resolveNextHopsandClearStats(unsigned int ecmpWidth) {
    helper_->resolveNextHopsandClearStats(ecmpWidth);
  }

  void shrinkECMP(unsigned int ecmpWidth) {
    helper_->shrinkECMP(ecmpWidth);
    resolveNextHopsandClearStats(ecmpWidth);
  }

  void expandECMP(unsigned int ecmpWidth) {
    helper_->expandECMP(ecmpWidth);
    resolveNextHopsandClearStats(ecmpWidth);
  }

 protected:
  void runLoadBalanceTest(
      unsigned int ecmpWidth,
      const cfg::LoadBalancer& loadBalancer,
      const std::vector<NextHopWeight>& weights,
      bool loopThroughFrontPanel = false,
      uint8_t deviation = 25) {
    auto setup = [=]() {
      setupECMPForwarding(ecmpWidth, loadBalancer, weights);
    };
    auto verify = [=]() {
      pumpTrafficPortAndVerifyLoadBalanced(
          ecmpWidth, weights, loopThroughFrontPanel, deviation);
    };
    verifyAcrossWarmBoots(setup, verify);
  }

  void runEcmpShrinkExpandLoadBalanceTest(
      unsigned int ecmpWidth,
      const cfg::LoadBalancer& loadBalancer,
      uint8_t deviation = 25) {
    unsigned int minLinksLoadbalanceTest = 1;
    auto setup = [=]() {
      setupECMPForwarding(ecmpWidth, loadBalancer, {} /*weights*/);
    };
    auto verify = [=]() {
      unsigned int width = ecmpWidth;

      while (width > minLinksLoadbalanceTest) {
        width--;
        shrinkECMP(width);
        pumpTrafficPortAndVerifyLoadBalanced(
            width,
            {}, /*weights*/
            false /*loopThroughFrontPanel*/,
            deviation);
      }

      while (width < ecmpWidth) {
        expandECMP(width);
        pumpTrafficPortAndVerifyLoadBalanced(
            width,
            {}, /*weights*/
            false /*loopThroughFrontPanel*/,
            deviation);
        width++;
      }
    };
    setup();
    verify();
  }

  EcmpTestHelperT* getEcmpSetupHelper() const {
    return helper_.get();
  }

 private:
  std::unique_ptr<EcmpTestHelperT> helper_;
};

} // namespace fboss
} // namespace facebook
