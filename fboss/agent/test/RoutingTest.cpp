/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include <gtest/gtest.h>

#include <folly/IPAddress.h>
#include <folly/IPAddressV4.h>
#include <folly/IPAddressV6.h>
#include <folly/MacAddress.h>
#include <folly/String.h>
#include <folly/io/IOBuf.h>

#include "fboss/agent/AddressUtil.h"
#include "fboss/agent/FbossError.h"
#include "fboss/agent/IPv4Handler.h"
#include "fboss/agent/IPv6Handler.h"
#include "fboss/agent/RxPacket.h"
#include "fboss/agent/SwitchStats.h"
#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/TxPacket.h"
#include "fboss/agent/TunManager.h"
#include "fboss/agent/packet/EthHdr.h"
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/hw/mock/MockRxPacket.h"
#include "fboss/agent/hw/mock/MockTxPacket.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/test/CounterCache.h"
#include "fboss/agent/test/MockTunManager.h"
#include "fboss/agent/test/TestUtils.h"

using namespace facebook::fboss;

using ::testing::_;

using folly::IPAddressV4;
using folly::IPAddressV6;

namespace {

// Interface route client ID
const auto kIntfRouteClient
    = StdClientIds2ClientID(StdClientIds::INTERFACE_ROUTE);

// Platform mac address
const folly::MacAddress kPlatformMac("02:01:02:03:04:05");
const folly::MacAddress kEmptyMac("00:00:00:00:00:00");

// Link-local addresses automatically assigned on all interfaces.
const folly::IPAddressV6 kIPv6LinkLocalAddr(
    IPAddressV6::LINK_LOCAL, kPlatformMac);

// Interface addresses
const folly::IPAddressV4 kIPv4IntfAddr1("10.0.0.11");
const folly::IPAddressV4 kIPv4IntfAddr2("10.0.0.21");
const folly::IPAddressV6 kIPv6IntfAddr1("face:b00c::11");
const folly::IPAddressV6 kIPv6IntfAddr2("face:b00c::21");

// Link local interface addresses
const folly::IPAddressV4 kllIPv4IntfAddr1("169.254.1.1");
const folly::IPAddressV4 kllIPv4IntfAddr2("169.254.2.2");
const folly::IPAddressV6 kllIPv6IntfAddr1("fe80::1");
const folly::IPAddressV6 kllIPv6IntfAddr2("fe80::2");

// Neighbor addresses
const folly::IPAddressV4 kIPv4NbhAddr1("10.0.0.10");
const folly::IPAddressV4 kIPv4NbhAddr2("10.0.0.20");
const folly::IPAddressV6 kIPv6NbhAddr1("face:b00c::10");
const folly::IPAddressV6 kIPv6NbhAddr2("face:b00c::20");
const folly::IPAddressV4 kllIPv4NbhAddr1("169.254.1.55");
const folly::IPAddressV4 kllIPv4NbhAddr2("169.254.2.55");
const folly::IPAddressV6 kllIPv6NbhAddr("fe80::7efe:90ff:fe2e:6e47");
const folly::MacAddress kNbhMacAddr1("00:02:c9:55:b7:a4");
const folly::MacAddress kNbhMacAddr2("6c:40:08:99:8d:d8");

// Multicast addresses and their corresponding mac addresses
const folly::IPAddressV4 kIPv4McastAddr("224.0.0.1");
const folly::IPAddressV6 kIPv6McastAddr("ff02::1");
const folly::MacAddress kIPv4MacAddr("01:00:5E:00:00:01");
const folly::MacAddress kIPv6MacAddr("33:33:00:00:00:01");

/**
 * Some constants for easy creation of packets
 */

const std::string kL2PacketHeader = \
  // Source MAC, Destination MAC (will be overwritten)
  "00 00 00 00 00 00  00 00 00 00 00 00"
  // 802.1q, VLAN 0 (Not used)
  "81 00  00 00"
  // Next header code (Will be overwritten)
  "00 00";

// length 20 bytes
const std::string kIPv4PacketHeader = \
  // Version(4), IHL(5), DSCP(0), ECN(0), Total Length(20 + 123)
  "45  00  00 8F"
  // Identification(0), Flags(0), Fragment offset(0)
  "00 00  00 00"
  // TTL(31), Protocol(11, UDP), Checksum (0, fake)
  "1F  11  00 00"
  // Source IP (will be overwritten)
  "00 00 00 00"
  // Destination IP (will be overwritten)
  "00 00 00 00";

// length 24 bytes
const std::string kIPv6PacketHeader = \
  // Version 6, traffic class, flow label
  "6e 00 00 00"
  // Payload length (123)
  "00 7b"
  // Next header: 17 (UDP), Hop Limit (255)
  "11 ff"
  // Source IP (wil be overwritten)
  "00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00"
  // Destination IP (will be overwritten)
  "00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00";

// length 123 bytes
const std::string kPayload = \
  "1a 0a 1a 0a 00 7b 58 75"
  "1c 1c 68 0a 74 65 72 72 61 67 72 61 70 68 08 02"
  "0b 63 73 77 32 33 61 2e 73 6e 63 31 15 e0 d4 03"
  "18 20 08 87 b2 a8 6f e8 48 71 f7 35 93 38 b6 8e"
  "b2 df de 66 e1 e5 9b 32 da 95 8a a7 51 4e 78 eb"
  "9e 14 1c 18 10 fe 80 00 00 00 00 00 00 02 1c 73"
  "ff fe ea d4 73 00 1c 18 04 0a 2e 01 48 00 00 26"
  "c2 9a c6 03 1b 00 16 84 c0 d5 a9 ac f9 9d 05 00"
  "18 00 00";

/**
 * Utility function to create V4 packet based on src/dst addresses.
 */
std::unique_ptr<MockRxPacket> createV4UnicastPacket(
    const folly::IPAddressV4& srcAddr,
    const folly::IPAddressV4& dstAddr,
    const folly::MacAddress& srcMac = kPlatformMac,
    const folly::MacAddress& dstMac = kPlatformMac) {
  auto pkt = MockRxPacket::fromHex(
      kL2PacketHeader + kIPv4PacketHeader + kPayload);

  // Write L2 header
  folly::io::RWPrivateCursor rwCursor(pkt->buf());
  TxPacket::writeEthHeader(
      &rwCursor, dstMac, srcMac, VlanID(0), IPv4Handler::ETHERTYPE_IPV4);

  // Write L3 src/dst IP Addresses
  rwCursor.skip(12);
  rwCursor.write<uint32_t>(srcAddr.toLong());
  rwCursor.write<uint32_t>(dstAddr.toLong());

  auto buf = pkt->buf();
  VLOG(2) << "\n" << folly::hexDump(buf->data(), buf->length());

  return pkt;
}

/**
 * Utility function to create V6 packet based on src/dst addresses.
 */
std::unique_ptr<MockRxPacket> createV6UnicastPacket(
    const folly::IPAddressV6& srcAddr,
    const folly::IPAddressV6& dstAddr,
    const folly::MacAddress& srcMac = kPlatformMac,
    const folly::MacAddress& dstMac = kPlatformMac) {
  auto pkt = MockRxPacket::fromHex(
      kL2PacketHeader + kIPv6PacketHeader + kPayload);

  // Write L2 header
  folly::io::RWPrivateCursor rwCursor(pkt->buf());
  TxPacket::writeEthHeader(
      &rwCursor, dstMac, srcMac, VlanID(0), IPv6Handler::ETHERTYPE_IPV6);

  // Write L3 src/dst IP addresses
  rwCursor.skip(8);
  rwCursor.push(srcAddr.bytes(), IPAddressV6::byteCount());
  rwCursor.push(dstAddr.bytes(), IPAddressV6::byteCount());

  auto buf = pkt->buf();
  VLOG(2) << "\n" << folly::hexDump(buf->data(), buf->length());

  return pkt;
}

/**
 * Utility function to create multicast packets with fixed destination
 * address.
 */
std::unique_ptr<MockRxPacket> createV4MulticastPacket(
    const folly::IPAddressV4& srcAddr) {
  return createV4UnicastPacket(
      srcAddr, kIPv4McastAddr, kPlatformMac, kIPv4MacAddr);
}
std::unique_ptr<MockRxPacket> createV6MulticastPacket(
    const folly::IPAddressV6& srcAddr) {
  return createV6UnicastPacket(
      srcAddr, kIPv6McastAddr, kPlatformMac, kIPv6MacAddr);
}

/**
 * Expectation comparision for RxPacket being sent to Host.
 */
RxMatchFn matchRxPacket(RxPacket* expPkt) {
  return [=](const RxPacket* rcvdPkt) {
    if (!folly::IOBufEqual()(*expPkt->buf(), *rcvdPkt->buf())) {
      throw FbossError("Expected rx-packet is not same as received packet");
    }

    // All good
    return;
  };
}

/**
 * Expectation comparision for TxPacket being sent to Switch
 */
TxMatchFn matchTxPacket(
    const folly::MacAddress& srcMac,
    const folly::MacAddress& dstMac,
    VlanID vlanID,
    uint16_t protocol,
    TxPacket* expPkt) {
  // Write these into header. NOTE expPkt is being modified for comparision.
  expPkt->buf()->prepend(EthHdr::SIZE);
  folly::io::RWPrivateCursor rwCursor(expPkt->buf());
  TxPacket::writeEthHeader(&rwCursor, dstMac, srcMac, vlanID, protocol);

  return [=](const TxPacket* rcvdPkt) {
    auto buf = rcvdPkt->buf();
    VLOG(2) << "-------------";
    VLOG(2) << "\n" << folly::hexDump(buf->data(), buf->length());
    buf = expPkt->buf();
    VLOG(2) << "\n" << folly::hexDump(buf->data(), buf->length());

    if (!folly::IOBufEqual()(*expPkt->buf(), *rcvdPkt->buf())) {
      throw FbossError("Expected tx-packet is not same as received packet");
    }

    // All good
    return;
  };
}

/**
 * Convenience function to convert RxPacket to TxPacket (just copying the buf)
 * L2 Header is advanced from RxPacket but headroom is provided in TxPacket.
 */
std::unique_ptr<MockTxPacket> toTxPacket(std::unique_ptr<MockRxPacket> rxPkt) {
  auto txPkt = std::make_unique<MockTxPacket>(rxPkt->buf()->length());

  auto txBuf = txPkt->buf();
  auto rxBuf = rxPkt->buf();
  txBuf->trimStart(EthHdr::SIZE);
  memcpy(
      txBuf->writableData(),
      rxBuf->data() + EthHdr::SIZE,
      rxBuf->length() - EthHdr::SIZE);
  VLOG(2) << "\n" << folly::hexDump(txBuf->data(), txBuf->length());

  return txPkt;
}

/**
 * Common stuff for testing software routing for packets flowing between Switch
 * and Linux host. We use same switch setup (VLANs, Interfaces) for all
 * test cases.
 */
class RoutingFixture : public ::testing::Test {
 public:
  void SetUp() override {
    auto config = getSwitchConfig();
    sw = createMockSw(&config, kPlatformMac, ENABLE_TUN);

    // Get TunManager pointer
    tunMgr = dynamic_cast<MockTunManager*>(sw->getTunManager());
    ASSERT_NE(tunMgr, static_cast<void*>(nullptr));

    // Add some ARP/NDP entries so that link-local addresses can be routed
    // as well.
    auto updateFn = [=] (const std::shared_ptr<SwitchState>& state) {
      std::shared_ptr<SwitchState> newState{state};
      auto* vlan1 = newState->getVlans()->getVlanIf(VlanID(1)).get();
      auto* vlan2 = newState->getVlans()->getVlanIf(VlanID(2)).get();
      auto* arpTable1 = vlan1->getArpTable().get()->modify(&vlan1, &newState);
      auto* ndpTable1 = vlan1->getNdpTable().get()->modify(&vlan1, &newState);
      auto* arpTable2 = vlan2->getArpTable().get()->modify(&vlan2, &newState);
      auto* ndpTable2 = vlan2->getNdpTable().get()->modify(&vlan2, &newState);

      arpTable1->addEntry(
          kIPv4NbhAddr1,
          kNbhMacAddr1,
          PortDescriptor(PortID(1)),
          InterfaceID(1));
      arpTable1->addEntry(
          kllIPv4NbhAddr1,
          kNbhMacAddr1,
          PortDescriptor(PortID(1)),
          InterfaceID(1));
      arpTable2->addEntry(
          kIPv4NbhAddr2,
          kNbhMacAddr2,
          PortDescriptor(PortID(2)),
          InterfaceID(2));
      arpTable2->addEntry(
          kllIPv4NbhAddr2,
          kNbhMacAddr2,
          PortDescriptor(PortID(2)),
          InterfaceID(2));
      ndpTable1->addEntry(
          kIPv6NbhAddr1,
          kNbhMacAddr1,
          PortDescriptor(PortID(1)),
          InterfaceID(1));
      ndpTable1->addEntry(
          kllIPv6NbhAddr,
          kNbhMacAddr1,
          PortDescriptor(PortID(1)),
          InterfaceID(1));
      ndpTable2->addEntry(
          kIPv6NbhAddr2,
          kNbhMacAddr2,
          PortDescriptor(PortID(2)),
          InterfaceID(2));
      ndpTable2->addEntry(
          kllIPv6NbhAddr,
          kNbhMacAddr2,
          PortDescriptor(PortID(2)),
          InterfaceID(2));

      return newState;
    };
    sw->updateState("ARP/NDP Entries Update", std::move(updateFn));

    // Apply initial config and verify basic initialization call sequence
    EXPECT_CALL(*tunMgr, sync(_)).Times(1);
    EXPECT_CALL(*tunMgr, startObservingUpdates()).Times(1);
    sw->initialConfigApplied(std::chrono::steady_clock::now());
  }

  void TearDown() override {
    tunMgr = nullptr;
    sw.reset();
  }

  /**
  * Utility function to initialize interface configuration. We assign
  * - 1 globally routable v6 address
  * - 1 globally routable v4 address
  * - 1 link-local v4 address (different on all interfaces)
  - - 1 link-local v6 address (different on all interfaces)
  * NOTE: v6 link-local address will be assigned automatically based on mac
  */
  static void initializeInterfaceConfig(cfg::Interface& intf, int32_t id) {
    intf.intfID = id;
    intf.vlanID = id;
    intf.name = folly::sformat("Interface-{}", id);
    intf.mtu = 9000;
    intf.__isset.mtu = true;
    intf.ipAddresses.resize(4);
    intf.ipAddresses[0] = folly::sformat("169.254.{}.{}/32", id, id);
    intf.ipAddresses[1] = folly::sformat("10.0.0.{}1/31", id);
    intf.ipAddresses[2] = folly::sformat("face:b00c::{}1/127", id);
    intf.ipAddresses[3] = folly::sformat("fe80::{}/64", id);
  }

  /**
  * VLANs => 1, 2
  * Interfaces => 1, 2 (corresponding to their vlans)
  * Ports => 1, 2 (corresponding to their vlans and interfaces)
  */
  static cfg::SwitchConfig getSwitchConfig() {
    cfg::SwitchConfig config;

    // Add two VLANs with ID 1 and 2
    config.vlans.resize(2);
    config.vlans[0].name = "Vlan-1";
    config.vlans[0].id = 1;
    config.vlans[0].routable = true;
    config.vlans[0].intfID = 1;
    config.vlans[1].name = "Vlan-2";
    config.vlans[1].id = 2;
    config.vlans[1].routable = true;
    config.vlans[1].intfID = 2;

    // Add two ports with ID 1 and 2 associated with VLAN 1 and 2 respectively
    config.ports.resize(2);
    config.vlanPorts.resize(2);
    for (int i = 0; i < 2; ++i) {
      auto& port = config.ports[i];
      port.logicalID = i + 1;
      port.state = cfg::PortState::UP;
      port.minFrameSize = 64;
      port.maxFrameSize = 9000;
      port.routable = true;
      port.ingressVlan = i + 1;

      auto& vlanPort = config.vlanPorts[i];
      vlanPort.vlanID = i + 1;
      vlanPort.logicalPort = i + 1;
      vlanPort.spanningTreeState = cfg::SpanningTreeState::FORWARDING;
      vlanPort.emitTags = 0;
    }

    config.interfaces.resize(2);
    initializeInterfaceConfig(config.interfaces[0], 1);
    initializeInterfaceConfig(config.interfaces[1], 2);

    return config;
  }

  // Member variables. Kept public for each access.
  std::unique_ptr<SwSwitch> sw;
  MockTunManager* tunMgr{nullptr};

  const InterfaceID ifID1{1};
  const InterfaceID ifID2{2};
};

/**
 * Verify flow of unicast packets from switch to host for both v4 and v6.
 * Interface is identified based on the dstAddr on packet among all switch
 * interfaces.
 */
TEST_F(RoutingFixture, SwitchToHostUnicast) {
  // Cache the current stats
  CounterCache counters(sw.get());

  // v4 packet destined to intf1 address from any address. Destination Linux
  // Inteface is identified based on srcVlan.
  // NOTE: the srcVlan and destAddr belong to different address. But packet will
  // still be forwarded to host because dest is one of interface's address.
  {
    auto pkt = createV4UnicastPacket(kIPv4NbhAddr2, kIPv4IntfAddr1);
    pkt->setSrcPort(PortID(2));
    pkt->setSrcVlan(VlanID(2));

    EXPECT_TUN_PKT(
        tunMgr, "V4 UcastPkt", ifID1, matchRxPacket(pkt.get())).Times(1);
    sw->packetReceived(pkt->clone());
    counters.update();
    counters.checkDelta(SwitchStats::kCounterPrefix + "host.rx.sum", 1);
    counters.checkDelta(SwitchStats::kCounterPrefix + "trapped.drops.sum", 0);
  }

  // v4 packet destined to non-interface address should be dropped.
  {
    const folly::IPAddressV4 unknownDest("10.152.35.65");
    auto pkt = createV4UnicastPacket(kIPv4NbhAddr1, unknownDest);
    pkt->setSrcPort(PortID(1));
    pkt->setSrcVlan(VlanID(1));

    EXPECT_TUN_PKT(
        tunMgr, "V4 UcastPkt", ifID1, matchRxPacket(pkt.get())).Times(0);
    sw->packetReceived(pkt->clone());
    counters.update();
    counters.checkDelta(SwitchStats::kCounterPrefix + "host.rx.sum", 0);
    counters.checkDelta(SwitchStats::kCounterPrefix + "trapped.drops.sum", 1);
  }

  // v6 packet destined to intf1 address from any address. Destination Linux
  // Interface is identified based on srcVlan (not verified here).
  {
    auto pkt = createV6UnicastPacket(kIPv6NbhAddr1, kIPv6IntfAddr1);
    pkt->setSrcPort(PortID(1));
    pkt->setSrcVlan(VlanID(1));

    EXPECT_TUN_PKT(
        tunMgr, "V6 UcastPkt", ifID1, matchRxPacket(pkt.get())).Times(1);
    sw->packetReceived(pkt->clone());
    counters.update();
    counters.checkDelta(SwitchStats::kCounterPrefix + "host.rx.sum", 1);
    counters.checkDelta(SwitchStats::kCounterPrefix + "trapped.drops.sum", 0);
  }

  // v6 packet destined to non-interface address should be dropped.
  {
    const folly::IPAddressV6 unknownDest("2803:6082:990:8c2d::1");
    auto pkt = createV6UnicastPacket(kIPv6NbhAddr2, unknownDest);
    pkt->setSrcPort(PortID(2));
    pkt->setSrcVlan(VlanID(2));

    EXPECT_TUN_PKT(
        tunMgr, "V6 UcastPkt", ifID2, matchRxPacket(pkt.get())).Times(0);
    sw->packetReceived(pkt->clone());
    counters.update();
    counters.checkDelta(SwitchStats::kCounterPrefix + "host.rx.sum", 0);
    counters.checkDelta(SwitchStats::kCounterPrefix + "trapped.drops.sum", 1);
  }
}

/**
 * Verify flow of link-local packets from switch to host for both v4 and v6.
 * Interface is identified based on the dstAddr on packet within scope of a
 * VLAN on which packet is received.
 *
 * NOTE: All interfaces in our UTs has same v4 and v6 link local address. It is
 * possible to have different link-local addresses on difference interfaces. It
 * doesn't matter from routing perspective.
 *
 */
TEST_F(RoutingFixture, SwitchToHostLinkLocalUnicast) {
  // Cache the current stats
  CounterCache counters(sw.get());

  // v4 packet destined to intf2 link-local address from any address but from
  // same VLAN as of intf2
  {
    auto pkt = createV4UnicastPacket(kllIPv4NbhAddr2, kllIPv4IntfAddr2);
    pkt->setSrcPort(PortID(2));
    pkt->setSrcVlan(VlanID(2));

    EXPECT_TUN_PKT(
        tunMgr, "V4 llUcastPkt", ifID2, matchRxPacket(pkt.get())).Times(1);
    sw->packetReceived(pkt->clone());
    counters.update();
    counters.checkDelta(SwitchStats::kCounterPrefix + "host.rx.sum", 1);
    counters.checkDelta(SwitchStats::kCounterPrefix + "trapped.drops.sum", 0);
  }

  // v4 link local packet destined to non-interface address should be dropped.
  {
    const folly::IPAddressV4 llDstAddrUnknown("169.254.3.4");
    auto pkt = createV4UnicastPacket(kllIPv4NbhAddr1, llDstAddrUnknown);
    pkt->setSrcPort(PortID(1));
    pkt->setSrcVlan(VlanID(1));

    EXPECT_TUN_PKT(
        tunMgr, "V4 llUcastPkt", ifID1, matchRxPacket(pkt.get())).Times(0);
    sw->packetReceived(pkt->clone());
    counters.update();
    counters.checkDelta(SwitchStats::kCounterPrefix + "host.rx.sum", 0);
    counters.checkDelta(SwitchStats::kCounterPrefix + "trapped.drops.sum", 1);
  }

  // v4 link local packet destined to interface address outside of srcVlan
  // should be routed like normal v4 IP packet. E.g. packet received on Vlan(2)
  // for ll-addr of Interface(1)
  // NOTE: Scope of link-local packet is purely within single VLAN.
  {
    auto pkt = createV4UnicastPacket(kllIPv4NbhAddr2, kllIPv4IntfAddr1);
    pkt->setSrcPort(PortID(2));
    pkt->setSrcVlan(VlanID(2));

    EXPECT_TUN_PKT(
        tunMgr, "V4 llUcastPkt", ifID1, matchRxPacket(pkt.get())).Times(1);
    sw->packetReceived(pkt->clone());
    counters.update();
    counters.checkDelta(SwitchStats::kCounterPrefix + "host.rx.sum", 1);
    counters.checkDelta(SwitchStats::kCounterPrefix + "trapped.drops.sum", 0);
  }

  // v6 packet destined to intf1 link-local address from any address.
  // Destination Linux interface is identified based on srcVlan
  // NOTE: We have two ipv6 link-local addresses
  {
    auto pkt = createV6UnicastPacket(kllIPv6NbhAddr, kIPv6LinkLocalAddr);
    pkt->setSrcPort(PortID(1));
    pkt->setSrcVlan(VlanID(1));

    EXPECT_TUN_PKT(
        tunMgr, "V6 llUcastPkt", ifID1, matchRxPacket(pkt.get())).Times(1);
    sw->packetReceived(pkt->clone());
    counters.update();
    counters.checkDelta(SwitchStats::kCounterPrefix + "host.rx.sum", 1);
    counters.checkDelta(SwitchStats::kCounterPrefix + "trapped.drops.sum", 0);
  }

  // v6 packet destined to unknwon link-local address should be dropped.
  {
    const folly::IPAddressV6 llDstAddrUnknown("fe80::202:c9ff:fe55:b7a4");
    auto pkt = createV6UnicastPacket(kllIPv6NbhAddr, llDstAddrUnknown);
    pkt->setSrcPort(PortID(2));
    pkt->setSrcVlan(VlanID(2));

    EXPECT_TUN_PKT(
        tunMgr, "V6 llUcastPkt", ifID2, matchRxPacket(pkt.get())).Times(0);
    sw->packetReceived(pkt->clone());
    counters.update();
    counters.checkDelta(SwitchStats::kCounterPrefix + "host.rx.sum", 0);
    counters.checkDelta(SwitchStats::kCounterPrefix + "trapped.drops.sum", 1);
  }

  // v6 packet destined to intf1 link-local address but received on vlan(2)
  // (outside of vlan), then it should be dropped.
  {
    auto pkt = createV6UnicastPacket(kllIPv6NbhAddr, kllIPv6IntfAddr1);
    pkt->setSrcPort(PortID(2));
    pkt->setSrcVlan(VlanID(2));

    EXPECT_TUN_PKT(
        tunMgr, "V6 llUcastPkt", ifID2, matchRxPacket(pkt.get())).Times(0);
    sw->packetReceived(pkt->clone());
    counters.update();
    counters.checkDelta(SwitchStats::kCounterPrefix + "host.rx.sum", 0);
    counters.checkDelta(SwitchStats::kCounterPrefix + "trapped.drops.sum", 1);
  }
}

/**
 * Verif flow of multicast packets from switch to host for both v4 and v6.
 * Interface is directly derived from VLAN.
 */
TEST_F(RoutingFixture, SwitchToHostMulticast) {
  // Cache the current stats
  CounterCache counters(sw.get());

  // Packet destined to v4 multicast address should always be forwarded to
  // Host regarless of it's dest/src IP
  {
    auto pkt = createV4MulticastPacket(kIPv4IntfAddr1);
    pkt->setSrcPort(PortID(1));
    pkt->setSrcVlan(VlanID(1));

    EXPECT_TUN_PKT(
        tunMgr, "V4 llMcastPkt", ifID1, matchRxPacket(pkt.get())).Times(1);
    sw->packetReceived(pkt->clone());
    counters.update();
    counters.checkDelta(SwitchStats::kCounterPrefix + "host.rx.sum", 1);
    counters.checkDelta(SwitchStats::kCounterPrefix + "trapped.drops.sum", 0);
  }

  // Packet destined to v6 multicast address should always be forwarded to
  // Host regarless of it's dest/src IP
  {
    auto pkt = createV6MulticastPacket(kIPv6IntfAddr1);
    pkt->setSrcPort(PortID(1));
    pkt->setSrcVlan(VlanID(1));

    EXPECT_TUN_PKT(
        tunMgr, "V6 llMcastPkt", ifID1, matchRxPacket(pkt.get())).Times(1);
    sw->packetReceived(pkt->clone());
    counters.update();
    counters.checkDelta(SwitchStats::kCounterPrefix + "host.rx.sum", 1);
    counters.checkDelta(SwitchStats::kCounterPrefix + "trapped.drops.sum", 0);
  }
}

/**
 * Verify flow of unicast packets from host to switch for both v4 and v6.
 * All outgoing L2 packets to switch must have platform MAC as the destination
 * address as it will trigger L3 lookup in hardware.
 */
TEST_F(RoutingFixture, HostToSwitchUnicast) {
  // Cache the current stats
  CounterCache counters(sw.get());

  // Any unicast packet (except link-local) is forwarded to CPU at L3 lookup
  // state by setting src/dst mac address to be the platform mac address.
  {
    auto pkt = toTxPacket(createV4UnicastPacket(
        kIPv4IntfAddr1, kIPv4NbhAddr1, kEmptyMac, kEmptyMac));

    auto pktCopy = pkt->clone();
    EXPECT_PKT(sw, "V4 UcastPkt", matchTxPacket(
        kPlatformMac,
        kPlatformMac,
        VlanID(1),
        IPv4Handler::ETHERTYPE_IPV4,
        pktCopy.get())).Times(1);
    sw->sendL3Packet(pkt->clone(), InterfaceID(1));

    counters.update();
    counters.checkDelta(SwitchStats::kCounterPrefix + "host.tx.sum", 1);
  }

  // Same as above test but for v6
  {
    auto pkt = toTxPacket(createV6UnicastPacket(
        kIPv6IntfAddr2, kIPv6NbhAddr2, kEmptyMac, kEmptyMac));

    auto pktCopy = pkt->clone();
    EXPECT_PKT(sw, "V6 UcastPkt", matchTxPacket(
        kPlatformMac,
        kPlatformMac,
        VlanID(2),
        IPv6Handler::ETHERTYPE_IPV6,
        pktCopy.get())).Times(1);
    sw->sendL3Packet(pkt->clone(), InterfaceID(2));

    counters.update();
    counters.checkDelta(SwitchStats::kCounterPrefix + "host.tx.sum", 1);
  }
}

/**
 * Verify flow of link local packets from host to switch for both v4 and v6.
 * All outgoing L2 packets must have their MAC resolved in software.
 */
TEST_F(RoutingFixture, HostToSwitchLinkLocal) {
  // Cache the current stats
  CounterCache counters(sw.get());

  // v4 link-local
  {
    auto pkt = toTxPacket(createV4UnicastPacket(
        kllIPv4IntfAddr1, kllIPv4NbhAddr1, kEmptyMac, kEmptyMac));

    auto pktCopy = pkt->clone();
    EXPECT_PKT(sw, "V4 UcastPkt", matchTxPacket(
        kPlatformMac,
        kPlatformMac, // NOTE: no neighbor mac address resolution like v6
        VlanID(1),
        IPv4Handler::ETHERTYPE_IPV4,
        pktCopy.get())).Times(1);
    sw->sendL3Packet(pkt->clone(), InterfaceID(1));

    counters.update();
    counters.checkDelta(SwitchStats::kCounterPrefix + "host.tx.sum", 1);
  }

  // Same as above test but for v6
  {
    auto pkt = toTxPacket(createV6UnicastPacket(
        kllIPv6IntfAddr2, kllIPv6NbhAddr, kEmptyMac, kEmptyMac));

    auto pktCopy = pkt->clone();
    EXPECT_PKT(sw, "V6 UcastPkt", matchTxPacket(
        kPlatformMac,
        kNbhMacAddr2,   // because vlan-id = 2
        VlanID(2),
        IPv6Handler::ETHERTYPE_IPV6,
        pktCopy.get())).Times(1);
    sw->sendL3Packet(pkt->clone(), InterfaceID(2));

    counters.update();
    counters.checkDelta(SwitchStats::kCounterPrefix + "host.tx.sum", 1);
  }

  // v6 packet with unknown link-local destination address (NDP not resolvable)
  // should trigger NDP request and packet gets dropped
  {
    const folly::IPAddressV6 llNbhAddrUnknown("fe80::face");
    auto pkt = toTxPacket(createV6UnicastPacket(
        kllIPv6IntfAddr2, llNbhAddrUnknown, kEmptyMac, kEmptyMac));

    // NDP Neighbor solicitation request will be sent and the actual packet
    // will be dropped.
    EXPECT_HW_CALL(sw, sendPacketSwitched_(_)).Times(1);
    sw->sendL3Packet(pkt->clone(), InterfaceID(1));

    counters.update();
    counters.checkDelta(SwitchStats::kCounterPrefix + "host.tx.sum", 0);
  }

  // v4 packet with unknown link-local destination address (ARP not resolvable)
  // should be just routed out of the box like other v4 IP packets. (unlike v6)
  {
    const folly::IPAddressV4 llNbhAddrUnknown("169.254.13.95");
    auto pkt = toTxPacket(createV4UnicastPacket(
        kllIPv4IntfAddr2, llNbhAddrUnknown, kEmptyMac, kEmptyMac));

    auto pktCopy = pkt->clone();
    EXPECT_PKT(sw, "V4 UcastPkt", matchTxPacket(
        kPlatformMac,
        kPlatformMac, // NOTE: no neighbor mac address resolution like v6
        VlanID(1),
        IPv4Handler::ETHERTYPE_IPV4,
        pktCopy.get())).Times(1);
    sw->sendL3Packet(pkt->clone(), InterfaceID(1));

    counters.update();
    counters.checkDelta(SwitchStats::kCounterPrefix + "host.tx.sum", 1);
  }
}

/**
 * Verify flow of multicast packets from host to switch for both v4 and v6.
 * All outgoing multicast packets must have their destination mac address
 * based on their IPAddress.
 */
TEST_F(RoutingFixture, HostToSwitchMulticast) {
  // Cache the current stats
  CounterCache counters(sw.get());

  // Any multicast packet sent to switch must have it's source mac address
  // set appropriately as per it's multicast IP address.
  // This test is for v4
  {
    auto pkt = toTxPacket(createV4UnicastPacket(
        kIPv4IntfAddr1, kIPv4McastAddr, kEmptyMac, kEmptyMac));

    auto pktCopy = pkt->clone();
    EXPECT_PKT(sw, "V4 McastPkt", matchTxPacket(
        kPlatformMac,
        kIPv4MacAddr,
        VlanID(1),
        IPv4Handler::ETHERTYPE_IPV4,
        pktCopy.get())).Times(1);
    sw->sendL3Packet(pkt->clone(), InterfaceID(1));

    counters.update();
    counters.checkDelta(SwitchStats::kCounterPrefix + "host.tx.sum", 1);
  }

  // Same as above test but for v6
  {
    auto pkt = toTxPacket(createV6UnicastPacket(
        kIPv6IntfAddr2, kIPv6McastAddr, kEmptyMac, kEmptyMac));

    auto pktCopy = pkt->clone();
    EXPECT_PKT(sw, "V6 McastPkt", matchTxPacket(
        kPlatformMac,
        kIPv6MacAddr,
        VlanID(2),
        IPv6Handler::ETHERTYPE_IPV6,
        pktCopy.get())).Times(1);
    sw->sendL3Packet(pkt->clone(), InterfaceID(2));

    counters.update();
    counters.checkDelta(SwitchStats::kCounterPrefix + "host.tx.sum", 1);
  }
}

} // anonymous namespace
