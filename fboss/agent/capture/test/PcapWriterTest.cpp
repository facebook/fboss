/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/capture/PcapWriter.h"
#include "fboss/agent/capture/test/PcapUtil.h"
#include "fboss/agent/hw/mock/MockRxPacket.h"

#include <folly/Exception.h>
#include <folly/ScopeGuard.h>
#include <gtest/gtest.h>

using namespace facebook::fboss;

void addPackets(PcapWriter* writer, uint32_t count) {
  // Create a packet to add to the queue
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
      "0a 00 00 0a");
  pkt->padToLength(68);
  pkt->setSrcPort(PortID(1));
  pkt->setSrcVlan(VlanID(1));

  // For now, just add the same packet N times
  for (uint32_t n = 0; n < count; ++n) {
    writer->addPkt(pkt.get());
  }
}

TEST(PcapWriterTest, Write) {
  char tmpPath[] = "fbossPcapTest.XXXXXX";
  int tmpFD = mkstemp(tmpPath);
  folly::checkUnixError(tmpFD, "failed to create temporary file");
  SCOPE_EXIT {
    close(tmpFD);
    unlink(tmpPath);
  };

  PcapWriter writer(tmpPath, true);
  uint32_t numPkts = 1000;
  addPackets(&writer, numPkts);
  writer.finish();
  // We add 1000 packets, and the default capacity is larger than that,
  // so there shouldn't be any dropped packets
  EXPECT_EQ(0, writer.numDropped());

  auto pcapPkts = readPcapFile(tmpPath);
  EXPECT_EQ(numPkts, pcapPkts.size());
  for (const auto& pktInfo : pcapPkts) {
    EXPECT_EQ(68, pktInfo.hdr.len);
    EXPECT_EQ(68, pktInfo.hdr.caplen);
  }
}

TEST(PcapWriterTest, Drop) {
  char tmpPath[] = "fbossPcapTest.XXXXXX";
  int tmpFD = mkstemp(tmpPath);
  folly::checkUnixError(tmpFD, "failed to create temporary file");
  SCOPE_EXIT {
    close(tmpFD);
    unlink(tmpPath);
  };

  // Only allow 2 packets in the queue at any point in time
  PcapWriter writer(tmpPath, true, 2);
  addPackets(&writer, 100);
  usleep(100);
  addPackets(&writer, 100);
  usleep(100);
  addPackets(&writer, 100);
  writer.finish();

  // There should be quite a few dropped packets.  Even if the disk is very
  // fast, the other thread still won't wake up fast enough to take all of our
  // packets.
  EXPECT_GT(writer.numDropped(), 0);

  auto pcapPkts = readPcapFile(tmpPath);
  EXPECT_EQ(300, pcapPkts.size() + writer.numDropped());
  for (const auto& pktInfo : pcapPkts) {
    EXPECT_EQ(68, pktInfo.hdr.len);
    EXPECT_EQ(68, pktInfo.hdr.caplen);
  }
}
