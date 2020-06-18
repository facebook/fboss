/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/test/dataplane_tests/HwTestQosUtils.h"

#include "fboss/agent/hw/test/HwSwitchEnsemble.h"

#include <folly/logging/xlog.h>

#include <chrono>
#include <thread>

#include <gtest/gtest.h>

using namespace std::chrono_literals;

namespace facebook::fboss::utility {

bool verifyQueueMappings(
    const HwPortStats& portStatsBefore,
    const std::map<int, std::vector<uint8_t>>& q2dscps,
    HwSwitchEnsemble* ensemble,
    facebook::fboss::PortID egressPort) {
  auto retries = 10;
  bool statsMatch;
  do {
    auto portStatsAfter = ensemble->getLatestPortStats(egressPort);
    statsMatch = true;
    for (const auto& _q2dscps : q2dscps) {
      auto queuePacketsBefore =
          portStatsBefore.queueOutPackets__ref()->find(_q2dscps.first)->second;
      auto queuePacketsAfter =
          portStatsAfter.queueOutPackets__ref()[_q2dscps.first];
      // Note, on some platforms, due to how loopbacked packets are pruned
      // from being broadcast, they will appear more than once on a queue
      // counter, so we can only check that the counter went up, not that it
      // went up by exactly one.
      if (queuePacketsAfter < queuePacketsBefore + _q2dscps.second.size()) {
        statsMatch = false;
        break;
      }
    }
    if (!statsMatch) {
      std::this_thread::sleep_for(20ms);
    } else {
      break;
    }
    XLOG(INFO) << " Retrying ...";
  } while (--retries && !statsMatch);
  return statsMatch;
}

// Packets processed by WRR queues should be in proportion to their weights
void verifyWRRHelper(
    const std::map<int16_t, int64_t>& queueStats,
    int maxWeightQueueId,
    const std::map<int, uint8_t>& wrrQueueToWeight) {
  /*
   * The normalized stats (stats/weight) for every queue should be identical
   * (within certain percentage variance).
   *
   * We compare normalized stats for every queue against normalized stats for
   * queue with max weight (or better precesion).
   */
  const double kVariance = 0.05; // i.e. + or -5%

  auto maxWeightQueueBytes = queueStats.find(maxWeightQueueId)->second;
  auto maxWeightQueueWeight = wrrQueueToWeight.at(maxWeightQueueId);
  auto maxWeightQueueNormalizedBytes =
      maxWeightQueueBytes / maxWeightQueueWeight;
  auto lowMaxWeightQueueNormalizedBytes =
      maxWeightQueueNormalizedBytes * (1 - kVariance);
  auto highMaxWeightQueueNormalizedBytes =
      maxWeightQueueNormalizedBytes * (1 + kVariance);

  XLOG(DBG0) << "Max weight:: QueueId: " << maxWeightQueueId
             << " stat: " << maxWeightQueueBytes
             << " weight: " << static_cast<int>(maxWeightQueueWeight)
             << " normalized (stat/weight): " << maxWeightQueueNormalizedBytes
             << " low normalized: " << lowMaxWeightQueueNormalizedBytes
             << " high normalized: " << highMaxWeightQueueNormalizedBytes;

  for (const auto& queueStat : queueStats) {
    auto currQueueId = queueStat.first;
    auto currQueueBytes = queueStat.second;
    auto currQueueWeight = wrrQueueToWeight.at(currQueueId);
    auto currQueueNormalizedBytes = currQueueBytes / currQueueWeight;

    XLOG(DBG0) << "Curr queue :: QueueId: " << currQueueId
               << " stat: " << currQueueBytes
               << " weight: " << static_cast<int>(currQueueWeight)
               << " normalized (stat/weight): " << currQueueNormalizedBytes;

    EXPECT_TRUE(
        lowMaxWeightQueueNormalizedBytes < currQueueNormalizedBytes &&
        currQueueNormalizedBytes < highMaxWeightQueueNormalizedBytes);
  }
}

// Only trafficQueueId should have traffic
void verifySPHelper(
    const std::map<int16_t, int64_t>& queueStats,
    int trafficQueueId) {
  XLOG(DBG0) << "trafficQueueId: " << trafficQueueId;
  for (const auto& queueStat : queueStats) {
    auto queueId = queueStat.first;
    auto statVal = queueStat.second;
    XLOG(DBG0) << "QueueId: " << queueId << " stats: " << statVal;
    EXPECT_TRUE(queueId != trafficQueueId ? statVal == 0 : statVal != 0);
  }
}
} // namespace facebook::fboss::utility
