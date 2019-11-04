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
    EXPECT_EQ(fs->scm.get(schedulerId).schedulingType, gotSchedType);
    EXPECT_EQ(fs->scm.get(schedulerId).weight, gotSchedWeight);
    EXPECT_EQ(fs->scm.get(schedulerId).maxBandwidthRate, gotMaxBwRate);
    EXPECT_EQ(fs->scm.get(schedulerId).minBandwidthRate, gotMinBwRate);
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
  EXPECT_EQ(fs->scm.map().size(), 1);
  schedulerApi->remove(saiSchedulerId);
  EXPECT_EQ(fs->scm.map().size(), 0);
}
