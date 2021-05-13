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

#include "fboss/agent/normalization/StatsExporter.h"

using namespace ::testing;

namespace facebook::fboss::normalization {

TEST(StatsExporterTest, publishPortStats) {
  gflags::FlagSaver __;
  FLAGS_normalized_counter_entity_prefix = "test_";

  auto statsExporter = GlogStatsExporter("dev");
  auto tags = std::make_shared<std::vector<std::string>>(
      std::vector<std::string>{"tag1", "tag2"});

  statsExporter.publishPortStats("eth1", "input_bps", 30, 100, 60, tags);
  statsExporter.publishPortStats("eth1", "output_bps", 30, 200, 15, tags);

  auto counterBuffer = statsExporter.getCounterBuffer();

  EXPECT_THAT(counterBuffer, SizeIs(2));
  EXPECT_EQ(counterBuffer[0].entity, "test_dev:eth1.FBNet");
  EXPECT_EQ(counterBuffer[0].key, "FBNet:interface.input_bps");
  EXPECT_EQ(counterBuffer[0].unixTime, 30);
  EXPECT_EQ(counterBuffer[0].value, 100);
  EXPECT_EQ(counterBuffer[0].intervalSec, 60);
  EXPECT_THAT(*counterBuffer[0].tags, ElementsAre("tag1", "tag2"));

  EXPECT_EQ(counterBuffer[1].entity, "test_dev:eth1.FBNet");
  EXPECT_EQ(counterBuffer[1].key, "FBNet:interface.output_bps");
  EXPECT_EQ(counterBuffer[1].unixTime, 30);
  EXPECT_EQ(counterBuffer[1].value, 200);
  EXPECT_EQ(counterBuffer[1].intervalSec, 15);
  EXPECT_THAT(*counterBuffer[1].tags, ElementsAre("tag1", "tag2"));

  statsExporter.flushCounters();
  EXPECT_THAT(statsExporter.getCounterBuffer(), IsEmpty());
}

} // namespace facebook::fboss::normalization
