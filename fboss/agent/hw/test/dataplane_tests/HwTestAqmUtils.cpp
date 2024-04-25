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
#include <gtest/gtest.h>
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/hw/test/HwTestPortUtils.h"
#include "fboss/agent/test/utils/AqmTestUtils.h"
#include "fboss/lib/CommonUtils.h"

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
  auto statsAtStart = ensemble->getLatestPortStats(port);
  auto prevStats = statsAtStart;
  // Disable TX to allow queue to build up
  utility::setCreditWatchdogAndPortTx(ensemble->getHwSwitch(), port, false);
  // Start by sending half the number of packets. Note that though TX is
  // disabled, some packets will leak through due to some credits being
  // available and the leak is different for different ASICs.
  sendPktsFn(port, numPackets / 2);
  int totalPacketsSent = numPackets / 2;

  auto getStatsIncrement = [&]() {
    auto newStats = ensemble->getLatestPortStats(port);
    uint64_t netOut = getOutPacketDelta(newStats, statsAtStart);
    uint64_t newOut = getOutPacketDelta(newStats, prevStats);
    prevStats = newStats;
    return std::pair(netOut, newOut);
  };

  std::pair<uint64_t, uint64_t> statsInfo;
  WITH_RETRIES_N_TIMED(10, std::chrono::milliseconds(1000), {
    statsInfo = getStatsIncrement();
    if (statsInfo.first == totalPacketsSent) {
      // TX is not disabled yet, needs more packet TX to get to the point
      // of running out of initial credits before TX is stopped!
      sendPktsFn(port, numPackets / 2);
      totalPacketsSent += numPackets / 2;
    }
    EXPECT_EVENTUALLY_TRUE(statsInfo.first < totalPacketsSent);
    // TX disable is in effect once we see that no more packets are
    // getting transmitted.
    EXPECT_EVENTUALLY_EQ(statsInfo.second, 0);
  });
  const auto statsWithTxDisabled = prevStats;

  XLOG(DBG3) << "Number of packets TXed with TX disable set is "
             << statsInfo.first;
  // TXed packets dont contribute to queue build up. Resend the number of
  // packets that got transmitted to ensure numPackets are in the queue.
  auto queuedPackets = totalPacketsSent - statsInfo.first;
  if (queuedPackets < numPackets) {
    sendPktsFn(port, numPackets - queuedPackets);
  }

  // Enable TX to send traffic out
  utility::setCreditWatchdogAndPortTx(ensemble->getHwSwitch(), port, true);

  // Return the stats at the point when port stopped actual TX!
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
