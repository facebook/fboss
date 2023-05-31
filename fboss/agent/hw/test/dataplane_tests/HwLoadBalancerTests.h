// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/hw/test/HwLinkStateDependentTest.h"
#include "fboss/agent/hw/test/LoadBalancerUtils.h"
#include "fboss/agent/hw/test/dataplane_tests/HwEcmpDataPlaneTestUtil.h"

#include <boost/preprocessor/cat.hpp>
#include <boost/preprocessor/facilities/expand.hpp>
#include <boost/preprocessor/stringize.hpp>

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
            !getPlatform()->getAsic()->isSupported(                          \
                HwAsic::Feature::WIDE_ECMP)) {                               \
      return;                                                                \
    }                                                                        \
    static bool kLoopThroughFrontPanelPort =                                 \
        (BOOST_PP_STRINGIZE(TRAFFIC_TYPE) != std::string{"Cpu"});            \
    runLoadBalanceTest(                                                      \
        8,                                                                   \
        facebook::fboss::utility::getEcmp##HASH_TYPE##HashConfig(            \
            getPlatform()),                                                  \
        facebook::fboss::utility::kHwTest##MULTIPATH_TYPE##Weights(),        \
        kLoopThroughFrontPanelPort,                                          \
        true);                                                               \
  }

#define RUN_HW_LOAD_BALANCER_NEGATIVE_TEST(                                  \
    TEST_FIXTURE, MULTIPATH_TYPE, HASH_TYPE, TRAFFIC_TYPE)                   \
  TEST_F(TEST_FIXTURE, TEST_NAME(MULTIPATH_TYPE, HASH_TYPE, TRAFFIC_TYPE)) { \
    if (BOOST_PP_STRINGIZE(MULTIPATH_TYPE) == std::string{"WideUcmp"} &&                    \
            !getPlatform()->getAsic()->isSupported(                          \
                HwAsic::Feature::WIDE_ECMP)) {                               \
      return;                                                                \
    }                                                                        \
    static bool kLoopThroughFrontPanelPort =                                 \
        (BOOST_PP_STRINGIZE(TRAFFIC_TYPE) != std::string{"Cpu"});            \
    runLoadBalanceTest(                                                      \
        8,                                                                   \
        facebook::fboss::utility::getEcmp##HASH_TYPE##HashConfig(            \
            getPlatform()),                                                  \
        facebook::fboss::utility::kHwTest##MULTIPATH_TYPE##Weights(),        \
        kLoopThroughFrontPanelPort,                                          \
        false);                                                              \
  }

#define RUN_SHRINK_EXPAND_HW_LOAD_BALANCER_TEST(TEST_FIXTURE, HASH_TYPE) \
  TEST_F(TEST_FIXTURE, ECMP_SHRINK_EXPAND_TEST_NAME(HASH_TYPE)) {        \
    runEcmpShrinkExpandLoadBalanceTest(                                  \
        8,                                                               \
        facebook::fboss::utility::getEcmp##HASH_TYPE##HashConfig(        \
            getPlatform()));                                             \
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

namespace facebook::fboss {

template <typename EcmpTestHelperT>
class HwLoadBalancerTest : public HwLinkStateDependentTest {
 private:
  cfg::SwitchConfig initialConfig() const override {
    auto cfg = utility::onePortPerInterfaceConfig(
        getHwSwitch(),
        masterLogicalPortIds(),
        getAsic()->desiredLoopbackModes());
    return cfg;
  }

  void SetUp() override {
    HwLinkStateDependentTest::SetUp();
    helper_ = getECMPHelper();
  }

  void programECMP(
      unsigned int ecmpWidth,
      const cfg::LoadBalancer& loadBalancer,
      const std::vector<NextHopWeight>& weights) {
    getEcmpSetupHelper()->programRoutes(ecmpWidth, weights);
    applyNewState(utility::setLoadBalancer(
        getPlatform(), getProgrammedState(), loadBalancer, scopeResolver()));
  }

  virtual std::unique_ptr<EcmpTestHelperT> getECMPHelper() = 0;

  void pumpTrafficPortAndVerifyLoadBalanced(
      unsigned int ecmpWidth,
      const std::vector<NextHopWeight>& weights,
      bool loopThroughFrontPanel,
      uint8_t deviation,
      bool loadBalanceExpected) {
    bool isLoadBalanced = utility::pumpTrafficAndVerifyLoadBalanced(
        [=]() { helper_->pumpTraffic(ecmpWidth, loopThroughFrontPanel); },
        [=]() {
          auto helper = helper_->ecmpSetupHelper();
          auto portDescs = helper->getPortDescs(ecmpWidth);
          auto ports = std::make_unique<std::vector<int32_t>>();
          for (auto portDesc : portDescs) {
            if (portDesc.isPhysicalPort()) {
              ports->push_back(portDesc.phyPortID());
            }
          }
          getHwSwitch()->clearPortStats(ports);
        },
        [=]() {
          return helper_->isLoadBalanced(ecmpWidth, weights, deviation);
        });

    if (loadBalanceExpected) {
      EXPECT_TRUE(isLoadBalanced);
    } else {
      EXPECT_FALSE(isLoadBalanced);
    }
  }

  void resolveNextHopsandClearStats(unsigned int ecmpWidth) {
    helper_->resolveNextHopsandClearStats(ecmpWidth);
  }

  void unresolveNextHop(unsigned int id) {
    helper_->unresolveNextHop(id);
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
  virtual bool skipTest() const {
    return false;
  }
  void runLoadBalanceTest(
      unsigned int ecmpWidth,
      const cfg::LoadBalancer& loadBalancer,
      const std::vector<NextHopWeight>& weights,
      bool loopThroughFrontPanel = false,
      bool loadBalanceExpected = true,
      uint8_t deviation = 25) {
    if (skipTest()) {
      return;
    }
    auto setup = [=]() { programECMP(ecmpWidth, loadBalancer, weights); };
    auto verify = [=]() {
      pumpTrafficPortAndVerifyLoadBalanced(
          ecmpWidth,
          weights,
          loopThroughFrontPanel,
          deviation,
          loadBalanceExpected);
    };
    verifyAcrossWarmBoots(setup, verify);
  }

  void runEcmpShrinkExpandLoadBalanceTest(
      unsigned int ecmpWidth,
      const cfg::LoadBalancer& loadBalancer,
      uint8_t deviation = 25) {
    if (skipTest()) {
      return;
    }
    unsigned int minLinksLoadbalanceTest = 1;
    auto setup = [=]() {
      programECMP(ecmpWidth, loadBalancer, {} /*weights*/);
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
            deviation,
            true);
        // Now that we are done checking, unresolve the shrunk next hop
        unresolveNextHop(width);
      }

      while (width < ecmpWidth) {
        expandECMP(width);
        pumpTrafficPortAndVerifyLoadBalanced(
            width,
            {}, /*weights*/
            false /*loopThroughFrontPanel*/,
            deviation,
            true);
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

} // namespace facebook::fboss
