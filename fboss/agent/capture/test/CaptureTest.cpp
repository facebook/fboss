/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/gen-cpp/switch_config_types.h"
#include "fboss/agent/ApplyThriftConfig.h"
#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/capture/PktCapture.h"
#include "fboss/agent/capture/PktCaptureManager.h"
#include "fboss/agent/capture/test/PcapUtil.h"
#include "fboss/agent/hw/mock/MockRxPacket.h"
#include "fboss/agent/packet/PktUtil.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/state/Interface.h"
#include "fboss/agent/test/TestUtils.h"

#include <folly/Memory.h>
#include <gtest/gtest.h>
#include <stdlib.h>

using namespace facebook::fboss;
using folly::make_unique;
using folly::StringPiece;
using std::make_shared;
using std::shared_ptr;
using std::string;
using std::unique_ptr;
using ::testing::_;
using ::testing::AtLeast;

namespace {


const MacAddress kPlatformMac("02:01:02:03:04:05");

unique_ptr<SwSwitch> setupSwitch() {
  // Setup the initial state object
  cfg::SwitchConfig thriftCfg;

  // Add VLAN 1, and ports 1-39 which belong to it.
  cfg::Vlan thriftVlan;
  thriftVlan.name = "Vlan1";
  thriftVlan.id = 1;
  thriftVlan.intfID = 1;
  thriftVlan.routable = true;
  thriftVlan.ipAddresses = {"10.0.0.1"};
  thriftVlan.dhcpRelayAddressV4 = "10.1.2.3";
  thriftCfg.vlans.push_back(thriftVlan);

  cfg::Interface thriftIface;
  thriftIface.intfID = 1;
  thriftIface.vlanID = 1;
  thriftIface.ipAddresses.push_back("10.0.0.1/24");
  thriftIface.mac = "02:01:02:03:04:05";
  thriftIface.__isset.mac = "02:01:02:03:04:05";
  thriftCfg.interfaces.push_back(thriftIface);

  for (int idx = 1; idx < 40; ++idx) {
    cfg::Port thriftPort;
    thriftPort.logicalID = idx;
    thriftPort.state = cfg::PortState::UP;
    thriftPort.parserType = cfg::ParserType::L3;
    thriftPort.routable = true;
    thriftPort.ingressVlan = 1;
    thriftCfg.ports.push_back(thriftPort);

    cfg::VlanPort thriftVlanPort;
    thriftVlanPort.vlanID = 1;
    thriftVlanPort.logicalPort = idx;
    thriftVlanPort.spanningTreeState = cfg::SpanningTreeState::FORWARDING;
    thriftVlanPort.emitTags = false;
    thriftCfg.vlanPorts.push_back(thriftVlanPort);
  }

  auto sw = createMockSw(&thriftCfg, kPlatformMac);
  sw->initialConfigApplied(std::chrono::steady_clock::now());

  return sw;
}

} // unnamed namespace

TEST(CaptureTest, FullCapture) {
  auto sw = setupSwitch();

  // Start a packet capture
  auto* mgr = sw->getCaptureMgr();
  auto capture = make_unique<PktCapture>("test", 100);
  mgr->startCapture(std::move(capture));

  // Create an IP packet for 10.0.0.10
  auto ipPktData = PktUtil::parseHexData(
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
    // Padding to 68 bytes
    "00 00 00 00 00 00 00 00 00 00 00 00 00 00 00"
    "00 00 00 00 00 00 00 00 00 00 00 00 00 00 00"
  );
  MockRxPacket ipPkt(ipPktData.clone());
  ipPkt.setSrcPort(PortID(1));
  ipPkt.setSrcVlan(VlanID(1));

  // Create an arp reply packet from 10.0.0.10
  auto arpPktData = PktUtil::parseHexData(
    // dst mac, src mac
    "02 01 02 03 04 05  02 05 00 00 01 02"
    // 802.1q, VLAN 1
    "81 00  00 01"
    // ARP, htype: ethernet, ptype: IPv4, hlen: 6, plen: 4
    "08 06  00 01  08 00  06  04"
    // ARP Reply
    "00 02"
    // Sender MAC
    "02 05 00 00 01 02"
    // Sender IP: 10.0.0.10
    "0a 00 00 0a"
    // Target MAC
    "02 01 02 03 04 05"
    // Target IP: 10.0.0.1
    "0a 00 00 01"
    // Padding to 68 bytes
    "00 00 00 00 00 00 00 00"
    "00 00 00 00 00 00 00 00"
    "00 00 00 00 00 00"
  );
  MockRxPacket arpPkt(arpPktData.clone());
  arpPkt.setSrcPort(PortID(3));
  arpPkt.setSrcVlan(VlanID(1));

  // The arp request we expect the switch to generate
  auto arpReqData = PktUtil::parseHexData(
    // dst mac, src mac
    "ff ff ff ff ff ff 02 01 02 03 04 05"
    // 802.1q, VLAN 1
    "81 00  00 01"
    // ARP, htype: ethernet, ptype: IPv4, hlen: 6, plen: 4
    "08 06  00 01  08 00  06  04"
    // ARP Request
    "00 01"
    // Sender MAC
    "02 01 02 03 04 05"
    // Sender IP: 10.0.0.1
    "0a 00 00 01"
    // Target MAC
    "00 00 00 00 00 00"
    // Target IP: 10.0.0.10
    "0a 00 00 0a"
    // Padding to 68 bytes
    "00 00 00 00 00 00 00 00"
    "00 00 00 00 00 00 00 00"
    "00 00 00 00 00 00"
  );
  // The updated IP request with the updated src and destination mac, and
  // decremented TTL.
  auto updatedIpPktData = PktUtil::parseHexData(
    // dst mac, src mac
    "02 05 00 00 01 02  02 01 02 03 04 05"
    // 802.1q, VLAN 1
    "81 00 00 01"
    // IPv4
    "08 00"
    // Version(4), IHL(5), DSCP(0), ECN(0), Total Length(20)
    "45  00  00 14"
    // Identification(0), Flags(0), Fragment offset(0)
    "00 00  00 00"
    // TTL(30), Protocol(6), Checksum (0, fake)
    "1E  06  00 00"
    // Source IP (1.2.3.4)
    "01 02 03 04"
    // Destination IP (10.0.0.10)
    "0a 00 00 0a"
    // Padding to 68 bytes
    "00 00 00 00 00 00 00 00 00 00 00 00 00 00 00"
    "00 00 00 00 00 00 00 00 00 00 00 00 00 00 00"
  );

  // Receive a packet for which we don't have an ARP entry
  // This should trigger the switch to send an ARP request
  // and set a pending entry.
  EXPECT_HW_CALL(sw, sendPacketSwitched_(_)).Times(1);
  EXPECT_HW_CALL(sw, stateChanged(_)).Times(1);
  sw->packetReceived(ipPkt.clone());
  waitForStateUpdates(sw.get());

  // Receive an ARP reply for the desired IP. This should cause the
  // arp entry to change from pending to active
  EXPECT_HW_CALL(sw, stateChanged(_)).Times(1);
  sw->packetReceived(arpPkt.clone());
  waitForStateUpdates(sw.get());

  // Re-send the original packet now that the ARP table is populated.
  sw->packetReceived(ipPkt.clone());

  // Stop the packet capture
  mgr->stopCapture("test");

  // Check the packet capture contents
  string captureDir = sw->getCaptureMgr()->getCaptureDir();
  string pcapPath = folly::to<string>(captureDir, "/test.pcap");
  auto pcapPkts = readPcapFile(pcapPath.c_str());

  // The capture should contain 4 packets
  EXPECT_EQ(4, pcapPkts.size());

  // The first packet should be the initial IP packet
  EXPECT_BUF_EQ(ipPktData, pcapPkts.at(0).data);

  // The second packet should be an ARP request from the switch
  EXPECT_BUF_EQ(arpReqData, pcapPkts.at(1).data);

  // The third packet should be the ARP reply
  EXPECT_BUF_EQ(arpPktData, pcapPkts.at(2).data);

  // The fourth packet should be the re-sent IP packet
  EXPECT_BUF_EQ(ipPktData, pcapPkts.at(3).data);

  // Ideally the fifth packet should be the forwarded IP packet,
  // with an updated destination MAC, source MAC, and TTL.
  //
  // However, for now SwSwitch doesn't yet forward IP packets in software.
  // It assumes all forwarding is done in hardware, and if we ever get a packet
  // trapped to the CPU then it needs ARP resolution.  Currently it just drops
  // the packet if it receives an IP packet where we have already programmed an
  // ARP entry in hardware.
  //
  // EXPECT_BUF_EQ(updatedIpPktData, pcapPkts.at(4).data);
}
