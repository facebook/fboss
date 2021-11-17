/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include <gtest/gtest.h>
#include <chrono>
#include "common/stats/ThreadCachedServiceData.h"
#include "fboss/agent/test/link_tests/LinkTest.h"
#include "fboss/lib/CommonUtils.h"

using namespace ::testing;
using namespace facebook;
using namespace facebook::fboss;

class PortStatsTest : public LinkTest {
 public:
  // unordered_map<portName, unorder_map<counterName, counter>>
  using XphyPortStats =
      std::unordered_map<std::string, std::unordered_map<std::string, int64_t>>;

  std::optional<XphyPortStats> getXphyPortStats(bool logErrors = false);
  void verifyXphyPortStats(
      const XphyPortStats& before,
      const XphyPortStats& after);
};

std::optional<PortStatsTest::XphyPortStats> PortStatsTest::getXphyPortStats(
    bool fatalErrors) {
  XphyPortStats portStats;

  tcData().publishStats();

  for (const auto& port : getCabledPorts()) {
    auto portName = getPortName(port);
    auto result = portStats.try_emplace(portName);
    auto iter = result.first;
    auto& portStat = iter->second;

    for (const auto side : {"system", "line"}) {
      for (const auto stat : {"fec_uncorrectable", "fec_correctable"}) {
        auto counterName = folly::to<std::string>(side, ".", stat, ".sum");
        auto fullCounterName =
            folly::to<std::string>(portName, ".xphy.", counterName);
        if (!tcData().hasCounter(fullCounterName)) {
          if (fatalErrors) {
            EXPECT_TRUE(tcData().hasCounter(fullCounterName))
                << fullCounterName << " absent";
          }
          return std::nullopt;
        }
        portStat[counterName] = tcData().getCounter(fullCounterName);
      }
    }
  }

  return portStats;
}

void PortStatsTest::verifyXphyPortStats(
    const XphyPortStats& before,
    const XphyPortStats& after) {
  for (const auto& port : getCabledPorts()) {
    auto portName = getPortName(port);

    for (const auto side : {"system", "line"}) {
      auto counterName = folly::to<std::string>(side, ".fec_uncorrectable.sum");
      EXPECT_EQ(
          before.at(portName).at(counterName),
          after.at(portName).at(counterName))
          << after.at(portName).at(counterName) -
              before.at(portName).at(counterName)
          << " uncorrectable FEC codewords on port " << portName;

      counterName = folly::to<std::string>(side, ".fec_correctable.sum");
      EXPECT_TRUE(
          before.at(portName).at(counterName) <=
          after.at(portName).at(counterName))
          << "Negative corrected codewords on port " << portName;
    }
  }
}

TEST_F(PortStatsTest, xphySanity) {
  std::optional<PortStatsTest::XphyPortStats> portStatsBefore;
  try {
    checkWithRetry(
        [this, &portStatsBefore] {
          return (portStatsBefore = getXphyPortStats());
        },
        kMaxNumXphyInfoCollectionCheck /* retries */,
        kSecondsBetweenXphyInfoCollectionCheck /* retry period */);
  } catch (const FbossError&) {
    // one last try with full logging
    portStatsBefore = getXphyPortStats(true);
    ASSERT_TRUE(portStatsBefore.has_value())
        << "Never has complete xphy port stats";
  }

  // sleep override
  std::this_thread::sleep_for(std::chrono::seconds(
      kSecondsBetweenXphyInfoCollectionCheck * kMaxNumXphyInfoCollectionCheck));

  verifyXphyPortStats(*portStatsBefore, *getXphyPortStats(true));
}
