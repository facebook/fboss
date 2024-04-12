// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/agent/test/utils/ConfigUtils.h"
#include "fboss/agent/test/utils/EcmpDataPlaneTestUtil.h"
#include "fboss/agent/test/utils/LoadBalancerTestUtils.h"

#include <boost/preprocessor/cat.hpp>
#include <boost/preprocessor/facilities/expand.hpp>
#include <boost/preprocessor/stringize.hpp>

#include <gtest/gtest.h>

#define TRAFFIC_NAME(TRAFFIC_TYPE) BOOST_PP_CAT(TRAFFIC_TYPE, Traffic)
#define HASH_NAME(HASH_TYPE) BOOST_PP_CAT(HASH_TYPE, Hash)
#define LB_NAME(MULTIPATH_TYPE) BOOST_PP_CAT(MULTIPATH_TYPE, LoadBalance)

#define TEST_NAME(MULTIPATH_TYPE, HASH_TYPE, TRAFFIC_TYPE) \
  BOOST_PP_CAT(                                            \
      LB_NAME(MULTIPATH_TYPE),                             \
      BOOST_PP_CAT(HASH_NAME(HASH_TYPE), TRAFFIC_NAME(TRAFFIC_TYPE)))

#define ECMP_SHRINK_EXPAND_TEST_NAME(HASH_TYPE) \
  BOOST_PP_CAT(                                 \
      ECMPShrinkExpandLoadBalance,              \
      BOOST_PP_CAT(HASH_NAME(HASH_TYPE), TRAFFIC_NAME(Cpu)))

/*
 * Run a particular combination of HwLoadBalancer test
 * For ASICs that don't support customization of hash fields, full hash
 * and half hash tests are identical. So skip half hash tests and just
 * run full hash tests
 * */
#define RUN_HW_LOAD_BALANCER_TEST(                                           \
    TEST_FIXTURE, MULTIPATH_TYPE, HASH_TYPE, TRAFFIC_TYPE)                   \
  TEST_F(TEST_FIXTURE, TEST_NAME(MULTIPATH_TYPE, HASH_TYPE, TRAFFIC_TYPE)) { \
    if (BOOST_PP_STRINGIZE(MULTIPATH_TYPE) == std::string{"WideUcmp"} &&                    \
            !isFeatureSupported(HwAsic::Feature::WIDE_ECMP)) {               \
      return;                                                                \
    }                                                                        \
    static bool kLoopThroughFrontPanelPort =                                 \
        (BOOST_PP_STRINGIZE(TRAFFIC_TYPE) != std::string{"Cpu"});            \
    runLoadBalanceTest(                                                      \
        8,                                                                   \
        facebook::fboss::utility::getEcmp##HASH_TYPE##HashConfig(            \
            *getHwAsic()),                                                   \
        facebook::fboss::utility::kHwTest##MULTIPATH_TYPE##Weights(),        \
        kLoopThroughFrontPanelPort,                                          \
        true);                                                               \
  }

#define RUN_HW_LOAD_BALANCER_NEGATIVE_TEST(                                  \
    TEST_FIXTURE, MULTIPATH_TYPE, HASH_TYPE, TRAFFIC_TYPE)                   \
  TEST_F(TEST_FIXTURE, TEST_NAME(MULTIPATH_TYPE, HASH_TYPE, TRAFFIC_TYPE)) { \
    if (BOOST_PP_STRINGIZE(MULTIPATH_TYPE) == std::string{"WideUcmp"} &&                    \
            !isFeatureSupported(HwAsic::Feature::WIDE_ECMP)) {               \
      return;                                                                \
    }                                                                        \
    static bool kLoopThroughFrontPanelPort =                                 \
        (BOOST_PP_STRINGIZE(TRAFFIC_TYPE) != std::string{"Cpu"});            \
    runLoadBalanceTest(                                                      \
        8,                                                                   \
        facebook::fboss::utility::getEcmp##HASH_TYPE##HashConfig(            \
            *getHwAsic()),                                                   \
        facebook::fboss::utility::kHwTest##MULTIPATH_TYPE##Weights(),        \
        kLoopThroughFrontPanelPort,                                          \
        false);                                                              \
  }

// Adding new load balancer test for DLB since DLB need deviation of ~30%
// in some runs.
#define RUN_HW_LOAD_BALANCER_TEST_FOR_DLB(                                   \
    TEST_FIXTURE, MULTIPATH_TYPE, HASH_TYPE, TRAFFIC_TYPE)                   \
  TEST_F(TEST_FIXTURE, TEST_NAME(MULTIPATH_TYPE, HASH_TYPE, TRAFFIC_TYPE)) { \
    static bool kLoopThroughFrontPanelPort =                                 \
        (BOOST_PP_STRINGIZE(TRAFFIC_TYPE) != std::string{"Cpu"});            \
    runDynamicLoadBalanceTest(                                               \
        8,                                                                   \
        facebook::fboss::utility::getEcmp##HASH_TYPE##HashConfig(            \
            *getHwAsic()),                                                   \
        facebook::fboss::utility::kHwTest##MULTIPATH_TYPE##Weights(),        \
        kLoopThroughFrontPanelPort,                                          \
        true,                                                                \
        35);                                                                 \
  }

#define RUN_HW_LOAD_BALANCER_TEST_FOR_ECMP_TO_DLB(                           \
    TEST_FIXTURE, MULTIPATH_TYPE, HASH_TYPE, TRAFFIC_TYPE)                   \
  TEST_F(TEST_FIXTURE, TEST_NAME(MULTIPATH_TYPE, HASH_TYPE, TRAFFIC_TYPE)) { \
    static bool kLoopThroughFrontPanelPort =                                 \
        (BOOST_PP_STRINGIZE(TRAFFIC_TYPE) != std::string{"Cpu"});            \
    runLoadBalanceTest(                                                      \
        8,                                                                   \
        facebook::fboss::utility::getEcmp##HASH_TYPE##HashConfig(            \
            *getHwAsic()),                                                   \
        facebook::fboss::utility::kHwTest##MULTIPATH_TYPE##Weights(),        \
        kLoopThroughFrontPanelPort,                                          \
        false, /* since ECMP expected to send traffic on single link */      \
        35 /* DLB needs 30% deviation */);                                   \
  }

#define RUN_SHRINK_EXPAND_HW_LOAD_BALANCER_TEST(TEST_FIXTURE, HASH_TYPE) \
  TEST_F(TEST_FIXTURE, ECMP_SHRINK_EXPAND_TEST_NAME(HASH_TYPE)) {        \
    runEcmpShrinkExpandLoadBalanceTest(                                  \
        8,                                                               \
        facebook::fboss::utility::getEcmp##HASH_TYPE##HashConfig(        \
            *getHwAsic()));                                              \
  }

#define RUN_HW_LOAD_BALANCER_TEST_CPU(TEST_FIXTURE, MULTIPATH_TYPE, HASH_TYPE) \
  RUN_HW_LOAD_BALANCER_TEST(TEST_FIXTURE, MULTIPATH_TYPE, HASH_TYPE, Cpu)

#define RUN_HW_LOAD_BALANCER_TEST_FRONT_PANEL( \
    TEST_FIXTURE, MULTIPATH_TYPE, HASH_TYPE)   \
  RUN_HW_LOAD_BALANCER_TEST(TEST_FIXTURE, MULTIPATH_TYPE, HASH_TYPE, FrontPanel)

#define RUN_HW_LOAD_BALANCER_NEGATIVE_TEST_FRONT_PANEL( \
    TEST_FIXTURE, MULTIPATH_TYPE, HASH_TYPE)            \
  RUN_HW_LOAD_BALANCER_NEGATIVE_TEST(                   \
      TEST_FIXTURE, MULTIPATH_TYPE, HASH_TYPE, FrontPanel)

#define RUN_ALL_HW_LOAD_BALANCER_TEST_CPU(TEST_FIXTURE)       \
  RUN_HW_LOAD_BALANCER_TEST_CPU(TEST_FIXTURE, Ecmp, Full)     \
  RUN_HW_LOAD_BALANCER_TEST_CPU(TEST_FIXTURE, Ecmp, Half)     \
  RUN_HW_LOAD_BALANCER_TEST_CPU(TEST_FIXTURE, Ucmp, Full)     \
  RUN_HW_LOAD_BALANCER_TEST_CPU(TEST_FIXTURE, Ucmp, Half)     \
  RUN_HW_LOAD_BALANCER_TEST_CPU(TEST_FIXTURE, WideUcmp, Full) \
  RUN_HW_LOAD_BALANCER_TEST_CPU(TEST_FIXTURE, WideUcmp, Half)

#define RUN_ALL_HW_LOAD_BALANCER_TEST_FRONT_PANEL(TEST_FIXTURE)       \
  RUN_HW_LOAD_BALANCER_TEST_FRONT_PANEL(TEST_FIXTURE, Ecmp, Full)     \
  RUN_HW_LOAD_BALANCER_TEST_FRONT_PANEL(TEST_FIXTURE, Ecmp, Half)     \
  RUN_HW_LOAD_BALANCER_TEST_FRONT_PANEL(TEST_FIXTURE, Ucmp, Full)     \
  RUN_HW_LOAD_BALANCER_TEST_FRONT_PANEL(TEST_FIXTURE, Ucmp, Half)     \
  RUN_HW_LOAD_BALANCER_TEST_FRONT_PANEL(TEST_FIXTURE, WideUcmp, Full) \
  RUN_HW_LOAD_BALANCER_TEST_FRONT_PANEL(TEST_FIXTURE, WideUcmp, Half)

#define RUN_SHRINK_EXPAND_HW_LOAD_BALANCER_TEST_CPU(TEST_FIXTURE) \
  RUN_SHRINK_EXPAND_HW_LOAD_BALANCER_TEST(TEST_FIXTURE, Full)     \
  RUN_SHRINK_EXPAND_HW_LOAD_BALANCER_TEST(TEST_FIXTURE, Half)

#define RUN_ALL_HW_LOAD_BALANCER_TEST(TEST_FIXTURE)       \
  RUN_ALL_HW_LOAD_BALANCER_TEST_CPU(TEST_FIXTURE)         \
  RUN_ALL_HW_LOAD_BALANCER_TEST_FRONT_PANEL(TEST_FIXTURE) \
  RUN_SHRINK_EXPAND_HW_LOAD_BALANCER_TEST_CPU(TEST_FIXTURE)

namespace facebook::fboss::utility {

template <typename EcmpTestHelperT, bool kFlowLetSwitching = false>
class HwLoadBalancerTestRunner {
 public:
  using SETUP_FN = std::function<void()>;
  using VERIFY_FN = std::function<void()>;
  using SETUP_POSTWB_FN = std::function<void()>;
  using VERIFY_POSTWB_FN = std::function<void()>;

  virtual ~HwLoadBalancerTestRunner() = default;

 protected:
  virtual void runTestAcrossWarmBoots(
      SETUP_FN setup,
      VERIFY_FN verify,
      SETUP_POSTWB_FN setupPostWarmboot,
      VERIFY_POSTWB_FN verifyPostWarmboot) = 0;

  virtual std::unique_ptr<EcmpTestHelperT> getECMPHelper() = 0;

  virtual TestEnsembleIf* getEnsemble() = 0;
  virtual const TestEnsembleIf* getEnsemble() const = 0;
  std::vector<PortID> getMasterLogicalPortIds() const {
    return getEnsemble()->masterLogicalPortIds();
  }

  void setEcmpHelper() {
    helper_ = getECMPHelper();
  }

  virtual bool skipTest() const {
    return false;
  }

  bool isFeatureSupported(HwAsic::Feature feature) const {
    return getEnsemble()->getHwAsicTable()->isFeatureSupportedOnAnyAsic(
        feature);
  }

  const HwAsic* getHwAsic() const {
    auto switchIds = getEnsemble()->getHwAsicTable()->getSwitchIDs();
    CHECK_GE(switchIds.size(), 1);
    return getEnsemble()->getHwAsicTable()->getHwAsic(*switchIds.begin());
  }

  void runLoadBalanceTest(
      unsigned int ecmpWidth,
      const cfg::LoadBalancer& loadBalancer,
      const std::vector<NextHopWeight>& weights,
      bool loopThroughFrontPanel = false,
      bool loadBalanceExpected = true,
      uint8_t deviation = 25) {
    if (skipTest()) {
#if defined(GTEST_SKIP)
      GTEST_SKIP();
#endif
      return;
    }
    auto setup = [=, this]() {
      helper_->programRoutesAndLoadBalancer(ecmpWidth, weights, loadBalancer);
    };
    auto verify = [=, this]() {
      helper_->pumpTrafficPortAndVerifyLoadBalanced(
          ecmpWidth,
          loopThroughFrontPanel,
          weights,
          deviation,
          loadBalanceExpected);
    };

    auto setupPostWB = [&]() {
      if constexpr (kFlowLetSwitching) {
        auto cfg = utility::onePortPerInterfaceConfig(
            getEnsemble(), getMasterLogicalPortIds());
        // Add flowlet config to convert ECMP to DLB
        cfg.udfConfig() = utility::addUdfFlowletAclConfig();
        utility::addFlowletConfigs(cfg, getMasterLogicalPortIds());
        getEnsemble()->applyNewConfig(cfg);
      }
    };

    auto verifyPostWB = [&]() {
      if constexpr (kFlowLetSwitching) {
        XLOG(DBG3) << "setting ECMP Member Status: ";
        setEcmpMemberStatus(getEnsemble());
        loadBalanceExpected = true;
        helper_->pumpTrafficPortAndVerifyLoadBalanced(
            ecmpWidth,
            loopThroughFrontPanel,
            weights,
            deviation,
            loadBalanceExpected);
        auto l3EcmpDlbFailPackets = getL3EcmpDlbFailPackets(getEnsemble());
        // utility::getL3EcmpDlbFailPackets(getEnsemble());
        XLOG(INFO) << " L3 ECMP Dlb fail packets: " << l3EcmpDlbFailPackets;
        // verfiy the Dlb fail packets is zero
        EXPECT_EQ(l3EcmpDlbFailPackets, 0);
      }
    };

    runTestAcrossWarmBoots(setup, verify, setupPostWB, verifyPostWB);
  }

  void runEcmpShrinkExpandLoadBalanceTest(
      unsigned int ecmpWidth,
      const cfg::LoadBalancer& loadBalancer,
      uint8_t deviation = 25) {
    if (skipTest()) {
#if defined(GTEST_SKIP)
      GTEST_SKIP();
#endif
      return;
    }
    unsigned int minLinksLoadbalanceTest = 1;
    auto setup = [=, this]() {
      helper_->programRoutesAndLoadBalancer(
          ecmpWidth, {} /*weights*/, loadBalancer);
    };
    auto verify = [=, this]() {
      unsigned int width = ecmpWidth;

      while (width > minLinksLoadbalanceTest) {
        width--;
        helper_->shrinkECMP(width);

        helper_->pumpTrafficPortAndVerifyLoadBalanced(
            width,
            false, /*loopThroughFrontPanel*/
            {}, /*weights*/
            deviation,
            true);
        // Now that we are done checking, unresolve the shrunk next hop
        helper_->unresolveNextHop(width);
      }

      while (width < ecmpWidth) {
        helper_->expandECMP(width);
        helper_->pumpTrafficPortAndVerifyLoadBalanced(
            width,
            false, /*loopThroughFrontPanel*/
            {}, /*weights*/
            deviation,
            true);
        width++;
      }
    };
    setup();
    verify();
  }

  void runDynamicLoadBalanceTest(
      unsigned int ecmpWidth,
      const cfg::LoadBalancer& loadBalancer,
      const std::vector<NextHopWeight>& weights,
      bool loopThroughFrontPanel = false,
      bool loadBalanceExpected = true,
      uint8_t deviation = 25) {
    if (skipTest()) {
#if defined(GTEST_SKIP)
      GTEST_SKIP();
#endif
      return;
    }
    auto setup = [=, this]() {
      helper_->programRoutesAndLoadBalancer(ecmpWidth, weights, loadBalancer);
    };
    auto verify = [=, this]() {
      // DLB engine can not detect port member hardware status
      // when in "phy" loopback mode.
      // Hence we are setting it forcibly here again for all the ecmp members.
      if constexpr (kFlowLetSwitching) {
        XLOG(DBG3) << "setting ECMP Member Status: ";
        // utility::setEcmpMemberStatus(getEnsemble());
        setEcmpMemberStatus(getEnsemble());
      }
      helper_->pumpTrafficPortAndVerifyLoadBalanced(
          ecmpWidth,
          loopThroughFrontPanel,
          weights,
          deviation,
          loadBalanceExpected);
    };

    auto setupPostWB = [&]() {
      auto cfg = utility::onePortPerInterfaceConfig(
          getEnsemble(), getMasterLogicalPortIds());
      addLoadBalancerToConfig(cfg, getHwAsic(), utility::LBHash::FULL_HASH);
      // Remove the flowlet configs
      getEnsemble()->applyNewConfig(cfg);
    };

    auto verifyPostWB = [&]() {
      // DLB config is removed. Since it is a single stream,
      // all the traffic will move via single ECMP path.
      // Hence set the loadBalanceExpected as false.
      loadBalanceExpected = false;
      helper_->pumpTrafficPortAndVerifyLoadBalanced(
          ecmpWidth,
          loopThroughFrontPanel,
          weights,
          deviation,
          loadBalanceExpected);
    };

    runTestAcrossWarmBoots(setup, verify, setupPostWB, verifyPostWB);
  }

  EcmpTestHelperT* getEcmpSetupHelper() const {
    return helper_.get();
  }

  virtual void setEcmpMemberStatus(const TestEnsembleIf* /*ensemble*/) {
    XLOG(FATAL) << "setEcmpMemberStatus not implemented out of HwSwitch";
  }

  virtual int getL3EcmpDlbFailPackets(
      const TestEnsembleIf* /*ensemble*/) const {
    XLOG(FATAL) << "getL3EcmpDlbFailPackets not implemented out of HwSwitch";
    return 0;
  }

 private:
  std::unique_ptr<EcmpTestHelperT> helper_;
};
} // namespace facebook::fboss::utility
