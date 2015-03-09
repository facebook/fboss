/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include <algorithm>
#include <string>
#include "common/stats/ServiceData.h"
#include <folly/Memory.h>
#include <folly/io/Cursor.h>
#include <folly/io/IOBuf.h>
#include "fboss/agent/FbossError.h"
#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/SwitchStats.h"
#include "fboss/agent/TxPacket.h"
#include "fboss/agent/DHCPv4Handler.h"
#include "fboss/agent/UDPHeader.h"
#include "fboss/agent/hw/mock/MockHwSwitch.h"
#include "fboss/agent/hw/mock/MockPlatform.h"
#include "fboss/agent/hw/mock/MockRxPacket.h"
#include "fboss/agent/packet/DHCPv4Packet.h"
#include "fboss/agent/packet/EthHdr.h"
#include "fboss/agent/packet/Ethertype.h"
#include "fboss/agent/packet/IPProto.h"
#include "fboss/agent/packet/IPv4Hdr.h"
#include "fboss/agent/state/ArpEntry.h"
#include "fboss/agent/state/ArpResponseTable.h"
#include "fboss/agent/state/ArpTable.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/state/Vlan.h"
#include "fboss/agent/state/VlanMap.h"
#include "fboss/agent/state/Interface.h"
#include "fboss/agent/test/CounterCache.h"
#include "fboss/agent/test/TestUtils.h"

#include <boost/cast.hpp>
#include <gtest/gtest.h>

using namespace facebook::fboss;
using folly::IPAddressV4;
using folly::MacAddress;
using folly::io::Cursor;
using std::make_shared;
using std::shared_ptr;
using std::unique_ptr;
using std::string;
using std::vector;
using std::back_inserter;
using std::replace;

using ::testing::_;
using testing::Return;


namespace {
const IPAddressV4 kVlanInterfaceIP("10.0.0.1");
const IPAddressV4 kDhcpV4Relay("20.20.20.20");
const MacAddress kPlatformMac("00:02:00:ab:cd:ef");
const MacAddress kClientMac("02:00:00:00:00:02");
const IPAddressV4 kClientIP("10.0.0.10");

const MacAddress kClientMacOverride("03:00:00:00:00:03");
const IPAddressV4 kDhcpOverride("30.30.30.30");

unique_ptr<SwSwitch> setupSwitch() {
  auto state = testStateA();
  const auto& vlans = state->getVlans();
  // Set up an arp response entry for VLAN 1, 10.0.0.1,
  // so that we can detect the packet to 10.0.0.1 is for myself
  auto respTable1 = make_shared<ArpResponseTable>();
  respTable1->setEntry(kVlanInterfaceIP, kPlatformMac, InterfaceID(1));
  vlans->getVlan(VlanID(1))->setArpResponseTable(respTable1);
  vlans->getVlan(VlanID(1))->setDhcpV4Relay(kDhcpV4Relay);
  DhcpV4OverrideMap overrides;
  overrides[kClientMacOverride] = kDhcpOverride;
  vlans->getVlan(VlanID(1))->setDhcpV4RelayOverrides(overrides);
  return createMockSw(state);
}

unique_ptr<MockRxPacket> makeDHCPPacket(
    string srcMac, string dstMac, string vlan,
    string srcIp, string dstIp, string srcPort,
    string dstPort, string bootpOp,
    string dhcpMsgTypeOpt,
    string appendOptions = "",
    string yiaddr = "00 00 00 00") {
  string chaddr = kClientMac.toString();
  replace(chaddr.begin(), chaddr.end(), ':', ' ');
  // Pad 10 zero bytes to fill in 16 byte chaddr
  chaddr += "00 00 00 00 00 00 00 00 00 00";
  auto pkt = MockRxPacket::fromHex(
    // Ethernet header
    dstMac + srcMac +
    "81 00" + vlan +
    // Ether type
    "08 00" +
    // IPv4 Header
    // Version, IHL, DSCP, ECN, Length (262)
    "45  00  01  06"
    // Id, Flags, frag offset,
    "00  00  00  00"
    //TTL, Protocol (UDP), checksum
    "ff  11  00  00" +
    // UDP Header
    srcIp + dstIp +
    srcPort + dstPort +
    // Length (252), checksum
    "00  fc  00  00" +
    //DHCPv4 Packet
    // op(1),htype(1), hlen(6), hops(1)
    bootpOp + "01  06  01"
    // xid (10.10.10.1)
    "0a  0a  0a  01"
    // secs(10) flags(0x0000)
    "00  0a  00  00"
    // ciaddr (10.10.10.2)
    "0a  0a  0a  02"
    // yiaddr
    + yiaddr +
    // siaddr (10.10.10.4)
    "0a  0a  0a  04"
    // giaddr (10.10.10.5)
    "0a  0a  0a  05"
    + chaddr +
    // Sname (abcd)
    "61  62  63  64"
    "00  00  00  00"
    "00  00  00  00"
    "00  00  00  00"
    "00  00  00  00"
    "00  00  00  00"
    "00  00  00  00"
    "00  00  00  00"
    "00  00  00  00"
    "00  00  00  00"
    "00  00  00  00"
    "00  00  00  00"
    "00  00  00  00"
    "00  00  00  00"
    "00  00  00  00"
    "00  00  00  00"
    // File (defg)
    "65  66  67  68"
    "00  00  00  00"
    "00  00  00  00"
    "00  00  00  00"
    "00  00  00  00"
    "00  00  00  00"
    "00  00  00  00"
    "00  00  00  00"
    "00  00  00  00"
    "00  00  00  00"
    "00  00  00  00"
    "00  00  00  00"
    "00  00  00  00"
    "00  00  00  00"
    "00  00  00  00"
    "00  00  00  00"
    "00  00  00  00"
    "00  00  00  00"
    "00  00  00  00"
    "00  00  00  00"
    "00  00  00  00"
    "00  00  00  00"
    "00  00  00  00"
    "00  00  00  00"
    "00  00  00  00"
    "00  00  00  00"
    "00  00  00  00"
    "00  00  00  00"
    "00  00  00  00"
    "00  00  00  00"
    "00  00  00  00"
    "00  00  00  00"
    //DHCP Cookie {99, 130, 83, 99};
    "63  82  53 63"
    // DHCP msg type option + 1 byte pad
    + dhcpMsgTypeOpt
    // Other options to append
    + appendOptions +
    // 3 X Pad, 1 X end
    "00  00  00 ff"
  );
  pkt->setSrcPort(PortID(1));
  pkt->setSrcVlan(VlanID(1));
  return pkt;
}

struct Option {
  uint8_t op{0};
  uint8_t optLen{0};
  vector<uint8_t> optData;
  bool checkPresent{false};
};

TxMatchFn checkDHCPPkt(MacAddress dstMac, VlanID vlan,
    IPAddressV4 srcIp, IPAddressV4 dstIp, uint16_t srcPort,
    uint16_t dstPort, IPAddressV4 giaddr, const vector<Option>& options) {
  return [=] (const TxPacket* txPacket) {
    const auto* buf = txPacket->buf();
    Cursor c(buf);
    EthHdr ethHdr(c);
    if (ethHdr.dstAddr != dstMac) {
      throw FbossError("expected dest MAC to be ", dstMac,
          "; got ", ethHdr.dstAddr);
    }
    if (VlanID(ethHdr.vlanTags[0].vid()) != vlan) {
      throw FbossError("expected vlan to be ", vlan,
          "; got ", VlanID(ethHdr.vlanTags[0].vid()));
    }
    if (ethHdr.etherType != ETHERTYPE_IPV4) {
      throw FbossError("expected ether type to be ", ETHERTYPE_IPV4,
          "; got ", ethHdr.etherType);
    }
    IPv4Hdr ipHdr(c);
    if (ipHdr.srcAddr != srcIp) {
      throw FbossError("expected source ip to be ", srcIp,
          "; got ", ipHdr.srcAddr);
    }
    if (ipHdr.dstAddr != dstIp) {
      throw FbossError("expected destination ip to be ", dstIp,
          "; got ", ipHdr.dstAddr);
    }
    if (ipHdr.protocol != IPPROTO_UDP) {
      throw FbossError("expected protocol to be ", IPPROTO_UDP,
          "; got ", ipHdr.protocol);
    }
    UDPHeader udpHdr;
    udpHdr.parse(&c);
    if (udpHdr.srcPort != srcPort) {
      throw FbossError("expected source port to be ", srcPort,
          "; got ", udpHdr.srcPort);
    }
    if (udpHdr.dstPort != dstPort) {
      throw FbossError("expected destination port to be ", dstPort,
          "; got ", udpHdr.dstPort);
    }
    DHCPv4Packet dhcpPkt;
    dhcpPkt.parse(&c);
    if (!giaddr.isZero() && dhcpPkt.giaddr != giaddr) {
      throw FbossError("expected giaddr to be ", kVlanInterfaceIP,
          "; got ", dhcpPkt.giaddr);
    }
    for (auto& option: options) {
      vector<uint8_t> optData;
      if (option.checkPresent) {
        if (!DHCPv4Packet::getOptionSlow(option.op, dhcpPkt.options, optData)) {
          throw FbossError("expected option ", (int)option.op, " to be "
              "present, but not found");
        }
        if (!equal(option.optData.begin(), option.optData.end(),
              optData.begin())) {
          throw FbossError("unexpected data for option ", (int)option.op);
        }
      } else {
        if (DHCPv4Packet::getOptionSlow(option.op, dhcpPkt.options, optData)) {
          throw FbossError("unexpected option ", (int)option.op, " found ");
        }
      }
    }
  };
}

TxMatchFn checkDHCPReq(IPAddressV4 dhcpRelay = kDhcpV4Relay) {
  MacAddress dstMac = kPlatformMac;
  VlanID vlan(1);
  IPAddressV4 srcIp = kVlanInterfaceIP;
  IPAddressV4 dstIp = dhcpRelay;
  uint16_t srcPort = DHCPv4Handler::kBootPSPort;
  uint16_t dstPort = DHCPv4Handler::kBootPSPort;
  IPAddressV4 giaddr = kVlanInterfaceIP;
  Option dhcpAgentOption;
  dhcpAgentOption.op = 82;
  dhcpAgentOption.optLen = 2 + kVlanInterfaceIP.byteCount();
  dhcpAgentOption.optData.push_back(DHCPv4Handler::AGENT_CIRCUIT_ID);
  dhcpAgentOption.optData.push_back(kVlanInterfaceIP.byteCount());
  copy(kVlanInterfaceIP.bytes(), kVlanInterfaceIP.bytes() +
      kVlanInterfaceIP.byteCount(),
      back_inserter(dhcpAgentOption.optData));
  dhcpAgentOption.checkPresent = true;
  vector<Option> optionsToCheck;
  optionsToCheck.push_back(dhcpAgentOption);
  return checkDHCPPkt(dstMac, vlan, srcIp, dstIp, srcPort, dstPort,
      giaddr, optionsToCheck);
}

TxMatchFn checkDHCPReply() {
  MacAddress dstMac = kClientMac;
  VlanID vlan(1);
  IPAddressV4 srcIp = kVlanInterfaceIP;
  IPAddressV4 dstIp = kClientIP;
  uint16_t srcPort = DHCPv4Handler::kBootPSPort;
  uint16_t dstPort = DHCPv4Handler::kBootPCPort;
  IPAddressV4 giaddr("0.0.0.0");
  Option dhcpAgentOption;
  dhcpAgentOption.op = 82;
  dhcpAgentOption.checkPresent = false;
  vector<Option> optionsToCheck;
  optionsToCheck.push_back(dhcpAgentOption);
  return checkDHCPPkt(dstMac, vlan, srcIp, dstIp, srcPort, dstPort,
      giaddr, optionsToCheck);
}

} // unnamed   namespace

TEST(DHCPv4HandlerTest, DHCPRequest) {
  auto sw = setupSwitch();
  VlanID vlanID(1);
  const char* senderIP = "00 00 00 00";
  // Client mac
  auto senderMac = kClientMac.toString();
  std::replace(senderMac.begin(), senderMac.end(), ':', ' ');
  const string targetMac = "ff ff ff ff ff ff";
  const string targetIP = "ff ff ff ff";
  const string bootpOp = "01";
  const string vlan = "00 01";
  const string srcPort = "00 43";
  const string dstPort = "00 44";
  // DHCP Message type (option = 53, len = 1, message type = DHCP discover
  const string dhcpMsgTypeOpt = "35  01  01";
  // Cache the current stats
  CounterCache counters(sw.get());

  // Sending an DHCP request should not trigger state update
  EXPECT_HW_CALL(sw, stateChanged(_)).Times(0);
  EXPECT_PLATFORM_CALL(sw, getLocalMac()).
    WillRepeatedly(Return(kPlatformMac));

  EXPECT_PKT(sw, "DHCP request", checkDHCPReq());

  auto dhcpPkt = makeDHCPPacket(senderMac, targetMac, vlan,
      senderIP, targetIP, srcPort, dstPort, bootpOp, dhcpMsgTypeOpt);
  sw->packetReceived(dhcpPkt->clone());

  counters.update();
  counters.checkDelta(SwitchStats::kCounterPrefix + "dhcpV4.pkt.sum", 1);
  counters.checkDelta(SwitchStats::kCounterPrefix + "trapped.pkts.sum", 1);
}

TEST(DHCPv4HandlerOverrideTest, DHCPRequest) {
  auto sw = setupSwitch();
  VlanID vlanID(1);
  const char* senderIP = "00 00 00 00";
  // Client mac
  auto senderMac = kClientMacOverride.toString();
  std::replace(senderMac.begin(), senderMac.end(), ':', ' ');
  const string targetMac = "ff ff ff ff ff ff";
  const string targetIP = "ff ff ff ff";
  const string bootpOp = "01";
  const string vlan = "00 01";
  const string srcPort = "00 43";
  const string dstPort = "00 44";
  // DHCP Message type (option = 53, len = 1, message type = DHCP discover
  const string dhcpMsgTypeOpt = "35  01  01";

  // Sending an DHCP request should not trigger state update
  EXPECT_HW_CALL(sw, stateChanged(_)).Times(0);
  EXPECT_PLATFORM_CALL(sw, getLocalMac()).
    WillRepeatedly(Return(kPlatformMac));

  EXPECT_PKT(sw, "DHCP request", checkDHCPReq(kDhcpOverride));

  auto dhcpPkt = makeDHCPPacket(senderMac, targetMac, vlan,
      senderIP, targetIP, srcPort, dstPort, bootpOp, dhcpMsgTypeOpt);
  sw->packetReceived(dhcpPkt->clone());
}

TEST(DHCPv4HandlerTest, DHCPReply) {
  auto sw = setupSwitch();
  VlanID vlanID(55);
  // Client mac
  auto senderMac = kPlatformMac.toString();
  std::replace(senderMac.begin(), senderMac.end(), ':', ' ');
  auto targetMac = kClientMac.toString();
  std::replace(targetMac.begin(), targetMac.end(), ':', ' ');
  // DHCP server IP
  const char* senderIP = "14 14 14 14";
  // Cpu IP
  const string targetIP = "0a 00 00 01";
  const string bootpOp = "02";
  const string vlan = "00 01";
  const string srcPort = "00 44";
  const string dstPort = "00 43";
  // DHCP Message type (option = 53, len = 1, message type = DHCP offer
  const string dhcpMsgTypeOpt = "35  01  02";
  // Client IP addr, should be used as destination IP for DHCP replies
  const string yiaddr = "0a 00 00 0a";
  // Agent option should get stripped from reply
  const string agentOption = "52 02 00 00";

  // Cache the current stats
  CounterCache counters(sw.get());

  // Sending an DHCP request should not trigger state update
  EXPECT_HW_CALL(sw, stateChanged(_)).Times(0);
  EXPECT_PLATFORM_CALL(sw, getLocalMac()).
    WillRepeatedly(Return(kPlatformMac));

  EXPECT_PKT(sw, "DHCP reply", checkDHCPReply());

  auto dhcpPkt = makeDHCPPacket(senderMac, targetMac, vlan,
      senderIP, targetIP, srcPort, dstPort, bootpOp, dhcpMsgTypeOpt,
      agentOption, yiaddr);
  sw->packetReceived(dhcpPkt->clone());

  counters.update();
  counters.checkDelta(SwitchStats::kCounterPrefix + "dhcpV4.pkt.sum", 1);
  counters.checkDelta(SwitchStats::kCounterPrefix + "trapped.pkts.sum", 1);
}

TEST(DHCPv4HandlerTest, DHCPBadRequest) {
  auto sw = setupSwitch();
  VlanID vlanID(1);
  const string senderIP = "00 00 00 00";
  auto senderMac = "00 00 00 00 00 01";
  const string targetMac = "ff ff ff ff ff ff";
  const string targetIP = "ff ff ff ff";
  const string bootpOp = "03"; // Bad bootp message type
  const string vlan = "00 01";
  const string srcPort = "00 43";
  const string dstPort = "00 44";
  // DHCP Message type (option = 53, len = 1, message type = DHCP discover
  const string dhcpMsgTypeOpt = "35  01  01";
  // Cache the current stats
  CounterCache counters(sw.get());

  // Sending an DHCP request should not trigger state update
  EXPECT_HW_CALL(sw, stateChanged(_)).Times(0);
  EXPECT_HW_CALL(sw, sendPacketSwitched_(_)).Times(0);
  EXPECT_PLATFORM_CALL(sw, getLocalMac()).
    WillRepeatedly(Return(kPlatformMac));


  auto dhcpPkt = makeDHCPPacket(senderMac, targetMac, vlan,
      senderIP, targetIP, srcPort, dstPort, bootpOp, dhcpMsgTypeOpt);
  sw->packetReceived(dhcpPkt->clone());

  counters.update();
  counters.checkDelta(SwitchStats::kCounterPrefix + "dhcpV4.pkt.sum", 1);
  counters.checkDelta(SwitchStats::kCounterPrefix + "dhcpV4.bad_pkt.sum", 1);
  counters.checkDelta(SwitchStats::kCounterPrefix + "dhcpV4.drop_pkt.sum", 1);
  counters.checkDelta(SwitchStats::kCounterPrefix + "trapped.pkts.sum", 1);
}
