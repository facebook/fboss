/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/switch_asics/HwAsic.h"
#include "fboss/agent/hw/test/HwTest.h"
#include "fboss/agent/state/RouteUpdater.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/test/EcmpSetupHelper.h"

#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/hw/test/HwLinkStateDependentTest.h"
#include "fboss/agent/hw/test/HwTestPacketUtils.h"

#include <folly/Format.h>

using folly::CIDRNetwork;
using folly::IPAddress;
using folly::IPAddressV4;
using folly::IPAddressV6;

namespace facebook::fboss {

namespace {

template <typename AddrT>
std::vector<CIDRNetwork> getPrefixes(uint16_t startIndex, int numPrefixes) {
  std::vector<CIDRNetwork> prefixes;
  for (auto i = 0; i < numPrefixes; ++i) {
    if constexpr (std::is_same_v<AddrT, folly::IPAddressV4>) {
      prefixes.emplace_back(CIDRNetwork{
          IPAddress(folly::sformat("10.10.{}.0", startIndex + i)), 24});
    } else {
      prefixes.emplace_back(CIDRNetwork{
          IPAddress(folly::sformat("2401:db00:0021:{:x}::", startIndex + i)),
          64});
    }
  }
  return prefixes;
}

template <typename AddrT>
int getBucketCapacity() {
  if constexpr (std::is_same_v<AddrT, folly::IPAddressV4>) {
    return 24;
  } else {
    return 16;
  }
}

template <typename AddrT>
std::pair<AddrT, AddrT> getSrcDstIp() {
  if constexpr (std::is_same_v<AddrT, folly::IPAddressV4>) {
    return {AddrT{"10.10.0.1"}, AddrT{"10.10.0.2"}};
  } else {
    return {AddrT("2401:db00:0021::1"), AddrT("2401:db00:0021::2")};
  }
}

} // namespace

class HwAlpmStressTest : public HwLinkStateDependentTest {
  static auto constexpr kEcmpWidth = 2;

  template <typename AddrT>
  void addRoutes(uint64_t startIndex, int numPrefixes) {
    utility::EcmpSetupAnyNPorts<AddrT> ecmpHelper(getProgrammedState());
    std::vector<RoutePrefix<AddrT>> routes;
    for (auto prefix : getPrefixes<AddrT>(startIndex, numPrefixes)) {
      if constexpr (std::is_same_v<AddrT, folly::IPAddressV4>) {
        routes.emplace_back(
            RoutePrefix<AddrT>{prefix.first.asV4(), prefix.second});
      } else {
        routes.emplace_back(
            RoutePrefix<AddrT>{prefix.first.asV6(), prefix.second});
      }
    }
    applyNewState(ecmpHelper.setupECMPForwarding(
        getProgrammedState(), kEcmpWidth, routes));
  }

 protected:
  cfg::SwitchConfig initialConfig() const override {
    return utility::twoL3IntfConfig(
        getHwSwitch(),
        masterLogicalPortIds()[0],
        masterLogicalPortIds()[1],
        cfg::PortLoopbackMode::MAC);
  }
  /*
   * ALPM algorithm on TD2 in a nutshell
   * Pivots - Less specific prefixes stored in TCAM. For a packet destination
   * pivots are searched. Pivot in turn points to a bucket, containing upto
   * 16(v6)/24(v4) more specific prefixes and their nhops.
   * Buckets - container for more specific prefixes and their nhops. If a match
   * cannot be found in bucket prefixes, pivot nhop information is used.
   * Once a pivot's pointed to bucket is full, the following happens
   * - 2 pivots are created by left shifting the pivot mask bits. E.g. a pivot
   *   with prefix X:20::/64, might get split into X:2::/60 and X::/60. Goal of
   *   pivot splitting is to evenly balance the more specific prefixes amongst
   *   the 2 buckets.
   * - 2 buckets are created and new pivots are made to point to them.
   * - Pivot nhop information is obtained by looking up in a internal trie data
   *   structure for pivots and finding the best match information.
   * In S199340, we hit a bug where nhop information for pivot corresponding
   * ::/0 was not recovered properly on warmboot. Which made the following
   * sequence of events problematic post warmboot
   *  - New prefixes were learnt causing a pivot split
   *  - The new pivot inherited its nhop information from the default pivot i.e.
   * the best match for the new pivot's prefix was the default v6 route.
   *
   *  Bucket capacity on TD2 is 16(v6)/24(v4) prefixes. The following test tries
   * to recreate the scenario by
   *  - Adding bucket size - 1 routes before warmboot. So these all fit into a
   * single bucket/pivot
   *  - Adding 5 more routes post warmboot, leading to a bucket split.
   *  - Sending a packet to destination that would fall under the new pivot
   * prefix. As long as the bug exists, these packets would be dropped.
   */
  template <typename AddrT>
  void run() {
    if (getPlatform()->getAsic()->getAsicType() !=
        HwAsic::AsicType::ASIC_TYPE_TRIDENT2) {
      return;
    }
    auto setup = [this]() {
      applyNewConfig(initialConfig());
      utility::EcmpSetupAnyNPorts<AddrT> ecmpHelper(getProgrammedState());
      applyNewState(ecmpHelper.setupECMPForwarding(
          ecmpHelper.resolveNextHops(getProgrammedState(), kEcmpWidth),
          kEcmpWidth));
      // 4 routes exist besides the ones we added
      // - Two interface routes
      // - Default route
      // - Link local routes (for v6)
      // Subtract that from #of routes to add
      addRoutes<AddrT>(1, getBucketCapacity<AddrT>() - 4);
    };

    auto setupPostWarmboot = [this]() {
      addRoutes<AddrT>(getBucketCapacity<AddrT>() - 4, 5);
    };
    auto verifyPostWarmboot = [this]() {
      auto vlanId = VlanID(initialConfig().vlanPorts[0].vlanID);
      auto mac = utility::getInterfaceMac(getProgrammedState(), vlanId);
      auto [sip, dip] = getSrcDstIp<AddrT>();
      auto txPacket = utility::makeUDPTxPacket(
          getHwSwitch(), vlanId, mac, mac, sip, dip, 8000, 8001);

      EXPECT_TRUE(
          getHwSwitchEnsemble()->ensureSendPacketSwitched(std::move(txPacket)));
    };
    verifyAcrossWarmBoots(
        setup, []() {}, setupPostWarmboot, verifyPostWarmboot);
  }
};

TEST_F(HwAlpmStressTest, splitPivotNhopsMatchDefaultRoute6) {
  run<IPAddressV6>();
}

TEST_F(HwAlpmStressTest, splitPivotNhopsMatchDefaultRoute4) {
  run<IPAddressV4>();
}
} // namespace facebook::fboss
