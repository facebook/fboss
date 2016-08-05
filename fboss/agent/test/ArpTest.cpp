/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "common/stats/ServiceData.h"
#include <folly/Memory.h>
#include <folly/io/Cursor.h>
#include <folly/io/IOBuf.h>
#include "fboss/agent/AddressUtil.h"
#include "fboss/agent/FbossError.h"
#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/TxPacket.h"
#include "fboss/agent/ArpHandler.h"
#include "fboss/agent/NeighborUpdater.h"
#include "fboss/agent/SwitchStats.h"
#include "fboss/agent/ThriftHandler.h"
#include "fboss/agent/hw/mock/MockHwSwitch.h"
#include "fboss/agent/hw/mock/MockPlatform.h"
#include "fboss/agent/hw/mock/MockRxPacket.h"
#include "fboss/agent/packet/PktUtil.h"
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
#include <future>

using namespace facebook::fboss;
using facebook::network::toBinaryAddress;
using facebook::network::toIPAddress;
using facebook::network::thrift::BinaryAddress;
using folly::io::Cursor;
using folly::IOBuf;
using folly::IPAddressV4;
using folly::make_unique;
using folly::StringPiece;
using std::make_shared;
using std::shared_ptr;
using std::string;
using std::unique_ptr;

using ::testing::_;

namespace {

unique_ptr<SwSwitch> setupSwitch(std::chrono::seconds arpTimeout) {
  // Setup a default state object
  auto state = testStateA();
  const auto& intfs = state->getInterfaces();
  const auto& vlans = state->getVlans();
  boost::container::flat_map<VlanID, std::shared_ptr<ArpResponseTable>> tables;
  for (const auto& intf : *intfs) {
    auto& table = tables[intf->getVlanID()];
    if (table == nullptr) {
      table = make_shared<ArpResponseTable>();
    }
    for (const auto& addr : intf->getAddresses()) {
      if (addr.first.isV4()) {
        table->setEntry(addr.first.asV4(), intf->getMac(), intf->getID());
      }
    }
  }
  for (auto& table : tables) {
    vlans->getVlan(table.first)->setArpResponseTable(table.second);
  }

  if (arpTimeout.count() > 0) {
    state->setArpTimeout(arpTimeout);
  }

  state->setMaxNeighborProbes(1);
  state->setStaleEntryInterval(std::chrono::seconds(1));

  auto sw = createMockSw(state);
  sw->initialConfigApplied(std::chrono::steady_clock::now());
  waitForStateUpdates(sw.get());

  return sw;
}

unique_ptr<SwSwitch> setupSwitch() {
  return setupSwitch(std::chrono::seconds(0));
}

TxMatchFn checkArpPkt(bool isRequest,
                      IPAddressV4 senderIP, MacAddress senderMAC,
                      IPAddressV4 targetIP, MacAddress targetMAC,
                      VlanID vlan) {
  return [=](const TxPacket* pkt) {
    const auto* buf = pkt->buf();
    // 64 bytes (minimum ethernet frame length),
    // plus 4 bytes for the 802.1q header
    EXPECT_EQ(68, buf->computeChainDataLength());

    Cursor c(buf);
    auto dstMac = PktUtil::readMac(&c);
    if (targetMAC != dstMac) {
      throw FbossError("expected dest MAC to be ", targetMAC,
                       "; got ", dstMac);
    }

    auto srcMac = PktUtil::readMac(&c);
    EXPECT_EQ(senderMAC, srcMac);
    if (senderMAC != srcMac) {
      throw FbossError(" expected src MAC to be ", senderMAC,
                       "; got ", srcMac);
    }

    auto ethertype = c.readBE<uint16_t>();
    if (ethertype != 0x8100) {
      throw FbossError(" expected VLAN tag to be present, found ethertype ",
                       ethertype);
    }

    auto tag = c.readBE<uint16_t>();
    if (tag != vlan) {
      throw FbossError("expected VLAN tag ", vlan, "; found ", tag);
    }

    auto innerEthertype = c.readBE<uint16_t>();
    if (innerEthertype != 0x0806) {
      throw FbossError("expected ARP ethertype, found ", innerEthertype);
    }

    auto htype = c.readBE<uint16_t>();
    if (htype != 1) {
      throw FbossError("expected htype=1, found ", htype);
    }
    auto ptype = c.readBE<uint16_t>();
    if (ptype != 0x0800) {
      throw FbossError("expected ptype=0x0800, found ", ptype);
    }
    auto hlen = c.readBE<uint8_t>();
    if (hlen != 6) {
      throw FbossError("expected hlen=6, found ", hlen);
    }
    auto plen = c.readBE<uint8_t>();
    if (plen != 4) {
      throw FbossError("expected plen=4, found ", plen);
    }

    auto op = c.readBE<uint16_t>();
    auto opExp = isRequest ? 1 : 2;
    if (op != opExp) {
      throw FbossError("expected ARP op=", opExp, ", found ", op);
    }
    auto pktSenderMAC = PktUtil::readMac(&c);
    if (pktSenderMAC != senderMAC) {
      throw FbossError("expected sender MAC ", senderMAC,
                       " found ", pktSenderMAC);
    }
    auto pktSenderIP = PktUtil::readIPv4(&c);
    if (pktSenderIP != senderIP) {
      throw FbossError("expected sender IP ", senderIP,
                       " found ", pktSenderIP);
    }
    auto pktTargetMAC = PktUtil::readMac(&c);
    auto expectMAC = isRequest ? MacAddress::ZERO : targetMAC;
    if (pktTargetMAC != expectMAC) {
      throw FbossError("expected target MAC ", expectMAC,
                       " found ", pktTargetMAC);
    }
    auto pktTargetIP = PktUtil::readIPv4(&c);
    if (pktTargetIP != targetIP) {
      throw FbossError("expected target IP ", targetIP,
                       " found ", pktTargetIP);
    }
  };
}

TxMatchFn checkArpReply(const char* senderIP, const char* senderMAC,
                          const char* targetIP, const char* targetMAC,
                          VlanID vlan) {
  bool isRequest = false;
  return checkArpPkt(isRequest, IPAddressV4(senderIP), MacAddress(senderMAC),
                     IPAddressV4(targetIP), MacAddress(targetMAC),
                     vlan);
}

TxMatchFn checkArpRequest(IPAddressV4 senderIP, MacAddress senderMAC,
                          IPAddressV4 targetIP, VlanID vlan) {
  bool isRequest = true;
  return checkArpPkt(isRequest, senderIP, senderMAC,
                     targetIP, MacAddress::BROADCAST, vlan);
}

/* This helper sends an arp request for targetIP and verifies it was correctly
   sent out. */
void testSendArpRequest(unique_ptr<SwSwitch>& sw, VlanID vlanID,
                        IPAddressV4 senderIP, IPAddressV4 targetIP) {

  auto state = sw->getState();
  auto vlan = state->getVlans()->getVlanIf(vlanID);
  EXPECT_NE(vlan, nullptr);
  auto intf = state->getInterfaces()->getInterfaceIf(RouterID(0), senderIP);
  EXPECT_NE(intf, nullptr);

  // Sending an ARP request will trigger state update for setting pending entry
  EXPECT_HW_CALL(sw, stateChanged(_)).Times(1);
  EXPECT_PKT(sw, "ARP request",
             checkArpRequest(senderIP, intf->getMac(), targetIP, vlanID));

  ArpHandler::sendArpRequest(sw.get(), vlan->getID(), intf->getMac(),
                             senderIP, targetIP);

  // Notify the updater that we sent an arp request
  sw->getNeighborUpdater()->sentArpRequest(vlanID, targetIP);

  waitForStateUpdates(sw.get());
}

} // unnamed namespace

TEST(ArpTest, SendRequest) {
  auto sw = setupSwitch();
  VlanID vlanID(1);
  IPAddressV4 senderIP = IPAddressV4("10.0.0.1");
  IPAddressV4 targetIP = IPAddressV4("10.0.0.2");

  // Cache the current stats
  CounterCache counters(sw.get());

  testSendArpRequest(sw, vlanID, senderIP, targetIP);
  counters.update();
  counters.checkDelta(SwitchStats::kCounterPrefix + "arp.request.tx.sum", 1);
  counters.checkDelta(SwitchStats::kCounterPrefix + "arp.request.rx.sum", 0);
  counters.checkDelta(SwitchStats::kCounterPrefix + "arp.reply.tx.sum", 0);
  counters.checkDelta(SwitchStats::kCounterPrefix + "arp.reply.rx.sum", 0);

  // Create an IP pkt for 10.0.0.10
  auto pkt = MockRxPacket::fromHex(
    // dst mac, src mac
    "02 00 01 00 00 01  02 00 02 01 02 03"
    // 802.1q, VLAN 1
    "81 00 00 01"
    // IPv4
    "08 00"
    // Version(4), IHL(5), DSCP(0), ECN(0), Total Length(20)
    "45  00  00 14"
    // Identification(0), Flags(0), Fragment offset(0)
    "00 00  00 00"
    // TTL(31), Protocol(6), Checksum (0, fake)
    "1F  06  00 00"
    // Source IP (1.2.3.4)
    "01 02 03 04"
    // Destination IP (10.0.0.10)
    "0a 00 00 0a"
  );
  pkt->padToLength(68);
  pkt->setSrcPort(PortID(1));
  pkt->setSrcVlan(vlanID);

  // Receiving this packet should trigger an ARP request out.
  // The state will also update because a pending arp entry will be created
  EXPECT_HW_CALL(sw, stateChanged(_)).Times(1);
  EXPECT_PKT(sw, "ARP request",
             checkArpRequest(senderIP, MacAddress("00:02:00:00:00:01"),
                             IPAddressV4("10.0.0.10"), vlanID));

  sw->packetReceived(pkt->clone());
  waitForStateUpdates(sw.get());

  counters.update();
  counters.checkDelta(SwitchStats::kCounterPrefix + "trapped.pkts.sum", 1);
  counters.checkDelta(SwitchStats::kCounterPrefix + "trapped.arp.sum", 0);
  counters.checkDelta(SwitchStats::kCounterPrefix + "arp.request.tx.sum", 1);
  counters.checkDelta(SwitchStats::kCounterPrefix + "arp.request.rx.sum", 0);
  counters.checkDelta(SwitchStats::kCounterPrefix + "arp.reply.tx.sum", 0);
  counters.checkDelta(SwitchStats::kCounterPrefix + "arp.reply.rx.sum", 0);
  counters.checkDelta(SwitchStats::kCounterPrefix + "trapped.drops.sum", 1);
  counters.checkDelta(SwitchStats::kCounterPrefix + "trapped.error.sum", 0);
  counters.checkDelta(SwitchStats::kCounterPrefix + "trapped.ipv4.sum", 1);
  counters.checkDelta(SwitchStats::kCounterPrefix + "ipv4.nexthop.sum", 1);
  counters.checkDelta(SwitchStats::kCounterPrefix + "ipv4.no_arp.sum", 0);

  // Create an IP pkt for 10.0.10.10
  pkt = MockRxPacket::fromHex(
    // dst mac, src mac
    "02 00 01 00 00 01  02 00 02 01 02 03"
    // 802.1q, VLAN 1
    "81 00 00 01"
    // IPv4
    "08 00"
    // Version(4), IHL(5), DSCP(0), ECN(0), Total Length(20)
    "45  00  00 14"
    // Identification(0), Flags(0), Fragment offset(0)
    "00 00  00 00"
    // TTL(31), Protocol(6), Checksum (0, fake)
    "1F  06  00 00"
    // Source IP (1.2.3.4)
    "01 02 03 04"
    // Destination IP (10.0.10.10)
    "0a 00 0a 0a"
  );
  pkt->padToLength(68);
  pkt->setSrcPort(PortID(1));
  pkt->setSrcVlan(vlanID);

  // Receiving this packet should not trigger a ARP request out,
  // because no interface is able to reach that subnet
  EXPECT_HW_CALL(sw, stateChanged(_)).Times(0);
  EXPECT_HW_CALL(sw, sendPacketSwitched_(_)).Times(0);

  sw->packetReceived(pkt->clone());

  waitForStateUpdates(sw.get());
  counters.update();
  counters.checkDelta(SwitchStats::kCounterPrefix + "trapped.pkts.sum", 1);
  counters.checkDelta(SwitchStats::kCounterPrefix + "trapped.arp.sum", 0);
  counters.checkDelta(SwitchStats::kCounterPrefix + "arp.request.tx.sum", 0);
  counters.checkDelta(SwitchStats::kCounterPrefix + "arp.request.rx.sum", 0);
  counters.checkDelta(SwitchStats::kCounterPrefix + "arp.reply.tx.sum", 0);
  counters.checkDelta(SwitchStats::kCounterPrefix + "arp.reply.rx.sum", 0);
  counters.checkDelta(SwitchStats::kCounterPrefix + "trapped.drops.sum", 1);
  counters.checkDelta(SwitchStats::kCounterPrefix + "trapped.error.sum", 0);
  counters.checkDelta(SwitchStats::kCounterPrefix + "trapped.ipv4.sum", 1);
  counters.checkDelta(SwitchStats::kCounterPrefix + "ipv4.nexthop.sum", 1);
  counters.checkDelta(SwitchStats::kCounterPrefix + "ipv4.no_arp.sum", 1);

  // Create an IP pkt for 10.1.1.10, reachable through 10.0.0.22 and 10.0.0.23
  pkt = MockRxPacket::fromHex(
    // dst mac, src mac
    "02 00 01 00 00 01  02 00 02 01 02 03"
    // 802.1q, VLAN 1
    "81 00 00 01"
    // IPv4
    "08 00"
    // Version(4), IHL(5), DSCP(0), ECN(0), Total Length(20)
    "45  00  00 14"
    // Identification(0), Flags(0), Fragment offset(0)
    "00 00  00 00"
    // TTL(31), Protocol(6), Checksum (0, fake)
    "1F  06  00 00"
    // Source IP (1.2.3.4)
    "01 02 03 04"
    // Destination IP (10.1.1.10)
    "0a 01 01 0a"
  );
  pkt->padToLength(68);
  pkt->setSrcPort(PortID(1));
  pkt->setSrcVlan(vlanID);

  // Receiving this packet should trigger an ARP request to 10.0.0.22 and
  // 10.0.0.23, which causes pending arp entries to be added to the state.
  EXPECT_HW_CALL(sw, stateChanged(_)).Times(testing::AtLeast(1));
  EXPECT_PKT(sw, "ARP request",
             checkArpRequest(senderIP, MacAddress("00:02:00:00:00:01"),
                             IPAddressV4("10.0.0.22"), vlanID));
  EXPECT_PKT(sw, "ARP request",
             checkArpRequest(senderIP, MacAddress("00:02:00:00:00:01"),
                             IPAddressV4("10.0.0.23"), vlanID));

  sw->packetReceived(pkt->clone());

  waitForStateUpdates(sw.get());
  counters.update();
  counters.checkDelta(SwitchStats::kCounterPrefix + "trapped.pkts.sum", 1);
  counters.checkDelta(SwitchStats::kCounterPrefix + "trapped.arp.sum", 0);
  counters.checkDelta(SwitchStats::kCounterPrefix + "arp.request.tx.sum", 2);
  counters.checkDelta(SwitchStats::kCounterPrefix + "arp.request.rx.sum", 0);
  counters.checkDelta(SwitchStats::kCounterPrefix + "arp.reply.tx.sum", 0);
  counters.checkDelta(SwitchStats::kCounterPrefix + "arp.reply.rx.sum", 0);
  counters.checkDelta(SwitchStats::kCounterPrefix + "trapped.drops.sum", 1);
  counters.checkDelta(SwitchStats::kCounterPrefix + "trapped.error.sum", 0);
  counters.checkDelta(SwitchStats::kCounterPrefix + "trapped.ipv4.sum", 1);
  counters.checkDelta(SwitchStats::kCounterPrefix + "ipv4.nexthop.sum", 1);
  counters.checkDelta(SwitchStats::kCounterPrefix + "ipv4.no_arp.sum", 0);
}

TEST(ArpTest, TableUpdates) {
  auto sw = setupSwitch();
  VlanID vlanID(1);

  // Create an ARP request for 10.0.0.1 from an unreachable source
  auto pkt = MockRxPacket::fromHex(
    // dst mac, src mac
    "ff ff ff ff ff ff  00 02 00 01 02 03"
    // 802.1q, VLAN 1
    "81 00  00 01"
    // ARP, htype: ethernet, ptype: IPv4, hlen: 6, plen: 4
    "08 06  00 01  08 00  06  04"
    // ARP Request
    "00 01"
    // Sender MAC
    "00 02 00 01 02 03"
    // Sender IP: 10.0.1.15
    "0a 00 01 0f"
    // Target MAC
    "00 00 00 00 00 00"
    // Target IP: 10.0.0.1
    "0a 00 00 01"
  );
  pkt->padToLength(68);
  pkt->setSrcPort(PortID(1));
  pkt->setSrcVlan(vlanID);

  // Cache the current stats
  CounterCache counters(sw.get());

  // Sending the ARP request to the switch should not trigger an update to the
  // ArpTable for VLAN 1, but will send a reply packet
  EXPECT_HW_CALL(sw, stateChanged(_)).Times(0);
  EXPECT_PKT(sw, "ARP reply",
             checkArpReply("10.0.0.1", "00:02:00:00:00:01",
                           "10.0.1.15", "00:02:00:01:02:03",
                           vlanID));

  // Inform the SwSwitch of the ARP request
  sw->packetReceived(pkt->clone());

  // Check the new ArpTable does not have any entry
  auto arpTable = sw->getState()->getVlans()->getVlan(vlanID)->getArpTable();
  EXPECT_EQ(0, arpTable->getAllNodes().size());

  // Create an ARP request for 10.0.0.1
  pkt = MockRxPacket::fromHex(
    // dst mac, src mac
    "ff ff ff ff ff ff  00 02 00 01 02 03"
    // 802.1q, VLAN 1
    "81 00  00 01"
    // ARP, htype: ethernet, ptype: IPv4, hlen: 6, plen: 4
    "08 06  00 01  08 00  06  04"
    // ARP Request
    "00 01"
    // Sender MAC
    "00 02 00 01 02 03"
    // Sender IP: 10.0.0.15
    "0a 00 00 0f"
    // Target MAC
    "00 00 00 00 00 00"
    // Target IP: 10.0.0.1
    "0a 00 00 01"
  );
  pkt->padToLength(68);
  pkt->setSrcPort(PortID(1));
  pkt->setSrcVlan(vlanID);

  // update counters
  counters.update();

  // Sending the ARP request to the switch should trigger an update to the
  // ArpTable for VLAN 1, and will then send a reply packet
  EXPECT_HW_CALL(sw, stateChanged(_));
  EXPECT_PKT(sw, "ARP reply",
             checkArpReply("10.0.0.1", "00:02:00:00:00:01",
                           "10.0.0.15", "00:02:00:01:02:03",
                           vlanID));

  // Inform the SwSwitch of the ARP request
  sw->packetReceived(pkt->clone());

  // Wait for any updates triggered by the packet to complete.
  waitForStateUpdates(sw.get());

  // Check the new ArpTable contents
  arpTable = sw->getState()->getVlans()->getVlan(vlanID)->getArpTable();
  EXPECT_EQ(1, arpTable->getAllNodes().size());
  auto entry = arpTable->getEntry(IPAddressV4("10.0.0.15"));
  EXPECT_EQ(MacAddress("00:02:00:01:02:03"), entry->getMac());
  EXPECT_EQ(PortID(1), entry->getPort());
  EXPECT_EQ(InterfaceID(1), entry->getIntfID());

  // Check the new stats
  counters.update();
  counters.checkDelta(SwitchStats::kCounterPrefix + "trapped.pkts.sum", 1);
  counters.checkDelta(SwitchStats::kCounterPrefix + "trapped.arp.sum", 1);
  counters.checkDelta(SwitchStats::kCounterPrefix + "arp.request.tx.sum", 0);
  counters.checkDelta(SwitchStats::kCounterPrefix + "arp.request.rx.sum", 1);
  counters.checkDelta(SwitchStats::kCounterPrefix + "arp.reply.tx.sum", 1);
  counters.checkDelta(SwitchStats::kCounterPrefix + "arp.reply.rx.sum", 0);
  counters.checkDelta(SwitchStats::kCounterPrefix + "trapped.drops.sum", 0);
  counters.checkDelta(SwitchStats::kCounterPrefix + "trapped.error.sum", 0);

  // Sending the same packet again should generate another response,
  // but not another table update.
  EXPECT_HW_CALL(sw, stateChanged(_)).Times(0);
  EXPECT_PKT(sw, "ARP reply",
             checkArpReply("10.0.0.1", "00:02:00:00:00:01",
                           "10.0.0.15", "00:02:00:01:02:03",
                           vlanID));
  sw->packetReceived(pkt->clone());

  // Check the counters again
  counters.update();
  counters.checkDelta(SwitchStats::kCounterPrefix + "trapped.pkts.sum", 1);
  counters.checkDelta(SwitchStats::kCounterPrefix + "trapped.arp.sum", 1);
  counters.checkDelta(SwitchStats::kCounterPrefix + "arp.request.tx.sum", 0);
  counters.checkDelta(SwitchStats::kCounterPrefix + "arp.request.rx.sum", 1);
  counters.checkDelta(SwitchStats::kCounterPrefix + "arp.reply.tx.sum", 1);
  counters.checkDelta(SwitchStats::kCounterPrefix + "arp.reply.rx.sum", 0);
  counters.checkDelta(SwitchStats::kCounterPrefix + "trapped.drops.sum", 0);
  counters.checkDelta(SwitchStats::kCounterPrefix + "trapped.error.sum", 0);

  // An ARP request from an unknown node that isn't destined for us shouldn't
  // generate a reply or a table update.
  pkt = MockRxPacket::fromHex(
    // dst mac, src mac
    "ff ff ff ff ff ff  00 02 00 01 02 04"
    // 802.1q, VLAN 1
    "81 00  00 01"
    // ARP, htype: ethernet, ptype: IPv4, hlen: 6, plen: 4
    "08 06  00 01  08 00  06  04"
    // ARP Request
    "00 01"
    // Sender MAC
    "00 02 00 01 02 04"
    // Sender IP: 10.0.0.16
    "0a 00 00 10"
    // Target MAC
    "00 00 00 00 00 00"
    // Target IP: 10.0.0.50
    "0a 00 00 32"
  );
  pkt->padToLength(68);
  pkt->setSrcPort(PortID(1));
  pkt->setSrcVlan(vlanID);

  EXPECT_HW_CALL(sw, stateChanged(_)).Times(0);
  EXPECT_HW_CALL(sw, sendPacketSwitched_(_)).Times(0);
  sw->packetReceived(pkt->clone());
  counters.update();
  counters.checkDelta(SwitchStats::kCounterPrefix + "trapped.pkts.sum", 1);
  counters.checkDelta(SwitchStats::kCounterPrefix + "trapped.arp.sum", 1);
  counters.checkDelta(SwitchStats::kCounterPrefix + "arp.request.tx.sum", 0);
  counters.checkDelta(SwitchStats::kCounterPrefix + "arp.request.rx.sum", 0);
  counters.checkDelta(SwitchStats::kCounterPrefix + "arp.reply.tx.sum", 0);
  counters.checkDelta(SwitchStats::kCounterPrefix + "arp.reply.rx.sum", 0);
  counters.checkDelta(SwitchStats::kCounterPrefix + "arp.not_mine.sum", 1);
  counters.checkDelta(SwitchStats::kCounterPrefix + "trapped.drops.sum", 1);
  counters.checkDelta(SwitchStats::kCounterPrefix + "trapped.error.sum", 0);

  // A request from a node we already have in the arp table, but isn't for us,
  // should generate a table update if the MAC is different from what we
  // already have.
  pkt = MockRxPacket::fromHex(
    // dst mac, src mac
    "ff ff ff ff ff ff  00 02 00 01 02 08"
    // 802.1q, VLAN 1
    "81 00  00 01"
    // ARP, htype: ethernet, ptype: IPv4, hlen: 6, plen: 4
    "08 06  00 01  08 00  06  04"
    // ARP Request
    "00 01"
    // Sender MAC
    "00 02 00 01 02 08"
    // Sender IP: 10.0.0.15
    "0a 00 00 0f"
    // Target MAC
    "00 00 00 00 00 00"
    // Target IP: 10.0.0.50
    "0a 00 00 32"
  );
  pkt->padToLength(68);
  pkt->setSrcPort(PortID(2));
  pkt->setSrcVlan(vlanID);

  EXPECT_HW_CALL(sw, stateChanged(_)).Times(1);
  EXPECT_HW_CALL(sw, sendPacketSwitched_(_)).Times(0);
  sw->packetReceived(pkt->clone());
  waitForStateUpdates(sw.get());
  counters.update();
  counters.checkDelta(SwitchStats::kCounterPrefix + "trapped.pkts.sum", 1);
  counters.checkDelta(SwitchStats::kCounterPrefix + "trapped.arp.sum", 1);
  counters.checkDelta(SwitchStats::kCounterPrefix + "arp.request.tx.sum", 0);
  counters.checkDelta(SwitchStats::kCounterPrefix + "arp.request.rx.sum", 0);
  counters.checkDelta(SwitchStats::kCounterPrefix + "arp.reply.tx.sum", 0);
  counters.checkDelta(SwitchStats::kCounterPrefix + "arp.reply.rx.sum", 0);
  counters.checkDelta(SwitchStats::kCounterPrefix + "arp.not_mine.sum", 1);
  counters.checkDelta(SwitchStats::kCounterPrefix + "trapped.drops.sum", 1);
  counters.checkDelta(SwitchStats::kCounterPrefix + "trapped.error.sum", 0);

  arpTable = sw->getState()->getVlans()->getVlan(vlanID)->getArpTable();
  EXPECT_EQ(1, arpTable->getAllNodes().size());
  entry = arpTable->getEntry(IPAddressV4("10.0.0.15"));
  EXPECT_EQ(MacAddress("00:02:00:01:02:08"), entry->getMac());
  EXPECT_EQ(PortID(2), entry->getPort());
  EXPECT_EQ(InterfaceID(1), entry->getIntfID());

  // Try another request for us from a node we haven't seen yet.
  // This should generate a reply and a table update
  pkt = MockRxPacket::fromHex(
    // dst mac, src mac
    "ff ff ff ff ff ff  00 02 00 01 02 23"
    // 802.1q, VLAN 1
    "81 00  00 01"
    // ARP, htype: ethernet, ptype: IPv4, hlen: 6, plen: 4
    "08 06  00 01  08 00  06  04"
    // ARP Request
    "00 01"
    // Sender MAC
    "00 02 00 01 02 23"
    // Sender IP: 10.0.0.20
    "0a 00 00 14"
    // Target MAC
    "00 00 00 00 00 00"
    // Target IP: 10.0.0.1
    "0a 00 00 01"
  );
  pkt->padToLength(68);
  pkt->setSrcPort(PortID(1));
  pkt->setSrcVlan(vlanID);

  EXPECT_HW_CALL(sw, stateChanged(_)).Times(1);
  EXPECT_PKT(sw, "ARP reply",
             checkArpReply("10.0.0.1", "00:02:00:00:00:01",
                           "10.0.0.20", "00:02:00:01:02:23",
                           vlanID));
  sw->packetReceived(pkt->clone());
  waitForStateUpdates(sw.get());

  counters.update();
  counters.checkDelta(SwitchStats::kCounterPrefix + "trapped.pkts.sum", 1);
  counters.checkDelta(SwitchStats::kCounterPrefix + "trapped.arp.sum", 1);
  counters.checkDelta(SwitchStats::kCounterPrefix + "arp.request.tx.sum", 0);
  counters.checkDelta(SwitchStats::kCounterPrefix + "arp.request.rx.sum", 1);
  counters.checkDelta(SwitchStats::kCounterPrefix + "arp.reply.tx.sum", 1);
  counters.checkDelta(SwitchStats::kCounterPrefix + "arp.reply.rx.sum", 0);
  counters.checkDelta(SwitchStats::kCounterPrefix + "trapped.drops.sum", 0);
  counters.checkDelta(SwitchStats::kCounterPrefix + "trapped.error.sum", 0);

  arpTable = sw->getState()->getVlans()->getVlan(vlanID)->getArpTable();
  EXPECT_EQ(2, arpTable->getAllNodes().size());
  entry = arpTable->getEntry(IPAddressV4("10.0.0.15"));
  EXPECT_EQ(MacAddress("00:02:00:01:02:08"), entry->getMac());
  EXPECT_EQ(PortID(2), entry->getPort());
  EXPECT_EQ(InterfaceID(1), entry->getIntfID());
  entry = arpTable->getEntry(IPAddressV4("10.0.0.20"));
  EXPECT_EQ(MacAddress("00:02:00:01:02:23"), entry->getMac());
  EXPECT_EQ(PortID(1), entry->getPort());
  EXPECT_EQ(InterfaceID(1), entry->getIntfID());

  // Try a request for an IP on another interface, to make sure
  // it generates an ARP entry with the correct interface ID.
  pkt = MockRxPacket::fromHex(
    // dst mac, src mac
    "ff ff ff ff ff ff  00 02 00 55 66 77"
    // 802.1q, VLAN 1
    "81 00  00 01"
    // ARP, htype: ethernet, ptype: IPv4, hlen: 6, plen: 4
    "08 06  00 01  08 00  06  04"
    // ARP Request
    "00 01"
    // Sender MAC
    "00 02 00 55 66 77"
    // Sender IP: 192.168.0.20
    "c0 a8 00 14"
    // Target MAC
    "00 00 00 00 00 00"
    // Target IP: 192.168.0.1
    "c0 a8 00 01"
  );
  pkt->padToLength(68);
  pkt->setSrcPort(PortID(5));
  pkt->setSrcVlan(vlanID);

  EXPECT_HW_CALL(sw, stateChanged(_)).Times(1);
  EXPECT_PKT(sw, "ARP reply",
             checkArpReply("192.168.0.1", "00:02:00:00:00:01",
                           "192.168.0.20", "00:02:00:55:66:77",
                           vlanID));
  sw->packetReceived(pkt->clone());
  waitForStateUpdates(sw.get());

  counters.update();
  counters.checkDelta(SwitchStats::kCounterPrefix + "trapped.pkts.sum", 1);
  counters.checkDelta(SwitchStats::kCounterPrefix + "trapped.arp.sum", 1);
  counters.checkDelta(SwitchStats::kCounterPrefix + "arp.request.tx.sum", 0);
  counters.checkDelta(SwitchStats::kCounterPrefix + "arp.request.rx.sum", 1);
  counters.checkDelta(SwitchStats::kCounterPrefix + "arp.reply.tx.sum", 1);
  counters.checkDelta(SwitchStats::kCounterPrefix + "arp.reply.rx.sum", 0);
  counters.checkDelta(SwitchStats::kCounterPrefix + "trapped.drops.sum", 0);
  counters.checkDelta(SwitchStats::kCounterPrefix + "trapped.error.sum", 0);

  arpTable = sw->getState()->getVlans()->getVlan(vlanID)->getArpTable();
  EXPECT_EQ(3, arpTable->getAllNodes().size());
  entry = arpTable->getEntry(IPAddressV4("10.0.0.15"));
  EXPECT_EQ(MacAddress("00:02:00:01:02:08"), entry->getMac());
  EXPECT_EQ(PortID(2), entry->getPort());
  EXPECT_EQ(InterfaceID(1), entry->getIntfID());
  entry = arpTable->getEntry(IPAddressV4("10.0.0.20"));
  EXPECT_EQ(MacAddress("00:02:00:01:02:23"), entry->getMac());
  EXPECT_EQ(PortID(1), entry->getPort());
  EXPECT_EQ(InterfaceID(1), entry->getIntfID());
  entry = arpTable->getEntry(IPAddressV4("192.168.0.20"));
  EXPECT_EQ(MacAddress("00:02:00:55:66:77"), entry->getMac());
  EXPECT_EQ(PortID(5), entry->getPort());
  EXPECT_EQ(InterfaceID(1), entry->getIntfID());
}

TEST(ArpTest, NotMine) {
  auto sw = setupSwitch();

  // Create an ARP request for 10.1.2.3
  auto pkt = MockRxPacket::fromHex(
    // dst mac, src mac
    "ff ff ff ff ff ff  00 02 00 01 02 03"
    // 802.1q, VLAN 1
    "81 00  00 01"
    // ARP, htype: ethernet, ptype: IPv4, hlen: 6, plen: 4
    "08 06  00 01  08 00  06  04"
    // ARP Request
    "00 01"
    // Sender MAC
    "00 02 00 01 02 03"
    // Sender IP: 10.1.2.15
    "0a 01 02 0f"
    // Target MAC
    "00 00 00 00 00 00"
    // Target IP: 10.1.2.3
    "0a 01 02 03"
  );
  pkt->padToLength(68);
  pkt->setSrcPort(PortID(1));
  pkt->setSrcVlan(VlanID(1));

  // Cache the current stats
  CounterCache counters(sw.get());

  // Inform the SwSwitch of the ARP request
  sw->packetReceived(std::move(pkt));

  // Check the new stats
  counters.update();
  counters.checkDelta(SwitchStats::kCounterPrefix + "trapped.pkts.sum", 1);
  counters.checkDelta(SwitchStats::kCounterPrefix + "trapped.arp.sum", 1);
  counters.checkDelta(SwitchStats::kCounterPrefix + "arp.not_mine.sum", 1);
  counters.checkDelta(SwitchStats::kCounterPrefix + "trapped.drops.sum", 1);
  counters.checkDelta(SwitchStats::kCounterPrefix + "trapped.error.sum", 0);
}

TEST(ArpTest, BadHlen) {
  auto sw = setupSwitch();

  // Create an ARP request with a bad hardware length specified
  auto pkt = MockRxPacket::fromHex(
    // dst mac, src mac
    "ff ff ff ff ff ff  00 02 00 01 02 03"
    // 802.1q, VLAN 1
    "81 00  00 01"
    // ARP, htype: ethernet, ptype: IPv4, hlen: 2, plen: 4
    "08 06  00 01  08 00  02  04"
    // ARP Request
    "00 01"
    // Sender MAC
    "00 02"
    // Sender IP: 10.0.0.15
    "0a 00 00 0f"
    // Target MAC
    "00 00"
    // Target IP: 10.0.0.1
    "0a 00 00 01"
  );
  pkt->padToLength(68);
  pkt->setSrcPort(PortID(1));
  pkt->setSrcVlan(VlanID(1));

  // Cache the current stats
  CounterCache counters(sw.get());

  // Inform the SwSwitch of the ARP request
  sw->packetReceived(std::move(pkt));

  // Check the new stats
  counters.update();
  counters.checkDelta(SwitchStats::kCounterPrefix + "trapped.pkts.sum", 1);
  counters.checkDelta(SwitchStats::kCounterPrefix + "trapped.arp.sum", 1);
  counters.checkDelta(SwitchStats::kCounterPrefix + "arp.unsupported.sum", 1);
  counters.checkDelta(SwitchStats::kCounterPrefix + "trapped.drops.sum", 1);
  counters.checkDelta(SwitchStats::kCounterPrefix + "trapped.error.sum", 0);
}

void sendArpReply(SwSwitch* sw, StringPiece ipStr, StringPiece macStr,
                  int port) {
  IPAddressV4 srcIP(ipStr);
  MacAddress srcMac(macStr);

  // The dest MAC and IP belong to the switch, assuming
  // it was configured with testStateA()
  MacAddress dstMac("00:02:00:00:00:01");
  IPAddressV4 dstIP("10.0.0.1");
  VlanID vlan(1);

  // Create an ARP reply
  auto buf = IOBuf::create(68);
  folly::io::Appender cursor(buf.get(), 0);
  cursor.push(dstMac.bytes(), MacAddress::SIZE);
  cursor.push(srcMac.bytes(), MacAddress::SIZE);
  cursor.writeBE<uint16_t>(0x8100); // 802.1Q
  cursor.writeBE<uint16_t>(static_cast<uint16_t>(vlan));
  cursor.writeBE<uint16_t>(0x0806); // ARP
  cursor.writeBE<uint16_t>(1); // htype: ethernet
  cursor.writeBE<uint16_t>(0x0800); // ptype: IPv4
  cursor.writeBE<uint8_t>(6); // hlen: 6
  cursor.writeBE<uint8_t>(4); // plen: 4
  cursor.writeBE<uint16_t>(2); // ARP reply
  cursor.push(srcMac.bytes(), MacAddress::SIZE); // sender MAC
  cursor.write<uint32_t>(srcIP.toLong()); // sender IP
  cursor.push(dstMac.bytes(), MacAddress::SIZE); // target MAC
  cursor.write<uint32_t>(dstIP.toLong()); // target IP

  auto pkt = make_unique<MockRxPacket>(std::move(buf));
  pkt->padToLength(68);
  pkt->setSrcPort(PortID(port));
  pkt->setSrcVlan(VlanID(1));

  // Inform the SwSwitch of the ARP request
  sw->packetReceived(std::move(pkt));
}

TEST(ArpTest, FlushEntry) {
  auto sw = setupSwitch();

  // Send ARP replies from several nodes to populate the table
  EXPECT_HW_CALL(sw, stateChanged(_)).Times(testing::AtLeast(1));
  sendArpReply(sw.get(), "10.0.0.11", "02:10:20:30:40:11", 2);
  sendArpReply(sw.get(), "10.0.0.15", "02:10:20:30:40:15", 3);
  sendArpReply(sw.get(), "10.0.0.7", "02:10:20:30:40:07", 1);
  sendArpReply(sw.get(), "10.0.0.22", "02:10:20:30:40:22", 4);

  // Wait for all state updates from the ARP replies to be applied
  waitForStateUpdates(sw.get());

  // Use the thrift API to confirm that the ARP table is populated as expected.
  // This also serves to test the thrift handler functionality.
  ThriftHandler thriftHandler(sw.get());

  std::vector<ArpEntryThrift> arpTable;
  thriftHandler.getArpTable(arpTable);
  ASSERT_EQ(arpTable.size(), 4);
  // Sort the results so we can check the exact ordering here.
  std::sort(
    arpTable.begin(),
    arpTable.end(),
    [](const ArpEntryThrift& a, const ArpEntryThrift& b) {
      return a.port < b.port;
    }
  );
  auto checkEntry = [&](int idx, StringPiece ip, StringPiece mac, int port) {
    SCOPED_TRACE(folly::to<string>("index ", idx));
    EXPECT_EQ(arpTable[idx].vlanID, 1);
    EXPECT_EQ(arpTable[idx].vlanName, "Vlan1");
    EXPECT_EQ(toIPAddress(arpTable[idx].ip).str(), ip);
    EXPECT_EQ(arpTable[idx].mac, mac);
    EXPECT_EQ(arpTable[idx].port, port);
  };
  checkEntry(0, "10.0.0.7", "02:10:20:30:40:07", 1);
  checkEntry(1, "10.0.0.11", "02:10:20:30:40:11", 2);
  checkEntry(2, "10.0.0.15", "02:10:20:30:40:15", 3);
  checkEntry(3, "10.0.0.22", "02:10:20:30:40:22", 4);

  // Via the thrift API, flush the ARP entry for 10.0.0.11
  EXPECT_HW_CALL(sw, stateChanged(_)).Times(1);
  auto binAddr = toBinaryAddress(IPAddressV4("10.0.0.11"));
  auto numFlushed = thriftHandler.flushNeighborEntry(
      make_unique<BinaryAddress>(binAddr), 1);
  EXPECT_EQ(numFlushed, 1);

  // Now check the table again
  arpTable.clear();
  thriftHandler.getArpTable(arpTable);
  ASSERT_EQ(arpTable.size(), 3);
  // Sort the results so we can check the exact ordering here.
  std::sort(
    arpTable.begin(),
    arpTable.end(),
    [](const ArpEntryThrift& a, const ArpEntryThrift& b) {
      return a.port < b.port;
    }
  );
  checkEntry(0, "10.0.0.7", "02:10:20:30:40:07", 1);
  checkEntry(1, "10.0.0.15", "02:10:20:30:40:15", 3);
  checkEntry(2, "10.0.0.22", "02:10:20:30:40:22", 4);

  // Calling flushNeighborEntry() with an IP that isn't present in the table
  // should do nothing and return 0
  EXPECT_HW_CALL(sw, stateChanged(_)).Times(0);
  binAddr = toBinaryAddress(IPAddressV4("10.0.0.254"));
  numFlushed = thriftHandler.flushNeighborEntry(
      make_unique<BinaryAddress>(binAddr), 1);
  EXPECT_EQ(numFlushed, 0);
  binAddr = toBinaryAddress(IPAddressV4("1.2.3.4"));
  numFlushed = thriftHandler.flushNeighborEntry(
      make_unique<BinaryAddress>(binAddr), 1);
  EXPECT_EQ(numFlushed, 0);

  // Calling flushNeighborEntry() with a bogus VLAN ID should throw an error.
  binAddr = toBinaryAddress(IPAddressV4("1.2.3.4"));
  auto binAddrPtr = make_unique<BinaryAddress>(binAddr);
  EXPECT_THROW(thriftHandler.flushNeighborEntry(std::move(binAddrPtr), 123),
               FbossError);
}

TEST(ArpTest, PendingArp) {
  auto sw = setupSwitch();

  VlanID vlanID(1);
  IPAddressV4 senderIP = IPAddressV4("10.0.0.1");
  IPAddressV4 targetIP = IPAddressV4("10.0.0.2");

  // Cache the current stats
  CounterCache counters(sw.get());

  testSendArpRequest(sw, vlanID, senderIP, targetIP);

  counters.update();
  counters.checkDelta(SwitchStats::kCounterPrefix + "arp.request.tx.sum", 1);
  counters.checkDelta(SwitchStats::kCounterPrefix + "arp.request.rx.sum", 0);
  counters.checkDelta(SwitchStats::kCounterPrefix + "arp.reply.tx.sum", 0);
  counters.checkDelta(SwitchStats::kCounterPrefix + "arp.reply.rx.sum", 0);

  // Create an IP pkt for 10.0.0.10
  auto pkt = MockRxPacket::fromHex(
    // dst mac, src mac
    "02 00 01 00 00 01  02 00 02 01 02 03"
    // 802.1q, VLAN 1
    "81 00 00 01"
    // IPv4
    "08 00"
    // Version(4), IHL(5), DSCP(0), ECN(0), Total Length(20)
    "45  00  00 14"
    // Identification(0), Flags(0), Fragment offset(0)
    "00 00  00 00"
    // TTL(31), Protocol(6), Checksum (0, fake)
    "1F  06  00 00"
    // Source IP (1.2.3.4)
    "01 02 03 04"
    // Destination IP (10.0.0.10)
    "0a 00 00 0a"
  );
  pkt->padToLength(68);
  pkt->setSrcPort(PortID(1));
  pkt->setSrcVlan(vlanID);

  // Receiving this packet should trigger an ARP request out,
  // and the state should now include a pending arp entry.
  EXPECT_HW_CALL(sw, stateChanged(_)).Times(1);
  EXPECT_PKT(sw, "ARP request",
             checkArpRequest(senderIP, MacAddress("00:02:00:00:00:01"),
                             IPAddressV4("10.0.0.10"), vlanID));

  sw->packetReceived(pkt->clone());

  // Should see a pending entry now
  waitForStateUpdates(sw.get());
  auto entry = sw->getState()->getVlans()->getVlanIf(vlanID)->getArpTable()
    ->getEntryIf(IPAddressV4("10.0.0.10"));
  EXPECT_NE(entry, nullptr);
  EXPECT_EQ(entry->isPending(), true);

  counters.update();
  counters.checkDelta(SwitchStats::kCounterPrefix + "trapped.pkts.sum", 1);
  counters.checkDelta(SwitchStats::kCounterPrefix + "trapped.arp.sum", 0);
  counters.checkDelta(SwitchStats::kCounterPrefix + "arp.request.tx.sum", 1);
  counters.checkDelta(SwitchStats::kCounterPrefix + "arp.request.rx.sum", 0);
  counters.checkDelta(SwitchStats::kCounterPrefix + "arp.reply.tx.sum", 0);
  counters.checkDelta(SwitchStats::kCounterPrefix + "arp.reply.rx.sum", 0);
  counters.checkDelta(SwitchStats::kCounterPrefix + "trapped.drops.sum", 1);
  counters.checkDelta(SwitchStats::kCounterPrefix + "trapped.error.sum", 0);
  counters.checkDelta(SwitchStats::kCounterPrefix + "trapped.ipv4.sum", 1);
  counters.checkDelta(SwitchStats::kCounterPrefix + "ipv4.nexthop.sum", 1);
  counters.checkDelta(SwitchStats::kCounterPrefix + "ipv4.no_arp.sum", 0);

  // Receiving this duplicate packet should NOT trigger an ARP request out,
  // and no state update for now.
  EXPECT_HW_CALL(sw, stateChanged(_)).Times(0);

  sw->packetReceived(pkt->clone());

  // Should still see a pending entry now
  waitForStateUpdates(sw.get());
  entry = sw->getState()->getVlans()->getVlanIf(vlanID)->getArpTable()
    ->getEntryIf(IPAddressV4("10.0.0.10"));
  EXPECT_NE(entry, nullptr);
  EXPECT_EQ(entry->isPending(), true);

  counters.update();
  counters.checkDelta(SwitchStats::kCounterPrefix + "trapped.pkts.sum", 1);
  counters.checkDelta(SwitchStats::kCounterPrefix + "trapped.arp.sum", 0);
  counters.checkDelta(SwitchStats::kCounterPrefix + "arp.request.tx.sum", 0);
  counters.checkDelta(SwitchStats::kCounterPrefix + "arp.request.rx.sum", 0);
  counters.checkDelta(SwitchStats::kCounterPrefix + "arp.reply.tx.sum", 0);
  counters.checkDelta(SwitchStats::kCounterPrefix + "arp.reply.rx.sum", 0);
  counters.checkDelta(SwitchStats::kCounterPrefix + "trapped.drops.sum", 1);
  counters.checkDelta(SwitchStats::kCounterPrefix + "trapped.error.sum", 0);
  counters.checkDelta(SwitchStats::kCounterPrefix + "trapped.ipv4.sum", 1);
  counters.checkDelta(SwitchStats::kCounterPrefix + "ipv4.nexthop.sum", 1);
  counters.checkDelta(SwitchStats::kCounterPrefix + "ipv4.no_arp.sum", 0);

  EXPECT_HW_CALL(sw, stateChanged(_)).Times(1);
  // Receive an arp reply for our pending entry
  sendArpReply(sw.get(), "10.0.0.10", "02:10:20:30:40:22", 1);

  // The entry should now be valid instead of pending
  waitForStateUpdates(sw.get());
  entry = sw->getState()->getVlans()->getVlanIf(vlanID)->getArpTable()
    ->getEntryIf(IPAddressV4("10.0.0.10"));
  EXPECT_NE(entry, nullptr);
  EXPECT_EQ(entry->isPending(), false);

  // Verify that we don't ever overwrite a valid entry with a pending one.
  // Receive the same packet again, no state update and the entry should still
  // be valid
  EXPECT_HW_CALL(sw, stateChanged(_)).Times(0);

  sw->packetReceived(pkt->clone());
  waitForStateUpdates(sw.get());
  entry = sw->getState()->getVlans()->getVlanIf(vlanID)->getArpTable()
    ->getEntryIf(IPAddressV4("10.0.0.10"));
  EXPECT_NE(entry, nullptr);
  EXPECT_EQ(entry->isPending(), false);
};

TEST(ArpTest, PendingArpCleanup) {
  std::chrono::seconds arpTimeout(1);
  auto sw = setupSwitch(arpTimeout);

  VlanID vlanID(1);
  IPAddressV4 senderIP = IPAddressV4("10.0.0.1");
  IPAddressV4 targetIP = IPAddressV4("10.0.0.2");

  // Cache the current stats
  CounterCache counters(sw.get());

  testSendArpRequest(sw, vlanID, senderIP, targetIP);

  counters.update();
  counters.checkDelta(SwitchStats::kCounterPrefix + "arp.request.tx.sum", 1);
  counters.checkDelta(SwitchStats::kCounterPrefix + "arp.request.rx.sum", 0);
  counters.checkDelta(SwitchStats::kCounterPrefix + "arp.reply.tx.sum", 0);
  counters.checkDelta(SwitchStats::kCounterPrefix + "arp.reply.rx.sum", 0);

  // Should see a pending entry now
  auto entry = sw->getState()->getVlans()->getVlanIf(vlanID)->getArpTable()
    ->getEntryIf(targetIP);
  EXPECT_NE(entry, nullptr);
  EXPECT_EQ(entry->isPending(), true);

  // Send an Arp request for a different neighbor
  IPAddressV4 targetIP2 = IPAddressV4("10.0.0.3");

  testSendArpRequest(sw, vlanID, senderIP, targetIP2);

  counters.update();
  counters.checkDelta(SwitchStats::kCounterPrefix + "arp.request.tx.sum", 1);
  counters.checkDelta(SwitchStats::kCounterPrefix + "arp.request.rx.sum", 0);
  counters.checkDelta(SwitchStats::kCounterPrefix + "arp.reply.tx.sum", 0);
  counters.checkDelta(SwitchStats::kCounterPrefix + "arp.reply.rx.sum", 0);

  // Should see another pending entry now
  waitForStateUpdates(sw.get());
  entry = sw->getState()->getVlans()->getVlanIf(vlanID)->getArpTable()
    ->getEntryIf(targetIP2);
  EXPECT_NE(entry, nullptr);
  EXPECT_EQ(entry->isPending(), true);

 // Wait for pending entries to expire
  EXPECT_HW_CALL(sw, stateChanged(_)).Times(testing::AtLeast(1));
  std::promise<bool> done;
  auto* evb = sw->getBackgroundEVB();
  evb->runInEventBaseThread([&]() {
      evb->tryRunAfterDelay([&]() {
        done.set_value(true);
      }, 1010);
    });
  done.get_future().wait();

  // Entries should be removed
  entry = sw->getState()->getVlans()->getVlanIf(vlanID)->getArpTable()
    ->getEntryIf(targetIP);
  auto entry2 = sw->getState()->getVlans()->getVlanIf(vlanID)->getArpTable()
    ->getEntryIf(targetIP2);
  EXPECT_EQ(entry, nullptr);
  EXPECT_EQ(entry2, nullptr);
}

TEST(ArpTest, ArpTableSerialization) {
  auto sw = setupSwitch();

  VlanID vlanID(1);
  IPAddressV4 senderIP = IPAddressV4("10.0.0.1");
  IPAddressV4 targetIP = IPAddressV4("10.0.0.2");

  EXPECT_HW_CALL(sw, stateChanged(_)).Times(0);
  auto vlan = sw->getState()->getVlans()->getVlanIf(vlanID);
  EXPECT_NE(vlan, nullptr);
  auto arpTable = vlan->getArpTable();
  EXPECT_NE(arpTable, nullptr);
  auto serializedArpTable = arpTable->toFollyDynamic();
  auto unserializedArpTable = arpTable->fromFollyDynamic(serializedArpTable);

  testSendArpRequest(sw, vlanID, senderIP, targetIP);

  vlan = sw->getState()->getVlans()->getVlanIf(vlanID);
  EXPECT_NE(vlan, nullptr);
  arpTable = vlan->getArpTable();
  EXPECT_NE(arpTable, nullptr);
  serializedArpTable = arpTable->toFollyDynamic();
  unserializedArpTable = arpTable->fromFollyDynamic(serializedArpTable);

  // Should also see a pending entry
  auto entry = sw->getState()->getVlans()->getVlanIf(vlanID)->getArpTable()
    ->getEntryIf(targetIP);

  EXPECT_NE(sw, nullptr);
}

TEST(ArpTest, ArpExpiration) {
  std::chrono::seconds arpTimeout(1);
  auto sw = setupSwitch(arpTimeout);

  VlanID vlanID(1);
  IPAddressV4 senderIP = IPAddressV4("10.0.0.1");
  IPAddressV4 targetIP = IPAddressV4("10.0.0.2");

  // Cache the current stats
  CounterCache counters(sw.get());

  testSendArpRequest(sw, vlanID, senderIP, targetIP);

  counters.update();
  counters.checkDelta(SwitchStats::kCounterPrefix + "arp.request.tx.sum", 1);
  counters.checkDelta(SwitchStats::kCounterPrefix + "arp.request.rx.sum", 0);
  counters.checkDelta(SwitchStats::kCounterPrefix + "arp.reply.tx.sum", 0);
  counters.checkDelta(SwitchStats::kCounterPrefix + "arp.reply.rx.sum", 0);

  // Should see a pending entry now
  auto entry = sw->getState()->getVlans()->getVlanIf(vlanID)->getArpTable()
    ->getEntryIf(targetIP);
  EXPECT_NE(entry, nullptr);
  EXPECT_EQ(entry->isPending(), true);

  // Send an Arp request for a different neighbor
  IPAddressV4 targetIP2 = IPAddressV4("10.0.0.3");

  testSendArpRequest(sw, vlanID, senderIP, targetIP2);

  counters.update();
  counters.checkDelta(SwitchStats::kCounterPrefix + "arp.request.tx.sum", 1);
  counters.checkDelta(SwitchStats::kCounterPrefix + "arp.request.rx.sum", 0);
  counters.checkDelta(SwitchStats::kCounterPrefix + "arp.reply.tx.sum", 0);
  counters.checkDelta(SwitchStats::kCounterPrefix + "arp.reply.rx.sum", 0);

  // Should see another pending entry now
  waitForStateUpdates(sw.get());
  entry = sw->getState()->getVlans()->getVlanIf(vlanID)->getArpTable()
    ->getEntryIf(targetIP2);
  EXPECT_NE(entry, nullptr);
  EXPECT_EQ(entry->isPending(), true);

  // Receive arp replies for our pending entries
  EXPECT_HW_CALL(sw, stateChanged(_)).Times(testing::AtLeast(1));
  sendArpReply(sw.get(), "10.0.0.2", "02:10:20:30:40:22", 1);
  sendArpReply(sw.get(), "10.0.0.3", "02:10:20:30:40:23", 1);

  // Have getAndClearNeighborHit return false for the first entry,
  // but true for the second. This should result in expiration
  // for the second entry kicking off first.
  EXPECT_HW_CALL(sw, getAndClearNeighborHit(_, testing::Eq(targetIP)))
    .WillRepeatedly(testing::Return(false));
  EXPECT_HW_CALL(sw, getAndClearNeighborHit(_, testing::Eq(targetIP2)))
    .WillRepeatedly(testing::Return(true));

  // The entries should now be valid instead of pending
  waitForStateUpdates(sw.get());
  entry = sw->getState()->getVlans()->getVlanIf(vlanID)->getArpTable()
    ->getEntryIf(targetIP);
  auto entry2 = sw->getState()->getVlans()->getVlanIf(vlanID)->getArpTable()
    ->getEntryIf(targetIP2);
  EXPECT_NE(entry, nullptr);
  EXPECT_NE(entry2, nullptr);
  EXPECT_EQ(entry->isPending(), false);
  EXPECT_EQ(entry2->isPending(), false);

  // Expect one more probe to be sent out before targetIP2 is expired
  auto intfs = sw->getState()->getInterfaces();
  auto intf = intfs->getInterfaceIf(RouterID(0), senderIP);
  EXPECT_PKT(sw, "ARP request",
             checkArpRequest(senderIP, intf->getMac(), targetIP2, vlanID));

 // Wait for the second entry to expire.
 // We wait 2.5 seconds(plus change):
 // Up to 1.5 seconds for lifetime.
 // 1 more second for probe
  EXPECT_HW_CALL(sw, stateChanged(_)).Times(testing::AtLeast(1));
  std::promise<bool> done;
  auto* evb = sw->getBackgroundEVB();
  evb->runInEventBaseThread([&]() {
      evb->tryRunAfterDelay([&]() {
        done.set_value(true);
      }, 2550);
    });
  done.get_future().wait();

  // The first entry should not be expired, but the second should be
  entry = sw->getState()->getVlans()->getVlanIf(vlanID)->getArpTable()
    ->getEntryIf(targetIP);
  entry2 = sw->getState()->getVlans()->getVlanIf(vlanID)->getArpTable()
    ->getEntryIf(targetIP2);
  EXPECT_NE(entry, nullptr);
  EXPECT_EQ(entry2, nullptr);

  // Now return true for the getAndClearNeighborHit calls on the second entry
  EXPECT_HW_CALL(sw, getAndClearNeighborHit(_, testing::Eq(targetIP)))
    .WillRepeatedly(testing::Return(true));

  // Expect one more probe to be sent out before targetIP is expired
  EXPECT_PKT(sw, "ARP request",
             checkArpRequest(senderIP, intf->getMac(), targetIP, vlanID));

 // Wait for the first entry to expire
  EXPECT_HW_CALL(sw, stateChanged(_)).Times(testing::AtLeast(1));
  std::promise<bool> done2;
  evb->runInEventBaseThread([&]() {
      evb->tryRunAfterDelay([&]() {
        done2.set_value(true);
      }, 2050);
    });
  done2.get_future().wait();

  // First entry should now be expired
  entry = sw->getState()->getVlans()->getVlanIf(vlanID)->getArpTable()
    ->getEntryIf(targetIP);
  EXPECT_EQ(entry, nullptr);
}

TEST(ArpTest, PortFlapRecover) {
  std::chrono::seconds arpTimeout(1);
  auto sw = setupSwitch(arpTimeout);

  // We only have special port down handling after the fib is
  // synced, so we set that here.
  sw->fibSynced();

  VlanID vlanID(1);
  IPAddressV4 senderIP = IPAddressV4("10.0.0.1");
  IPAddressV4 targetIP = IPAddressV4("10.0.0.2");

  // Cache the current stats
  CounterCache counters(sw.get());

  testSendArpRequest(sw, vlanID, senderIP, targetIP);

  counters.update();
  counters.checkDelta(SwitchStats::kCounterPrefix + "arp.request.tx.sum", 1);
  counters.checkDelta(SwitchStats::kCounterPrefix + "arp.request.rx.sum", 0);
  counters.checkDelta(SwitchStats::kCounterPrefix + "arp.reply.tx.sum", 0);
  counters.checkDelta(SwitchStats::kCounterPrefix + "arp.reply.rx.sum", 0);

  // Should see a pending entry now
  auto entry = sw->getState()->getVlans()->getVlanIf(vlanID)->getArpTable()
    ->getEntryIf(targetIP);
  EXPECT_NE(entry, nullptr);
  EXPECT_EQ(entry->isPending(), true);

  // Send an Arp request for two different neighbors
  IPAddressV4 targetIP2 = IPAddressV4("10.0.0.3");
  IPAddressV4 targetIP3 = IPAddressV4("10.0.0.4");

  testSendArpRequest(sw, vlanID, senderIP, targetIP2);
  testSendArpRequest(sw, vlanID, senderIP, targetIP3);

  counters.update();
  counters.checkDelta(SwitchStats::kCounterPrefix + "arp.request.tx.sum", 2);
  counters.checkDelta(SwitchStats::kCounterPrefix + "arp.request.rx.sum", 0);
  counters.checkDelta(SwitchStats::kCounterPrefix + "arp.reply.tx.sum", 0);
  counters.checkDelta(SwitchStats::kCounterPrefix + "arp.reply.rx.sum", 0);

  // Should see two more pending entries now
  waitForStateUpdates(sw.get());
  auto entry2 = sw->getState()->getVlans()->getVlanIf(vlanID)->getArpTable()
    ->getEntryIf(targetIP2);
  auto entry3 = sw->getState()->getVlans()->getVlanIf(vlanID)->getArpTable()
    ->getEntryIf(targetIP3);
  EXPECT_NE(entry2, nullptr);
  EXPECT_EQ(entry2->isPending(), true);
  EXPECT_NE(entry3, nullptr);
  EXPECT_EQ(entry3->isPending(), true);

  // Receive arp replies for our pending entries
  EXPECT_HW_CALL(sw, stateChanged(_)).Times(testing::AtLeast(1));
  sendArpReply(sw.get(), "10.0.0.2", "02:10:20:30:40:22", 1);
  sendArpReply(sw.get(), "10.0.0.3", "02:10:20:30:40:23", 1);
  sendArpReply(sw.get(), "10.0.0.4", "02:10:20:30:40:24", 2);

  // The entries should now be valid instead of pending
  waitForStateUpdates(sw.get());
  entry = sw->getState()->getVlans()->getVlanIf(vlanID)->getArpTable()
    ->getEntryIf(targetIP);
  entry2 = sw->getState()->getVlans()->getVlanIf(vlanID)->getArpTable()
    ->getEntryIf(targetIP2);
  entry3 = sw->getState()->getVlans()->getVlanIf(vlanID)->getArpTable()
    ->getEntryIf(targetIP3);
  EXPECT_NE(entry, nullptr);
  EXPECT_NE(entry2, nullptr);
  EXPECT_NE(entry3, nullptr);
  EXPECT_EQ(entry->isPending(), false);
  EXPECT_EQ(entry2->isPending(), false);
  EXPECT_EQ(entry3->isPending(), false);

  // send a port down event to the switch for port 1
  EXPECT_HW_CALL(sw, stateChanged(_)).Times(testing::AtLeast(1));
  sw->linkStateChanged(PortID(1), false);
  // port down handling is async on the bg evb, so
  // block on something coming off of that
  waitForBackgroundThread(sw.get());
  waitForStateUpdates(sw.get());
  // both entries should now be pending
  entry = sw->getState()->getVlans()->getVlanIf(vlanID)->getArpTable()
    ->getEntryIf(targetIP);
  entry2 = sw->getState()->getVlans()->getVlanIf(vlanID)->getArpTable()
    ->getEntryIf(targetIP2);
  entry3 = sw->getState()->getVlans()->getVlanIf(vlanID)->getArpTable()
    ->getEntryIf(targetIP3);
  EXPECT_NE(entry, nullptr);
  EXPECT_NE(entry2, nullptr);
  EXPECT_NE(entry3, nullptr);
  EXPECT_EQ(entry->isPending(), true);
  EXPECT_EQ(entry2->isPending(), true);
  EXPECT_EQ(entry3->isPending(), false);

  // send a port up event to the switch for port 1
  EXPECT_HW_CALL(sw, stateChanged(_)).Times(testing::AtLeast(1));
  sw->linkStateChanged(PortID(1), true);

  // Receive arp replies for our pending entries
  sendArpReply(sw.get(), "10.0.0.2", "02:10:20:30:40:22", 1);
  sendArpReply(sw.get(), "10.0.0.3", "02:10:20:30:40:23", 1);

  // The entries should now be valid again
  waitForStateUpdates(sw.get());
  entry = sw->getState()->getVlans()->getVlanIf(vlanID)->getArpTable()
    ->getEntryIf(targetIP);
  entry2 = sw->getState()->getVlans()->getVlanIf(vlanID)->getArpTable()
    ->getEntryIf(targetIP2);
  entry3 = sw->getState()->getVlans()->getVlanIf(vlanID)->getArpTable()
    ->getEntryIf(targetIP3);
  EXPECT_NE(entry, nullptr);
  EXPECT_NE(entry2, nullptr);
  EXPECT_NE(entry3, nullptr);
  EXPECT_EQ(entry->isPending(), false);
  EXPECT_EQ(entry2->isPending(), false);
  EXPECT_EQ(entry3->isPending(), false);
}
