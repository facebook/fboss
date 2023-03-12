/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/HwSysPortFb303Stats.h"
#include "fboss/agent/hw/StatsConstants.h"

#include <fb303/ServiceData.h>
#include <folly/logging/xlog.h>

#include <gtest/gtest.h>
using namespace facebook::fboss;
using namespace facebook::fb303;
using namespace std::chrono;

namespace {
constexpr auto kPortName = "rdsw.foo:eth1/1/1";
HwSysPortFb303Stats::QueueId2Name kQueue2Name = {
    {1, "gold"},
    {2, "silver"},
};

HwSysPortStats getInitedStats() {
  return {
      apache::thrift::FragileConstructor(),
      {{1, 1}, {2, 1}}, // queueOutDiscardBytes
      {{1, 2}, {2, 2}}, // queueOutBytes
      {{1, 0}, {2, 10}}, // queueWatermarkBytes
      {{1, 3}, {2, 3}}, // queueWredDroppedPackets
      0, // timestamp
      "test", // portName
  };
}

void updateStats(HwSysPortFb303Stats& portStats) {
  auto now = duration_cast<seconds>(system_clock::now().time_since_epoch());
  // To get last increment from monotonic counter we need to update it twice
  HwSysPortStats empty{};
  // Need to populate queue stats, since by default these
  // maps are empty
  *empty.queueOutDiscardBytes_() = *empty.queueOutBytes_() =
      *empty.queueWatermarkBytes_() = {{1, 0}, {2, 0}};
  portStats.updateStats(empty, now);
  portStats.updateStats(getInitedStats(), now);
}

void verifyUpdatedStats(const HwSysPortFb303Stats& portStats) {
  auto curValue{1};
  for (auto counterName : portStats.kQueueStatKeys()) {
    for (const auto& queueIdAndName : kQueue2Name) {
      EXPECT_EQ(
          portStats.getCounterLastIncrement(HwSysPortFb303Stats::statName(
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

TEST(HwSysPortFb303StatsTest, StatName) {
  EXPECT_EQ(
      HwSysPortFb303Stats::statName(kOutBytes(), kPortName),
      folly::to<std::string>(kPortName, '.', kOutBytes()));
  EXPECT_EQ(
      HwSysPortFb303Stats::statName(kOutBytes(), kPortName, 1, "gold"),
      folly::to<std::string>(kPortName, '.', "queue1.gold.", kOutBytes()));
}

TEST(HwSysPortFb303StatsTest, StatsInit) {
  HwSysPortFb303Stats stats(kPortName, kQueue2Name);
  for (auto statKey : stats.kQueueStatKeys()) {
    for (const auto& queueIdAndName : kQueue2Name) {
      EXPECT_TRUE(fbData->getStatMap()->contains(HwSysPortFb303Stats::statName(
          statKey, kPortName, queueIdAndName.first, queueIdAndName.second)));
    }
  }
}
TEST(HwSysPortFb303StatsTest, StatsDeInit) {
  { HwSysPortFb303Stats stats(kPortName); }
  HwSysPortFb303Stats dummy("dummy");
  for (auto statKey : dummy.kQueueStatKeys()) {
    for (const auto& queueIdAndName : kQueue2Name) {
      EXPECT_FALSE(fbData->getStatMap()->contains(HwSysPortFb303Stats::statName(
          statKey, kPortName, queueIdAndName.first, queueIdAndName.second)));
    }
  }
}

TEST(HwSysPortFb303StatsTest, ReInit) {
  constexpr auto kNewPortName = "eth1/2/1";

  HwSysPortFb303Stats stats(kPortName, kQueue2Name);
  stats.portNameChanged(kNewPortName);
  for (auto statKey : stats.kQueueStatKeys()) {
    for (const auto& queueIdAndName : kQueue2Name) {
      EXPECT_TRUE(fbData->getStatMap()->contains(HwSysPortFb303Stats::statName(
          statKey, kNewPortName, queueIdAndName.first, queueIdAndName.second)));
      EXPECT_FALSE(fbData->getStatMap()->contains(HwSysPortFb303Stats::statName(
          statKey, kPortName, queueIdAndName.first, queueIdAndName.second)));
    }
  }
}

TEST(HwSysPortFb303Stats, UpdateStats) {
  HwSysPortFb303Stats portStats(kPortName, kQueue2Name);
  updateStats(portStats);
  verifyUpdatedStats(portStats);
}

TEST(HwSysPortFb303StatsTest, PortName) {
  constexpr auto kNewPortName = "eth1/2/1";
  HwSysPortFb303Stats stats(kPortName, kQueue2Name);
  EXPECT_EQ(*stats.portStats().portName_(), kPortName);
  stats.portNameChanged(kNewPortName);
  EXPECT_EQ(*stats.portStats().portName_(), kNewPortName);
}

TEST(HwSysPortFb303StatsTest, RenameQueue) {
  HwSysPortFb303Stats stats(kPortName, kQueue2Name);
  stats.queueChanged(1, "platinum");
  auto newQueueMapping = kQueue2Name;
  for (auto statKey : stats.kQueueStatKeys()) {
    EXPECT_TRUE(fbData->getStatMap()->contains(
        HwSysPortFb303Stats::statName(statKey, kPortName, 1, "platinum")));
    EXPECT_FALSE(fbData->getStatMap()->contains(
        HwSysPortFb303Stats::statName(statKey, kPortName, 1, "gold")));
    // No impact on silver
    EXPECT_TRUE(fbData->getStatMap()->contains(
        HwSysPortFb303Stats::statName(statKey, kPortName, 2, "silver")));
  }
}

TEST(HwSysPortFb303StatsTest, AddQueue) {
  HwSysPortFb303Stats stats(kPortName, kQueue2Name);
  stats.queueChanged(3, "platinum");
  auto newQueueMapping = kQueue2Name;
  for (auto statKey : stats.kQueueStatKeys()) {
    EXPECT_TRUE(fbData->getStatMap()->contains(
        HwSysPortFb303Stats::statName(statKey, kPortName, 1, "gold")));
    EXPECT_TRUE(fbData->getStatMap()->contains(
        HwSysPortFb303Stats::statName(statKey, kPortName, 2, "silver")));
    EXPECT_TRUE(fbData->getStatMap()->contains(
        HwSysPortFb303Stats::statName(statKey, kPortName, 3, "platinum")));
  }
}

TEST(HwSysPortFb303StatsTest, RemoveQueue) {
  HwSysPortFb303Stats stats(kPortName, kQueue2Name);
  stats.queueRemoved(1);
  auto newQueueMapping = kQueue2Name;
  for (auto statKey : stats.kQueueStatKeys()) {
    EXPECT_FALSE(fbData->getStatMap()->contains(
        HwSysPortFb303Stats::statName(statKey, kPortName, 1, "gold")));
    EXPECT_TRUE(fbData->getStatMap()->contains(
        HwSysPortFb303Stats::statName(statKey, kPortName, 2, "silver")));
  }
}

TEST(HwSysPortFb303Stats, portNameChangeResetsValue) {
  HwSysPortFb303Stats portStats(kPortName, kQueue2Name);
  updateStats(portStats);
  auto kNewPortName = "fab1/1/1";
  portStats.portNameChanged(kNewPortName);
  for (auto counterName : portStats.kQueueStatKeys()) {
    for (const auto& queueIdAndName : kQueue2Name) {
      EXPECT_TRUE(fbData->getStatMap()->contains(HwSysPortFb303Stats::statName(
          counterName,
          kNewPortName,
          queueIdAndName.first,
          queueIdAndName.second)));
      EXPECT_EQ(
          portStats.getCounterLastIncrement(HwSysPortFb303Stats::statName(
              counterName,
              kNewPortName,
              queueIdAndName.first,
              queueIdAndName.second)),
          0);
      EXPECT_FALSE(fbData->getStatMap()->contains(HwSysPortFb303Stats::statName(
          counterName,
          kPortName,
          queueIdAndName.first,
          queueIdAndName.second)));
    }
  }
}

TEST(HwSysPortFb303Stats, queueNameChangeResetsValue) {
  HwSysPortFb303Stats portStats(kPortName, kQueue2Name);
  updateStats(portStats);
  portStats.queueChanged(1, "platinum");
  portStats.queueChanged(2, "bronze");
  HwSysPortFb303Stats::QueueId2Name newQueues = {
      {1, "platinum"}, {2, "bronze"}};
  for (auto counterName : portStats.kQueueStatKeys()) {
    for (const auto& queueIdAndName : newQueues) {
      EXPECT_TRUE(fbData->getStatMap()->contains(HwSysPortFb303Stats::statName(
          counterName,
          kPortName,
          queueIdAndName.first,
          queueIdAndName.second)));
      EXPECT_EQ(
          portStats.getCounterLastIncrement(HwSysPortFb303Stats::statName(
              counterName,
              kPortName,
              queueIdAndName.first,
              queueIdAndName.second)),
          0);
    }
  }
  for (auto counterName : portStats.kQueueStatKeys()) {
    for (const auto& queueIdAndName : kQueue2Name) {
      EXPECT_FALSE(fbData->getStatMap()->contains(HwSysPortFb303Stats::statName(
          counterName,
          kPortName,
          queueIdAndName.first,
          queueIdAndName.second)));
    }
  }
}
