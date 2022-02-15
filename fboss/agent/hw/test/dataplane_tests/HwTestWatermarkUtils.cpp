/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/test/dataplane_tests/HwTestWatermarkUtils.h"
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/hw/test/HwTestPortUtils.h"

namespace facebook::fboss::utility {

int getRoundedBufferThreshold(HwSwitch* hwSwitch, int expThreshold) {
  int threshold{};
  if (HwAsic::AsicType::ASIC_TYPE_TAJO ==
      hwSwitch->getPlatform()->getAsic()->getAsicType()) {
    /*
     * TAJO splits queue buffers into 16 blocks, watermarks and
     * ECN/WRED thresholds can only be reported / configured in
     * the order of these block thresholds as captured below.
     *
     * Doc: https://fburl.com/nil3f15m
     */
    const std::vector<int> kTajoQuantizedThresholds{
        (50 * 384),
        (256 * 384),
        (512 * 384),
        (1024 * 384),
        (2 * 1024 * 384),
        (3 * 1024 * 384),
        (6 * 1024 * 384),
        (7 * 1024 * 384),
        (8 * 1024 * 384),
        (9 * 1024 * 384),
        (10 * 1024 * 384),
        (11 * 1024 * 384),
        (12 * 1024 * 384),
        (15 * 1024 * 384),
        (16000 * 384)};
    auto it = std::upper_bound(
        kTajoQuantizedThresholds.begin(),
        kTajoQuantizedThresholds.end(),
        expThreshold);
    if (it == kTajoQuantizedThresholds.end()) {
      FbossError("Invalid threshold for ASIC, ", expThreshold);
    }
    threshold = *it;
  } else {
    // Thresholds are applied in multiples of unit buffer size
    auto bufferUnitSize =
        hwSwitch->getPlatform()->getAsic()->getPacketBufferUnitSize();
    threshold =
        bufferUnitSize * ((expThreshold + bufferUnitSize - 1) / bufferUnitSize);
  }

  return threshold;
}

size_t getEffectiveBytesPerPacket(HwSwitch* hwSwitch, int pktSizeBytes) {
  auto packetBufferUnitBytes =
      hwSwitch->getPlatform()->getAsic()->getPacketBufferUnitSize();
  auto bufferDescriptorBytes =
      hwSwitch->getPlatform()->getAsic()->getPacketBufferDescriptorSize();

  assert(packetBufferUnitBytes);
  size_t effectiveBytesPerPkt = packetBufferUnitBytes *
      ((pktSizeBytes + bufferDescriptorBytes + packetBufferUnitBytes - 1) /
       packetBufferUnitBytes);
  return effectiveBytesPerPkt;
}

void sendPacketsWithQueueBuildup(
    std::function<void(PortID port, int numPacketsToSend)> sendPktsFn,
    HwSwitchEnsemble* ensemble,
    PortID port,
    int numPackets) {
  auto getOutPacketDelta = [](auto& after, auto& before) {
    return (
        (*after.outMulticastPkts__ref() + *after.outBroadcastPkts__ref() +
         *after.outUnicastPkts__ref()) -
        (*before.outMulticastPkts__ref() + *before.outBroadcastPkts__ref() +
         *before.outUnicastPkts__ref()));
  };
  /*
   * Packets are sent in 2 blocks, 80% first and 20% later. This is
   * to ensure that stats read will give the right numbers for
   * watermarks if thats is of interest, which means we need a minimum
   * of 5 packets to split as 4:1.
   */
  CHECK_GE(numPackets, 5) << "Number of packets to send should be >= 5";
  int eightyPercentPackets = (numPackets * 80) / 100;
  auto stats0 = ensemble->getLatestPortStats(port);
  // Disable TX to allow queue to build up
  utility::setPortTxEnable(ensemble->getHwSwitch(), port, false);
  sendPktsFn(port, eightyPercentPackets);

  auto stats1 = ensemble->getLatestPortStats(port);
  auto txedPackets = getOutPacketDelta(stats1, stats0);
  /*
   * The expectation is that 80% of packets is sufficiently
   * large to have some packets TXed out and some remain in
   * the queue. If all of these 80% packets are being sent
   * out, more packets are needed to ensure we have some
   * packets left in the queue!
   */
  CHECK_NE(txedPackets, eightyPercentPackets)
      << "Need to send more than " << numPackets
      << " packets to trigger queue build up";

  /*
   * TXed packets dont contribute to queue build up, hence send
   * those many packets again to build the queue as expected, along
   * with the remaning 20% packets.
   */
  sendPktsFn(port, txedPackets + (numPackets - eightyPercentPackets));

  // Enable TX to send traffic out
  utility::setPortTxEnable(ensemble->getHwSwitch(), port, true);
}

}; // namespace facebook::fboss::utility
