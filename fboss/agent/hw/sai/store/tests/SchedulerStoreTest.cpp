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
#include "fboss/agent/hw/sai/store/SaiObject.h"
#include "fboss/agent/hw/sai/store/SaiStore.h"
#include "fboss/agent/hw/sai/store/tests/SaiStoreTest.h"

using namespace facebook::fboss;

class SchedulerStoreTest : public SaiStoreTest {
 public:
  void SetUp() override {
    fs = FakeSai::getInstance();
    sai_api_initialize(0, nullptr);
    saiApiTable = SaiApiTable::getInstance();
    saiApiTable->queryApis();
  }
  std::shared_ptr<FakeSai> fs;
  std::shared_ptr<SaiApiTable> saiApiTable;

  SchedulerSaiId createScheduler(
      sai_scheduling_type_t type,
      uint8_t weight,
      sai_meter_type_t meterType,
      uint64_t minBwRate,
      uint64_t maxBwRate) {
    SaiSchedulerTraits::CreateAttributes c =
        SaiSchedulerTraits::CreateAttributes{
            type, weight, meterType, minBwRate, maxBwRate};
    return saiApiTable->schedulerApi().create<SaiSchedulerTraits>(c, 0);
  }
};

TEST_F(SchedulerStoreTest, loadScheduler) {
  // create a Scheduler
  auto id = createScheduler(
      SAI_SCHEDULING_TYPE_STRICT, 0, SAI_METER_TYPE_BYTES, 1200, 40000);
  SaiStore s(0);
  s.reload();
  auto& store = s.get<SaiSchedulerTraits>();
  SaiSchedulerTraits::AdapterHostKey k{
      SAI_SCHEDULING_TYPE_STRICT, 0, SAI_METER_TYPE_BYTES, 1200, 40000};
  auto got = store.get(k);
  EXPECT_EQ(got->adapterKey(), id);
}

TEST_F(SchedulerStoreTest, schedulerLoadCtor) {
  auto id = createScheduler(
      SAI_SCHEDULING_TYPE_WRR, 10, SAI_METER_TYPE_BYTES, 20000, 65000);
  SaiObject<SaiSchedulerTraits> obj(id);
  EXPECT_EQ(obj.adapterKey(), id);
  EXPECT_EQ(
      GET_OPT_ATTR(Scheduler, SchedulingType, obj.attributes()),
      SAI_SCHEDULING_TYPE_WRR);
  EXPECT_EQ(GET_OPT_ATTR(Scheduler, SchedulingWeight, obj.attributes()), 10);
  EXPECT_EQ(GET_OPT_ATTR(Scheduler, MinBandwidthRate, obj.attributes()), 20000);
  EXPECT_EQ(GET_OPT_ATTR(Scheduler, MaxBandwidthRate, obj.attributes()), 65000);
}

TEST_F(SchedulerStoreTest, schedulerCreateCtor) {
  SaiSchedulerTraits::AdapterHostKey k{
      SAI_SCHEDULING_TYPE_DWRR, 24, SAI_METER_TYPE_BYTES, 21000, 81892};
  SaiSchedulerTraits::CreateAttributes c = SaiSchedulerTraits::CreateAttributes{
      SAI_SCHEDULING_TYPE_DWRR, 24, SAI_METER_TYPE_BYTES, 21000, 81892};
  SaiObject<SaiSchedulerTraits> obj(k, c, 0);
  EXPECT_EQ(
      GET_OPT_ATTR(Scheduler, SchedulingType, obj.attributes()),
      SAI_SCHEDULING_TYPE_DWRR);
  EXPECT_EQ(GET_OPT_ATTR(Scheduler, SchedulingWeight, obj.attributes()), 24);
  EXPECT_EQ(GET_OPT_ATTR(Scheduler, MinBandwidthRate, obj.attributes()), 21000);
  EXPECT_EQ(GET_OPT_ATTR(Scheduler, MaxBandwidthRate, obj.attributes()), 81892);
}

TEST_F(SchedulerStoreTest, serDeser) {
  // create a Scheduler
  auto id = createScheduler(
      SAI_SCHEDULING_TYPE_STRICT, 0, SAI_METER_TYPE_BYTES, 1200, 40000);
  verifyAdapterKeySerDeser<SaiSchedulerTraits>({id});
}

TEST_F(SchedulerStoreTest, toStr) {
  // create a Scheduler
  std::ignore = createScheduler(
      SAI_SCHEDULING_TYPE_STRICT, 0, SAI_METER_TYPE_BYTES, 1200, 40000);
  verifyToStr<SaiSchedulerTraits>();
}
