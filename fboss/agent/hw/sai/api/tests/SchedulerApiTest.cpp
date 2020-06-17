/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/sai/api/SchedulerApi.h"
#include "fboss/agent/hw/sai/fake/FakeSai.h"

#include <folly/logging/xlog.h>

#include <gtest/gtest.h>

using namespace facebook::fboss;

class SchedulerApiTest : public ::testing::Test {
 public:
  void SetUp() override {
    fs = FakeSai::getInstance();
    sai_api_initialize(0, nullptr);
    schedulerApi = std::make_unique<SchedulerApi>();
  }
  std::shared_ptr<FakeSai> fs;
  std::unique_ptr<SchedulerApi> schedulerApi;

  SchedulerSaiId createScheduler(
      bool wrr,
      bool bytes,
      uint8_t schedWeight,
      uint64_t minRate,
      uint64_t maxRate) {
    SaiSchedulerTraits::Attributes::SchedulingType type(
        wrr ? SAI_SCHEDULING_TYPE_STRICT : SAI_SCHEDULING_TYPE_WRR);
    SaiSchedulerTraits::Attributes::MeterType meterType(
        bytes ? SAI_METER_TYPE_BYTES : SAI_METER_TYPE_PACKETS);
    SaiSchedulerTraits::Attributes::SchedulingWeight weight(schedWeight);
    SaiSchedulerTraits::Attributes::MinBandwidthRate minBwRate(minRate);
    SaiSchedulerTraits::Attributes::MaxBandwidthRate maxBwRate(maxRate);
    SaiSchedulerTraits::CreateAttributes a{
        type, weight, meterType, minBwRate, maxBwRate};
    auto saiSchedulerId = schedulerApi->create<SaiSchedulerTraits>(a, 0);
    return saiSchedulerId;
  }

  void checkScheduler(SchedulerSaiId schedulerId) {
    SaiSchedulerTraits::Attributes::SchedulingType schedTypeAttribute;
    auto gotSchedType =
        schedulerApi->getAttribute(schedulerId, schedTypeAttribute);
    SaiSchedulerTraits::Attributes::SchedulingWeight scheduWeightAttribute;
    auto gotSchedWeight =
        schedulerApi->getAttribute(schedulerId, scheduWeightAttribute);
    SaiSchedulerTraits::Attributes::MinBandwidthRate minBwRateAttribute;
    auto gotMinBwRate =
        schedulerApi->getAttribute(schedulerId, minBwRateAttribute);
    SaiSchedulerTraits::Attributes::MaxBandwidthRate maxBwRateAttribute;
    auto gotMaxBwRate =
        schedulerApi->getAttribute(schedulerId, maxBwRateAttribute);
    EXPECT_EQ(
        fs->scheduleManager.get(schedulerId).schedulingType, gotSchedType);
    EXPECT_EQ(fs->scheduleManager.get(schedulerId).weight, gotSchedWeight);
    EXPECT_EQ(
        fs->scheduleManager.get(schedulerId).maxBandwidthRate, gotMaxBwRate);
    EXPECT_EQ(
        fs->scheduleManager.get(schedulerId).minBandwidthRate, gotMinBwRate);
  }
};

TEST_F(SchedulerApiTest, createScheduler) {
  uint64_t minBwRate = 6000;
  uint64_t maxBwRate = 8000;
  uint8_t weight = 240;
  auto saiSchedulerId =
      createScheduler(true, true, weight, minBwRate, maxBwRate);
  checkScheduler(saiSchedulerId);
  schedulerApi->remove(saiSchedulerId);
}

TEST_F(SchedulerApiTest, removeScheduler) {
  uint64_t minBwRate = 3734;
  uint64_t maxBwRate = 12000;
  uint8_t weight = 192;
  auto saiSchedulerId =
      createScheduler(false, false, weight, minBwRate, maxBwRate);
  checkScheduler(saiSchedulerId);
  EXPECT_EQ(fs->scheduleManager.map().size(), 1);
  schedulerApi->remove(saiSchedulerId);
  EXPECT_EQ(fs->scheduleManager.map().size(), 0);
}

TEST_F(SchedulerApiTest, getSchedulerOptionalAttribute) {
  uint64_t minBwRate = 6000;
  uint64_t maxBwRate = 8000;
  uint8_t weight = 240;
  auto saiSchedulerId =
      createScheduler(true, true, weight, minBwRate, maxBwRate);
  checkScheduler(saiSchedulerId);

  auto maxBwRateGot = schedulerApi->getAttribute(
      saiSchedulerId, SaiSchedulerTraits::Attributes::MaxBandwidthRate());
  auto minBwRateGot = schedulerApi->getAttribute(
      saiSchedulerId, SaiSchedulerTraits::Attributes::MinBandwidthRate());
  auto maxBwBurstRateGot = schedulerApi->getAttribute(
      saiSchedulerId, SaiSchedulerTraits::Attributes::MaxBandwidthBurstRate());
  auto minBwBurstRateGot = schedulerApi->getAttribute(
      saiSchedulerId, SaiSchedulerTraits::Attributes::MinBandwidthBurstRate());

  // Min/max bandwidth rates are required and set during creation
  // Min/max bandwidth burst rates are not set (default value of 0)
  EXPECT_EQ(maxBwRateGot, maxBwRate);
  EXPECT_EQ(minBwRateGot, minBwRate);
  EXPECT_EQ(maxBwBurstRateGot, 0);
  EXPECT_EQ(minBwBurstRateGot, 0);
}

TEST_F(SchedulerApiTest, setSchedulerAttribute) {
  auto saiSchedulerId = createScheduler(true, true, 240, 6000, 8000);
  checkScheduler(saiSchedulerId);

  SaiSchedulerTraits::Attributes::SchedulingType schedulingType{
      SAI_SCHEDULING_TYPE_WRR};
  SaiSchedulerTraits::Attributes::SchedulingWeight schedulingWeight{42};
  SaiSchedulerTraits::Attributes::MeterType meterType{SAI_METER_TYPE_PACKETS};
  SaiSchedulerTraits::Attributes::MinBandwidthRate minBwRate{5000};
  SaiSchedulerTraits::Attributes::MaxBandwidthRate maxBwRate{9000};
  SaiSchedulerTraits::Attributes::MinBandwidthBurstRate minBwBurstRate{8000};
  SaiSchedulerTraits::Attributes::MaxBandwidthBurstRate maxBwBurstRate{9999};

  schedulerApi->setAttribute(saiSchedulerId, schedulingType);
  schedulerApi->setAttribute(saiSchedulerId, schedulingWeight);
  schedulerApi->setAttribute(saiSchedulerId, meterType);
  schedulerApi->setAttribute(saiSchedulerId, minBwRate);
  schedulerApi->setAttribute(saiSchedulerId, maxBwRate);
  schedulerApi->setAttribute(saiSchedulerId, minBwBurstRate);
  schedulerApi->setAttribute(saiSchedulerId, maxBwBurstRate);

  EXPECT_EQ(
      schedulingType,
      schedulerApi->getAttribute(
          saiSchedulerId, SaiSchedulerTraits::Attributes::SchedulingType()));
  EXPECT_EQ(
      schedulingWeight,
      schedulerApi->getAttribute(
          saiSchedulerId, SaiSchedulerTraits::Attributes::SchedulingWeight()));
  EXPECT_EQ(
      meterType,
      schedulerApi->getAttribute(
          saiSchedulerId, SaiSchedulerTraits::Attributes::MeterType()));
  EXPECT_EQ(
      minBwRate,
      schedulerApi->getAttribute(
          saiSchedulerId, SaiSchedulerTraits::Attributes::MinBandwidthRate()));
  EXPECT_EQ(
      maxBwRate,
      schedulerApi->getAttribute(
          saiSchedulerId, SaiSchedulerTraits::Attributes::MaxBandwidthRate()));
  EXPECT_EQ(
      minBwBurstRate,
      schedulerApi->getAttribute(
          saiSchedulerId,
          SaiSchedulerTraits::Attributes::MinBandwidthBurstRate()));
  EXPECT_EQ(
      maxBwBurstRate,
      schedulerApi->getAttribute(
          saiSchedulerId,
          SaiSchedulerTraits::Attributes::MaxBandwidthBurstRate()));
}

TEST_F(SchedulerApiTest, formatSchedulerAttributes) {
  SaiSchedulerTraits::Attributes::SchedulingType st(SAI_SCHEDULING_TYPE_WRR);
  EXPECT_EQ("SchedulingType: 1", fmt::format("{}", st));
  SaiSchedulerTraits::Attributes::SchedulingWeight sw(100);
  EXPECT_EQ("SchedulingWeight: 100", fmt::format("{}", sw));
  SaiSchedulerTraits::Attributes::MeterType mt(SAI_METER_TYPE_BYTES);
  EXPECT_EQ("MeterType: 1", fmt::format("{}", mt));
  SaiSchedulerTraits::Attributes::MinBandwidthRate minbr(100000000);
  EXPECT_EQ("MinBandwidthRate: 100000000", fmt::format("{}", minbr));
  SaiSchedulerTraits::Attributes::MinBandwidthBurstRate minbbr(10000000000);
  EXPECT_EQ("MinBandwidthBurstRate: 10000000000", fmt::format("{}", minbbr));
  SaiSchedulerTraits::Attributes::MaxBandwidthRate maxbr(400000000);
  EXPECT_EQ("MaxBandwidthRate: 400000000", fmt::format("{}", maxbr));
  SaiSchedulerTraits::Attributes::MaxBandwidthBurstRate maxbbr(40000000000);
  EXPECT_EQ("MaxBandwidthBurstRate: 40000000000", fmt::format("{}", maxbbr));
}
