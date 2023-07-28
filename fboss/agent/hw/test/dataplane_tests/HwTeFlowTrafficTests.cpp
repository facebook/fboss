/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include <folly/IPAddress.h>
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/hw/test/HwLinkStateDependentTest.h"
#include "fboss/agent/hw/test/HwTeFlowTestUtils.h"
#include "fboss/agent/hw/test/HwTestPacketUtils.h"
#include "fboss/agent/hw/test/HwTestTeFlowUtils.h"
#include "fboss/agent/hw/test/LoadBalancerUtils.h"
#include "fboss/agent/hw/test/dataplane_tests/HwTestQosUtils.h"
#include "fboss/agent/packet/PktFactory.h"
#include "fboss/agent/packet/PktUtil.h"
#include "fboss/agent/state/RouteNextHop.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/state/TeFlowEntry.h"
#include "fboss/agent/test/EcmpSetupHelper.h"

using namespace facebook::fboss::utility;

namespace facebook::fboss {

using facebook::network::toBinaryAddress;
using folly::IPAddress;
using folly::StringPiece;

namespace {
static folly::IPAddressV6 kDefaultAddr{"::"};
static folly::IPAddressV6 kAddr1{"100::"};
static std::string kNhopAddrB("2::2");
static std::string kIfName2("fboss2001");
static std::string kCounterID0("counter0");
static std::string kCounterID1("counter1");
static std::string kCounterID2("counter2");
static int kPrefixLength1(56);
} // namespace

class HwTeFlowTrafficTest : public HwLinkStateDependentTest {
 protected:
  void SetUp() override {
    FLAGS_enable_exact_match = true;
    HwLinkStateDependentTest::SetUp();
  }

  cfg::SwitchConfig initialConfig() const override {
    std::vector<PortID> ports = {
        masterLogicalPortIds()[0],
        masterLogicalPortIds()[1],
    };
    auto cfg = utility::onePortPerInterfaceConfig(
        getHwSwitch(), std::move(ports), getAsic()->desiredLoopbackModes());
    return cfg;
  }

  bool skipTest() {
    return !getPlatform()->getAsic()->isSupported(HwAsic::Feature::EXACT_MATCH);
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
    auto srcMac = utility::MacAddressGenerator().get(intfMac.u64NBO() + 1);

    EthHdr::VlanTags_t vlans{
        VlanTag(vlanId, static_cast<uint16_t>(ETHERTYPE::ETHERTYPE_VLAN))};

    EthHdr eth{intfMac, srcMac, std::move(vlans), 0x86DD};
    // construct l3 hdr
    IPv6Hdr ip6{folly::IPAddressV6("1::10"), dst};
    ip6.nextHeader = 17; /* udp payload */
    if (dscp) {
      ip6.trafficClass = (dscp.value() << 2); // last two bits are ECN
    }
    UDPHeader udp(4049, 4050, 1);
    utility::UDPDatagram datagram(udp, {0xff});
    auto ethFrame = utility::EthFrame(
        eth, utility::IPPacket<folly::IPAddressV6>(ip6, datagram));
    auto pkt = ethFrame.getTxPacket(getHwSwitch());
    XLOG(DBG2) << "sending packet: ";
    XLOG(DBG2) << PktUtil::hexDump(pkt->buf());
    // send pkt on src port, let it loop back in switch and be l3 switched
    getHwSwitchEnsemble()->ensureSendPacketOutOfPort(std::move(pkt), from);
    return ethFrame.length();
  }

  void resolveNextHop(PortDescriptor port) {
    applyNewState(ecmpHelper_->resolveNextHops(
        getProgrammedState(),
        {
            port,
        }));
  }

  void addRoute(folly::IPAddressV6 prefix, uint8_t mask, PortDescriptor port) {
    ecmpHelper_->programRoutes(
        getRouteUpdater(), {port}, {RoutePrefixV6{prefix, mask}});
  }

  folly::MacAddress getIntfMac() const {
    return utility::getFirstInterfaceMac(getProgrammedState());
  }

  PortDescriptor portDesc1() const {
    return PortDescriptor(masterLogicalInterfacePortIds()[0]);
  }

  PortDescriptor portDesc2() const {
    return PortDescriptor(masterLogicalInterfacePortIds()[1]);
  }

  void disableTTLDecrements(const EcmpSetupTargetedPorts6& ecmpHelper) {
    for (const auto& nextHop :
         {ecmpHelper.nhop(portDesc1()), ecmpHelper.nhop(portDesc2())}) {
      utility::ttlDecrementHandlingForLoopbackTraffic(
          getHwSwitch(), ecmpHelper.getRouterId(), nextHop);
    }
  }

  void createL3DataplaneFlood(folly::IPAddressV6 dstIp, PortID from) {
    XLOG(INFO) << "creating data plane flood";
    auto vlan = utility::firstVlanID(initialConfig());
    if (!vlan) {
      throw FbossError("VLAN id unavailable for test");
    }
    auto vlanId = *vlan;

    const auto intfMac = utility::getInterfaceMac(getProgrammedState(), vlanId);
    auto srcMac = utility::MacAddressGenerator().get(intfMac.u64NBO() + 1);

    utility::pumpTraffic(
        getHwSwitch(),
        intfMac,
        {folly::IPAddress("1::10")},
        {dstIp},
        8000,
        8001,
        1,
        vlan,
        from,
        255,
        srcMac);
  }

  std::unique_ptr<utility::EcmpSetupTargetedPorts6> ecmpHelper_;
};

TEST_F(HwTeFlowTrafficTest, validateTeFlow) {
  if (this->skipTest()) {
#if defined(GTEST_SKIP)
    GTEST_SKIP();
#endif
    return;
  }

  auto setup = [=]() {
    ecmpHelper_ = std::make_unique<utility::EcmpSetupTargetedPorts6>(
        getProgrammedState(), RouterID(0));
    setExactMatchCfg(getHwSwitchEnsemble(), kPrefixLength1);
    // Add 100::/32 lpm route entry
    this->resolveNextHop(PortDescriptor(masterLogicalPortIds()[0]));
    this->resolveNextHop(PortDescriptor(masterLogicalPortIds()[1]));
    this->addRoute(kAddr1, 32, PortDescriptor(masterLogicalPortIds()[0]));

    // Add three TeFlow EM entries so that stats increments on non zero
    // action index for TH4.
    auto flowEntry0 = makeFlowEntry(
        "102::", kNhopAddrB, kIfName2, masterLogicalPortIds()[0], kCounterID1);
    addFlowEntry(getHwSwitchEnsemble(), flowEntry0);

    auto flowEntry1 = makeFlowEntry(
        "101::", kNhopAddrB, kIfName2, masterLogicalPortIds()[0], kCounterID2);
    addFlowEntry(getHwSwitchEnsemble(), flowEntry1);

    auto flowEntry2 = makeFlowEntry(
        "100::", kNhopAddrB, kIfName2, masterLogicalPortIds()[0], kCounterID0);
    addFlowEntry(getHwSwitchEnsemble(), flowEntry2);

    // verfiy the EM entry programming
    EXPECT_EQ(utility::getNumTeFlowEntries(getHwSwitch()), 3);
    utility::checkSwHwTeFlowMatch(
        getHwSwitch(),
        getProgrammedState(),
        makeFlowKey("100::", masterLogicalPortIds()[0]));
  };

  auto verify = [=]() {
    // Send a packet to hit lpm route entry and verify
    auto outPktsBefore0 = getPortOutPkts(
        this->getLatestPortStats(this->masterLogicalPortIds()[0]));
    auto outPktsBefore1 = getPortOutPkts(
        this->getLatestPortStats(this->masterLogicalPortIds()[1]));
    this->sendL3Packet(
        folly::IPAddressV6("0100:0000:100::1"),
        this->masterLogicalPortIds()[1],
        DSCP(16));
    auto outPktsAfter0 = getPortOutPkts(
        this->getLatestPortStats(this->masterLogicalPortIds()[0]));
    auto outPktsAfter1 = getPortOutPkts(
        this->getLatestPortStats(this->masterLogicalPortIds()[1]));
    // Sending a L3 packet via masterLogicalPortIds()[1] and
    // L3 lpm route nexthop is forwarding to masterLogicalPortIds()[0]
    // Hence the outpacket count should increment both.
    EXPECT_EQ((outPktsAfter0 - outPktsBefore0), 1);
    EXPECT_EQ((outPktsAfter1 - outPktsBefore1), 1);

    auto byteCountBefore =
        utility::getTeFlowOutBytes(getHwSwitch(), kCounterID0);
    // Send a packet to hit TeFlow EM entry and verify
    outPktsBefore0 = getPortOutPkts(
        this->getLatestPortStats(this->masterLogicalPortIds()[0]));
    outPktsBefore1 = getPortOutPkts(
        this->getLatestPortStats(this->masterLogicalPortIds()[1]));
    auto expectedLen = this->sendL3Packet(
        folly::IPAddressV6("100::"), this->masterLogicalPortIds()[0], DSCP(16));
    outPktsAfter0 = getPortOutPkts(
        this->getLatestPortStats(this->masterLogicalPortIds()[0]));
    outPktsAfter1 = getPortOutPkts(
        this->getLatestPortStats(this->masterLogicalPortIds()[1]));
    // Sending a L3 packet via masterLogicalPortIds()[0] and
    // TeFlow EM entry nexthop is forwarding to masterLogicalPortIds()[1]
    // Hence the outpacket count should increment both.
    EXPECT_EQ((outPktsAfter0 - outPktsBefore0), 1);
    EXPECT_EQ((outPktsAfter1 - outPktsBefore1), 1);

    EXPECT_EQ(
        utility::getTeFlowOutBytes(getHwSwitch(), kCounterID0) -
            byteCountBefore,

        expectedLen);
  };

  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(HwTeFlowTrafficTest, validateAddDelTeFlows) {
  auto setup = [=]() {
    ecmpHelper_ = std::make_unique<utility::EcmpSetupTargetedPorts6>(
        getProgrammedState(), RouterID(0));
    setExactMatchCfg(getHwSwitchEnsemble(), kPrefixLength1);
    this->resolveNextHop(PortDescriptor(masterLogicalPortIds()[0]));
    this->resolveNextHop(PortDescriptor(masterLogicalPortIds()[1]));
    // Add default route entry
    this->addRoute(kDefaultAddr, 0, PortDescriptor(masterLogicalPortIds()[0]));
    // Make flow entries for addTeFlows
    std::vector<std::string> flows = {"100::", "101::", "102::"};
    auto flowEntries = makeFlowEntries(
        flows,
        kNhopAddrB,
        kIfName2,
        masterLogicalPortIds()[0],
        kCounterID0,
        kPrefixLength1);
    // Add TeFlows
    addSyncFlowEntries(getHwSwitchEnsemble(), flowEntries, false);
    // Verfiy the EM entry programming
    EXPECT_EQ(utility::getNumTeFlowEntries(getHwSwitch()), 3);
  };

  auto verify = [=]() {
    // Read counters before sending a packet
    auto byteCountBefore =
        utility::getTeFlowOutBytes(getHwSwitch(), kCounterID0);
    auto outPktsBefore0 = getPortOutPkts(
        this->getLatestPortStats(this->masterLogicalPortIds()[0]));
    auto outPktsBefore1 = getPortOutPkts(
        this->getLatestPortStats(this->masterLogicalPortIds()[1]));
    // Send a L3 packet via masterLogicalPortIds()[0] to hit the EM entry
    auto expectedLen = this->sendL3Packet(
        folly::IPAddressV6("100::"), this->masterLogicalPortIds()[0], DSCP(16));
    // Read counters after sending a packet
    auto outPktsAfter0 = getPortOutPkts(
        this->getLatestPortStats(this->masterLogicalPortIds()[0]));
    auto outPktsAfter1 = getPortOutPkts(
        this->getLatestPortStats(this->masterLogicalPortIds()[1]));
    auto byteCountAfter =
        utility::getTeFlowOutBytes(getHwSwitch(), kCounterID0);
    // Sending a L3 packet via masterLogicalPortIds()[0] and
    // TeFlow EM entry nexthop is forwarding to masterLogicalPortIds()[1]
    // Hence the outpacket count should increment both.
    EXPECT_EQ((outPktsAfter0 - outPktsBefore0), 1);
    EXPECT_EQ((outPktsAfter1 - outPktsBefore1), 1);
    EXPECT_EQ(byteCountAfter - byteCountBefore, expectedLen);

    // Delete flows
    std::vector<std::string> flows = {"100::", "101::", "102::"};
    auto teFlows = makeTeFlows(flows, masterLogicalPortIds()[0]);
    // Delete TeFlows
    deleteFlowEntries(getHwSwitchEnsemble(), teFlows);
    // verfiy the EM entries are deleted
    EXPECT_EQ(utility::getNumTeFlowEntries(getHwSwitch()), 0);

    // Make flow entries for addTeFlows
    auto flowEntries = makeFlowEntries(
        flows,
        kNhopAddrB,
        kIfName2,
        masterLogicalPortIds()[0],
        kCounterID0,
        kPrefixLength1);
    // Add TeFlows again for warmboot check
    addSyncFlowEntries(getHwSwitchEnsemble(), flowEntries, false);
    // Verfiy the EM entry programming
    EXPECT_EQ(utility::getNumTeFlowEntries(getHwSwitch()), 3);
  };

  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(HwTeFlowTrafficTest, validateSyncTeFlows) {
  auto setup = [=]() {
    ecmpHelper_ = std::make_unique<utility::EcmpSetupTargetedPorts6>(
        getProgrammedState(), getIntfMac());
    setExactMatchCfg(getHwSwitchEnsemble(), kPrefixLength1);
    this->resolveNextHop(PortDescriptor(masterLogicalPortIds()[0]));
    this->resolveNextHop(PortDescriptor(masterLogicalPortIds()[1]));
    // Add default route entry
    this->addRoute(kDefaultAddr, 0, PortDescriptor(masterLogicalPortIds()[0]));
    // Make flow entries for addTeFlows
    std::vector<std::string> flows = {"100::", "101::", "102::"};
    auto flowEntries = makeFlowEntries(
        flows,
        kNhopAddrB,
        kIfName2,
        masterLogicalPortIds()[1],
        kCounterID0,
        kPrefixLength1);
    // add TeFlows
    addSyncFlowEntries(getHwSwitchEnsemble(), flowEntries, false);
    // verfiy the EM entry programming
    EXPECT_EQ(utility::getNumTeFlowEntries(getHwSwitch()), 3);
    // Disable TTL decrement
    disableTTLDecrements(*ecmpHelper_);
  };

  auto verify = [=]() {
    // Pump line rate traffic to destIp kAddr1 via src Port 1
    // should  forward the packets via EM entry to Port 1
    this->createL3DataplaneFlood(
        folly::IPAddressV6(kAddr1), this->masterLogicalPortIds()[1]);
    // Read counters before sync
    auto byteCountBefore =
        utility::getTeFlowOutBytes(getHwSwitch(), kCounterID0);
    auto outPktsBefore0 = getPortOutPkts(
        this->getLatestPortStats(this->masterLogicalPortIds()[0]));
    auto outPktsBefore1 = getPortOutPkts(
        this->getLatestPortStats(this->masterLogicalPortIds()[1]));
    // Check packets are not going out via LPM route
    EXPECT_EQ(outPktsBefore0, 0);
    EXPECT_GT(outPktsBefore1, 0);
    EXPECT_GT(byteCountBefore, 0);
    // Make flow entries for syncTeFlow
    std::vector<std::string> flows = {"100::", "101::", "102::"};
    auto flowEntries = makeFlowEntries(
        flows,
        kNhopAddrB,
        kIfName2,
        masterLogicalPortIds()[1],
        kCounterID0,
        kPrefixLength1);
    // syncTeFlows
    addSyncFlowEntries(getHwSwitchEnsemble(), flowEntries, true);
    // Read counters after sync
    auto byteCountAfter =
        utility::getTeFlowOutBytes(getHwSwitch(), kCounterID0);
    auto outPktsAfter0 = getPortOutPkts(
        this->getLatestPortStats(this->masterLogicalPortIds()[0]));
    auto outPktsAfter1 = getPortOutPkts(
        this->getLatestPortStats(this->masterLogicalPortIds()[1]));
    // Check packets are not going out via LPM route
    // since all entries are same sync should not reprogram EM entries
    EXPECT_EQ(outPktsAfter0, 0);
    EXPECT_GT(outPktsAfter1, outPktsBefore1);
    EXPECT_GT(byteCountAfter, byteCountBefore);
  };

  verifyAcrossWarmBoots(setup, verify);
}

} // namespace facebook::fboss
