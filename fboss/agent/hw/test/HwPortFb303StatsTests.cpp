/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/HwPortFb303Stats.h"
#include "fboss/agent/hw/StatsConstants.h"

#include <fb303/ServiceData.h>
#include <folly/logging/xlog.h>

#include <gtest/gtest.h>
using namespace facebook::fboss;
using namespace facebook::fb303;
using namespace std::chrono;

namespace {
constexpr auto kPortName = "eth1/1/1";
HwPortFb303Stats::QueueId2Name kQueue2Name = {
    {1, "gold"},
    {2, "silver"},
};

HwPortStats getInitedStats() {
  return {
      apache::thrift::FragileConstructor(),
      1, // inBytes
      2, // inUcastPackets
      3, // inMulticastPkts
      4, // inBroadcastPkts
      5, // inDiscards
      6, // inErrors
      7, // inPause
      8, // inIpv4HdrErrors
      9, // inIpv6HdrErrors
      10, // inDstNullDiscards
      11, // inDiscardsRaw
      12, // outBytes
      13, // outUnicastPkts
      14, // outMulticastPkts
      15, // outBroadcastPkts
      16, // outDiscards
      17, // outErrors
      18, // outPause
      19, // outCongestionDiscardPkts
      {{1, 1}, {2, 1}}, // queueOutDiscards
      {{1, 2}, {2, 2}}, // queueOutBytes
      20, // outEcnCounter
      {{1, 3}, {2, 3}}, // queueOutPackets
  };
}

void updateStats(HwPortFb303Stats& portStats) {
  auto now = duration_cast<seconds>(system_clock::now().time_since_epoch());
  // To get last increment from monotonic counter we need to update it twice
  HwPortStats empty{};
  // Need to populate queue stats, since by default these
  // maps are empty
  empty.queueOutDiscardBytes_ = empty.queueOutBytes_ =
      empty.queueOutPackets_ = {{1, 0}, {2, 0}};
  portStats.updateStats(empty, now);
  portStats.updateStats(getInitedStats(), now);
}

void verifyUpdatedStats(const HwPortFb303Stats& portStats) {
  auto curValue{1};
  for (auto counterName : HwPortFb303Stats::kPortStatKeys()) {
    // +1 because first initialization is to -1
    EXPECT_EQ(
        portStats.getCounterLastIncrement(
            HwPortFb303Stats::statName(counterName, kPortName)),
        curValue++ + 1);
  }
  curValue = 1;
  for (auto counterName : HwPortFb303Stats::kQueueStatKeys()) {
    for (const auto& queueIdAndName : kQueue2Name) {
      EXPECT_EQ(
          portStats.getCounterLastIncrement(HwPortFb303Stats::statName(
              counterName,
              kPortName,
              queueIdAndName.first,
              queueIdAndName.second)),
          curValue);
    }
    ++curValue;
  }
}
} // namespace

TEST(HwPortFb303StatsTest, StatName) {
  EXPECT_EQ(
      HwPortFb303Stats::statName(kOutBytes(), kPortName),
      folly::to<std::string>(kPortName, '.', kOutBytes()));
  EXPECT_EQ(
      HwPortFb303Stats::statName(kOutBytes(), kPortName, 1, "gold"),
      folly::to<std::string>(kPortName, '.', "queue1.gold.", kOutBytes()));
}

TEST(HwPortFb303StatsTest, StatsInit) {
  HwPortFb303Stats stats(kPortName, kQueue2Name);
  for (auto statKey : HwPortFb303Stats::kPortStatKeys()) {
    EXPECT_TRUE(fbData->getStatMap()->contains(
        HwPortFb303Stats::statName(statKey, kPortName)));
  }
  for (auto statKey : HwPortFb303Stats::kQueueStatKeys()) {
    for (const auto& queueIdAndName : kQueue2Name) {
      EXPECT_TRUE(fbData->getStatMap()->contains(HwPortFb303Stats::statName(
          statKey, kPortName, queueIdAndName.first, queueIdAndName.second)));
    }
  }
}

TEST(HwPortFb303StatsTest, StatsDeInit) {
  { HwPortFb303Stats stats(kPortName); }
  for (auto statKey : HwPortFb303Stats::kPortStatKeys()) {
    EXPECT_FALSE(fbData->getStatMap()->contains(
        HwPortFb303Stats::statName(statKey, kPortName)));
  }
}

TEST(HwPortFb303StatsTest, ReInit) {
  constexpr auto kNewPortName = "eth1/2/1";

  HwPortFb303Stats stats(kPortName, kQueue2Name);
  stats.portNameChanged(kNewPortName);
  for (const auto& sName : HwPortFb303Stats::kPortStatKeys()) {
    EXPECT_TRUE(fbData->getStatMap()->contains(
        HwPortFb303Stats::statName(sName, kNewPortName)));
    EXPECT_FALSE(fbData->getStatMap()->contains(
        HwPortFb303Stats::statName(sName, kPortName)));
  }
  for (auto statKey : HwPortFb303Stats::kQueueStatKeys()) {
    for (const auto& queueIdAndName : kQueue2Name) {
      EXPECT_TRUE(fbData->getStatMap()->contains(HwPortFb303Stats::statName(
          statKey, kNewPortName, queueIdAndName.first, queueIdAndName.second)));
      EXPECT_FALSE(fbData->getStatMap()->contains(HwPortFb303Stats::statName(
          statKey, kPortName, queueIdAndName.first, queueIdAndName.second)));
    }
  }
}

TEST(HwPortFb303Stats, UpdateStats) {
  HwPortFb303Stats portStats(kPortName, kQueue2Name);
  updateStats(portStats);
  verifyUpdatedStats(portStats);
}

TEST(HwPortFb303StatsTest, RenameQueue) {
  HwPortFb303Stats stats(kPortName, kQueue2Name);
  stats.addOrUpdateQueue(1, "platinum");
  auto newQueueMapping = kQueue2Name;
  for (auto statKey : HwPortFb303Stats::kQueueStatKeys()) {
    EXPECT_TRUE(fbData->getStatMap()->contains(
        HwPortFb303Stats::statName(statKey, kPortName, 1, "platinum")));
    EXPECT_FALSE(fbData->getStatMap()->contains(
        HwPortFb303Stats::statName(statKey, kPortName, 1, "gold")));
    // No impact on silver
    EXPECT_TRUE(fbData->getStatMap()->contains(
        HwPortFb303Stats::statName(statKey, kPortName, 2, "silver")));
  }
}

TEST(HwPortFb303StatsTest, AddQueue) {
  HwPortFb303Stats stats(kPortName, kQueue2Name);
  stats.addOrUpdateQueue(3, "platinum");
  auto newQueueMapping = kQueue2Name;
  for (auto statKey : HwPortFb303Stats::kQueueStatKeys()) {
    EXPECT_TRUE(fbData->getStatMap()->contains(
        HwPortFb303Stats::statName(statKey, kPortName, 1, "gold")));
    EXPECT_TRUE(fbData->getStatMap()->contains(
        HwPortFb303Stats::statName(statKey, kPortName, 2, "silver")));
    EXPECT_TRUE(fbData->getStatMap()->contains(
        HwPortFb303Stats::statName(statKey, kPortName, 3, "platinum")));
  }
}

TEST(HwPortFb303StatsTest, RemoveQueue) {
  HwPortFb303Stats stats(kPortName, kQueue2Name);
  stats.removeQueue(1);
  auto newQueueMapping = kQueue2Name;
  for (auto statKey : HwPortFb303Stats::kQueueStatKeys()) {
    EXPECT_FALSE(fbData->getStatMap()->contains(
        HwPortFb303Stats::statName(statKey, kPortName, 1, "gold")));
    EXPECT_TRUE(fbData->getStatMap()->contains(
        HwPortFb303Stats::statName(statKey, kPortName, 2, "silver")));
  }
}

TEST(HwPortFb303Stats, portNameChangeStatsRetainValue) {
  HwPortFb303Stats portStats(kPortName, kQueue2Name);
  updateStats(portStats);
  portStats.portNameChanged("fab1/1/1");
  verifyUpdatedStats(portStats);
}

TEST(HwPortFb303Stats, queueNameChangeStatsRetainValue) {
  HwPortFb303Stats portStats(kPortName, kQueue2Name);
  updateStats(portStats);
  portStats.addOrUpdateQueue(1, "platinium");
  verifyUpdatedStats(portStats);
}
