// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <gtest/gtest.h>

#include "fboss/agent/test/AgentHwTest.h"

#include "fboss/agent/test/utils/ConfigUtils.h"
#include "fboss/agent/test/utils/LoadBalancerTestRunner.h"

namespace facebook::fboss {

template <typename EcmpTestHelperT, bool kWideEcmp>
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
    if constexpr (!kWideEcmp) {
      return {
          production_features::ProductionFeature::ECMP_LOAD_BALANCER,
      };
    } else {
      return {
          production_features::ProductionFeature::ECMP_LOAD_BALANCER,
          production_features::ProductionFeature::WIDE_ECMP,
      };
    }
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
      cfg::SwitchingMode preWBMode = cfg::SwitchingMode::FIXED_ASSIGNMENT,
      cfg::SwitchingMode postWBMode = cfg::SwitchingMode::FLOWLET_QUALITY,
      uint8_t deviation = 25) {
    Runner::runDynamicLoadBalanceTest(
        ecmpWidth,
        loadBalancer,
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

template <typename EcmpDataPlateUtils, bool kWideEcmp = false>
class AgentIpLoadBalancerTest
    : public AgentLoadBalancerTest<EcmpDataPlateUtils, kWideEcmp> {
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

template <typename EcmpDataPlateUtils, bool kWideEcmp = false>
class AgentIp2MplsLoadBalancerTest
    : public AgentLoadBalancerTest<EcmpDataPlateUtils, kWideEcmp> {
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
    LabelForwardingAction::LabelForwardingType type,
    bool kWideEcmp = false>
class AgentMpls2MplsLoadBalancerTest
    : public AgentLoadBalancerTest<EcmpDataPlateUtils, kWideEcmp> {
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

class AgentLoadBalancerTestV4Wide : public AgentIpLoadBalancerTest<
                                        utility::HwIpV4EcmpDataPlaneTestUtil,
                                        true> {};

class AgentLoadBalancerTestV6
    : public AgentIpLoadBalancerTest<utility::HwIpV6EcmpDataPlaneTestUtil> {};

class AgentLoadBalancerTestV6Wide : public AgentIpLoadBalancerTest<
                                        utility::HwIpV6EcmpDataPlaneTestUtil,
                                        true> {};

class AgentLoadBalancerTestV4ToMpls
    : public AgentIp2MplsLoadBalancerTest<
          utility::HwIpV4EcmpDataPlaneTestUtil> {};

class AgentLoadBalancerTestV4ToMplsWide
    : public AgentIp2MplsLoadBalancerTest<
          utility::HwIpV4EcmpDataPlaneTestUtil,
          true> {};

class AgentLoadBalancerTestV6ToMpls
    : public AgentIp2MplsLoadBalancerTest<
          utility::HwIpV6EcmpDataPlaneTestUtil> {};

class AgentLoadBalancerTestV6ToMplsWide
    : public AgentIp2MplsLoadBalancerTest<
          utility::HwIpV6EcmpDataPlaneTestUtil,
          true> {};

class AgentLoadBalancerTestV4InMplsSwap
    : public AgentMpls2MplsLoadBalancerTest<
          utility::HwMplsV4EcmpDataPlaneTestUtil,
          LabelForwardingAction::LabelForwardingType::SWAP> {};

class AgentLoadBalancerTestV4InMplsSwapWide
    : public AgentMpls2MplsLoadBalancerTest<
          utility::HwMplsV4EcmpDataPlaneTestUtil,
          LabelForwardingAction::LabelForwardingType::SWAP,
          true> {};

class AgentLoadBalancerTestV6InMplsSwap
    : public AgentMpls2MplsLoadBalancerTest<
          utility::HwMplsV6EcmpDataPlaneTestUtil,
          LabelForwardingAction::LabelForwardingType::SWAP> {};

class AgentLoadBalancerTestV6InMplsSwapWide
    : public AgentMpls2MplsLoadBalancerTest<
          utility::HwMplsV6EcmpDataPlaneTestUtil,
          LabelForwardingAction::LabelForwardingType::SWAP,
          true> {};

class AgentLoadBalancerTestV4InMplsPhp
    : public AgentMpls2MplsLoadBalancerTest<
          utility::HwMplsV4EcmpDataPlaneTestUtil,
          LabelForwardingAction::LabelForwardingType::PHP> {};

class AgentLoadBalancerTestV4InMplsPhpWide
    : public AgentMpls2MplsLoadBalancerTest<
          utility::HwMplsV4EcmpDataPlaneTestUtil,
          LabelForwardingAction::LabelForwardingType::PHP,
          true> {};

class AgentLoadBalancerTestV6InMplsPhp
    : public AgentMpls2MplsLoadBalancerTest<
          utility::HwMplsV6EcmpDataPlaneTestUtil,
          LabelForwardingAction::LabelForwardingType::PHP> {};

class AgentLoadBalancerTestV6InMplsPhpWide
    : public AgentMpls2MplsLoadBalancerTest<
          utility::HwMplsV6EcmpDataPlaneTestUtil,
          LabelForwardingAction::LabelForwardingType::PHP,
          true> {};

RUN_ALL_HW_LOAD_BALANCER_ECMP_TEST_CPU(AgentLoadBalancerTestV4)
RUN_ALL_HW_LOAD_BALANCER_ECMP_TEST_CPU(AgentLoadBalancerTestV6)
RUN_ALL_HW_LOAD_BALANCER_ECMP_TEST_CPU(AgentLoadBalancerTestV4ToMpls)
RUN_ALL_HW_LOAD_BALANCER_ECMP_TEST_CPU(AgentLoadBalancerTestV6ToMpls)

RUN_ALL_HW_LOAD_BALANCER_UCMP_TEST_CPU(AgentLoadBalancerTestV4)
RUN_ALL_HW_LOAD_BALANCER_UCMP_TEST_CPU(AgentLoadBalancerTestV6)
RUN_ALL_HW_LOAD_BALANCER_UCMP_TEST_CPU(AgentLoadBalancerTestV4ToMpls)
RUN_ALL_HW_LOAD_BALANCER_UCMP_TEST_CPU(AgentLoadBalancerTestV6ToMpls)

RUN_ALL_HW_LOAD_BALANCER_WIDE_UCMP_TEST_CPU(AgentLoadBalancerTestV4Wide)
RUN_ALL_HW_LOAD_BALANCER_WIDE_UCMP_TEST_CPU(AgentLoadBalancerTestV6Wide)
RUN_ALL_HW_LOAD_BALANCER_WIDE_UCMP_TEST_CPU(AgentLoadBalancerTestV4ToMplsWide)
RUN_ALL_HW_LOAD_BALANCER_WIDE_UCMP_TEST_CPU(AgentLoadBalancerTestV6ToMplsWide)

RUN_ALL_HW_LOAD_BALANCER_ECMP_TEST_FRONT_PANEL(AgentLoadBalancerTestV4)
RUN_ALL_HW_LOAD_BALANCER_ECMP_TEST_FRONT_PANEL(AgentLoadBalancerTestV6)
RUN_ALL_HW_LOAD_BALANCER_ECMP_TEST_FRONT_PANEL(AgentLoadBalancerTestV4ToMpls)
RUN_ALL_HW_LOAD_BALANCER_ECMP_TEST_FRONT_PANEL(AgentLoadBalancerTestV6ToMpls)

RUN_ALL_HW_LOAD_BALANCER_UCMP_TEST_FRONT_PANEL(AgentLoadBalancerTestV4)
RUN_ALL_HW_LOAD_BALANCER_UCMP_TEST_FRONT_PANEL(AgentLoadBalancerTestV6)
RUN_ALL_HW_LOAD_BALANCER_UCMP_TEST_FRONT_PANEL(AgentLoadBalancerTestV4ToMpls)
RUN_ALL_HW_LOAD_BALANCER_UCMP_TEST_FRONT_PANEL(AgentLoadBalancerTestV6ToMpls)

RUN_ALL_HW_LOAD_BALANCER_WIDE_UCMP_TEST_FRONT_PANEL(AgentLoadBalancerTestV4Wide)
RUN_ALL_HW_LOAD_BALANCER_WIDE_UCMP_TEST_FRONT_PANEL(AgentLoadBalancerTestV6Wide)
RUN_ALL_HW_LOAD_BALANCER_WIDE_UCMP_TEST_FRONT_PANEL(
    AgentLoadBalancerTestV4ToMplsWide)
RUN_ALL_HW_LOAD_BALANCER_WIDE_UCMP_TEST_FRONT_PANEL(
    AgentLoadBalancerTestV6ToMplsWide)

RUN_ALL_HW_LOAD_BALANCER_ECMP_TEST_FRONT_PANEL(
    AgentLoadBalancerTestV4InMplsSwap)
RUN_ALL_HW_LOAD_BALANCER_ECMP_TEST_FRONT_PANEL(
    AgentLoadBalancerTestV6InMplsSwap)
RUN_ALL_HW_LOAD_BALANCER_ECMP_TEST_FRONT_PANEL(AgentLoadBalancerTestV4InMplsPhp)
RUN_ALL_HW_LOAD_BALANCER_ECMP_TEST_FRONT_PANEL(AgentLoadBalancerTestV6InMplsPhp)

RUN_ALL_HW_LOAD_BALANCER_UCMP_TEST_FRONT_PANEL(
    AgentLoadBalancerTestV4InMplsSwap)
RUN_ALL_HW_LOAD_BALANCER_UCMP_TEST_FRONT_PANEL(
    AgentLoadBalancerTestV6InMplsSwap)
RUN_ALL_HW_LOAD_BALANCER_UCMP_TEST_FRONT_PANEL(AgentLoadBalancerTestV4InMplsPhp)
RUN_ALL_HW_LOAD_BALANCER_UCMP_TEST_FRONT_PANEL(AgentLoadBalancerTestV6InMplsPhp)

RUN_ALL_HW_LOAD_BALANCER_WIDE_UCMP_TEST_FRONT_PANEL(
    AgentLoadBalancerTestV4InMplsSwapWide)
RUN_ALL_HW_LOAD_BALANCER_WIDE_UCMP_TEST_FRONT_PANEL(
    AgentLoadBalancerTestV6InMplsSwapWide)
RUN_ALL_HW_LOAD_BALANCER_WIDE_UCMP_TEST_FRONT_PANEL(
    AgentLoadBalancerTestV4InMplsPhpWide)
RUN_ALL_HW_LOAD_BALANCER_WIDE_UCMP_TEST_FRONT_PANEL(
    AgentLoadBalancerTestV6InMplsPhpWide)

} // namespace facebook::fboss
