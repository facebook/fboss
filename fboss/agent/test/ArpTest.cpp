/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include <fb303/ServiceData.h>
#include <folly/Memory.h>
#include <folly/io/Cursor.h>
#include <folly/io/IOBuf.h>
#include "fboss/agent/AddressUtil.h"
#include "fboss/agent/ArpHandler.h"
#include "fboss/agent/FbossError.h"
#include "fboss/agent/NeighborUpdater.h"
#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/SwitchStats.h"
#include "fboss/agent/ThriftHandler.h"
#include "fboss/agent/TxPacket.h"
#include "fboss/agent/hw/mock/MockHwSwitch.h"
#include "fboss/agent/hw/mock/MockPlatform.h"
#include "fboss/agent/hw/mock/MockRxPacket.h"
#include "fboss/agent/packet/PktUtil.h"
#include "fboss/agent/state/ArpEntry.h"
#include "fboss/agent/state/ArpResponseTable.h"
#include "fboss/agent/state/ArpTable.h"
#include "fboss/agent/state/Interface.h"
#include "fboss/agent/state/PortDescriptor.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/state/Vlan.h"
#include "fboss/agent/state/VlanMap.h"
#include "fboss/agent/test/CounterCache.h"
#include "fboss/agent/test/HwTestHandle.h"
#include "fboss/agent/test/NeighborEntryTest.h"
#include "fboss/agent/test/TestUtils.h"

#include <boost/range/combine.hpp>
#include <gtest/gtest.h>
#include <array>
#include <future>
#include <string>

using namespace facebook::fboss;
using facebook::network::toBinaryAddress;
using facebook::network::toIPAddress;
using facebook::network::thrift::BinaryAddress;
using folly::IOBuf;
using folly::IPAddressV4;
using folly::StringPiece;
using folly::io::Cursor;
using std::make_shared;
using std::make_unique;
using std::shared_ptr;
using std::string;
using std::unique_ptr;

using ::testing::_;

namespace {
const uint8_t kNCStrictPriorityQueue = 7;

unique_ptr<HwTestHandle> setupTestHandle(
    std::chrono::seconds arpTimeout = std::chrono::seconds(0),
    uint32_t maxProbes = 1,
    std::chrono::seconds staleTimeout = std::chrono::seconds(1)) {
  // Setup a default state object
  auto cfg = testConfigA();
  auto handle = createTestHandle(&cfg);
  auto updater = handle->getSw()->getRouteUpdater();

  RouteNextHopSet nexthops;
  // resolved by intf 1
  nexthops.emplace(
      UnresolvedNextHop(folly::IPAddress("10.0.0.22"), UCMP_DEFAULT_WEIGHT));
  // resolved by intf 1
  nexthops.emplace(
      UnresolvedNextHop(folly::IPAddress("10.0.0.23"), UCMP_DEFAULT_WEIGHT));
  // un-resolvable
  nexthops.emplace(
      UnresolvedNextHop(folly::IPAddress("1.1.2.10"), UCMP_DEFAULT_WEIGHT));

  updater.addRoute(
      RouterID(0),
      folly::IPAddress("10.1.1.0"),
      24,
      ClientID(1001),
      RouteNextHopEntry(nexthops, AdminDistance::MAX_ADMIN_DISTANCE));

  updater.program();

  handle->getSw()->initialConfigApplied(std::chrono::steady_clock::now());
  auto scopeResolver = handle->getSw()->getScopeResolver();
  handle->getSw()->updateState(
      " set timers",
      [arpTimeout, maxProbes, staleTimeout, &scopeResolver](auto inState) {
        inState = inState->clone();
        auto switchSettings = make_shared<SwitchSettings>();
        if (arpTimeout.count() > 0) {
          switchSettings->setArpTimeout(arpTimeout);
          switchSettings->setStaleEntryInterval(staleTimeout);
        }
        switchSettings->setMaxNeighborProbes(maxProbes);
        auto multiSwitchSwitchSettings = make_shared<MultiSwitchSettings>();
        multiSwitchSwitchSettings->addNode(
            scopeResolver->scope(switchSettings).matcherString(),
            switchSettings);
        inState->resetSwitchSettings(multiSwitchSwitchSettings);
        inState->publish();
        return inState;
      });

  waitForStateUpdates(handle->getSw());
  return handle;
}

TxMatchFn checkArpPkt(
    bool isRequest,
    IPAddressV4 senderIP,
    MacAddress senderMAC,
    IPAddressV4 targetIP,
    MacAddress targetMAC,
    VlanID vlan) {
  return [=](const TxPacket* pkt) {
    const auto* buf = pkt->buf();
    // 64 bytes (minimum ethernet frame length),
    // plus 4 bytes for the 802.1q header
    EXPECT_EQ(68, buf->computeChainDataLength());

    Cursor c(buf);
    auto dstMac = PktUtil::readMac(&c);
    if (targetMAC != dstMac) {
      throw FbossError("expected dest MAC to be ", targetMAC, "; got ", dstMac);
    }

    auto srcMac = PktUtil::readMac(&c);
    EXPECT_EQ(senderMAC, srcMac);
    if (senderMAC != srcMac) {
      throw FbossError(" expected src MAC to be ", senderMAC, "; got ", srcMac);
    }

    auto ethertype = c.readBE<uint16_t>();
    if (ethertype != 0x8100) {
      throw FbossError(
          " expected VLAN tag to be present, found ethertype ", ethertype);
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
      throw FbossError(
          "expected sender MAC ", senderMAC, " found ", pktSenderMAC);
    }
    auto pktSenderIP = PktUtil::readIPv4(&c);
    if (pktSenderIP != senderIP) {
      throw FbossError("expected sender IP ", senderIP, " found ", pktSenderIP);
    }
    auto pktTargetMAC = PktUtil::readMac(&c);
    auto expectMAC = isRequest ? MacAddress::ZERO : targetMAC;
    if (pktTargetMAC != expectMAC) {
      throw FbossError(
          "expected target MAC ", expectMAC, " found ", pktTargetMAC);
    }
    auto pktTargetIP = PktUtil::readIPv4(&c);
    if (pktTargetIP != targetIP) {
      throw FbossError("expected target IP ", targetIP, " found ", pktTargetIP);
    }
  };
}

TxMatchFn checkArpReply(
    const char* senderIP,
    const char* senderMAC,
    const char* targetIP,
    const char* targetMAC,
    VlanID vlan) {
  bool isRequest = false;
  return checkArpPkt(
      isRequest,
      IPAddressV4(senderIP),
      MacAddress(senderMAC),
      IPAddressV4(targetIP),
      MacAddress(targetMAC),
      vlan);
}

TxMatchFn checkArpRequest(
    IPAddressV4 senderIP,
    MacAddress senderMAC,
    IPAddressV4 targetIP,
    VlanID vlan) {
  bool isRequest = true;
  return checkArpPkt(
      isRequest, senderIP, senderMAC, targetIP, MacAddress::BROADCAST, vlan);
}

const std::shared_ptr<ArpEntry>
getArpEntry(SwSwitch* sw, IPAddressV4 ip, VlanID vlanID = VlanID(1)) {
  return sw->getState()
      ->getVlans()
      ->getNodeIf(vlanID)
      ->getArpTable()
      ->getEntryIf(ip);
}

/* This helper sends an arp request for targetIP and verifies it was correctly
   sent out. */
void testSendArpRequest(
    SwSwitch* sw,
    VlanID vlanID,
    IPAddressV4 senderIP,
    IPAddressV4 targetIP) {
  auto state = sw->getState();
  auto vlan = state->getVlans()->getNodeIf(vlanID);
  EXPECT_NE(vlan, nullptr);
  auto intf = state->getInterfaces()->getInterfaceIf(RouterID(0), senderIP);
  EXPECT_NE(intf, nullptr);

  // Cache the current stats
  CounterCache counters(sw);

  // Expect ARP entry to be created
  WaitForArpEntryCreation arpCreate(sw, targetIP);
  if (state->getMaxNeighborProbes() > 1) {
    EXPECT_MANY_SWITCHED_PKTS(
        sw,
        "ARP request",
        checkArpRequest(senderIP, intf->getMac(), targetIP, vlanID));
  } else {
    EXPECT_SWITCHED_PKT(
        sw,
        "ARP request",
        checkArpRequest(senderIP, intf->getMac(), targetIP, vlanID));
  }
  ArpHandler::sendArpRequest(
      sw, vlan->getID(), intf->getMac(), senderIP, targetIP);

  // Notify the updater that we sent an arp request
  sw->getNeighborUpdater()->sentArpRequest(vlanID, targetIP);

  waitForStateUpdates(sw);
  EXPECT_TRUE(arpCreate.wait());

  counters.update();
  counters.checkDelta(SwitchStats::kCounterPrefix + "arp.request.tx.sum", 1);
  counters.checkDelta(SwitchStats::kCounterPrefix + "arp.request.rx.sum", 0);
  counters.checkDelta(SwitchStats::kCounterPrefix + "arp.reply.tx.sum", 0);
  counters.checkDelta(SwitchStats::kCounterPrefix + "arp.reply.rx.sum", 0);
}

} // unnamed namespace

TEST(ArpTest, BasicSendRequest) {
  auto handle = setupTestHandle();
  auto sw = handle->getSw();
  VlanID vlanID(1);
  IPAddressV4 senderIP = IPAddressV4("10.0.0.1");
  IPAddressV4 targetIP = IPAddressV4("10.0.0.2");

  WaitForArpEntryExpiration arpExpiry(sw, targetIP);
  // Cache the current stats
  CounterCache counters(sw);

  auto state = sw->getState();
  auto vlan = state->getVlans()->getNodeIf(vlanID);
  EXPECT_NE(vlan, nullptr);
  auto intf = state->getInterfaces()->getInterfaceIf(RouterID(0), senderIP);
  EXPECT_NE(intf, nullptr);

  // Sending an ARP request will trigger state update for setting pending entry
  EXPECT_STATE_UPDATE_TIMES(sw, 2);
  EXPECT_SWITCHED_PKT(
      sw,
      "ARP request",
      checkArpRequest(senderIP, intf->getMac(), targetIP, vlanID));

  ArpHandler::sendArpRequest(
      sw, vlan->getID(), intf->getMac(), senderIP, targetIP);

  // Notify the updater that we sent an arp request
  sw->getNeighborUpdater()->sentArpRequest(vlanID, targetIP);

  waitForStateUpdates(sw);
  EXPECT_TRUE(arpExpiry.wait());

  counters.update();
  counters.checkDelta(SwitchStats::kCounterPrefix + "arp.request.tx.sum", 1);
  counters.checkDelta(SwitchStats::kCounterPrefix + "arp.request.rx.sum", 0);
  counters.checkDelta(SwitchStats::kCounterPrefix + "arp.reply.tx.sum", 0);
  counters.checkDelta(SwitchStats::kCounterPrefix + "arp.reply.rx.sum", 0);
}

TEST(ArpTest, TableUpdates) {
  auto handle = setupTestHandle();
  auto sw = handle->getSw();
  VlanID vlanID(1);

  // Create an ARP request for 10.0.0.1 from an unreachable source
  auto buf = make_unique<IOBuf>(PktUtil::parseHexData(
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
      "0a 00 00 01"));

  // Cache the current stats
  CounterCache counters(sw);

  // Sending the ARP request to the switch should not trigger an update to the
  // ArpTable for VLAN 1, but will send a reply packet
  EXPECT_STATE_UPDATE_TIMES(sw, 0);
  EXPECT_OUT_OF_PORT_PKT(
      sw,
      "ARP reply",
      checkArpReply(
          "10.0.0.1",
          "00:02:00:00:00:01",
          "10.0.1.15",
          "00:02:00:01:02:03",
          vlanID),
      PortID(1),
      std::optional<uint8_t>(kNCStrictPriorityQueue));

  // Inform the SwSwitch of the ARP request
  handle->rxPacket(std::move(buf), PortID(1), vlanID);
  sw->getNeighborUpdater()->waitForPendingUpdates();

  // Check the new ArpTable does not have any entry
  auto arpTable = sw->getState()->getVlans()->getNode(vlanID)->getArpTable();
  EXPECT_EQ(0, arpTable->size());

  // Create an ARP request for 10.0.0.1
  auto hex = PktUtil::parseHexData(
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
      "0a 00 00 01");

  // update counters
  counters.update();

  // Sending the ARP request to the switch should trigger an update to the
  // ArpTable for VLAN 1, and will then send a reply packet
  EXPECT_STATE_UPDATE_TIMES(sw, 2);
  EXPECT_OUT_OF_PORT_PKT(
      sw,
      "ARP reply",
      checkArpReply(
          "10.0.0.1",
          "00:02:00:00:00:01",
          "10.0.0.15",
          "00:02:00:01:02:03",
          vlanID),
      PortID(1),
      std::optional<uint8_t>(kNCStrictPriorityQueue));

  // Inform the SwSwitch of the ARP request
  handle->rxPacket(make_unique<IOBuf>(hex), PortID(1), vlanID);
  sw->getNeighborUpdater()->waitForPendingUpdates();

  // Wait for any updates triggered by the packet to complete.
  waitForStateUpdates(sw);

  // Check the new ArpTable contents
  arpTable = sw->getState()->getVlans()->getNode(vlanID)->getArpTable();
  EXPECT_EQ(1, arpTable->size());
  auto entry = arpTable->getEntry(IPAddressV4("10.0.0.15"));
  EXPECT_EQ(MacAddress("00:02:00:01:02:03"), entry->getMac());
  EXPECT_EQ(PortDescriptor(PortID(1)), entry->getPort());
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
  EXPECT_STATE_UPDATE_TIMES(sw, 0);
  EXPECT_OUT_OF_PORT_PKT(
      sw,
      "ARP reply",
      checkArpReply(
          "10.0.0.1",
          "00:02:00:00:00:01",
          "10.0.0.15",
          "00:02:00:01:02:03",
          vlanID),
      PortID(1),
      std::optional<uint8_t>(kNCStrictPriorityQueue));
  handle->rxPacket(make_unique<IOBuf>(hex), PortID(1), vlanID);
  sw->getNeighborUpdater()->waitForPendingUpdates();

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
  buf = make_unique<IOBuf>(PktUtil::parseHexData(
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
      "0a 00 00 32"));

  EXPECT_STATE_UPDATE_TIMES(sw, 0);
  EXPECT_HW_CALL(sw, sendPacketSwitchedAsync_(_)).Times(0);
  handle->rxPacket(std::move(buf), PortID(1), vlanID);
  sw->getNeighborUpdater()->waitForPendingUpdates();

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
  buf = make_unique<IOBuf>(PktUtil::parseHexData(
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
      "0a 00 00 32"));

  // Expect 2+ updates, one for the arp entry update (MAC changed)
  // And then 1 (or 2 depends on coalescing) for Static mac updates
  EXPECT_STATE_UPDATE_TIMES_ATLEAST(sw, 2);
  EXPECT_HW_CALL(sw, sendPacketSwitchedAsync_(_)).Times(0);
  handle->rxPacket(std::move(buf), PortID(2), vlanID);
  sw->getNeighborUpdater()->waitForPendingUpdates();
  waitForStateUpdates(sw);

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

  arpTable = sw->getState()->getVlans()->getNode(vlanID)->getArpTable();
  EXPECT_EQ(1, arpTable->size());
  entry = arpTable->getEntry(IPAddressV4("10.0.0.15"));
  EXPECT_EQ(MacAddress("00:02:00:01:02:08"), entry->getMac());
  EXPECT_EQ(PortDescriptor(PortID(2)), entry->getPort());
  EXPECT_EQ(InterfaceID(1), entry->getIntfID());

  // Try another request for us from a node we haven't seen yet.
  // This should generate a reply and a table update
  buf = make_unique<IOBuf>(PktUtil::parseHexData(
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
      "0a 00 00 01"));

  // Arp resolution also triggers a static
  // MAC entry creation update
  EXPECT_STATE_UPDATE_TIMES(sw, 2);
  EXPECT_OUT_OF_PORT_PKT(
      sw,
      "ARP reply",
      checkArpReply(
          "10.0.0.1",
          "00:02:00:00:00:01",
          "10.0.0.20",
          "00:02:00:01:02:23",
          vlanID),
      PortID(1),
      std::optional<uint8_t>(kNCStrictPriorityQueue));
  handle->rxPacket(std::move(buf), PortID(1), vlanID);
  sw->getNeighborUpdater()->waitForPendingUpdates();
  waitForStateUpdates(sw);

  counters.update();
  counters.checkDelta(SwitchStats::kCounterPrefix + "trapped.pkts.sum", 1);
  counters.checkDelta(SwitchStats::kCounterPrefix + "trapped.arp.sum", 1);
  counters.checkDelta(SwitchStats::kCounterPrefix + "arp.request.tx.sum", 0);
  counters.checkDelta(SwitchStats::kCounterPrefix + "arp.request.rx.sum", 1);
  counters.checkDelta(SwitchStats::kCounterPrefix + "arp.reply.tx.sum", 1);
  counters.checkDelta(SwitchStats::kCounterPrefix + "arp.reply.rx.sum", 0);
  counters.checkDelta(SwitchStats::kCounterPrefix + "trapped.drops.sum", 0);
  counters.checkDelta(SwitchStats::kCounterPrefix + "trapped.error.sum", 0);

  arpTable = sw->getState()->getVlans()->getNode(vlanID)->getArpTable();
  EXPECT_EQ(2, arpTable->size());
  entry = arpTable->getEntry(IPAddressV4("10.0.0.15"));
  EXPECT_EQ(MacAddress("00:02:00:01:02:08"), entry->getMac());
  EXPECT_EQ(PortDescriptor(PortID(2)), entry->getPort());
  EXPECT_EQ(InterfaceID(1), entry->getIntfID());
  entry = arpTable->getEntry(IPAddressV4("10.0.0.20"));
  EXPECT_EQ(MacAddress("00:02:00:01:02:23"), entry->getMac());
  EXPECT_EQ(PortDescriptor(PortID(1)), entry->getPort());
  EXPECT_EQ(InterfaceID(1), entry->getIntfID());

  // Try a request for an IP on another interface, to make sure
  // it generates an ARP entry with the correct interface ID.
  buf = make_unique<IOBuf>(PktUtil::parseHexData(
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
      "c0 a8 00 01"));

  // Arp resolution also triggers a static
  // MAC entry creation update
  EXPECT_STATE_UPDATE_TIMES(sw, 2);
  EXPECT_OUT_OF_PORT_PKT(
      sw,
      "ARP reply",
      checkArpReply(
          "192.168.0.1",
          "00:02:00:00:00:01",
          "192.168.0.20",
          "00:02:00:55:66:77",
          vlanID),
      PortID(5),
      std::optional<uint8_t>(kNCStrictPriorityQueue));
  handle->rxPacket(std::move(buf), PortID(5), vlanID);
  sw->getNeighborUpdater()->waitForPendingUpdates();
  waitForStateUpdates(sw);

  counters.update();
  counters.checkDelta(SwitchStats::kCounterPrefix + "trapped.pkts.sum", 1);
  counters.checkDelta(SwitchStats::kCounterPrefix + "trapped.arp.sum", 1);
  counters.checkDelta(SwitchStats::kCounterPrefix + "arp.request.tx.sum", 0);
  counters.checkDelta(SwitchStats::kCounterPrefix + "arp.request.rx.sum", 1);
  counters.checkDelta(SwitchStats::kCounterPrefix + "arp.reply.tx.sum", 1);
  counters.checkDelta(SwitchStats::kCounterPrefix + "arp.reply.rx.sum", 0);
  counters.checkDelta(SwitchStats::kCounterPrefix + "trapped.drops.sum", 0);
  counters.checkDelta(SwitchStats::kCounterPrefix + "trapped.error.sum", 0);

  arpTable = sw->getState()->getVlans()->getNode(vlanID)->getArpTable();
  EXPECT_EQ(3, arpTable->size());
  entry = arpTable->getEntry(IPAddressV4("10.0.0.15"));
  EXPECT_EQ(MacAddress("00:02:00:01:02:08"), entry->getMac());
  EXPECT_EQ(PortDescriptor(PortID(2)), entry->getPort());
  EXPECT_EQ(InterfaceID(1), entry->getIntfID());
  entry = arpTable->getEntry(IPAddressV4("10.0.0.20"));
  EXPECT_EQ(MacAddress("00:02:00:01:02:23"), entry->getMac());
  EXPECT_EQ(PortDescriptor(PortID(1)), entry->getPort());
  EXPECT_EQ(InterfaceID(1), entry->getIntfID());
  entry = arpTable->getEntry(IPAddressV4("192.168.0.20"));
  EXPECT_EQ(MacAddress("00:02:00:55:66:77"), entry->getMac());
  EXPECT_EQ(PortDescriptor(PortID(5)), entry->getPort());
  EXPECT_EQ(InterfaceID(1), entry->getIntfID());
}

TEST(ArpTest, NotMine) {
  auto handle = setupTestHandle();
  auto sw = handle->getSw();

  // Create an ARP request for 10.1.2.3
  auto buf = make_unique<IOBuf>(PktUtil::parseHexData(
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
      "0a 01 02 03"));

  // Cache the current stats
  CounterCache counters(sw);

  // Inform the SwSwitch of the ARP request
  handle->rxPacket(std::move(buf), PortID(1), VlanID(1));
  sw->getNeighborUpdater()->waitForPendingUpdates();

  // Check the new stats
  counters.update();
  counters.checkDelta(SwitchStats::kCounterPrefix + "trapped.pkts.sum", 1);
  counters.checkDelta(SwitchStats::kCounterPrefix + "trapped.arp.sum", 1);
  counters.checkDelta(SwitchStats::kCounterPrefix + "arp.not_mine.sum", 1);
  counters.checkDelta(SwitchStats::kCounterPrefix + "trapped.drops.sum", 1);
  counters.checkDelta(SwitchStats::kCounterPrefix + "trapped.error.sum", 0);
}

TEST(ArpTest, BadHlen) {
  auto handle = setupTestHandle();
  auto sw = handle->getSw();

  // Create an ARP request with a bad hardware length specified
  auto buf = make_unique<IOBuf>(PktUtil::parseHexData(
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
      "0a 00 00 01"));

  // Cache the current stats
  CounterCache counters(sw);

  // Inform the SwSwitch of the ARP request
  handle->rxPacket(std::move(buf), PortID(1), VlanID(1));
  sw->getNeighborUpdater()->waitForPendingUpdates();

  // Check the new stats
  counters.update();
  counters.checkDelta(SwitchStats::kCounterPrefix + "trapped.pkts.sum", 1);
  counters.checkDelta(SwitchStats::kCounterPrefix + "trapped.arp.sum", 1);
  counters.checkDelta(SwitchStats::kCounterPrefix + "arp.unsupported.sum", 1);
  counters.checkDelta(SwitchStats::kCounterPrefix + "trapped.drops.sum", 1);
  counters.checkDelta(SwitchStats::kCounterPrefix + "trapped.error.sum", 0);
}

void sendArpReply(
    HwTestHandle* handle,
    StringPiece ipStr,
    StringPiece macStr,
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

  // Inform the SwSwitch of the ARP request
  handle->rxPacket(std::move(buf), PortID(port), VlanID(1));
  handle->getSw()->getNeighborUpdater()->waitForPendingUpdates();
}

TEST(ArpTest, FlushEntry) {
  auto handle = setupTestHandle();
  auto sw = handle->getSw();

  // Send ARP replies from several nodes to populate the table
  EXPECT_STATE_UPDATE_TIMES_ATLEAST(sw, 1);
  sendArpReply(handle.get(), "10.0.0.11", "02:10:20:30:40:11", 2);
  sendArpReply(handle.get(), "10.0.0.15", "02:10:20:30:40:15", 3);
  sendArpReply(handle.get(), "10.0.0.7", "02:10:20:30:40:07", 1);
  sendArpReply(handle.get(), "10.0.0.22", "02:10:20:30:40:22", 4);

  // Wait for all state updates from the ARP replies to be applied
  waitForStateUpdates(sw);

  // Use the thrift API to confirm that the ARP table is populated as expected.
  // This also serves to test the thrift handler functionality.
  ThriftHandler thriftHandler(sw);

  std::vector<ArpEntryThrift> arpTable;
  thriftHandler.getArpTable(arpTable);
  ASSERT_EQ(arpTable.size(), 4);
  // Sort the results so we can check the exact ordering here.
  std::sort(
      arpTable.begin(),
      arpTable.end(),
      [](const ArpEntryThrift& a, const ArpEntryThrift& b) {
        return *a.port() < *b.port();
      });
  auto checkEntry = [&](int idx, StringPiece ip, StringPiece mac, int port) {
    SCOPED_TRACE(folly::to<string>("index ", idx));
    EXPECT_EQ(*arpTable[idx].vlanID(), 1);
    EXPECT_EQ(*arpTable[idx].vlanName(), "Vlan1");
    EXPECT_EQ(toIPAddress(*arpTable[idx].ip()).str(), ip);
    EXPECT_EQ(*arpTable[idx].mac(), mac);
    EXPECT_EQ(*arpTable[idx].port(), port);
  };
  checkEntry(0, "10.0.0.7", "02:10:20:30:40:07", 1);
  checkEntry(1, "10.0.0.11", "02:10:20:30:40:11", 2);
  checkEntry(2, "10.0.0.15", "02:10:20:30:40:15", 3);
  checkEntry(3, "10.0.0.22", "02:10:20:30:40:22", 4);

  // Via the thrift API, flush the ARP entry for 10.0.0.11
  // This in turn will trigger static MAC entry pruning update
  EXPECT_STATE_UPDATE_TIMES_ATLEAST(sw, 2);
  auto binAddr = toBinaryAddress(IPAddressV4("10.0.0.11"));
  auto numFlushed =
      thriftHandler.flushNeighborEntry(make_unique<BinaryAddress>(binAddr), 1);
  EXPECT_EQ(numFlushed, 1);
  waitForStateUpdates(sw);

  // Now check the table again
  arpTable.clear();
  thriftHandler.getArpTable(arpTable);
  ASSERT_EQ(arpTable.size(), 3);
  // Sort the results so we can check the exact ordering here.
  std::sort(
      arpTable.begin(),
      arpTable.end(),
      [](const ArpEntryThrift& a, const ArpEntryThrift& b) {
        return *a.port() < *b.port();
      });
  checkEntry(0, "10.0.0.7", "02:10:20:30:40:07", 1);
  checkEntry(1, "10.0.0.15", "02:10:20:30:40:15", 3);
  checkEntry(2, "10.0.0.22", "02:10:20:30:40:22", 4);

  // Calling flushNeighborEntry() with an IP that isn't present in the table
  // should do nothing and return 0
  EXPECT_STATE_UPDATE_TIMES(sw, 0);
  binAddr = toBinaryAddress(IPAddressV4("10.0.0.254"));
  numFlushed =
      thriftHandler.flushNeighborEntry(make_unique<BinaryAddress>(binAddr), 1);
  EXPECT_EQ(numFlushed, 0);
  binAddr = toBinaryAddress(IPAddressV4("1.2.3.4"));
  numFlushed =
      thriftHandler.flushNeighborEntry(make_unique<BinaryAddress>(binAddr), 1);
  EXPECT_EQ(numFlushed, 0);

  // Calling flushNeighborEntry() with a bogus VLAN ID should throw an error.
  binAddr = toBinaryAddress(IPAddressV4("1.2.3.4"));
  auto binAddrPtr = make_unique<BinaryAddress>(binAddr);
  EXPECT_THROW(
      thriftHandler.flushNeighborEntry(std::move(binAddrPtr), 123), FbossError);
}

TEST(ArpTest, PendingArp) {
  auto handle = setupTestHandle();
  auto sw = handle->getSw();

  VlanID vlanID(1);
  IPAddressV4 senderIP = IPAddressV4("10.0.0.1");

  // Cache the current stats
  CounterCache counters(sw);

  // Create an IP pkt for 10.0.0.10
  auto hex = PktUtil::parseHexData(
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
      "0a 00 00 0a");

  // Receiving this packet should trigger an ARP request out,
  // and the state should now include a pending arp entry.
  EXPECT_SWITCHED_PKT(
      sw,
      "ARP request",
      checkArpRequest(
          senderIP,
          MacAddress("00:02:00:00:00:01"),
          IPAddressV4("10.0.0.10"),
          vlanID));

  handle->rxPacket(make_unique<IOBuf>(hex), PortID(1), vlanID);
  sw->getNeighborUpdater()->waitForPendingUpdates();

  // Should see a pending entry now
  waitForStateUpdates(sw);
  auto entry = getArpEntry(sw, IPAddressV4("10.0.0.10"), vlanID);
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

  handle->rxPacket(make_unique<IOBuf>(hex), PortID(1), vlanID);
  sw->getNeighborUpdater()->waitForPendingUpdates();

  // Should still see a pending entry now
  waitForStateUpdates(sw);
  entry = getArpEntry(sw, IPAddressV4("10.0.0.10"), vlanID);
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
  counters.checkDelta(SwitchStats::kCounterPrefix + "ipv4.no_arp.sum", 1);

  // Receive an arp reply for our pending entry
  sendArpReply(handle.get(), "10.0.0.10", "02:10:20:30:40:22", 1);

  // The entry should now be valid instead of pending
  waitForStateUpdates(sw);
  entry = getArpEntry(sw, IPAddressV4("10.0.0.10"), vlanID);
  EXPECT_NE(entry, nullptr);
  EXPECT_EQ(entry->isPending(), false);

  // Verify that we don't ever overwrite a valid entry with a pending one.
  // Receive the same packet again, entry should still be valid

  handle->rxPacket(make_unique<IOBuf>(hex), PortID(1), vlanID);
  sw->getNeighborUpdater()->waitForPendingUpdates();
  waitForStateUpdates(sw);
  entry = getArpEntry(sw, IPAddressV4("10.0.0.10"), vlanID);
  EXPECT_NE(entry, nullptr);
  EXPECT_EQ(entry->isPending(), false);
};

TEST(ArpTest, PendingArpCleanup) {
  auto handle = setupTestHandle(std::chrono::seconds(1));
  auto sw = handle->getSw();

  VlanID vlanID(1);
  IPAddressV4 senderIP = IPAddressV4("10.0.0.1");
  std::array<IPAddressV4, 2> targetIP = {
      IPAddressV4("10.0.0.2"), IPAddressV4("10.0.0.3")};

  // Wait for pending entries to expire
  std::array<std::unique_ptr<WaitForArpEntryExpiration>, 2> arpExpirations;
  std::transform(
      targetIP.begin(),
      targetIP.end(),
      arpExpirations.begin(),
      [&](const IPAddressV4& ip) {
        return make_unique<WaitForArpEntryExpiration>(sw, ip);
      });

  testSendArpRequest(sw, vlanID, senderIP, targetIP[0]);

  // Send an Arp request for a different neighbor
  testSendArpRequest(sw, vlanID, senderIP, targetIP[1]);

  std::promise<bool> done;
  auto* evb = sw->getBackgroundEvb();
  evb->runInEventBaseThread(
      [&]() { evb->tryRunAfterDelay([&]() { done.set_value(true); }, 1010); });
  done.get_future().wait(); // Entries should be removed

  for (auto& arpExpiry : arpExpirations) {
    EXPECT_TRUE(arpExpiry->wait());
  }
}

TEST(ArpTest, ArpTableSerialization) {
  auto handle = setupTestHandle();
  auto sw = handle->getSw();

  VlanID vlanID(1);
  IPAddressV4 senderIP = IPAddressV4("10.0.0.1");
  IPAddressV4 targetIP = IPAddressV4("10.0.0.2");

  auto vlan = sw->getState()->getVlans()->getNodeIf(vlanID);
  EXPECT_NE(vlan, nullptr);
  auto arpTable = vlan->getArpTable();
  EXPECT_NE(arpTable, nullptr);
  auto serializedArpTable = arpTable->toThrift();
  auto unserializedArpTable = std::make_shared<ArpTable>(serializedArpTable);

  testSendArpRequest(sw, vlanID, senderIP, targetIP);

  EXPECT_STATE_UPDATE_TIMES(sw, 0);
  vlan = sw->getState()->getVlans()->getNodeIf(vlanID);
  EXPECT_NE(vlan, nullptr);
  arpTable = vlan->getArpTable();
  EXPECT_NE(arpTable, nullptr);
  serializedArpTable = arpTable->toThrift();
  unserializedArpTable = std::make_shared<ArpTable>(serializedArpTable);

  // Should also see a pending entry
  auto entry = getArpEntry(sw, targetIP, vlanID);

  EXPECT_NE(entry, nullptr);
  EXPECT_TRUE(entry->isPending());
  EXPECT_NE(sw, nullptr);
}

TEST(ArpTest, ArpExpiration) {
  auto handle = setupTestHandle(std::chrono::seconds(1));
  auto sw = handle->getSw();

  VlanID vlanID(1);
  IPAddressV4 senderIP = IPAddressV4("10.0.0.1");
  std::array<IPAddressV4, 2> targetIP = {
      IPAddressV4("10.0.0.2"), IPAddressV4("10.0.0.3")};
  std::array<MacAddress, 2> targetMAC = {
      MacAddress("02:10:20:30:40:22"), MacAddress("02:10:20:30:40:23")};

  for (auto ip : targetIP) {
    testSendArpRequest(sw, vlanID, senderIP, ip);
  }

  std::array<std::unique_ptr<WaitForArpEntryReachable>, 2> arpReachables;

  std::transform(
      targetIP.begin(),
      targetIP.end(),
      arpReachables.begin(),
      [&](const IPAddressV4& ip) {
        return make_unique<WaitForArpEntryReachable>(sw, ip);
      });

  // Receive arp replies for our pending entries
  for (auto tuple : boost::combine(targetIP, targetMAC)) {
    auto ip = tuple.get<0>();
    auto mac = tuple.get<1>();
    sendArpReply(handle.get(), ip.str(), mac.toString(), 1);
  }

  // Have getAndClearNeighborHit return false for the first entry,
  // but true for the second. This should result in expiration
  // for the second entry kicking off first.
  EXPECT_HW_CALL(sw, getAndClearNeighborHit(_, testing::Eq(targetIP[0])))
      .WillRepeatedly(testing::Return(false));
  EXPECT_HW_CALL(sw, getAndClearNeighborHit(_, testing::Eq(targetIP[1])))
      .WillRepeatedly(testing::Return(true));

  // The entries should now be valid instead of pending
  waitForStateUpdates(sw);
  for (auto& arpReachable : arpReachables) {
    EXPECT_TRUE(arpReachable->wait());
  }

  // Expect one more probe to be sent out before targetIP[1] is expired
  auto intfs = sw->getState()->getInterfaces();
  auto intf = intfs->getInterfaceIf(RouterID(0), senderIP);
  EXPECT_SWITCHED_PKT(
      sw,
      "ARP request",
      checkArpRequest(senderIP, intf->getMac(), targetIP[1], vlanID));

  // Wait for the second entry to expire.
  // We wait 2.5 seconds(plus change):
  // Up to 1.5 seconds for lifetime.
  // 1 more second for probe
  EXPECT_STATE_UPDATE_TIMES_ATLEAST(sw, 1);
  std::array<unique_ptr<WaitForArpEntryExpiration>, 2> arpExpirations;

  std::transform(
      targetIP.begin(),
      targetIP.end(),
      arpExpirations.begin(),
      [&](const IPAddressV4& ip) {
        return make_unique<WaitForArpEntryExpiration>(sw, ip);
      });

  std::promise<bool> done;

  auto* evb = sw->getBackgroundEvb();
  evb->runInEventBaseThread(
      [&]() { evb->tryRunAfterDelay([&]() { done.set_value(true); }, 2550); });
  done.get_future().wait();

  // The first entry should not be expired, but the second should be
  EXPECT_TRUE(getArpEntry(sw, targetIP[0]) != nullptr);
  EXPECT_TRUE(arpExpirations[1]->wait());

  auto entry = getArpEntry(sw, targetIP[0], vlanID);
  EXPECT_NE(entry, nullptr);

  // Now return true for the getAndClearNeighborHit calls on the second entry
  EXPECT_HW_CALL(sw, getAndClearNeighborHit(_, testing::Eq(targetIP[0])))
      .WillRepeatedly(testing::Return(true));

  // Expect one more probe to be sent out before targetIP is expired
  EXPECT_SWITCHED_PKT(
      sw,
      "ARP request",
      checkArpRequest(senderIP, intf->getMac(), targetIP[0], vlanID));

  // Wait for the first entry to expire
  std::promise<bool> done2;
  evb->runInEventBaseThread(
      [&]() { evb->tryRunAfterDelay([&]() { done2.set_value(true); }, 2050); });
  done2.get_future().wait();

  // First entry should now be expired
  EXPECT_TRUE(arpExpirations[0]->wait());
}

TEST(ArpTest, FlushEntryWithConcurrentUpdate) {
  auto handle = setupTestHandle();
  auto sw = handle->getSw();
  ThriftHandler thriftHandler(sw);

  PortID portID(1);
  // ensure port is up
  sw->linkStateChanged(portID, true);

  VlanID vlanID(1);
  std::vector<IPAddressV4> targetIPs;
  for (uint32_t i = 1; i <= 255; i++) {
    targetIPs.push_back(IPAddressV4("10.0.0." + std::to_string(i)));
  }

  // populate arp entries first before flush
  {
    std::array<std::unique_ptr<WaitForArpEntryReachable>, 255> arpReachables;
    std::transform(
        targetIPs.begin(),
        targetIPs.end(),
        arpReachables.begin(),
        [&](const IPAddressV4& ip) {
          return make_unique<WaitForArpEntryReachable>(sw, ip);
        });
    for (auto& ip : targetIPs) {
      sendArpReply(handle.get(), ip.str(), "02:10:20:30:40:22", portID);
    }
    for (auto& arpReachable : arpReachables) {
      EXPECT_TRUE(arpReachable->wait());
    }
  }

  std::atomic<bool> done{false};
  std::thread arpReplies([&handle, &targetIPs, &portID, &done]() {
    int index = 0;
    while (!done) {
      sendArpReply(
          handle.get(), targetIPs[index].str(), "02:10:20:30:40:22", portID);
      index = (index + 1) % targetIPs.size();
      usleep(1000);
    }
  });

  // flush all arp entries for 10 times
  for (uint32_t i = 0; i < 10; i++) {
    int numFlushed = 0;
    for (auto& ip : targetIPs) {
      numFlushed += thriftHandler.flushNeighborEntry(
          make_unique<BinaryAddress>(toBinaryAddress(ip)), vlanID);
    }
    XLOG(DBG) << "iter" << i << " flushed " << numFlushed << " entries";
  }
  // let TSAN/ASAN catch any racing issues
  done = true;
  arpReplies.join();
}

TEST(ArpTest, PortFlapRecover) {
  auto handle = setupTestHandle();
  auto sw = handle->getSw();

  // ensure port is up
  sw->linkStateChanged(PortID(1), true);

  VlanID vlanID(1);
  IPAddressV4 senderIP = IPAddressV4("10.0.0.1");
  std::vector<IPAddressV4> targetIP = {
      IPAddressV4("10.0.0.2"),
      IPAddressV4("10.0.0.3"),
      IPAddressV4("10.0.0.4")};
  std::vector<MacAddress> targetMAC = {
      MacAddress("02:10:20:30:40:22"),
      MacAddress("02:10:20:30:40:23"),
      MacAddress("02:10:20:30:40:24")};
  std::vector<unsigned int> targetPort = {1, 1, 2};

  // Cache the current stats
  CounterCache counters(sw);

  // have arp entries to reachable state
  for (auto tuple : boost::combine(targetIP, targetMAC, targetPort)) {
    auto ip = tuple.get<0>();
    auto mac = tuple.get<1>();
    auto port = tuple.get<2>();

    testSendArpRequest(sw, vlanID, senderIP, ip);

    WaitForArpEntryReachable arpReachable(sw, ip);
    sendArpReply(handle.get(), ip.str(), mac.toString(), port);
    waitForStateUpdates(sw);
    EXPECT_TRUE(arpReachable.wait());
  }

  counters.update();
  counters.checkDelta(SwitchStats::kCounterPrefix + "arp.request.tx.sum", 3);
  counters.checkDelta(SwitchStats::kCounterPrefix + "arp.request.rx.sum", 0);
  counters.checkDelta(SwitchStats::kCounterPrefix + "arp.reply.tx.sum", 0);
  counters.checkDelta(SwitchStats::kCounterPrefix + "arp.reply.rx.sum", 3);

  // flapping port 1, so prune port 2's IP, MAC, PortID and waiter
  auto unaffectedIP = targetIP.back();
  targetIP.pop_back();
  targetMAC.pop_back();
  targetPort.pop_back();

  // entries related to flapped port would go pending
  std::vector<std::unique_ptr<WaitForArpEntryPending>> arpPendings(2);
  std::transform(
      targetIP.begin(),
      targetIP.end(),
      arpPendings.begin(),
      [&](const IPAddressV4& ip) {
        return make_unique<WaitForArpEntryPending>(sw, ip);
      });

  // send a port down event to the switch for port 1
  EXPECT_STATE_UPDATE_TIMES_ATLEAST(sw, 1);
  sw->linkStateChanged(PortID(1), false);

  // purging neighbor entries occurs on the background EVB via NeighorUpdater as
  // a StateObserver.
  // block until NeighborUpdater::stateChanged() has been invoked
  waitForStateUpdates(sw);
  // block until neighbor purging logic has been executed on the background evb
  waitForBackgroundThread(sw);
  // block until updates to neighbor entries have been picked up by SwitchState
  waitForStateUpdates(sw);

  // both entries should now be pending
  for (auto& arpPending : arpPendings) {
    EXPECT_TRUE(arpPending->wait());
  }

  // ARP entry related to unflapped port must remain unaffected
  auto unaffectedEntry = getArpEntry(sw, unaffectedIP, vlanID);
  EXPECT_NE(unaffectedEntry, nullptr);
  EXPECT_EQ(unaffectedEntry->isPending(), false);

  // send a port up event to the switch for port 1
  EXPECT_STATE_UPDATE_TIMES_ATLEAST(sw, 1);
  sw->linkStateChanged(PortID(1), true);
  waitForStateUpdates(sw);

  // Receive arp replies for entries on flapped port, port 1
  for (auto tuple : boost::combine(targetIP, targetMAC, targetPort)) {
    auto ip = tuple.get<0>();
    auto mac = tuple.get<1>();
    auto port = tuple.get<2>();

    WaitForArpEntryReachable arpReachable(sw, ip);
    sendArpReply(handle.get(), ip.str(), mac.toString(), port);
    waitForStateUpdates(sw);
    EXPECT_TRUE(arpReachable.wait());
  }
  unaffectedEntry = getArpEntry(sw, unaffectedIP, vlanID);
  EXPECT_NE(unaffectedEntry, nullptr);
  EXPECT_EQ(unaffectedEntry->isPending(), false);
}

TEST(ArpTest, receivedPacketWithDirectlyConnectedDestination) {
  auto handle = setupTestHandle();
  auto sw = handle->getSw();
  VlanID vlanID(1);
  IPAddressV4 senderIP = IPAddressV4("10.0.0.1");
  IPAddressV4 targetIP = IPAddressV4("10.0.0.10");

  WaitForArpEntryExpiration arpExpiry(sw, targetIP);

  // Cache the current stats
  CounterCache counters(sw);

  // Create an IP pkt for 10.0.0.10
  auto buf = make_unique<IOBuf>(PktUtil::parseHexData(
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
      "0a 00 00 0a"));

  // Receiving this packet should trigger an ARP request out.
  EXPECT_SWITCHED_PKT(
      sw,
      "ARP request",
      checkArpRequest(
          senderIP, MacAddress("00:02:00:00:00:01"), targetIP, vlanID));
  // The state will be updated twice: once to create a pending ARP entry and
  // once to expire the ARP entry
  EXPECT_STATE_UPDATE_TIMES(sw, 2);

  handle->rxPacket(std::move(buf), PortID(1), vlanID);
  sw->getNeighborUpdater()->waitForPendingUpdates();

  waitForStateUpdates(sw);
  EXPECT_TRUE(arpExpiry.wait());

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
}

TEST(ArpTest, receivedPacketWithNoRouteToDestination) {
  auto handle = setupTestHandle();
  auto sw = handle->getSw();
  VlanID vlanID(1);

  // Cache the current stats
  CounterCache counters(sw);

  // Create an IP pkt for 10.0.10.10
  auto buf = make_unique<IOBuf>(PktUtil::parseHexData(
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
      "0a 00 0a 0a"));

  // Receiving this packet should not trigger a ARP request out,
  // because no interface is able to reach that subnet
  EXPECT_STATE_UPDATE_TIMES(sw, 0);
  EXPECT_HW_CALL(sw, sendPacketSwitchedAsync_(_)).Times(0);

  handle->rxPacket(std::move(buf), PortID(1), vlanID);
  sw->getNeighborUpdater()->waitForPendingUpdates();

  waitForStateUpdates(sw);

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
}

TEST(ArpTest, receivedPacketWithRouteToDestination) {
  auto handle = setupTestHandle();
  auto sw = handle->getSw();
  VlanID vlanID(1);
  IPAddressV4 senderIP = IPAddressV4("10.0.0.1");
  std::array<IPAddressV4, 2> nextHops = {
      IPAddressV4("10.0.0.22"), IPAddressV4("10.0.0.23")};

  // Cache the current stats
  CounterCache counters(sw);

  std::array<std::unique_ptr<WaitForArpEntryExpiration>, 2> arpExpirations;

  std::transform(
      nextHops.begin(),
      nextHops.end(),
      arpExpirations.begin(),
      [&](const IPAddressV4& ip) {
        return make_unique<WaitForArpEntryExpiration>(sw, ip);
      });

  // Create an IP pkt for 10.1.1.10, reachable through 10.0.0.22 and 10.0.0.23
  auto buf = make_unique<IOBuf>(PktUtil::parseHexData(
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
      "0a 01 01 0a"));

  // Receiving this packet should trigger an ARP request to 10.0.0.22 and
  // 10.0.0.23, which causes pending arp entries to be added to the state.
  EXPECT_STATE_UPDATE_TIMES_ATLEAST(sw, 2);
  for (auto nexthop : nextHops) {
    EXPECT_SWITCHED_PKT(
        sw,
        "ARP request",
        checkArpRequest(
            senderIP, MacAddress("00:02:00:00:00:01"), nexthop, vlanID));
  }

  handle->rxPacket(std::move(buf), PortID(1), vlanID);
  sw->getNeighborUpdater()->waitForPendingUpdates();

  waitForStateUpdates(sw);
  for (auto& arpExpiry : arpExpirations) {
    EXPECT_TRUE(arpExpiry->wait());
  }

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
