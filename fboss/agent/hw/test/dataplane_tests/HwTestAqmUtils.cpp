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
#include "fboss/agent/test/utils/AqmTestUtils.h"

namespace {
static constexpr auto kJerichoWordSize{16};
static constexpr auto kJerichoEcnThresholdIncrements{1024};
} // namespace

namespace facebook::fboss::utility {

int getRoundedBufferThreshold(
    HwSwitch* hwSwitch,
    int expectedThreshold,
    bool roundUp) {
  return getRoundedBufferThreshold(
      hwSwitch->getPlatform()->getAsic(), expectedThreshold, roundUp);
}

size_t getEffectiveBytesPerPacket(HwSwitch* hwSwitch, int pktSizeBytes) {
  return getEffectiveBytesPerPacket(
      hwSwitch->getPlatform()->getAsic(), pktSizeBytes);
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
  utility::setCreditWatchdogAndPortTx(ensemble->getHwSwitch(), port, false);
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
  utility::setCreditWatchdogAndPortTx(ensemble->getHwSwitch(), port, true);

  // Return the stats before numPackets were sent
  return statsWithTxDisabled;
}

double getAlphaFromScalingFactor(
    HwSwitch* hwSwitch,
    cfg::MMUScalingFactor scalingFactor) {
  return getAlphaFromScalingFactor(
      hwSwitch->getPlatform()->getAsic(), scalingFactor);
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
