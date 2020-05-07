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
#include <string>

using folly::CIDRNetwork;
using folly::IPAddress;
using folly::IPAddressV6;
using std::make_shared;
using std::shared_ptr;
using std::string;

namespace facebook::fboss {

namespace {

std::vector<CIDRNetwork> getV6Prefixes(uint16_t startIndex, int numPrefixes) {
  std::vector<CIDRNetwork> prefixes;
  for (auto i = 0; i < numPrefixes; ++i) {
    prefixes.emplace_back(CIDRNetwork{
        IPAddress(folly::sformat("2401:db00:0021:{:x}::", startIndex + i)),
        64});
  }
  return prefixes;
}

} // namespace

class HwAlpmStressTest : public HwLinkStateDependentTest {
 protected:
  cfg::SwitchConfig initialConfig() const override {
    return utility::twoL3IntfConfig(
        getHwSwitch(),
        masterLogicalPortIds()[0],
        masterLogicalPortIds()[1],
        cfg::PortLoopbackMode::MAC);
  }
  void addV6Routes(uint64_t startIndex, int numPrefixes) {
    utility::EcmpSetupAnyNPorts6 ecmpHelper(getProgrammedState());
    std::vector<RoutePrefixV6> routes;
    for (auto prefix : getV6Prefixes(startIndex, numPrefixes)) {
      routes.emplace_back(RoutePrefixV6{prefix.first.asV6(), prefix.second});
    }
    applyNewState(ecmpHelper.setupECMPForwarding(
        getProgrammedState(), kEcmpWidth, routes));
  }
  static auto constexpr kEcmpWidth = 2;
  static auto constexpr kBucketCapacity = 16;
};

TEST_F(HwAlpmStressTest, splitPivotNhopsMatchDefaultRoute) {
  /*
   * ALPM algorithm on TD2 in a nutshell
   * Pivots - Less specific prefixes stored in TCAM. For a packet destination
   * pivots are searched. Pivot in turn points to a bucket, containing upto
   * 16 more specific prefixes and their nhops.
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
   *  Bucket capacity on TD2 is 16 prefixes. The following test tries to
   * recreate the scenario by
   *  - Adding 15 routes before warmboot. So these all fit into a single
   * bucket/pivot
   *  - Adding 5 more routes post warmboot, leading to a bucket split.
   *  - Sending a packet to destination that would fall under the new pivot
   * prefix. As long as the bug exists, these packets would be dropped.
   */
  if (getPlatform()->getAsic()->getAsicType() !=
      HwAsic::AsicType::ASIC_TYPE_TRIDENT2) {
    return;
  }
  auto setup = [this]() {
    applyNewConfig(initialConfig());
    utility::EcmpSetupAnyNPorts6 ecmpHelper(getProgrammedState());
    applyNewState(ecmpHelper.setupECMPForwarding(
        ecmpHelper.resolveNextHops(getProgrammedState(), kEcmpWidth),
        kEcmpWidth));
    // 4 routes exist besides the ones we added
    // - Two interface routes
    // - Default route
    // - Link local routes
    // Subtract that from #of routes to addd
    addV6Routes(1, kBucketCapacity - 4);
  };

  auto setupPostWarmboot = [this]() { addV6Routes(kBucketCapacity - 4, 5); };
  auto verifyPostWarmboot = [this]() {
    auto vlanId = VlanID(initialConfig().vlanPorts[0].vlanID);
    auto mac = utility::getInterfaceMac(getProgrammedState(), vlanId);
    auto txPacket = utility::makeUDPTxPacket(
        getHwSwitch(),
        vlanId,
        mac,
        mac,
        folly::IPAddressV6("2401:db00:0021::1"),
        folly::IPAddressV6("2401:db00:0021::2"),
        8000,
        8001);
    EXPECT_TRUE(
        getHwSwitchEnsemble()->ensureSendPacketSwitched(std::move(txPacket)));
  };
  verifyAcrossWarmBoots(setup, []() {}, setupPostWarmboot, verifyPostWarmboot);
}

} // namespace facebook::fboss
