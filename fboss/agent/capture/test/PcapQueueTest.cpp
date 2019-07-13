/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/capture/PcapQueue.h"
#include "fboss/agent/capture/PcapPkt.h"
#include "fboss/agent/hw/mock/MockRxPacket.h"

#include <gtest/gtest.h>
#include <thread>

using namespace facebook::fboss;
using folly::ByteRange;

void pktWaitThread(PcapQueue* queue, std::vector<PcapPkt>* results) {
  std::vector<PcapPkt> pkts;
  while (true) {
    bool gotPkts = queue->wait(&pkts);
    if (!gotPkts) {
      return;
    }
    for (PcapPkt& pkt : pkts) {
      results->push_back(std::move(pkt));
    }
    pkts.clear();
  }
}

TEST(PcapQueueTest, SimpleAdd) {
  PcapQueue queue(100);
  std::vector<PcapPkt> waitedPkts;

  std::thread waiter([&]() { pktWaitThread(&queue, &waitedPkts); });

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

  queue.addPkt(pkt.get());
  queue.finish();
  waiter.join();

  ASSERT_EQ(1, waitedPkts.size());
  EXPECT_TRUE(waitedPkts[0].isRx());

  ByteRange expectedPktData = pkt->buf()->coalesce();
  auto waitedPktBufClone = waitedPkts[0].buf()->clone();
  ByteRange waitedPktData = waitedPktBufClone->coalesce();
  EXPECT_EQ(expectedPktData, waitedPktData);
}
