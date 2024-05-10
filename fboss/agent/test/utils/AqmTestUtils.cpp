/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/test/utils/AqmTestUtils.h"
#include "fboss/agent/FbossError.h"
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/hw/switch_asics/HwAsic.h"
#include "fboss/agent/test/TestEnsembleIf.h"
#include "fboss/agent/test/utils/PortTestUtils.h"
#include "fboss/lib/CommonUtils.h"

#include <gtest/gtest.h>

namespace {
static constexpr auto kJerichoWordSize{16};
static constexpr auto kJerichoEcnThresholdIncrements{1024};
} // namespace

namespace facebook::fboss::utility {

int applyYubaFloatingPointRounding(int threshold) {
  uint64_t val = static_cast<uint64_t>(threshold);
  constexpr int mantissaLength = 5;
  constexpr int maxMantissa = 0b11111;
  constexpr int maxExponent = 0b1111;
  constexpr int u64BitLength = 64;

  // Edge Case
  if (val == 0) {
    return 0;
  }

  // length of a uint64_t - MSB of the value gives
  // the number of bits to represent the value.
  int bitLength = u64BitLength - __builtin_clzll(val);

  // Number of bits preceding the 5 most significant bits
  int exponent = std::max(bitLength - mantissaLength, 0);
  int mantissa = val >> exponent;

  // Check upper limit for exponent.
  if (exponent > maxExponent) {
    mantissa = maxMantissa;
    exponent = maxExponent;
  }

  // Threshold is a rounded value based on 9-bit representation.
  // Setting the lower bits to 1 will round up to max representable
  // value for the given 9-bits.
  int lower_bits = (1 << exponent) - 1;

  // Add one more buffer, since q_size must be pass threshold to mark/drop
  return ((mantissa << exponent) | lower_bits) + 1;
}

int getRoundedBufferThreshold(
    const HwAsic* asic,
    int expectedThreshold,
    bool roundUp) {
  int threshold{};
  if (cfg::AsicType::ASIC_TYPE_EBRO == asic->getAsicType()) {
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
  } else if (asic->getAsicType() == cfg::AsicType::ASIC_TYPE_YUBA) {
    threshold = (applyYubaFloatingPointRounding(expectedThreshold) /
                 asic->getPacketBufferUnitSize()) *
        asic->getPacketBufferUnitSize();
  } else if (asic->getAsicType() == cfg::AsicType::ASIC_TYPE_JERICHO2) {
    // Jericho2 ECN thresholds are in multiples of 1K
    threshold = kJerichoEcnThresholdIncrements *
        ceil(static_cast<double>(expectedThreshold) /
             kJerichoEcnThresholdIncrements);
  } else {
    // Thresholds are applied in multiples of unit buffer size
    auto bufferUnitSize = asic->getPacketBufferUnitSize();
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

size_t getEffectiveBytesPerPacket(const HwAsic* asic, int pktSizeBytes) {
  auto bufferDescriptorBytes = asic->getPacketBufferDescriptorSize();

  size_t effectiveBytesPerPkt;
  if (asic->getAsicType() == cfg::AsicType::ASIC_TYPE_JERICHO2) {
    // For Jericho2, the buffer unit size does not matter, each packet has
    // the overhead added to it and rounded to the next multiple of word size.
    effectiveBytesPerPkt = kJerichoWordSize *
        ((pktSizeBytes + bufferDescriptorBytes + kJerichoWordSize - 1) /
         kJerichoWordSize);
  } else {
    auto packetBufferUnitBytes = asic->getPacketBufferUnitSize();
    assert(packetBufferUnitBytes);
    effectiveBytesPerPkt = packetBufferUnitBytes *
        ((pktSizeBytes + bufferDescriptorBytes + packetBufferUnitBytes - 1) /
         packetBufferUnitBytes);
  }
  return effectiveBytesPerPkt;
}

double getAlphaFromScalingFactor(
    const HwAsic* asic,
    cfg::MMUScalingFactor scalingFactor) {
  int powof = asic->getBufferDynThreshFromScalingFactor(scalingFactor);
  return pow(2, powof);
}

HwPortStats sendPacketsWithQueueBuildup(
    std::function<void(PortID port, int numPacketsToSend)> sendPktsFn,
    TestEnsembleIf* ensemble,
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
  utility::setCreditWatchdogAndPortTx(ensemble, port, false);
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
  utility::setCreditWatchdogAndPortTx(ensemble, port, true);

  // Return the stats at the point when port stopped actual TX!
  return statsWithTxDisabled;
}

}; // namespace facebook::fboss::utility
