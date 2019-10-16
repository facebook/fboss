/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/test/dataplane_tests/HwLoadBalancerTests.h"

#include "fboss/agent/Platform.h"
#include "fboss/agent/hw/test/HwTest.h"
#include "fboss/agent/hw/test/HwTestPacketUtils.h"
#include "fboss/agent/test/EcmpSetupHelper.h"

#include "fboss/agent/state/Interface.h"
#include "fboss/agent/state/SwitchState.h"

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

class HwLoadBalancerTestV6
    : public HwLoadBalancerTest<utility::HwIpV6EcmpDataPlaneTestUtil> {
  std::unique_ptr<utility::HwIpV6EcmpDataPlaneTestUtil> getECMPHelper()
      override {
    return std::make_unique<utility::HwIpV6EcmpDataPlaneTestUtil>(
        getHwSwitchEnsemble(), RouterID(0));
  }

  void setupECMPHelper(
      unsigned int ecmpWidth,
      const std::vector<NextHopWeight>& weights) override {
    getEcmpSetupHelper()->setupECMPForwarding(ecmpWidth, weights);
  }
};

// ECMP, CPU originated traffic
TEST_F(HwLoadBalancerTestV6, ECMPLoadBalanceFullHashCpuTraffic) {
  runLoadBalanceTest(8, getEcmpFullHashConfig(), {});
}

TEST_F(HwLoadBalancerTestV6, ECMPLoadBalanceHalfHashCpuTraffic) {
  runLoadBalanceTest(8, getEcmpHalfHashConfig(), {});
}

// ECMP, CPU originated traffic Shrink/Expand
TEST_F(HwLoadBalancerTestV6, ECMShrinkExpandLoadBalanceFullHashCpuTraffic) {
  runEcmpShrinkExpandLoadBalanceTest(8, getEcmpFullHashConfig());
}

TEST_F(HwLoadBalancerTestV6, ECMPShrinkExpandLoadBalanceHalfHashCpuTraffic) {
  runEcmpShrinkExpandLoadBalanceTest(8, getEcmpHalfHashConfig());
}

// UCMP, CPU originated traffic
TEST_F(HwLoadBalancerTestV6, UCMPLoadBalanceFullHashCpuTraffic) {
  runLoadBalanceTest(8, getEcmpFullHashConfig(), kUcmpWeights);
}

TEST_F(HwLoadBalancerTestV6, UCMPLoadBalanceHalfHashCpuTraffic) {
  runLoadBalanceTest(8, getEcmpHalfHashConfig(), kUcmpWeights);
}

// ECMP, Front Panel traffic
TEST_F(HwLoadBalancerTestV6, ECMPLoadBalanceFullHashFrontPanelTraffic) {
  runLoadBalanceTest(
      8, getEcmpFullHashConfig(), {}, true /* loopThroughFrontPanelPort */);
}

TEST_F(HwLoadBalancerTestV6, ECMPLoadBalanceHalfHashFrontPanelTraffic) {
  runLoadBalanceTest(
      8, getEcmpHalfHashConfig(), {}, true /* loopThroughFrontPanelPort */);
}

// UCMP, Front Panel traffic
TEST_F(HwLoadBalancerTestV6, UCMPLoadBalanceFullHashFrontPanelTraffic) {
  runLoadBalanceTest(
      8,
      getEcmpFullHashConfig(),
      kUcmpWeights,
      true /* loopThroughFrontPanelPort */);
}

TEST_F(HwLoadBalancerTestV6, UCMPLoadBalanceHalfHashFrontPanelTraffic) {
  runLoadBalanceTest(
      8,
      getEcmpHalfHashConfig(),
      kUcmpWeights,
      true /* loopThroughFrontPanelPort */);
}
} // namespace fboss
} // namespace facebook
