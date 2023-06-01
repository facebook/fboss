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

std::vector<PfcPriority> kEnabledPfcPriorities(
    {static_cast<PfcPriority>(2), static_cast<PfcPriority>(3)});

HwPortStats getInitedStats() {
  MacsecStats macsecStats{
      apache::thrift::FragileConstructor(),
      // ingress macsec port stats
      mka::MacsecPortStats{
          apache::thrift::FragileConstructor(),
          1, // preMacsecDropPkts
          2, // controlPkts
          3, // dataPkts
          4, // octetsEncrypted
          5, // inBadOrNoMacsecTagDroppedPkts
          6, // inNoSciDroppedPkts
          7, // inUnknownSciPkts
          8, // inOverrunDroppedPkts
          9, // inDelayedPkts
          10, // inLateDroppedPkts
          11, // inNotValidDroppedPkts
          12, // inInvalidPkts
          13, // inNoSaDroppedPkts
          14, // inUnusedSaPkts
          16, // inCurrentXpn
          0, // outTooLongDroppedPkts
          0, // outCurrentXpn
          15 // noMacsecTagPkts
      },
      // egress macsec port stats
      mka::MacsecPortStats{
          apache::thrift::FragileConstructor(),
          1, // preMacsecDropPkts
          2, // controlPkts
          3, // dataPkts
          4, // octetsEncrypted
          0, // inBadOrNoMacsecTagDroppedPkts
          0, // inNoSciDroppedPkts
          0, // inUnknownSciPkts
          0, // inOverrunDroppedPkts
          0, // inDelayedPkts
          0, // inLateDroppedPkts
          0, // inNotValidDroppedPkts
          0, // inInvalidPkts
          0, // inNoSaDroppedPkts
          0, // inUnusedSaPkts
          0, // inCurrentXpn
          5, // outTooLongDroppedPkts
          7, // outCurrentXpn
          6 // noMacsecTagPkts
      },
      {}, // ingress flow stats
      {}, // egress flow stats
      {{}}, // rx SA stats
      {{}}, // tx SA stats
      // Macsec Acl stats ingress
      mka::MacsecAclStats{
          apache::thrift::FragileConstructor(),
          0, // defaultAclStats
          0, // controlAclStats
          0, // flowAclStats
      },
      // Macsec Acl stats egress
      mka::MacsecAclStats{
          apache::thrift::FragileConstructor(),
          0, // defaultAclStats
          0, // controlAclStats
          0, // flowAclStats
      },
  };
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
      20, // wredDroppedPackets
      {{1, 1}, {2, 1}}, // queueOutDiscards
      {{1, 3}, {2, 3}}, // queueOutBytes
      21, // outEcnCounter
      {{1, 4}, {2, 4}}, // queueOutPackets
      {{1, 2}, {2, 2}}, // queueOutDiscardPackets
      {{1, 0}, {2, 10}}, // queueWatermarkBytes
      22, // fecCorrectableErrors
      23, // fecUncorrectableErrors
      24, // inPfcCtrl_
      25, // outPfcCtrl_
      {{0, 1}, {7, 1}}, // inPfc_
      {{0, 2}, {7, 2}}, // inPfcXon_
      {{0, 3}, {7, 3}}, // outPfc_
      {{1, 5}, {2, 5}}, // queueWredDroppedPackets
      {{1, 6}, {2, 6}}, // queueEcnMarkedPackets
      0, // timestamp
      "test", // portName
      macsecStats,
      24, // inLabelMissDiscards_
      {}, // queueWatermarkLevel
      25 // inCongestionDiscards
  };
}

void updateStats(HwPortFb303Stats& portStats) {
  auto now = duration_cast<seconds>(system_clock::now().time_since_epoch());
  // To get last increment from monotonic counter we need to update it twice
  HwPortStats empty{};
  MacsecStats emptyMacsecStats{
      apache::thrift::FragileConstructor(),
      mka::MacsecPortStats{
          apache::thrift::FragileConstructor(),
          0, // preMacsecDropPkts
          0, // controlPkts
          0, // dataPkts
          0, // octetsEncrypted
          0, // inBadOrNoMacsecTagDroppedPkts
          0, // inNoSciDroppedPkts
          0, // inUnknownSciPkts
          0, // inOverrunDroppedPkts
          0, // inDelayedPkts
          0, // inLateDroppedPkts
          0, // inNotValidDroppedPkts
          0, // inInvalidPkts
          0, // inNoSaDroppedPkts
          0, // inUnusedSaPkts
          0, // inCurrentXpn
          0, // outTooLongDroppedPkts
          0, // outCurrentXpn
          0 // noMacsecTagPkts
      }, // ingress  macsec port stats
      mka::MacsecPortStats{
          apache::thrift::FragileConstructor(),
          0, // preMacsecDropPkts
          0, // controlPkts
          0, // dataPkts
          0, // octetsEncrypted
          0, // inBadOrNoMacsecTagDroppedPkts
          0, // inNoSciDroppedPkts
          0, // inUnknownSciPkts
          0, // inOverrunDroppedPkts
          0, // inDelayedPkts
          0, // inLateDroppedPkts
          0, // inNotValidDroppedPkts
          0, // inInvalidPkts
          0, // inNoSaDroppedPkts
          0, // inUnusedSaPkts
          0, // inCurrentXpn
          0, // outTooLongDroppedPkts
          0, // outCurrentXpn
          0 // noMacsecTagPkts
      }, // egress  macsec port stats
      {}, // ingress flow stats
      {}, // egress flow stats
      {{}}, // rx SA stats
      {{}}, // tx SA stats
      mka::MacsecAclStats{
          apache::thrift::FragileConstructor(),
          0,
          0,
          0,
      }, // macsec ingress Acl stats
      mka::MacsecAclStats{
          apache::thrift::FragileConstructor(),
          0,
          0,
          0,
      }, // macsec egress Acl stats
  };
  empty.macsecStats() = emptyMacsecStats;
  // Need to populate queue stats, since by default these
  // maps are empty
  *empty.queueOutDiscardPackets_() = *empty.queueOutDiscardBytes_() =
      *empty.queueOutBytes_() = *empty.queueOutPackets_() =
          *empty.queueWatermarkBytes_() = *empty.queueEcnMarkedPackets_() =
              *empty.queueWredDroppedPackets_() = {{1, 0}, {2, 0}};
  portStats.updateStats(empty, now);
  portStats.updateStats(getInitedStats(), now);
}

void verifyUpdatedStats(const HwPortFb303Stats& portStats) {
  auto curValue{1};
  for (auto counterName : portStats.kPortStatKeys()) {
    // +1 because first initialization is to -1
    auto actualVal = portStats.getCounterLastIncrement(
        HwPortFb303Stats::statName(counterName, kPortName));
    auto expectedVal = (curValue++) + 1;
    EXPECT_EQ(actualVal, expectedVal) << "failed for " << counterName;
    XLOG(DBG2) << counterName << ": " << actualVal << " " << expectedVal;
  }
  curValue = 1;
  for (auto counterName : portStats.kQueueStatKeys()) {
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
  curValue = 1;
  for (auto counterName : portStats.kInMacsecPortStatKeys()) {
    EXPECT_EQ(
        portStats.getCounterLastIncrement(
            HwPortFb303Stats::statName(counterName, kPortName)),
        curValue++);
  }
  curValue = 1;
  for (auto counterName : portStats.kOutMacsecPortStatKeys()) {
    EXPECT_EQ(
        portStats.getCounterLastIncrement(
            HwPortFb303Stats::statName(counterName, kPortName)),
        curValue++);
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
  HwPortFb303Stats stats(kPortName, kQueue2Name, kEnabledPfcPriorities);
  for (auto statKey : stats.kPortStatKeys()) {
    EXPECT_TRUE(fbData->getStatMap()->contains(
        HwPortFb303Stats::statName(statKey, kPortName)));
  }
  for (auto statKey : stats.kQueueStatKeys()) {
    for (const auto& queueIdAndName : kQueue2Name) {
      EXPECT_TRUE(fbData->getStatMap()->contains(HwPortFb303Stats::statName(
          statKey, kPortName, queueIdAndName.first, queueIdAndName.second)));
    }
  }
  for (auto statKey : stats.kPfcStatKeys()) {
    for (auto pfcPriority : kEnabledPfcPriorities) {
      EXPECT_TRUE(fbData->getStatMap()->contains(
          HwPortFb303Stats::statName(statKey, kPortName, pfcPriority)));
    }
    EXPECT_TRUE(fbData->getStatMap()->contains(
        HwPortFb303Stats::statName(statKey, kPortName)));
  }
}

TEST(HwPortFb303StatsTest, StatsDeInit) {
  { HwPortFb303Stats stats(kPortName); }
  HwPortFb303Stats dummy("dummy");
  for (auto statKey : dummy.kPortStatKeys()) {
    EXPECT_FALSE(fbData->getStatMap()->contains(
        HwPortFb303Stats::statName(statKey, kPortName)));
  }
  for (auto statKey : dummy.kQueueStatKeys()) {
    for (const auto& queueIdAndName : kQueue2Name) {
      EXPECT_FALSE(fbData->getStatMap()->contains(HwPortFb303Stats::statName(
          statKey, kPortName, queueIdAndName.first, queueIdAndName.second)));
    }
  }
}

TEST(HwPortFb303StatsTest, ReInit) {
  constexpr auto kNewPortName = "eth1/2/1";

  HwPortFb303Stats stats(kPortName, kQueue2Name, kEnabledPfcPriorities);
  stats.portNameChanged(kNewPortName);
  for (const auto& sName : stats.kPortStatKeys()) {
    EXPECT_TRUE(fbData->getStatMap()->contains(
        HwPortFb303Stats::statName(sName, kNewPortName)));
    EXPECT_FALSE(fbData->getStatMap()->contains(
        HwPortFb303Stats::statName(sName, kPortName)));
  }
  for (auto statKey : stats.kQueueStatKeys()) {
    for (const auto& queueIdAndName : kQueue2Name) {
      EXPECT_TRUE(fbData->getStatMap()->contains(HwPortFb303Stats::statName(
          statKey, kNewPortName, queueIdAndName.first, queueIdAndName.second)));
      EXPECT_FALSE(fbData->getStatMap()->contains(HwPortFb303Stats::statName(
          statKey, kPortName, queueIdAndName.first, queueIdAndName.second)));
    }
  }
  for (auto statKey : stats.kPfcStatKeys()) {
    for (auto pfcPriority : kEnabledPfcPriorities) {
      EXPECT_TRUE(fbData->getStatMap()->contains(
          HwPortFb303Stats::statName(statKey, kNewPortName, pfcPriority)));
      EXPECT_FALSE(fbData->getStatMap()->contains(
          HwPortFb303Stats::statName(statKey, kPortName, pfcPriority)));
    }
    EXPECT_TRUE(fbData->getStatMap()->contains(
        HwPortFb303Stats::statName(statKey, kNewPortName)));
    EXPECT_FALSE(fbData->getStatMap()->contains(
        HwPortFb303Stats::statName(statKey, kPortName)));
  }
}

TEST(HwPortFb303Stats, UpdateStats) {
  HwPortFb303Stats portStats(kPortName, kQueue2Name);
  updateStats(portStats);
  verifyUpdatedStats(portStats);
}

TEST(HwPortFb303StatsTest, PortName) {
  constexpr auto kNewPortName = "eth1/2/1";
  HwPortFb303Stats stats(kPortName, kQueue2Name);
  EXPECT_EQ(*stats.portStats().portName_(), kPortName);
  stats.portNameChanged(kNewPortName);
  EXPECT_EQ(*stats.portStats().portName_(), kNewPortName);
}

TEST(HwPortFb303StatsTest, RenameQueue) {
  HwPortFb303Stats stats(kPortName, kQueue2Name);
  stats.queueChanged(1, "platinum");
  auto newQueueMapping = kQueue2Name;
  for (auto statKey : stats.kQueueStatKeys()) {
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
  stats.queueChanged(3, "platinum");
  auto newQueueMapping = kQueue2Name;
  for (auto statKey : stats.kQueueStatKeys()) {
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
  stats.queueRemoved(1);
  auto newQueueMapping = kQueue2Name;
  for (auto statKey : stats.kQueueStatKeys()) {
    EXPECT_FALSE(fbData->getStatMap()->contains(
        HwPortFb303Stats::statName(statKey, kPortName, 1, "gold")));
    EXPECT_TRUE(fbData->getStatMap()->contains(
        HwPortFb303Stats::statName(statKey, kPortName, 2, "silver")));
  }
}

TEST(HwPortFb303Stats, portNameChangeResetsValue) {
  HwPortFb303Stats portStats(kPortName, kQueue2Name);
  updateStats(portStats);
  auto kNewPortName = "fab1/1/1";
  portStats.portNameChanged(kNewPortName);
  for (auto counterName : portStats.kPortStatKeys()) {
    EXPECT_EQ(
        portStats.getCounterLastIncrement(
            HwPortFb303Stats::statName(counterName, kNewPortName)),
        0);
    EXPECT_TRUE(fbData->getStatMap()->contains(
        HwPortFb303Stats::statName(counterName, kNewPortName)));
    EXPECT_FALSE(fbData->getStatMap()->contains(
        HwPortFb303Stats::statName(counterName, kPortName)));
  }
  for (auto counterName : portStats.kQueueStatKeys()) {
    for (const auto& queueIdAndName : kQueue2Name) {
      EXPECT_TRUE(fbData->getStatMap()->contains(HwPortFb303Stats::statName(
          counterName,
          kNewPortName,
          queueIdAndName.first,
          queueIdAndName.second)));
      EXPECT_EQ(
          portStats.getCounterLastIncrement(HwPortFb303Stats::statName(
              counterName,
              kNewPortName,
              queueIdAndName.first,
              queueIdAndName.second)),
          0);
      EXPECT_FALSE(fbData->getStatMap()->contains(HwPortFb303Stats::statName(
          counterName,
          kPortName,
          queueIdAndName.first,
          queueIdAndName.second)));
    }
  }
}

TEST(HwPortFb303Stats, queueNameChangeResetsValue) {
  HwPortFb303Stats portStats(kPortName, kQueue2Name);
  updateStats(portStats);
  portStats.queueChanged(1, "platinum");
  portStats.queueChanged(2, "bronze");
  HwPortFb303Stats::QueueId2Name newQueues = {{1, "platinum"}, {2, "bronze"}};
  for (auto counterName : portStats.kQueueStatKeys()) {
    for (const auto& queueIdAndName : newQueues) {
      EXPECT_TRUE(fbData->getStatMap()->contains(HwPortFb303Stats::statName(
          counterName,
          kPortName,
          queueIdAndName.first,
          queueIdAndName.second)));
      EXPECT_EQ(
          portStats.getCounterLastIncrement(HwPortFb303Stats::statName(
              counterName,
              kPortName,
              queueIdAndName.first,
              queueIdAndName.second)),
          0);
    }
  }
  for (auto counterName : portStats.kQueueStatKeys()) {
    for (const auto& queueIdAndName : kQueue2Name) {
      EXPECT_FALSE(fbData->getStatMap()->contains(HwPortFb303Stats::statName(
          counterName,
          kPortName,
          queueIdAndName.first,
          queueIdAndName.second)));
    }
  }
}

TEST(HwPortFb303StatsTest, ChangePfcPriority) {
  HwPortFb303Stats stats(kPortName, kQueue2Name, kEnabledPfcPriorities);
  std::vector<PfcPriority> newPriorities(
      {static_cast<PfcPriority>(5), static_cast<PfcPriority>(6)});
  stats.pfcPriorityChanged(newPriorities);
  for (auto statKey : stats.kPfcStatKeys()) {
    for (auto pfcPriority : kEnabledPfcPriorities) {
      EXPECT_FALSE(fbData->getStatMap()->contains(
          HwPortFb303Stats::statName(statKey, kPortName, pfcPriority)));
    }
    for (auto pfcPriority : newPriorities) {
      EXPECT_TRUE(fbData->getStatMap()->contains(
          HwPortFb303Stats::statName(statKey, kPortName, pfcPriority)));
    }
    EXPECT_TRUE(fbData->getStatMap()->contains(
        HwPortFb303Stats::statName(statKey, kPortName)));
  }
}
