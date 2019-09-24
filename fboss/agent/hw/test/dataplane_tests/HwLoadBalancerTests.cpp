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

class HwLoadBalancerTest : public HwLinkStateDependentTest {
 private:
  cfg::SwitchConfig initialConfig() const override {
    auto cfg = utility::onePortPerVlanConfig(
        getHwSwitch(), masterLogicalPortIds(), cfg::PortLoopbackMode::MAC);
    return cfg;
  }

  template <typename AddrT>
  void setupECMPForwarding(
      utility::EcmpSetupAnyNPorts<AddrT>& ecmpHelper,
      int ecmpWidth,
      const std::vector<NextHopWeight>& weights) {
    auto newState = ecmpHelper.setupECMPForwarding(
        ecmpHelper.resolveNextHops(getProgrammedState(), ecmpWidth),
        ecmpWidth,
        {{AddrT(), 0}},
        weights);
    applyNewState(newState);
  }

  void SetUp() override {
    HwLinkStateDependentTest::SetUp();
    ecmpHelper6 = std::make_unique<utility::EcmpSetupAnyNPorts6>(
        getProgrammedState(), RouterID(0));
    ecmpHelper4 = std::make_unique<utility::EcmpSetupAnyNPorts4>(
        getProgrammedState(), RouterID(0));
  }

  void setupECMP6andECMP4Forwarding(
      unsigned int ecmpWidth,
      const cfg::LoadBalancer& loadBalancer,
      const std::vector<NextHopWeight>& weights) {
    setupECMPForwarding(*ecmpHelper6, ecmpWidth, weights);
    setupECMPForwarding(*ecmpHelper4, ecmpWidth, weights);
    applyNewState(
        addLoadBalancer(getPlatform(), getProgrammedState(), loadBalancer));
  }

  void pumpTrafficPortAndVerifyLoadBalanced(
      unsigned int ecmpWidth,
      bool isV6,
      const std::vector<NextHopWeight>& weights,
      bool loopThroughFrontPanel) {
    folly::Optional<PortID> frontPanelPortToLoopTraffic;
    if (loopThroughFrontPanel) {
      frontPanelPortToLoopTraffic = isV6
          ? ecmpHelper6->ecmpPortDescriptorAt(ecmpWidth).phyPortID()
          : ecmpHelper4->ecmpPortDescriptorAt(ecmpWidth).phyPortID();
    }
    utility::pumpTraffic(
        isV6,
        getHwSwitch(),
        getPlatform()->getLocalMac(),
        frontPanelPortToLoopTraffic);
    auto ecmpPorts = isV6 ? ecmpHelper6->ecmpPortDescs(ecmpWidth)
                          : ecmpHelper4->ecmpPortDescs(ecmpWidth);
    // Don't tolerate a deviation of > 25%
    EXPECT_TRUE(
        utility::isLoadBalanced(getHwSwitchEnsemble(), ecmpPorts, weights, 25));
  }

  void resolveNextHopsandClearStats(unsigned int ecmpWidth) {
    applyNewState(
        ecmpHelper6->resolveNextHops(getProgrammedState(), ecmpWidth));
    applyNewState(
        ecmpHelper4->resolveNextHops(getProgrammedState(), ecmpWidth));
    while (ecmpWidth) {
      ecmpWidth--;
      auto v6Ports = {static_cast<int32_t>(
          ecmpHelper6->nhop(ecmpWidth).portDesc.phyPortID())};
      auto v4Ports = {static_cast<int32_t>(
          ecmpHelper4->nhop(ecmpWidth).portDesc.phyPortID())};
      getHwSwitch()->clearPortStats(
          std::make_unique<std::vector<int32_t>>(v6Ports));
      getHwSwitch()->clearPortStats(
          std::make_unique<std::vector<int32_t>>(v4Ports));
    }
  }

  void shrinkECMP(bool isV6, unsigned int ecmpWidth) {
    if (isV6) {
      bringDownPort(ecmpHelper6->nhop(ecmpWidth).portDesc.phyPortID());
    } else {
      bringDownPort(ecmpHelper4->nhop(ecmpWidth).portDesc.phyPortID());
    }
    resolveNextHopsandClearStats(ecmpWidth);
  }

  void expandECMP(bool isV6, unsigned int ecmpWidth) {
    if (isV6) {
      bringUpPort(ecmpHelper6->nhop(ecmpWidth).portDesc.phyPortID());
    } else {
      bringUpPort(ecmpHelper4->nhop(ecmpWidth).portDesc.phyPortID());
    }
    resolveNextHopsandClearStats(ecmpWidth);
  }

 protected:
  void runLoadBalanceTest(
      unsigned int ecmpWidth,
      const cfg::LoadBalancer& loadBalancer,
      bool isV6,
      const std::vector<NextHopWeight>& weights,
      bool loopThroughFrontPanel = false) {
    auto setup = [=]() {
      setupECMP6andECMP4Forwarding(ecmpWidth, loadBalancer, weights);
    };
    auto verify = [=]() {
      pumpTrafficPortAndVerifyLoadBalanced(
          ecmpWidth, isV6, weights, loopThroughFrontPanel);
    };
    verifyAcrossWarmBoots(setup, verify);
  }

  void runEcmpShrinkExpandLoadBalanceTest(
      unsigned int ecmpWidth,
      const cfg::LoadBalancer& loadBalancer,
      bool isV6) {
    unsigned int minLinksLoadbalanceTest = 1;
    auto setup = [=]() {
      setupECMP6andECMP4Forwarding(ecmpWidth, loadBalancer, {} /*weights*/);
    };
    auto verify = [=]() {
      unsigned int width = ecmpWidth;

      while (width > minLinksLoadbalanceTest) {
        width--;
        shrinkECMP(isV6, width);
        pumpTrafficPortAndVerifyLoadBalanced(
            width,
            isV6,
            {}, /*weights*/
            false /*loopThroughFrontPanel*/);
      }

      while (width < ecmpWidth) {
        expandECMP(isV6, width);
        pumpTrafficPortAndVerifyLoadBalanced(
            width,
            isV6,
            {}, /*weights*/
            false /*loopThroughFrontPanel*/);
        width++;
      }
    };
    setup();
    verify();
  }

 private:
  std::unique_ptr<utility::EcmpSetupAnyNPorts6> ecmpHelper6;
  std::unique_ptr<utility::EcmpSetupAnyNPorts4> ecmpHelper4;
};

// ECMP, CPU originated traffic
TEST_F(HwLoadBalancerTest, ECMPLoadBalanceFullHashV6CpuTraffic) {
  runLoadBalanceTest(8, getEcmpFullHashConfig(), true /* isV6 */, {});
}
TEST_F(HwLoadBalancerTest, ECMPLoadBalanceFullHashV4CpuTraffic) {
  runLoadBalanceTest(8, getEcmpFullHashConfig(), false /* isV6 */, {});
}
TEST_F(HwLoadBalancerTest, ECMPLoadBalanceHalfHashV6CpuTraffic) {
  runLoadBalanceTest(8, getEcmpHalfHashConfig(), true /* isV6 */, {});
}
TEST_F(HwLoadBalancerTest, ECMPLoadBalanceHalfHashV4CpuTraffic) {
  runLoadBalanceTest(8, getEcmpHalfHashConfig(), false /* isV6 */, {});
}

// ECMP, CPU originated traffic Shrink/Expand
TEST_F(HwLoadBalancerTest, ECMShrinkExpandLoadBalanceFullHashV6CpuTraffic) {
  runEcmpShrinkExpandLoadBalanceTest(8, getEcmpFullHashConfig(), true /*isV6*/);
}
TEST_F(HwLoadBalancerTest, ECMPShrinkExapndLoadBalanceFullHashV4CpuTraffic) {
  runEcmpShrinkExpandLoadBalanceTest(
      8, getEcmpFullHashConfig(), false /*isV6*/);
}
TEST_F(HwLoadBalancerTest, ECMPShrinkExpandLoadBalanceHalfHashV6CpuTraffic) {
  runEcmpShrinkExpandLoadBalanceTest(8, getEcmpHalfHashConfig(), true /*isV6*/);
}
TEST_F(HwLoadBalancerTest, ECMPShrinkExpandLoadBalanceHalfHashV4CpuTraffic) {
  runEcmpShrinkExpandLoadBalanceTest(
      8, getEcmpHalfHashConfig(), false /*isV6*/);
}

// UCMP, CPU originated traffic
TEST_F(HwLoadBalancerTest, UCMPLoadBalanceFullHashV6CpuTraffic) {
  runLoadBalanceTest(8, getEcmpFullHashConfig(), true /* isV6 */, kUcmpWeights);
}

TEST_F(HwLoadBalancerTest, UCMPLoadBalanceFullHashV4CpuTraffic) {
  runLoadBalanceTest(
      8, getEcmpFullHashConfig(), false /* isV6 */, kUcmpWeights);
}

TEST_F(HwLoadBalancerTest, UCMPLoadBalanceHalfHashV6CpuTraffic) {
  runLoadBalanceTest(8, getEcmpHalfHashConfig(), true /* isV6 */, kUcmpWeights);
}

TEST_F(HwLoadBalancerTest, UCMPLoadBalanceHalfHashV4CpuTraffic) {
  runLoadBalanceTest(
      8, getEcmpHalfHashConfig(), false /* isV6 */, kUcmpWeights);
}

// ECMP, Front Panel traffic
TEST_F(HwLoadBalancerTest, ECMPLoadBalanceFullHashV6FrontPanelTraffic) {
  runLoadBalanceTest(
      8,
      getEcmpFullHashConfig(),
      true /* isV6 */,
      {},
      true /* loopThroughFrontPanelPort */);
}
TEST_F(HwLoadBalancerTest, ECMPLoadBalanceFullHashV4FrontPanelTraffic) {
  runLoadBalanceTest(
      8,
      getEcmpFullHashConfig(),
      false /* isV6 */,
      {},
      true /* loopThroughFrontPanelPort */);
}
TEST_F(HwLoadBalancerTest, ECMPLoadBalanceHalfHashV6FrontPanelTraffic) {
  runLoadBalanceTest(
      8,
      getEcmpHalfHashConfig(),
      true /* isV6 */,
      {},
      true /* loopThroughFrontPanelPort */);
}
TEST_F(HwLoadBalancerTest, ECMPLoadBalanceHalfHashV4FrontPanelTraffic) {
  runLoadBalanceTest(
      8,
      getEcmpHalfHashConfig(),
      false /* isV6 */,
      {},
      true /* loopThroughFrontPanelPort */);
}

// UCMP, Front Panel traffic
TEST_F(HwLoadBalancerTest, UCMPLoadBalanceFullHashV6FrontPanelTraffic) {
  runLoadBalanceTest(
      8,
      getEcmpFullHashConfig(),
      true /* isV6 */,
      kUcmpWeights,
      true /* loopThroughFrontPanelPort */);
}

TEST_F(HwLoadBalancerTest, UCMPLoadBalanceFullHashV4FrontPanelTraffic) {
  runLoadBalanceTest(
      8,
      getEcmpFullHashConfig(),
      false /* isV6 */,
      kUcmpWeights,
      true /* loopThroughFrontPanelPort */);
}

TEST_F(HwLoadBalancerTest, UCMPLoadBalanceHalfHashV6FrontPanelTraffic) {
  runLoadBalanceTest(
      8,
      getEcmpHalfHashConfig(),
      true /* isV6 */,
      kUcmpWeights,
      true /* loopThroughFrontPanelPort */);
}

TEST_F(HwLoadBalancerTest, UCMPLoadBalanceHalfHashV4FrontPanelTraffic) {
  runLoadBalanceTest(
      8,
      getEcmpHalfHashConfig(),
      false /* isV6 */,
      kUcmpWeights,
      true /* loopThroughFrontPanelPort */);
}

} // namespace fboss
} // namespace facebook
