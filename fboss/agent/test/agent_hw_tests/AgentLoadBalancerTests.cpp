// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <gtest/gtest.h>

#include "fboss/agent/test/AgentHwTest.h"

#include "fboss/agent/test/utils/ConfigUtils.h"
#include "fboss/agent/test/utils/LoadBalancerTestRunner.h"

namespace facebook::fboss {

template <typename EcmpTestHelperT>
class AgentLoadBalancerTest
    : public AgentHwTest,
      public utility::HwLoadBalancerTestRunner<EcmpTestHelperT, false> {
 public:
  using Runner = utility::HwLoadBalancerTestRunner<EcmpTestHelperT, false>;
  using Test = AgentHwTest;
  using SETUP_FN = typename Runner::SETUP_FN;
  using VERIFY_FN = typename Runner::VERIFY_FN;
  using SETUP_POSTWB_FN = typename Runner::SETUP_POSTWB_FN;
  using VERIFY_POSTWB_FN = typename Runner::VERIFY_POSTWB_FN;

  void SetUp() override {
    AgentHwTest::SetUp();
    Runner::setEcmpHelper();
  }

  std::vector<production_features::ProductionFeature>
  getProductionFeaturesVerified() const override {
    return {
        production_features::ProductionFeature::ECMP_LOAD_BALANCER,
    };
  }

  void runLoadBalanceTest(
      unsigned int ecmpWidth,
      const cfg::LoadBalancer& loadBalancer,
      const std::vector<NextHopWeight>& weights,
      bool loopThroughFrontPanel = false,
      bool loadBalanceExpected = true,
      uint8_t deviation = 25) {
    Runner::runLoadBalanceTest(
        ecmpWidth,
        loadBalancer,
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
      uint8_t deviation = 25) {
    Runner::runDynamicLoadBalanceTest(
        ecmpWidth,
        loadBalancer,
        weights,
        loopThroughFrontPanel,
        loadBalanceExpected,
        deviation);
  }

  void runEcmpShrinkExpandLoadBalanceTest(
      unsigned int ecmpWidth,
      const cfg::LoadBalancer& loadBalancer,
      uint8_t deviation = 25) {
    Runner::runEcmpShrinkExpandLoadBalanceTest(
        ecmpWidth, loadBalancer, deviation);
  }

  void runTestAcrossWarmBoots(
      SETUP_FN setup,
      VERIFY_FN verify,
      SETUP_POSTWB_FN setupPostWarmboot,
      VERIFY_POSTWB_FN verifyPostWarmboot) override {
    Test::verifyAcrossWarmBoots(
        std::move(setup),
        std::move(verify),
        std::move(setupPostWarmboot),
        std::move(verifyPostWarmboot));
  }

  TestEnsembleIf* getEnsemble() override {
    return getAgentEnsemble();
  }

  const TestEnsembleIf* getEnsemble() const override {
    // isFeatureSupported();
    return getAgentEnsemble();
  }
};

template <typename EcmpDataPlateUtils>
class AgentIpLoadBalancerTest
    : public AgentLoadBalancerTest<EcmpDataPlateUtils> {
 public:
  std::unique_ptr<EcmpDataPlateUtils> getECMPHelper() override {
    if (!this->getEnsemble()) {
      // run during listing produciton features
      return nullptr;
    }
    return std::make_unique<EcmpDataPlateUtils>(
        this->getEnsemble(), RouterID(0));
  }
};

template <typename EcmpDataPlateUtils>
class AgentIp2MplsLoadBalancerTest
    : public AgentLoadBalancerTest<EcmpDataPlateUtils> {
 public:
  std::unique_ptr<EcmpDataPlateUtils> getECMPHelper() override {
    if (!this->getEnsemble()) {
      // run during listing produciton features
      return nullptr;
    }
    return std::make_unique<EcmpDataPlateUtils>(
        this->getEnsemble(), RouterID(0), utility::kHwTestLabelStacks());
  }
};

template <
    typename EcmpDataPlateUtils,
    LabelForwardingAction::LabelForwardingType type>
class AgentMpls2MplsLoadBalancerTest
    : public AgentLoadBalancerTest<EcmpDataPlateUtils> {
 public:
  std::unique_ptr<EcmpDataPlateUtils> getECMPHelper() override {
    if (!this->getEnsemble()) {
      // run during listing produciton features
      return nullptr;
    }
    return std::make_unique<EcmpDataPlateUtils>(
        this->getEnsemble(),
        MPLSHdr::Label(utility::kHwTestMplsLabel, 0, false, 127),
        type);
  }
};

class AgentLoadBalancerTestV4
    : public AgentIpLoadBalancerTest<utility::HwIpV4EcmpDataPlaneTestUtil> {};

class AgentLoadBalancerTestV6
    : public AgentIpLoadBalancerTest<utility::HwIpV6EcmpDataPlaneTestUtil> {};

class AgentLoadBalancerTestV4ToMpls
    : public AgentIp2MplsLoadBalancerTest<
          utility::HwIpV4EcmpDataPlaneTestUtil> {};

class AgentLoadBalancerTestV6ToMpls
    : public AgentIp2MplsLoadBalancerTest<
          utility::HwIpV6EcmpDataPlaneTestUtil> {};

class AgentLoadBalancerTestV4InMplsSwap
    : public AgentMpls2MplsLoadBalancerTest<
          utility::HwMplsV4EcmpDataPlaneTestUtil,
          LabelForwardingAction::LabelForwardingType::SWAP> {};

class AgentLoadBalancerTestV6InMplsSwap
    : public AgentMpls2MplsLoadBalancerTest<
          utility::HwMplsV6EcmpDataPlaneTestUtil,
          LabelForwardingAction::LabelForwardingType::SWAP> {};

class AgentLoadBalancerTestV4InMplsPhp
    : public AgentMpls2MplsLoadBalancerTest<
          utility::HwMplsV4EcmpDataPlaneTestUtil,
          LabelForwardingAction::LabelForwardingType::PHP> {};

class AgentLoadBalancerTestV6InMplsPhp
    : public AgentMpls2MplsLoadBalancerTest<
          utility::HwMplsV6EcmpDataPlaneTestUtil,
          LabelForwardingAction::LabelForwardingType::PHP> {};

RUN_ALL_HW_LOAD_BALANCER_ECMP_TEST_CPU(AgentLoadBalancerTestV4)
RUN_ALL_HW_LOAD_BALANCER_ECMP_TEST_CPU(AgentLoadBalancerTestV6)
RUN_ALL_HW_LOAD_BALANCER_ECMP_TEST_CPU(AgentLoadBalancerTestV4ToMpls)
RUN_ALL_HW_LOAD_BALANCER_ECMP_TEST_CPU(AgentLoadBalancerTestV6ToMpls)

RUN_ALL_HW_LOAD_BALANCER_ECMP_TEST_FRONT_PANEL(AgentLoadBalancerTestV4)
RUN_ALL_HW_LOAD_BALANCER_ECMP_TEST_FRONT_PANEL(AgentLoadBalancerTestV6)
RUN_ALL_HW_LOAD_BALANCER_ECMP_TEST_FRONT_PANEL(AgentLoadBalancerTestV4ToMpls)
RUN_ALL_HW_LOAD_BALANCER_ECMP_TEST_FRONT_PANEL(AgentLoadBalancerTestV6ToMpls)

RUN_ALL_HW_LOAD_BALANCER_ECMP_TEST_FRONT_PANEL(
    AgentLoadBalancerTestV4InMplsSwap)
RUN_ALL_HW_LOAD_BALANCER_ECMP_TEST_FRONT_PANEL(
    AgentLoadBalancerTestV6InMplsSwap)
RUN_ALL_HW_LOAD_BALANCER_ECMP_TEST_FRONT_PANEL(AgentLoadBalancerTestV4InMplsPhp)
RUN_ALL_HW_LOAD_BALANCER_ECMP_TEST_FRONT_PANEL(AgentLoadBalancerTestV6InMplsPhp)

} // namespace facebook::fboss
