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

#include <gtest/gtest.h>
using namespace facebook::fboss;
using namespace facebook::fb303;
using namespace std::chrono;

namespace {
std::array<folly::StringPiece, 20> kStatNames = {
    kInBytes(),
    kInUnicastPkts(),
    kInMulticastPkts(),
    kInBroadcastPkts(),
    kInDiscards(),
    kInErrors(),
    kInPause(),
    kInIpv4HdrErrors(),
    kInIpv6HdrErrors(),
    kInDstNullDiscards(),
    kInDiscardsRaw(),
    kOutBytes(),
    kOutUnicastPkts(),
    kOutMulticastPkts(),
    kOutBroadcastPkts(),
    kOutDiscards(),
    kOutErrors(),
    kOutPause(),
    kOutCongestionDiscards(),
    kOutEcnCounter(),
};
}

TEST(HwPortFb303StatsTest, StatsInit) {
  HwPortFb303Stats stats("eth1/1/1");
  for (const auto& statName : stats.statNames()) {
    EXPECT_TRUE(fbData->getStatMap()->contains(statName));
  }
}

TEST(HwPortFb303StatsTest, StatsDeInit) {
  std::vector<std::string> statNames;
  {
    HwPortFb303Stats stats("eth1/1/1");
    statNames = stats.statNames();
  }
  for (const auto& statName : statNames) {
    EXPECT_FALSE(fbData->getStatMap()->contains(statName));
  }
}

TEST(HwPortFb303StatsTest, ReInit) {
  constexpr auto kOrigPortName = "eth1/1/1";
  constexpr auto kNewPortName = "eth1/2/1";

  HwPortFb303Stats stats(kOrigPortName);
  stats.reinitPortStats(kNewPortName);
  for (const auto& sName : kStatNames) {
    EXPECT_TRUE(fbData->getStatMap()->contains(
        HwPortFb303Stats::statName(sName, kNewPortName)));
    EXPECT_FALSE(fbData->getStatMap()->contains(
        HwPortFb303Stats::statName(sName, kOrigPortName)));
  }
}

TEST(HwPortFb303Stats, UpdateStats) {
  HwPortStats stats{
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
      {}, // queueOutDiscards
      {}, // queueOutBytes
      20, // outEcnCounter
      {}, // queueOutPackets
  };
  constexpr auto kPortName = "eth1/1/1";
  HwPortFb303Stats portStats(kPortName);
  auto now = duration_cast<seconds>(system_clock::now().time_since_epoch());
  // To get last increment from monotonic counter we need to update it twice
  portStats.updateStats(HwPortStats{}, now);
  portStats.updateStats(stats, now);
  auto curValue{1};
  for (auto counterName : kStatNames) {
    // +1 because first initialization is to -1
    EXPECT_EQ(
        portStats.getPortCounterLastIncrement(counterName), curValue++ + 1);
  }
}
