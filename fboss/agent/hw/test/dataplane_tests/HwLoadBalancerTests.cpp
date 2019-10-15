/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/Platform.h"
#include "fboss/agent/hw/test/HwLinkStateDependentTest.h"
#include "fboss/agent/hw/test/HwTest.h"
#include "fboss/agent/hw/test/HwTestPacketUtils.h"
#include "fboss/agent/test/EcmpSetupHelper.h"

#include "fboss/agent/state/Interface.h"
#include "fboss/agent/state/SwitchState.h"

#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/hw/test/LoadBalancerUtils.h"
#include "fboss/agent/hw/test/dataplane_tests/HwEcmpDataPlaneTestUtil.h"

#include <folly/IPAddress.h>
#include <folly/Optional.h>

#include <memory>
#include <string>
#include <vector>

using folly::IPAddress;
using folly::IPAddressV6;
using std::string;
using std::vector;

using facebook::fboss::utility::addLoadBalancer;
using facebook::fboss::utility::getEcmpFullHashConfig;
using facebook::fboss::utility::getEcmpHalfHashConfig;

namespace {
const std::vector<facebook::fboss::NextHopWeight>
    kUcmpWeights{10, 20, 30, 40, 50, 60, 70, 80};
}

namespace facebook {
namespace fboss {

template <typename HwEcmpDataPlaneTestUtilT>
class HwLoadBalancerTest : public HwLinkStateDependentTest {
 private:
  cfg::SwitchConfig initialConfig() const override {
    auto cfg = utility::onePortPerVlanConfig(
        getHwSwitch(), masterLogicalPortIds(), cfg::PortLoopbackMode::MAC);
    return cfg;
  }

  void SetUp() override {
    HwLinkStateDependentTest::SetUp();
    helper = std::make_unique<HwEcmpDataPlaneTestUtilT>(
        getHwSwitchEnsemble(), RouterID(0));
  }

  void setupECMPForwarding(
      unsigned int ecmpWidth,
      const cfg::LoadBalancer& loadBalancer,
      const std::vector<NextHopWeight>& weights) {
    helper->setupECMPForwarding(ecmpWidth, weights);
    applyNewState(
        addLoadBalancer(getPlatform(), getProgrammedState(), loadBalancer));
  }

  void pumpTrafficPortAndVerifyLoadBalanced(
      unsigned int ecmpWidth,
      const std::vector<NextHopWeight>& weights,
      bool loopThroughFrontPanel) {
    helper->pumpTraffic(ecmpWidth, loopThroughFrontPanel);
    // Don't tolerate a deviation of > 25%
    EXPECT_TRUE(helper->isLoadBalanced(ecmpWidth, weights, 25));
  }

  void resolveNextHopsandClearStats(unsigned int ecmpWidth) {
    helper->resolveNextHopsandClearStats(ecmpWidth);
  }

  void shrinkECMP(unsigned int ecmpWidth) {
    helper->shrinkECMP(ecmpWidth);
    resolveNextHopsandClearStats(ecmpWidth);
  }

  void expandECMP(unsigned int ecmpWidth) {
    helper->expandECMP(ecmpWidth);
    resolveNextHopsandClearStats(ecmpWidth);
  }

 protected:
  void runLoadBalanceTest(
      unsigned int ecmpWidth,
      const cfg::LoadBalancer& loadBalancer,
      const std::vector<NextHopWeight>& weights,
      bool loopThroughFrontPanel = false) {
    auto setup = [=]() {
      setupECMPForwarding(ecmpWidth, loadBalancer, weights);
    };
    auto verify = [=]() {
      pumpTrafficPortAndVerifyLoadBalanced(
          ecmpWidth, weights, loopThroughFrontPanel);
    };
    verifyAcrossWarmBoots(setup, verify);
  }

  void runEcmpShrinkExpandLoadBalanceTest(
      unsigned int ecmpWidth,
      const cfg::LoadBalancer& loadBalancer) {
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
            false /*loopThroughFrontPanel*/);
      }

      while (width < ecmpWidth) {
        expandECMP(width);
        pumpTrafficPortAndVerifyLoadBalanced(
            width,
            {}, /*weights*/
            false /*loopThroughFrontPanel*/);
        width++;
      }
    };
    setup();
    verify();
  }

 private:
  std::unique_ptr<HwEcmpDataPlaneTestUtilT> helper;
};

class HwLoadBalancerTestIpV4
    : public HwLoadBalancerTest<utility::HwIpV4EcmpDataPlaneTestUtil> {};

class HwLoadBalancerTestIpV6
    : public HwLoadBalancerTest<utility::HwIpV6EcmpDataPlaneTestUtil> {};

// ECMP, CPU originated traffic
TEST_F(HwLoadBalancerTestIpV6, ECMPLoadBalanceFullHashCpuTraffic) {
  runLoadBalanceTest(8, getEcmpFullHashConfig(), {});
}
TEST_F(HwLoadBalancerTestIpV4, ECMPLoadBalanceFullHashCpuTraffic) {
  runLoadBalanceTest(8, getEcmpFullHashConfig(), {});
}
TEST_F(HwLoadBalancerTestIpV6, ECMPLoadBalanceHalfHashCpuTraffic) {
  runLoadBalanceTest(8, getEcmpHalfHashConfig(), {});
}
TEST_F(HwLoadBalancerTestIpV4, ECMPLoadBalanceHalfHashCpuTraffic) {
  runLoadBalanceTest(8, getEcmpHalfHashConfig(), {});
}

// ECMP, CPU originated traffic Shrink/Expand
TEST_F(HwLoadBalancerTestIpV6, ECMShrinkExpandLoadBalanceFullHashCpuTraffic) {
  runEcmpShrinkExpandLoadBalanceTest(8, getEcmpFullHashConfig());
}
TEST_F(HwLoadBalancerTestIpV4, ECMPShrinkExapndLoadBalanceFullHashCpuTraffic) {
  runEcmpShrinkExpandLoadBalanceTest(8, getEcmpFullHashConfig());
}
TEST_F(HwLoadBalancerTestIpV6, ECMPShrinkExpandLoadBalanceHalfHashCpuTraffic) {
  runEcmpShrinkExpandLoadBalanceTest(8, getEcmpHalfHashConfig());
}
TEST_F(HwLoadBalancerTestIpV4, ECMPShrinkExpandLoadBalanceHalfHashCpuTraffic) {
  runEcmpShrinkExpandLoadBalanceTest(8, getEcmpHalfHashConfig());
}

// UCMP, CPU originated traffic
TEST_F(HwLoadBalancerTestIpV6, UCMPLoadBalanceFullHashCpuTraffic) {
  runLoadBalanceTest(8, getEcmpFullHashConfig(), kUcmpWeights);
}

TEST_F(HwLoadBalancerTestIpV4, UCMPLoadBalanceFullHashCpuTraffic) {
  runLoadBalanceTest(8, getEcmpFullHashConfig(), kUcmpWeights);
}

TEST_F(HwLoadBalancerTestIpV6, UCMPLoadBalanceHalfHashCpuTraffic) {
  runLoadBalanceTest(8, getEcmpHalfHashConfig(), kUcmpWeights);
}

TEST_F(HwLoadBalancerTestIpV4, UCMPLoadBalanceHalfHashCpuTraffic) {
  runLoadBalanceTest(8, getEcmpHalfHashConfig(), kUcmpWeights);
}

// ECMP, Front Panel traffic
TEST_F(HwLoadBalancerTestIpV6, ECMPLoadBalanceFullHashFrontPanelTraffic) {
  runLoadBalanceTest(
      8, getEcmpFullHashConfig(), {}, true /* loopThroughFrontPanelPort */);
}
TEST_F(HwLoadBalancerTestIpV4, ECMPLoadBalanceFullHashFrontPanelTraffic) {
  runLoadBalanceTest(
      8, getEcmpFullHashConfig(), {}, true /* loopThroughFrontPanelPort */);
}
TEST_F(HwLoadBalancerTestIpV6, ECMPLoadBalanceHalfHashFrontPanelTraffic) {
  runLoadBalanceTest(
      8, getEcmpHalfHashConfig(), {}, true /* loopThroughFrontPanelPort */);
}
TEST_F(HwLoadBalancerTestIpV4, ECMPLoadBalanceHalfHashFrontPanelTraffic) {
  runLoadBalanceTest(
      8, getEcmpHalfHashConfig(), {}, true /* loopThroughFrontPanelPort */);
}

// UCMP, Front Panel traffic
TEST_F(HwLoadBalancerTestIpV6, UCMPLoadBalanceFullHashFrontPanelTraffic) {
  runLoadBalanceTest(
      8,
      getEcmpFullHashConfig(),
      kUcmpWeights,
      true /* loopThroughFrontPanelPort */);
}

TEST_F(HwLoadBalancerTestIpV4, UCMPLoadBalanceFullHashFrontPanelTraffic) {
  runLoadBalanceTest(
      8,
      getEcmpFullHashConfig(),
      kUcmpWeights,
      true /* loopThroughFrontPanelPort */);
}

TEST_F(HwLoadBalancerTestIpV6, UCMPLoadBalanceHalfHashFrontPanelTraffic) {
  runLoadBalanceTest(
      8,
      getEcmpHalfHashConfig(),
      kUcmpWeights,
      true /* loopThroughFrontPanelPort */);
}

TEST_F(HwLoadBalancerTestIpV4, UCMPLoadBalanceHalfHashFrontPanelTraffic) {
  runLoadBalanceTest(
      8,
      getEcmpHalfHashConfig(),
      kUcmpWeights,
      true /* loopThroughFrontPanelPort */);
}
} // namespace fboss
} // namespace facebook
