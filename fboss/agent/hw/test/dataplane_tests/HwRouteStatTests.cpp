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

#include <fb303/ServiceData.h>

namespace {
folly::IPAddressV6 kAddr1{"2401::201:ab00"};
folly::IPAddressV6 kAddr2{"2401::201:ac00"};
folly::IPAddressV6 kAddr3{"2401::201:ad00"};
folly::IPAddressV4 kAddr4{"10.10.10.10"};
std::optional<facebook::fboss::RouteCounterID> kCounterID1("route.counter.0");
std::optional<facebook::fboss::RouteCounterID> kCounterID2("route.counter.1");
std::optional<facebook::fboss::RouteCounterID> kCounterID3("route.counter.2");
constexpr int kUdpSrcPort = 4049;
constexpr int kUdpDstPort = 4050;
} // namespace

namespace facebook::fboss {

class HwRouteStatTest : public HwLinkStateDependentTest {
 protected:
  void SetUp() override {
    HwLinkStateDependentTest::SetUp();
    ecmpHelper6_ = std::make_unique<utility::EcmpSetupTargetedPorts6>(
        getProgrammedState(), RouterID(0));
    ecmpHelper4_ = std::make_unique<utility::EcmpSetupTargetedPorts4>(
        getProgrammedState(), RouterID(0));
  }

  cfg::SwitchConfig initialConfig() const override {
    std::vector<PortID> ports = {
        masterLogicalPortIds()[0],
        masterLogicalPortIds()[1],
    };
    auto config = utility::onePortPerInterfaceConfig(
        getHwSwitch(),
        std::move(ports),
        getAsic()->desiredLoopbackMode(),
        true);
    config.switchSettings()->maxRouteCounterIDs() = 3;
    return config;
  }

  HwSwitchEnsemble::Features featuresDesired() const override {
    return {HwSwitchEnsemble::LINKSCAN, HwSwitchEnsemble::PACKET_RX};
  }

  void addRoute(
      folly::IPAddress prefix,
      uint8_t mask,
      PortDescriptor port,
      std::optional<RouteCounterID> counterID) {
    applyNewState(ecmpHelper6_->resolveNextHops(
        getProgrammedState(),
        {
            port,
        }));

    if (prefix.isV6()) {
      ecmpHelper6_->programRoutes(
          getRouteUpdater(),
          {port},
          {RoutePrefixV6{prefix.asV6(), mask}},
          {},
          counterID);
    } else {
      ecmpHelper4_->programRoutes(
          getRouteUpdater(),
          {port},
          {RoutePrefixV4{prefix.asV4(), mask}},
          {},
          counterID);
    }
  }

  uint32_t sendL3Packet(
      folly::IPAddressV6 dst,
      PortID from,
      std::optional<DSCP> dscp = std::nullopt) {
    // TODO: Remove the dependency on VLAN below
    auto vlan = utility::firstVlanID(initialConfig());
    if (!vlan) {
      throw FbossError("VLAN id unavailable for test");
    }
    auto vlanId = *vlan;
    // construct eth hdr
    const auto intfMac = utility::getInterfaceMac(getProgrammedState(), vlanId);
    const auto srcMac =
        utility::MacAddressGenerator().get(intfMac.u64HBO() + 1);

    EthHdr::VlanTags_t vlans{
        VlanTag(vlanId, static_cast<uint16_t>(ETHERTYPE::ETHERTYPE_VLAN))};

    EthHdr eth{intfMac, srcMac, std::move(vlans), 0x86DD};

    // construct l3 hdr
    UDPHeader udp(kUdpSrcPort, kUdpDstPort, 1);
    utility::UDPDatagram datagram(udp, {0xff});
    IPv6Hdr ip6{folly::IPAddressV6("1::10"), dst};
    ip6.nextHeader = IPPROTO_UDP;
    if (dscp) {
      ip6.trafficClass = (dscp.value() << 2); // last two bits are ECN
    }
    // get tx packet
    auto ethFrame = utility::EthFrame(
        eth, utility::IPPacket<folly::IPAddressV6>(ip6, datagram));
    auto pkt = ethFrame.getTxPacket(getHwSwitch());
    // send pkt on src port, let it loop back in switch and be l3 switched
    getHwSwitchEnsemble()->ensureSendPacketOutOfPort(std::move(pkt), from);
    return ethFrame.length();
  }

  bool skipTest() const {
    return !getPlatform()->getAsic()->isSupported(
        HwAsic::Feature::ROUTE_COUNTERS);
  }

  std::unique_ptr<utility::EcmpSetupTargetedPorts6> ecmpHelper6_;
  std::unique_ptr<utility::EcmpSetupTargetedPorts4> ecmpHelper4_;
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
    addRoute(
        kAddr4, 24, PortDescriptor(masterLogicalPortIds()[0]), kCounterID2);
  };
  auto verify = [=]() {
    // verify unique counters
    auto count1Before = utility::getRouteStat(getHwSwitch(), kCounterID1);
    auto count2Before = utility::getRouteStat(getHwSwitch(), kCounterID2);
    auto counter1Expected = sendL3Packet(kAddr1, masterLogicalPortIds()[1]);
    auto counter2Expected = sendL3Packet(kAddr2, masterLogicalPortIds()[1]);
    auto count1After = utility::getRouteStat(getHwSwitch(), kCounterID1);
    auto count2After = utility::getRouteStat(getHwSwitch(), kCounterID2);
    EXPECT_EQ(count1After - count1Before, counter1Expected);
    EXPECT_EQ(count2After - count2Before, counter2Expected);

    // verify shared route counters
    auto countBefore = utility::getRouteStat(getHwSwitch(), kCounterID2);
    counter2Expected = sendL3Packet(kAddr2, masterLogicalPortIds()[1]);
    counter2Expected += sendL3Packet(kAddr3, masterLogicalPortIds()[1]);
    auto countAfter = utility::getRouteStat(getHwSwitch(), kCounterID2);
    EXPECT_EQ(countAfter - countBefore, counter2Expected);

    auto statMap = facebook::fb303::fbData->getStatMap();
    EXPECT_TRUE(statMap->contains(*kCounterID1));
    EXPECT_TRUE(statMap->contains(*kCounterID2));
  };
  verifyAcrossWarmBoots(setup, verify);
}

// modify counter id
TEST_F(HwRouteStatTest, CounterModify) {
  if (skipTest()) {
    return;
  }
  auto setup = [=]() {
    addRoute(
        kAddr1, 120, PortDescriptor(masterLogicalPortIds()[0]), kCounterID1);
    addRoute(
        kAddr2, 120, PortDescriptor(masterLogicalPortIds()[0]), kCounterID2);
  };
  auto verify = [=]() {
    auto countBefore = utility::getRouteStat(getHwSwitch(), kCounterID1);
    auto counter1Expected = sendL3Packet(kAddr1, masterLogicalPortIds()[1]);
    auto countAfter = utility::getRouteStat(getHwSwitch(), kCounterID1);
    EXPECT_EQ(countAfter - countBefore, counter1Expected);

    // modify the route - no change to counter id
    addRoute(
        kAddr1, 120, PortDescriptor(masterLogicalPortIds()[1]), kCounterID1);
    countBefore = utility::getRouteStat(getHwSwitch(), kCounterID1);
    counter1Expected = sendL3Packet(kAddr1, masterLogicalPortIds()[1]);
    countAfter = utility::getRouteStat(getHwSwitch(), kCounterID1);
    EXPECT_EQ(countAfter - countBefore, counter1Expected);

    // modify the counter id
    addRoute(
        kAddr1, 120, PortDescriptor(masterLogicalPortIds()[0]), kCounterID2);
    countBefore = utility::getRouteStat(getHwSwitch(), kCounterID2);
    auto counter2Expected = sendL3Packet(kAddr1, masterLogicalPortIds()[1]);
    countAfter = utility::getRouteStat(getHwSwitch(), kCounterID2);
    EXPECT_EQ(countAfter - countBefore, counter2Expected);

    // counter id changing from valid to null
    countBefore = utility::getRouteStat(getHwSwitch(), kCounterID2);
    addRoute(
        kAddr1, 120, PortDescriptor(masterLogicalPortIds()[0]), std::nullopt);
    // This pkt wont be counted
    sendL3Packet(kAddr1, masterLogicalPortIds()[1]);
    countAfter = utility::getRouteStat(getHwSwitch(), kCounterID2);
    EXPECT_EQ(countAfter - countBefore, 0);

    // counter id changing from null to valid
    addRoute(
        kAddr1, 120, PortDescriptor(masterLogicalPortIds()[0]), kCounterID2);
    countBefore = utility::getRouteStat(getHwSwitch(), kCounterID2);
    counter2Expected = sendL3Packet(kAddr1, masterLogicalPortIds()[1]);
    countAfter = utility::getRouteStat(getHwSwitch(), kCounterID2);
    EXPECT_EQ(countAfter - countBefore, counter2Expected);
    addRoute(
        kAddr1, 120, PortDescriptor(masterLogicalPortIds()[0]), kCounterID1);
  };
  verifyAcrossWarmBoots(setup, verify);
}
} // namespace facebook::fboss
