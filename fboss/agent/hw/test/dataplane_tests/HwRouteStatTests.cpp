/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include <folly/IPAddressV6.h>

#include "fboss/agent/hw/switch_asics/HwAsic.h"
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/hw/test/HwLinkStateDependentTest.h"
#include "fboss/agent/hw/test/HwTestAclUtils.h"
#include "fboss/agent/hw/test/HwTestCoppUtils.h"
#include "fboss/agent/hw/test/HwTestMplsUtils.h"
#include "fboss/agent/hw/test/HwTestPacketSnooper.h"
#include "fboss/agent/hw/test/HwTestPacketTrapEntry.h"
#include "fboss/agent/hw/test/HwTestPacketUtils.h"
#include "fboss/agent/hw/test/HwTestRouteUtils.h"
#include "fboss/agent/hw/test/TrafficPolicyUtils.h"
#include "fboss/agent/packet/PktFactory.h"
#include "fboss/agent/packet/PktUtil.h"
#include "fboss/agent/state/LabelForwardingEntry.h"
#include "fboss/agent/state/RouteNextHop.h"
#include "fboss/agent/test/EcmpSetupHelper.h"

namespace {
folly::IPAddressV6 kAddr1{"2401::201:ab00"};
folly::IPAddressV6 kAddr2{"2401::201:ac00"};
folly::IPAddressV6 kAddr3{"2401::201:ad00"};
std::optional<facebook::fboss::RouteCounterID> kCounterID1("route.counter.0");
std::optional<facebook::fboss::RouteCounterID> kCounterID2("route.counter.1");
std::optional<facebook::fboss::RouteCounterID> kCounterID3("route.counter.2");
} // namespace

namespace facebook::fboss {

class HwRouteStatTest : public HwLinkStateDependentTest {
 protected:
  void SetUp() override {
    HwLinkStateDependentTest::SetUp();
    ecmpHelper_ = std::make_unique<utility::EcmpSetupTargetedPorts6>(
        getProgrammedState(), RouterID(0));
  }

  cfg::SwitchConfig initialConfig() const override {
    std::vector<PortID> ports = {
        masterLogicalPortIds()[0],
        masterLogicalPortIds()[1],
    };
    auto config = utility::onePortPerVlanConfig(
        getHwSwitch(), std::move(ports), cfg::PortLoopbackMode::MAC, true);
    config.switchSettings_ref()->maxRouteCounterIDs_ref() = 3;
    return config;
  }

  HwSwitchEnsemble::Features featuresDesired() const override {
    return {HwSwitchEnsemble::LINKSCAN, HwSwitchEnsemble::PACKET_RX};
  }

  void addRoute(
      folly::IPAddressV6 prefix,
      uint8_t mask,
      PortDescriptor port,
      std::optional<RouteCounterID> counterID) {
    applyNewState(ecmpHelper_->resolveNextHops(
        getProgrammedState(),
        {
            port,
        }));

    ecmpHelper_->programRoutes(
        getRouteUpdater(),
        {port},
        {RoutePrefixV6{prefix, mask}},
        {},
        counterID);
  }

  void sendL3Packet(
      folly::IPAddressV6 dst,
      PortID from,
      std::optional<DSCP> dscp = std::nullopt) {
    CHECK(ecmpHelper_);
    auto vlanId = utility::firstVlanID(initialConfig());
    // construct eth hdr
    const auto intfMac = utility::getInterfaceMac(getProgrammedState(), vlanId);

    EthHdr::VlanTags_t vlans{
        VlanTag(vlanId, static_cast<uint16_t>(ETHERTYPE::ETHERTYPE_VLAN))};

    EthHdr eth{intfMac, intfMac, std::move(vlans), 0x86DD};
    // construct l3 hdr
    IPv6Hdr ip6{folly::IPAddressV6("1::10"), dst};
    ip6.nextHeader = 59; /* no next header */
    if (dscp) {
      ip6.trafficClass = (dscp.value() << 2); // last two bits are ECN
    }
    // get tx packet
    auto pkt =
        utility::EthFrame(eth, utility::IPPacket<folly::IPAddressV6>(ip6))
            .getTxPacket(getHwSwitch());
    // send pkt on src port, let it loop back in switch and be l3 switched
    getHwSwitchEnsemble()->ensureSendPacketOutOfPort(std::move(pkt), from);
  }

  bool skipTest() const {
    return !getPlatform()->getAsic()->isSupported(
        HwAsic::Feature::ROUTE_COUNTERS);
  }

  std::unique_ptr<utility::EcmpSetupTargetedPorts6> ecmpHelper_;
};

TEST_F(HwRouteStatTest, RouteEntryTest) {
  if (skipTest()) {
    return;
  }
  auto setup = [=]() {
    addRoute(
        kAddr1, 120, PortDescriptor(masterLogicalPortIds()[0]), kCounterID1);
    addRoute(
        kAddr2, 120, PortDescriptor(masterLogicalPortIds()[0]), kCounterID2);
    addRoute(
        kAddr3, 120, PortDescriptor(masterLogicalPortIds()[0]), kCounterID2);
  };
  auto verify = [=]() {
    // verify unique counters
    auto count1Before = utility::getRouteStat(getHwSwitch(), kCounterID1);
    auto count2Before = utility::getRouteStat(getHwSwitch(), kCounterID2);
    sendL3Packet(kAddr1, masterLogicalPortIds()[1]);
    sendL3Packet(kAddr2, masterLogicalPortIds()[1]);
    auto count1After = utility::getRouteStat(getHwSwitch(), kCounterID1);
    auto count2After = utility::getRouteStat(getHwSwitch(), kCounterID2);
    EXPECT_EQ(count1After - count1Before, 1);
    EXPECT_EQ(count2After - count2Before, 1);

    // verify shared route counters
    auto countBefore = utility::getRouteStat(getHwSwitch(), kCounterID2);
    sendL3Packet(kAddr2, masterLogicalPortIds()[1]);
    sendL3Packet(kAddr3, masterLogicalPortIds()[1]);
    auto countAfter = utility::getRouteStat(getHwSwitch(), kCounterID2);
    EXPECT_EQ(countAfter - countBefore, 2);
  };
  setup();
  verify();
}

// modify counter id
TEST_F(HwRouteStatTest, CounterModify) {
  if (skipTest()) {
    return;
  }
  auto setup = [=]() {
    addRoute(
        kAddr1, 120, PortDescriptor(masterLogicalPortIds()[0]), kCounterID1);
  };
  auto verify = [=]() {
    auto countBefore = utility::getRouteStat(getHwSwitch(), kCounterID1);
    sendL3Packet(kAddr1, masterLogicalPortIds()[1]);
    auto countAfter = utility::getRouteStat(getHwSwitch(), kCounterID1);
    EXPECT_EQ(countAfter - countBefore, 1);
    // modify the counter id
    addRoute(
        kAddr1, 120, PortDescriptor(masterLogicalPortIds()[0]), kCounterID2);
    countBefore = utility::getRouteStat(getHwSwitch(), kCounterID2);
    sendL3Packet(kAddr1, masterLogicalPortIds()[1]);
    countAfter = utility::getRouteStat(getHwSwitch(), kCounterID2);
    EXPECT_EQ(countAfter - countBefore, 1);

    // counter id changing from valid to null
    addRoute(
        kAddr1, 120, PortDescriptor(masterLogicalPortIds()[0]), std::nullopt);
    // This pkt wont be counted
    sendL3Packet(kAddr1, masterLogicalPortIds()[1]);
    // counter id changing from null to valid
    addRoute(
        kAddr1, 120, PortDescriptor(masterLogicalPortIds()[0]), kCounterID2);
    sendL3Packet(kAddr1, masterLogicalPortIds()[1]);
    EXPECT_EQ(utility::getRouteStat(getHwSwitch(), kCounterID2), 1);
    addRoute(
        kAddr1, 120, PortDescriptor(masterLogicalPortIds()[0]), kCounterID1);
  };
  setup();
  verify();
}
} // namespace facebook::fboss
