/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/ApplyThriftConfig.h"
#include "fboss/agent/NeighborUpdater.h"
#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/capture/PktCapture.h"
#include "fboss/agent/capture/PktCaptureManager.h"
#include "fboss/agent/capture/test/PcapUtil.h"
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/hw/mock/MockRxPacket.h"
#include "fboss/agent/packet/PktUtil.h"
#include "fboss/agent/state/Interface.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/test/HwTestHandle.h"
#include "fboss/agent/test/TestUtils.h"

#include <folly/Memory.h>
#include <gtest/gtest.h>

using namespace facebook::fboss;
using folly::StringPiece;
using std::make_shared;
using std::make_unique;
using std::shared_ptr;
using std::string;
using std::unique_ptr;
using ::testing::_;
using ::testing::AtLeast;

namespace {

unique_ptr<HwTestHandle> setupTestHandle() {
  // Setup the initial state object
  cfg::SwitchConfig thriftCfg;
  thriftCfg.switchSettings()->switchIdToSwitchInfo() = {
      std::make_pair(0, createSwitchInfo(cfg::SwitchType::NPU))};

  // Add VLAN 1, and ports 1-39 which belong to it.
  cfg::Vlan thriftVlan;
  *thriftVlan.name() = "Vlan1";
  *thriftVlan.id() = 1;
  thriftVlan.intfID() = 1;
  *thriftVlan.routable() = true;
  *thriftVlan.ipAddresses() = {"10.0.0.1"};
  thriftVlan.dhcpRelayAddressV4() = "10.1.2.3";
  thriftCfg.vlans()->push_back(thriftVlan);

  cfg::Interface thriftIface;
  *thriftIface.intfID() = 1;
  *thriftIface.vlanID() = 1;
  thriftIface.ipAddresses()->push_back("10.0.0.1/24");
  thriftIface.mac() = "02:01:02:03:04:05";
  thriftCfg.interfaces()->push_back(thriftIface);

  for (int idx = 1; idx < 10; ++idx) {
    cfg::Port thriftPort;
    preparedMockPortConfig(thriftPort, idx);
    *thriftPort.parserType() = cfg::ParserType::L3;
    *thriftPort.routable() = true;
    *thriftPort.ingressVlan() = 1;
    thriftCfg.ports()->push_back(thriftPort);

    cfg::VlanPort thriftVlanPort;
    *thriftVlanPort.vlanID() = 1;
    *thriftVlanPort.logicalPort() = idx;
    *thriftVlanPort.spanningTreeState() = cfg::SpanningTreeState::FORWARDING;
    *thriftVlanPort.emitTags() = false;
    thriftCfg.vlanPorts()->push_back(thriftVlanPort);
  }

  auto handle = createTestHandle(&thriftCfg);
  handle->getSw()->initialConfigApplied(std::chrono::steady_clock::now());

  return handle;
}

} // unnamed namespace

TEST(CaptureTest, FullCapture) {
  auto handle = setupTestHandle();
  auto sw = handle->getSw();

  // Start a packet capture
  auto* mgr = sw->getCaptureMgr();
  auto capture =
      make_unique<PktCapture>("test", 100, CaptureDirection::CAPTURE_TX_RX);
  auto rx_capture =
      make_unique<PktCapture>("rx", 100, CaptureDirection::CAPTURE_ONLY_RX);
  auto tx_capture =
      make_unique<PktCapture>("tx", 100, CaptureDirection::CAPTURE_ONLY_TX);

  // start captures
  mgr->startCapture(std::move(capture));
  mgr->startCapture(std::move(rx_capture));
  mgr->startCapture(std::move(tx_capture));

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
      "00 00 00 00 00 00 00 00 00 00 00 00 00 00 00");
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
      "00 00 00 00 00 00");
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
      "00 00 00 00 00 00");
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
      "00 00 00 00 00 00 00 00 00 00 00 00 00 00 00");

  // Receive a packet for which we don't have an ARP entry
  // This should trigger the switch to send an ARP request
  // and set a pending entry.
  EXPECT_HW_CALL(sw, sendPacketSwitchedAsync_(_)).Times(1);
  EXPECT_STATE_UPDATE_TIMES(sw, 1);
  sw->packetReceived(ipPkt.clone());
  sw->getNeighborUpdater()->waitForPendingUpdates();
  waitForStateUpdates(sw);

  // Receive an ARP reply for the desired IP. This should cause the
  // arp entry to change from pending to active. That in turn would
  // trigger a static l2 entry add update
  EXPECT_STATE_UPDATE_TIMES(sw, 2);
  sw->packetReceived(arpPkt.clone());
  sw->getNeighborUpdater()->waitForPendingUpdates();
  waitForStateUpdates(sw);

  // Re-send the original packet now that the ARP table is populated.
  sw->packetReceived(ipPkt.clone());

  EXPECT_EQ(mgr->getCaptureCount("test"), 4);
  EXPECT_EQ(mgr->getCaptureCount("rx"), 3);
  EXPECT_EQ(mgr->getCaptureCount("tx"), 1);

  // Stop the packet captures
  mgr->stopCapture("test");
  mgr->stopCapture("rx");
  mgr->stopCapture("tx");

  // Check the packet capture contents
  string captureDir = sw->getCaptureMgr()->getCaptureDir();
  string pcapPath = folly::to<string>(captureDir, "/test.pcap");
  string rxPcapPath = folly::to<string>(captureDir, "/rx.pcap");
  string txPcapPath = folly::to<string>(captureDir, "/tx.pcap");
  auto pcapPkts = readPcapFile(pcapPath.c_str());
  auto rxPcapPkts = readPcapFile(rxPcapPath.c_str());
  auto txPcapPkts = readPcapFile(txPcapPath.c_str());

  // The capture should contain 4 packets, 3 Rx, 1 Tx
  EXPECT_EQ(4, pcapPkts.size());
  EXPECT_EQ(3, rxPcapPkts.size());
  EXPECT_EQ(1, txPcapPkts.size());

  // The first packet should be the initial IP packet.
  // Also the 1st Rx.
  EXPECT_BUF_EQ(ipPktData, pcapPkts.at(0).data);
  EXPECT_BUF_EQ(ipPktData, rxPcapPkts.at(0).data);

  // The second packet should be an ARP request from the switch.
  // Also the 1st Tx.
  EXPECT_BUF_EQ(arpReqData, pcapPkts.at(1).data);
  EXPECT_BUF_EQ(arpReqData, txPcapPkts.at(0).data);

  // The third packet should be the ARP reply.
  // Also the 2nd Rx.
  EXPECT_BUF_EQ(arpPktData, pcapPkts.at(2).data);
  EXPECT_BUF_EQ(arpPktData, rxPcapPkts.at(1).data);

  // The fourth packet should be the re-sent IP packet.
  // Also the 3rd Rx.
  EXPECT_BUF_EQ(ipPktData, pcapPkts.at(3).data);
  EXPECT_BUF_EQ(ipPktData, rxPcapPkts.at(2).data);

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
