/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/HwCpuFb303Stats.h"
#include "fboss/agent/hw/StatsConstants.h"

#include <fb303/ServiceData.h>
#include <folly/logging/xlog.h>

#include <gtest/gtest.h>
using namespace facebook::fboss;
using namespace facebook::fb303;
using namespace std::chrono;

namespace {
HwCpuFb303Stats::QueueId2Name kQueue2Name = {
    {1, "high"},
    {2, "low"},
};

HwPortStats getInitedStats() {
  return {
      apache::thrift::FragileConstructor(),
      0, // inBytes
      0, // inUcastPackets
      0, // inMulticastPkts
      0, // inBroadcastPkts
      0, // inDiscards
      0, // inErrors
      0, // inPause
      0, // inIpv4HdrErrors
      0, // inIpv6HdrErrors
      0, // inDstNullDiscards
      0, // inDiscardsRaw
      0, // outBytes
      0, // outUnicastPkts
      0, // outMulticastPkts
      0, // outBroadcastPkts
      0, // outDiscards
      0, // outErrors
      0, // outPause
      0, // outCongestionDiscardPkts
      0, // wredDroppedPackets
      {{1, 0}, {2, 0}}, // queueOutDiscards
      {{1, 0}, {2, 0}}, // queueOutBytes
      0, // outEcnCounter
      {{1, 1}, {2, 1}}, // queueOutPackets
      {{1, 2}, {2, 2}}, // queueOutDiscardPackets
      {{0, 0}, {0, 0}}, // queueWatermarkBytes
      0, // fecCorrectableErrors
      0, // fecUncorrectableErrors
      0, // inPfcCtrl_
      0, // outPfcCtrl_
      {{0, 1}, {7, 1}}, // inPfc_
      {{0, 2}, {7, 2}}, // inPfcXon_
      {{0, 3}, {7, 3}}, // outPfc_
      {{1, 0}, {2, 0}}, // queueWredDroppedPackets
      {{1, 0}, {2, 0}}, // queueEcnMarkedPackets
      0, // timestamp
      "test", // portName
      {}, // macsec stats,
      0, // inLabelMissDiscards_
      {}, // queueWatermarkLevel
      0 // inCongestionDiscards
  };
}

void updateStats(HwCpuFb303Stats& cpuStats) {
  auto now = duration_cast<seconds>(system_clock::now().time_since_epoch());
  // To get last increment from monotonic counter we need to update it twice
  HwPortStats empty{};
  // Need to populate queue stats, since by default these
  // maps are empty
  *empty.queueOutDiscardPackets_() =
      *empty.queueOutPackets_() = {{1, 0}, {2, 0}};
  cpuStats.updateStats(empty, now);
  cpuStats.updateStats(getInitedStats(), now);
}

void verifyUpdatedStats(const HwCpuFb303Stats& cpuStats) {
  auto curValue{1};
  curValue = 1;
  for (auto counterName : HwCpuFb303Stats::kQueueStatKeys()) {
    for (const auto& queueIdAndName : kQueue2Name) {
      EXPECT_EQ(
          cpuStats.getCounterLastIncrement(HwCpuFb303Stats::statName(
              counterName, queueIdAndName.first, queueIdAndName.second)),
          curValue);
    }
    ++curValue;
  }
}
} // namespace

TEST(HwCpuFb303StatsTest, StatName) {
  for (auto statKey : HwCpuFb303Stats::kQueueStatKeys()) {
    EXPECT_EQ(
        HwCpuFb303Stats::statName(statKey, 1, "high"),
        folly::to<std::string>("cpu.queue1.cpuQueue-high.", statKey));
  }
}
TEST(HwCpuFb303StatsTest, StatsInit) {
  HwCpuFb303Stats stats(kQueue2Name);
  for (auto statKey : HwCpuFb303Stats::kQueueStatKeys()) {
    for (const auto& queueIdAndName : kQueue2Name) {
      EXPECT_TRUE(fbData->getStatMap()->contains(HwCpuFb303Stats::statName(
          statKey, queueIdAndName.first, queueIdAndName.second)));
    }
  }
}

TEST(HwCpuFb303StatsTest, StatsDeInit) {
  { HwCpuFb303Stats stats(kQueue2Name); }
  for (auto statKey : HwCpuFb303Stats::kQueueStatKeys()) {
    for (const auto& queueIdAndName : kQueue2Name) {
      EXPECT_FALSE(fbData->getStatMap()->contains(HwCpuFb303Stats::statName(
          statKey, queueIdAndName.first, queueIdAndName.second)));
    }
  }
}

TEST(HwCpuFb303Stats, UpdateStats) {
  HwCpuFb303Stats cpuStats(kQueue2Name);
  updateStats(cpuStats);
  verifyUpdatedStats(cpuStats);
}
TEST(HwCpuFb303StatsTest, RenameQueue) {
  HwCpuFb303Stats stats(kQueue2Name);
  stats.queueChanged(1, "very_high");
  auto newQueueMapping = kQueue2Name;
  for (auto statKey : HwCpuFb303Stats::kQueueStatKeys()) {
    EXPECT_TRUE(fbData->getStatMap()->contains(
        HwCpuFb303Stats::statName(statKey, 1, "very_high")));
    EXPECT_FALSE(fbData->getStatMap()->contains(
        HwCpuFb303Stats::statName(statKey, 1, "high")));
    // No impact on low
    EXPECT_TRUE(fbData->getStatMap()->contains(
        HwCpuFb303Stats::statName(statKey, 2, "low")));
  }
}

TEST(HwCpuFb303StatsTest, AddQueue) {
  HwCpuFb303Stats stats(kQueue2Name);
  stats.queueChanged(3, "very_high");
  auto newQueueMapping = kQueue2Name;
  for (auto statKey : HwCpuFb303Stats::kQueueStatKeys()) {
    EXPECT_TRUE(fbData->getStatMap()->contains(
        HwCpuFb303Stats::statName(statKey, 1, "high")));
    EXPECT_TRUE(fbData->getStatMap()->contains(
        HwCpuFb303Stats::statName(statKey, 2, "low")));
    EXPECT_TRUE(fbData->getStatMap()->contains(
        HwCpuFb303Stats::statName(statKey, 3, "very_high")));
  }
}
TEST(HwCpuFb303StatsTest, RemoveQueue) {
  HwCpuFb303Stats stats(kQueue2Name);
  stats.queueRemoved(1);
  auto newQueueMapping = kQueue2Name;
  for (auto statKey : HwCpuFb303Stats::kQueueStatKeys()) {
    EXPECT_FALSE(fbData->getStatMap()->contains(
        HwCpuFb303Stats::statName(statKey, 1, "high")));
    EXPECT_TRUE(fbData->getStatMap()->contains(
        HwCpuFb303Stats::statName(statKey, 2, "low")));
  }
}

TEST(HwCpuFb303Stats, queueNameChangeResetsValue) {
  HwCpuFb303Stats cpuStats(kQueue2Name);
  updateStats(cpuStats);
  cpuStats.queueChanged(1, "very_high");
  cpuStats.queueChanged(2, "very_low");
  HwCpuFb303Stats::QueueId2Name newQueues = {{1, "very_high"}, {2, "very_low"}};
  for (auto counterName : HwCpuFb303Stats::kQueueStatKeys()) {
    for (const auto& queueIdAndName : newQueues) {
      EXPECT_TRUE(fbData->getStatMap()->contains(HwCpuFb303Stats::statName(
          counterName, queueIdAndName.first, queueIdAndName.second)));
      EXPECT_EQ(
          cpuStats.getCounterLastIncrement(HwCpuFb303Stats::statName(
              counterName, queueIdAndName.first, queueIdAndName.second)),
          0);
    }
  }
  for (auto counterName : HwCpuFb303Stats::kQueueStatKeys()) {
    for (const auto& queueIdAndName : kQueue2Name) {
      EXPECT_FALSE(fbData->getStatMap()->contains(HwCpuFb303Stats::statName(
          counterName, queueIdAndName.first, queueIdAndName.second)));
    }
  }
}

TEST(HwCpuFb303Stats, getCpuStats) {
  HwCpuFb303Stats cpuStats(kQueue2Name);
  updateStats(cpuStats);

  const auto cpuPortStats = cpuStats.getCpuPortStats();
  EXPECT_EQ(cpuPortStats.queueToName_()->size(), 2);
  EXPECT_EQ(cpuPortStats.queueInPackets_()->size(), 2);
  EXPECT_EQ(cpuPortStats.queueDiscardPackets_()->size(), 2);

  for (const auto& queueIdAndName : *cpuPortStats.queueToName_()) {
    const auto& iter1 =
        cpuPortStats.queueDiscardPackets_()->find(queueIdAndName.first);
    EXPECT_TRUE(iter1 != cpuPortStats.queueDiscardPackets_()->end());
    EXPECT_EQ(iter1->second, 2);

    const auto& iter2 =
        cpuPortStats.queueInPackets_()->find(queueIdAndName.first);
    EXPECT_TRUE(iter2 != cpuPortStats.queueInPackets_()->end());
    EXPECT_EQ(iter2->second, 1);
  }
}
