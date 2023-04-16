/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/test/dataplane_tests/HwTestAqmUtils.h"
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/hw/test/HwTestPortUtils.h"

namespace {
static constexpr auto kJerichoWordSize{16};
} // namespace

namespace facebook::fboss::utility {

int getRoundedBufferThreshold(
    HwSwitch* hwSwitch,
    int expectedThreshold,
    bool roundUp) {
  int threshold{};
  if (cfg::AsicType::ASIC_TYPE_EBRO ==
      hwSwitch->getPlatform()->getAsic()->getAsicType()) {
    /*
     * Ebro splits queue buffers into 16 blocks, watermarks and
     * ECN/WRED thresholds can only be reported / configured in
     * the order of these block thresholds as captured below.
     *
     * Doc: https://fburl.com/nil3f15m
     */
    const std::vector<int> kEbroQuantizedThresholds{
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
    auto it = std::lower_bound(
        kEbroQuantizedThresholds.begin(),
        kEbroQuantizedThresholds.end(),
        expectedThreshold);

    if (roundUp) {
      if (it == kEbroQuantizedThresholds.end()) {
        FbossError("Invalid threshold for ASIC, ", expectedThreshold);
      } else {
        threshold = *it;
      }
    } else {
      if (it != kEbroQuantizedThresholds.end() && *it == expectedThreshold) {
        threshold = *it;
      } else if (it != kEbroQuantizedThresholds.begin()) {
        threshold = *(std::prev(it));
      } else {
        FbossError("Invalid threshold for ASIC, ", expectedThreshold);
      }
    }
  } else if (
      hwSwitch->getPlatform()->getAsic()->getAsicType() ==
      cfg::AsicType::ASIC_TYPE_JERICHO2) {
    // Jericho2 tracks buffer usage in words and hence round up to the
    // next multiple of word size.
    threshold = kJerichoWordSize *
        ceil(static_cast<double>(expectedThreshold) / kJerichoWordSize);
  } else {
    // Thresholds are applied in multiples of unit buffer size
    auto bufferUnitSize =
        hwSwitch->getPlatform()->getAsic()->getPacketBufferUnitSize();
    /*
     * Round up:
     * For expected threshold of 300B with round up, we need 2 cells worth
     * bytes in case of TH3, to get the same, we do 300 / CELLSIZE to find
     * the number of CELLs needed to accommodate this size, which comes out
     * to be 1.18 (rounded up to 2) and then multiply it by CELLSIZE
     * to get the number of bytes for 2 cells, which will be 508B.
     *
     * Round down:
     * For expected threshold of 600B with round down, we need 2 cell worth
     * bytes in case of TH3 (no use case), to get the same, we do
     * 600 / CELLSIZE to find the number of CELLs needed to accommodate this
     * size, which comes out to be 2.36 (integer round down to 2) and then
     * multiply it by CELLSIZE to get the number of bytes for 2 cells, which
     * will be 508B.
     */
    threshold = roundUp ? bufferUnitSize *
            ceil(static_cast<double>(expectedThreshold) / bufferUnitSize)
                        : bufferUnitSize *
            floor(static_cast<double>(expectedThreshold) / bufferUnitSize);
  }

  return threshold;
}

size_t getEffectiveBytesPerPacket(HwSwitch* hwSwitch, int pktSizeBytes) {
  auto bufferDescriptorBytes =
      hwSwitch->getPlatform()->getAsic()->getPacketBufferDescriptorSize();

  size_t effectiveBytesPerPkt;
  if (hwSwitch->getPlatform()->getAsic()->getAsicType() ==
      cfg::AsicType::ASIC_TYPE_JERICHO2) {
    // For Jericho2, the buffer unit size does not matter, each packet has
    // the overhead added to it and rounded to the next multiple of word size.
    effectiveBytesPerPkt = kJerichoWordSize *
        ((pktSizeBytes + bufferDescriptorBytes + kJerichoWordSize - 1) /
         kJerichoWordSize);
  } else {
    auto packetBufferUnitBytes =
        hwSwitch->getPlatform()->getAsic()->getPacketBufferUnitSize();
    assert(packetBufferUnitBytes);
    effectiveBytesPerPkt = packetBufferUnitBytes *
        ((pktSizeBytes + bufferDescriptorBytes + packetBufferUnitBytes - 1) /
         packetBufferUnitBytes);
  }
  return effectiveBytesPerPkt;
}

HwPortStats sendPacketsWithQueueBuildup(
    std::function<void(PortID port, int numPacketsToSend)> sendPktsFn,
    HwSwitchEnsemble* ensemble,
    PortID port,
    int numPackets) {
  auto getOutPacketDelta = [](auto& after, auto& before) {
    return (
        (*after.outMulticastPkts_() + *after.outBroadcastPkts_() +
         *after.outUnicastPkts_()) -
        (*before.outMulticastPkts_() + *before.outBroadcastPkts_() +
         *before.outUnicastPkts_()));
  };
  /*
   * Packets are sent in 2 blocks, 80% first and 20% later. This is
   * to ensure that stats read will give the right numbers for
   * watermarks if thats is of interest, which means we need a minimum
   * of 5 packets to split as 4:1.
   */
  CHECK_GE(numPackets, 5) << "Number of packets to send should be >= 5";
  int eightyPercentPackets = (numPackets * 80) / 100;
  auto statsAtStart = ensemble->getLatestPortStats(port);
  // Disable TX to allow queue to build up
  utility::setPortTxEnable(ensemble->getHwSwitch(), port, false);
  sendPktsFn(port, eightyPercentPackets);

  auto waitForStatsIncrement = [&](const auto& newStats) {
    static auto prevStats = statsAtStart;
    auto portStatsIter = newStats.find(port);
    uint64_t netOut{0}, newOut{0};
    if (portStatsIter != newStats.end()) {
      netOut = getOutPacketDelta(portStatsIter->second, statsAtStart);
      newOut = getOutPacketDelta(portStatsIter->second, prevStats);
    }
    prevStats = portStatsIter->second;
    /*
     * Wait for stats to be different from statsAtStart, but remains
     * the same for atleast one iteration, to ensure stats stop
     * incrementing before we return
     */
    return netOut > 0 && newOut == 0;
  };

  constexpr auto kNumRetries{5};
  ensemble->waitPortStatsCondition(
      waitForStatsIncrement,
      kNumRetries,
      std::chrono::milliseconds(std::chrono::seconds(1)));
  auto statsWithTxDisabled = ensemble->getLatestPortStats(port);
  auto txedPackets = getOutPacketDelta(statsWithTxDisabled, statsAtStart);
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

  XLOG(DBG3) << "Number of packets TXed with TX disable set is " << txedPackets;
  /*
   * TXed packets dont contribute to queue build up, hence send
   * those many packets again to build the queue as expected, along
   * with the remaning 20% packets.
   */
  sendPktsFn(port, txedPackets + (numPackets - eightyPercentPackets));

  // Enable TX to send traffic out
  utility::setPortTxEnable(ensemble->getHwSwitch(), port, true);

  // Return the stats before numPackets were sent
  return statsWithTxDisabled;
}

double getAlphaFromScalingFactor(
    HwSwitch* hwSwitch,
    cfg::MMUScalingFactor scalingFactor) {
  int powof =
      hwSwitch->getPlatform()->getAsic()->getBufferDynThreshFromScalingFactor(
          scalingFactor);
  return pow(2, powof);
}

uint32_t getQueueLimitBytes(
    HwSwitch* hwSwitch,
    std::optional<cfg::MMUScalingFactor> queueScalingFactor) {
  uint32_t queueLimitBytes{};
  if (queueScalingFactor.has_value()) {
    // Dynamic queue limit for this platform!
    auto alpha =
        getAlphaFromScalingFactor(hwSwitch, queueScalingFactor.value());
    queueLimitBytes = static_cast<uint32_t>(
        getEgressSharedPoolLimitBytes(hwSwitch) * alpha / (1 + alpha));
  } else {
    queueLimitBytes =
        hwSwitch->getPlatform()->getAsic()->getStaticQueueLimitBytes();
  }
  return queueLimitBytes;
}

}; // namespace facebook::fboss::utility
