/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "fboss/agent/normalization/PortStatsProcessor.h"
#include "fboss/agent/normalization/StatsExporter.h"
#include "fboss/agent/normalization/TransformHandler.h"

using namespace ::testing;

namespace facebook::fboss::normalization {
namespace {
HwPortStats makeHwPortStatsEth0T0() {
  HwPortStats stats;
  stats.inBytes__ref() = 1000;
  stats.outBytes__ref() = 2000;
  stats.inDiscards__ref() = 10;
  stats.outDiscards__ref() = 30;

  stats.timestamp__ref() = 1000;
  return stats;
}

HwPortStats makeHwPortStatsEth1T0() {
  HwPortStats stats;
  stats.inBytes__ref() = 3000;
  stats.outBytes__ref() = 6000;
  stats.inDiscards__ref() = 20;
  stats.outDiscards__ref() = 70;

  stats.timestamp__ref() = 1000;
  return stats;
}

HwPortStats makeHwPortStatsEth0T1() {
  HwPortStats stats;
  stats.inBytes__ref() = 2000;
  stats.outBytes__ref() = 4000;
  stats.inDiscards__ref() = 20;
  stats.outDiscards__ref() = 60;

  stats.timestamp__ref() = 1010;
  return stats;
}

HwPortStats makeHwPortStatsEth1T1() {
  HwPortStats stats;
  stats.inBytes__ref() = 5000;
  stats.outBytes__ref() = 9000;
  stats.inDiscards__ref() = 70;
  stats.outDiscards__ref() = 100;

  stats.timestamp__ref() = 1010;
  return stats;
}

folly::F14FastMap<std::string, HwPortStats> makeHwPortStatsMapT0() {
  folly::F14FastMap<std::string, HwPortStats> statsMap;
  statsMap.emplace("eth0", makeHwPortStatsEth0T0());
  statsMap.emplace("eth1", makeHwPortStatsEth1T0());
  return statsMap;
}

folly::F14FastMap<std::string, HwPortStats> makeHwPortStatsMapT1() {
  folly::F14FastMap<std::string, HwPortStats> statsMap;
  statsMap.emplace("eth0", makeHwPortStatsEth0T1());
  statsMap.emplace("eth1", makeHwPortStatsEth1T1());
  return statsMap;
}

class MockStatsExporter : public StatsExporter {
 public:
  explicit MockStatsExporter(const std::string& deviceName)
      : StatsExporter(deviceName) {}

  MOCK_METHOD4(
      publishPortStats,
      void(
          const std::string& portName,
          const std::string& propertyName,
          int64_t timestamp,
          double value));
  MOCK_METHOD0(flushCounters, void());
};

} // namespace

TEST(PortStatsProcessorTest, processStats) {
  TransformHandler handler;
  MockStatsExporter exporter("dev");

  {
    // t0
    auto hwStatsMap = makeHwPortStatsMapT0();
    PortStatsProcessor processor(&handler, &exporter);
    EXPECT_CALL(exporter, publishPortStats(_, _, _, _)).Times(0);
    EXPECT_CALL(exporter, flushCounters()).Times(1);
    processor.processStats(hwStatsMap);
    Mock::VerifyAndClearExpectations(&exporter);
  }

  {
    // t1
    auto hwStatsMap = makeHwPortStatsMapT1();
    PortStatsProcessor processor(&handler, &exporter);
    EXPECT_CALL(exporter, publishPortStats("eth0", "input_bps", 1010, 800))
        .Times(1);
    EXPECT_CALL(exporter, publishPortStats("eth0", "output_bps", 1010, 1600))
        .Times(1);
    EXPECT_CALL(
        exporter, publishPortStats("eth0", "total_input_discards", 1010, 1))
        .Times(1);
    EXPECT_CALL(
        exporter, publishPortStats("eth0", "total_output_discards", 1010, 3))
        .Times(1);

    EXPECT_CALL(exporter, publishPortStats("eth1", "input_bps", 1010, 1600))
        .Times(1);
    EXPECT_CALL(exporter, publishPortStats("eth1", "output_bps", 1010, 2400))
        .Times(1);
    EXPECT_CALL(
        exporter, publishPortStats("eth1", "total_input_discards", 1010, 5))
        .Times(1);
    EXPECT_CALL(
        exporter, publishPortStats("eth1", "total_output_discards", 1010, 3))
        .Times(1);
    EXPECT_CALL(exporter, flushCounters()).Times(1);
    processor.processStats(hwStatsMap);
    Mock::VerifyAndClearExpectations(&exporter);
  }
}

} // namespace facebook::fboss::normalization
