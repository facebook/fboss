/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/HwResourceStatsPublisher.h"
#include "fboss/agent/hw/gen-cpp2/hardware_stats_types.h"
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/hw/test/HwTest.h"
#include "fboss/agent/state/Route.h"
#include "fboss/agent/test/EcmpSetupHelper.h"

#include <fb303/ServiceData.h>

#include <folly/IPAddressV4.h>
#include <folly/IPAddressV6.h>
#include <folly/logging/xlog.h>

using namespace facebook::fb303;

namespace facebook::fboss {

class HwResourceStatsTest : public HwTest {
 public:
  void SetUp() override {
    HwTest::SetUp();
    applyNewConfig(config());
  }

 protected:
  cfg::SwitchConfig config() const {
    return utility::onePortPerVlanConfig(getHwSwitch(), masterLogicalPortIds());
  }
  using Prefix6 = typename Route<folly::IPAddressV6>::Prefix;
  using Prefix4 = typename Route<folly::IPAddressV4>::Prefix;
  Prefix6 kPrefix6() const {
    return Prefix6{folly::IPAddressV6("100::"), 64};
  }
  Prefix4 kPrefix4() const {
    return Prefix4{folly::IPAddressV4("100.100.100.0"), 24};
  }
};

TEST_F(HwResourceStatsTest, l3Stats) {
  if (!isSupported(HwAsic::Feature::RESOURCE_USAGE_STATS)) {
#if defined(GTEST_SKIP)
    GTEST_SKIP();
#endif
    return;
  }
  auto setup = [] {};
  auto verify = [this] {
    auto constexpr kEcmpWidth = 2;
    utility::EcmpSetupAnyNPorts4 ecmp4(getProgrammedState());
    utility::EcmpSetupAnyNPorts6 ecmp6(getProgrammedState());
    auto v6RouteFreeBefore = fbData->getCounter(kLpmIpv6Free);
    auto v4RouteFreeBefore = fbData->getCounter(kLpmIpv4Free);
    auto ecmpFreeBefore = fbData->getCounter(kL3EcmpGroupsFree);
    auto v4NextHopsFreeBefore = fbData->getCounter(kL3Ipv4NextHopsFree);
    auto v6NextHopsFreeBefore = fbData->getCounter(kL3Ipv6NextHopsFree);
    auto v4HostFreeBefore = fbData->getCounter(kL3Ipv4HostFree);
    auto v6HostFreeBefore = fbData->getCounter(kL3Ipv6HostFree);
    applyNewState(ecmp4.setupECMPForwarding(
        getProgrammedState(), kEcmpWidth, {kPrefix4()}));
    applyNewState(ecmp6.setupECMPForwarding(
        getProgrammedState(), kEcmpWidth, {kPrefix6()}));
    applyNewState(ecmp4.resolveNextHops(getProgrammedState(), kEcmpWidth));
    applyNewState(ecmp6.resolveNextHops(getProgrammedState(), kEcmpWidth));
    auto v6RouteFreeAfter = fbData->getCounter(kLpmIpv6Free);
    auto v4RouteFreeAfter = fbData->getCounter(kLpmIpv4Free);
    auto ecmpFreeAfter = fbData->getCounter(kL3EcmpGroupsFree);
    auto v4NextHopsFreeAfter = fbData->getCounter(kL3Ipv4NextHopsFree);
    auto v6NextHopsFreeAfter = fbData->getCounter(kL3Ipv6NextHopsFree);
    auto v4HostFreeAfter = fbData->getCounter(kL3Ipv4HostFree);
    auto v6HostFreeAfter = fbData->getCounter(kL3Ipv6HostFree);
    // Less than since v4, v6 routes, nhops, neighbors compete for the same
    // resources so free counts would not necessarily decrement by 1
    EXPECT_LT(v6RouteFreeAfter, v6RouteFreeBefore);
    EXPECT_LT(v4RouteFreeAfter, v4RouteFreeBefore);
    EXPECT_LT(v6NextHopsFreeAfter, v6NextHopsFreeBefore);
    EXPECT_LT(v4NextHopsFreeAfter, v4NextHopsFreeBefore);
    EXPECT_LT(v6HostFreeAfter, v6HostFreeBefore);
    EXPECT_LT(v4HostFreeAfter, v4HostFreeBefore);
    EXPECT_EQ(ecmpFreeAfter, ecmpFreeBefore - 2);
    // Unresolve so we can rerun verify for many (warmboot) iterations
    applyNewState(ecmp4.pruneECMPRoutes(getProgrammedState(), {kPrefix4()}));
    applyNewState(ecmp6.pruneECMPRoutes(getProgrammedState(), {kPrefix6()}));
    applyNewState(ecmp4.unresolveNextHops(getProgrammedState(), kEcmpWidth));
    applyNewState(ecmp6.unresolveNextHops(getProgrammedState(), kEcmpWidth));
  };
  verifyAcrossWarmBoots(setup, verify);
};
} // namespace facebook::fboss
