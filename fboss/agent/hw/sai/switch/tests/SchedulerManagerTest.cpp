/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/FbossError.h"
#include "fboss/agent/hw/sai/api/SaiApiTable.h"
#include "fboss/agent/hw/sai/fake/FakeSai.h"
#include "fboss/agent/hw/sai/switch/SaiManagerTable.h"
#include "fboss/agent/hw/sai/switch/SaiPortManager.h"
#include "fboss/agent/hw/sai/switch/SaiQueueManager.h"
#include "fboss/agent/hw/sai/switch/SaiSchedulerManager.h"
#include "fboss/agent/hw/sai/switch/tests/ManagerTestBase.h"
#include "fboss/agent/state/PortQueue.h"
#include "fboss/agent/types.h"

#include <string>

#include <gtest/gtest.h>

using namespace facebook::fboss;

class SchedulerManagerTest : public ManagerTestBase {
 public:
  void SetUp() override {
    setupStage = SetupStage::PORT;
    ManagerTestBase::SetUp();
  }

  std::shared_ptr<SaiScheduler> createScheduler(
      cfg::QueueScheduling schedType,
      uint8_t weight,
      cfg::PortQueueRate::Type queueRateType,
      uint32_t min,
      uint64_t max) {
    PortQueue portQueue;
    cfg::PortQueueRate portQueueRate;
    cfg::Range range;
    *range.minimum_ref() = min;
    *range.maximum_ref() = max;
    if (queueRateType == cfg::PortQueueRate::Type::pktsPerSec) {
      portQueueRate.set_pktsPerSec(range);
    } else {
      portQueueRate.set_kbitsPerSec(range);
    }
    portQueue.setWeight(weight);
    portQueue.setScheduling(schedType);
    portQueue.setPortQueueRate(portQueueRate);
    return saiManagerTable->schedulerManager().createScheduler(portQueue);
  }

  void checkScheduler(
      SchedulerSaiId saiSchedulerId,
      cfg::QueueScheduling schedType,
      uint8_t weight,
      cfg::PortQueueRate::Type queueRateType,
      uint64_t min,
      uint64_t max) {
    auto& schedulerApi = saiApiTable->schedulerApi();
    SaiSchedulerTraits::Attributes::SchedulingType schedulingType;
    auto gotSchedType =
        schedulerApi.getAttribute(saiSchedulerId, schedulingType);
    SaiSchedulerTraits::Attributes::MeterType meterType;
    auto gotMeterType = schedulerApi.getAttribute(saiSchedulerId, meterType);
    if (queueRateType == cfg::PortQueueRate::Type::kbitsPerSec) {
      min = min * 1000 / 8;
      max = max * 1000 / 8;
    }
    SaiSchedulerTraits::Attributes::MinBandwidthRate minBwRate;
    auto gotMinBwRate = schedulerApi.getAttribute(saiSchedulerId, minBwRate);
    SaiSchedulerTraits::Attributes::MaxBandwidthRate maxBwRate;
    auto gotMaxBwRate = schedulerApi.getAttribute(saiSchedulerId, maxBwRate);
    SaiSchedulerTraits::Attributes::SchedulingWeight schedWeight;
    auto gotSchedWeight =
        schedulerApi.getAttribute(saiSchedulerId, schedWeight);
    auto expectedSchedType =
        (schedType == cfg::QueueScheduling::STRICT_PRIORITY
             ? SAI_SCHEDULING_TYPE_STRICT
             : SAI_SCHEDULING_TYPE_WRR);
    auto expectedMeterType =
        (queueRateType == PortQueueRate::Type::pktsPerSec
             ? SAI_METER_TYPE_PACKETS
             : SAI_METER_TYPE_BYTES);
    if (schedType == cfg::QueueScheduling::WEIGHTED_ROUND_ROBIN) {
      EXPECT_EQ(weight, gotSchedWeight);
    }
    EXPECT_EQ(expectedMeterType, gotMeterType);
    EXPECT_EQ(min, gotMinBwRate);
    EXPECT_EQ(max, gotMaxBwRate);
    EXPECT_EQ(expectedSchedType, gotSchedType);
  }
};

TEST_F(SchedulerManagerTest, createScheduler) {
  auto schedType = cfg::QueueScheduling::WEIGHTED_ROUND_ROBIN;
  uint8_t weight = 32;
  uint64_t minPps = 8000;
  uint64_t maxPps = 12000;
  auto queueRateType = cfg::PortQueueRate::Type::pktsPerSec;
  auto scheduler =
      createScheduler(schedType, weight, queueRateType, minPps, maxPps);
  checkScheduler(
      scheduler->adapterKey(),
      schedType,
      weight,
      queueRateType,
      minPps,
      maxPps);
}

TEST_F(SchedulerManagerTest, createDuplicateScheduler) {
  auto schedType = cfg::QueueScheduling::WEIGHTED_ROUND_ROBIN;
  uint8_t weight = 32;
  uint64_t minKbps = 7000;
  uint64_t maxKbps = 12000;
  auto queueRateType = cfg::PortQueueRate::Type::kbitsPerSec;
  auto scheduler1 =
      createScheduler(schedType, weight, queueRateType, minKbps, maxKbps);
  auto scheduler2 =
      createScheduler(schedType, weight, queueRateType, minKbps, maxKbps);
  EXPECT_EQ(scheduler1->adapterKey(), scheduler2->adapterKey());
  checkScheduler(
      scheduler1->adapterKey(),
      schedType,
      weight,
      queueRateType,
      minKbps,
      maxKbps);
}

TEST_F(SchedulerManagerTest, shareSchedulerWithMultipleQueues) {
  auto portHandle = saiManagerTable->portManager().getPortHandle(PortID(1));
  PortSaiId portSaiId = portHandle->port->adapterKey();
  std::vector<uint8_t> queueIds = {1, 2, 3, 4};
  auto schedType = cfg::QueueScheduling::WEIGHTED_ROUND_ROBIN;
  uint8_t weight = 32;
  uint64_t minPps = 12000;
  uint64_t maxPps = 82000;
  auto queueRateType = cfg::PortQueueRate::Type::pktsPerSec;
  auto queueConfig = makeQueueConfig(
      {queueIds},
      cfg::StreamType::UNICAST,
      cfg::QueueScheduling::WEIGHTED_ROUND_ROBIN,
      weight,
      minPps,
      maxPps);
  auto queueSaiIds = getPortQueueSaiIds(portHandle);
  auto queueHandles =
      saiManagerTable->queueManager().loadQueues(portSaiId, queueSaiIds);
  std::shared_ptr<SaiScheduler> scheduler = nullptr;
  for (auto& queueHandle : queueHandles) {
    if (!scheduler) {
      scheduler = queueHandle.second->scheduler;
    } else {
      // Only the configured port queues will have scheduler
      if (!queueHandle.second->scheduler) {
        continue;
      }
      EXPECT_EQ(
          scheduler->adapterKey(), queueHandle.second->scheduler->adapterKey());
      checkScheduler(
          scheduler->adapterKey(),
          schedType,
          weight,
          queueRateType,
          minPps,
          maxPps);
    }
  }
}

TEST_F(SchedulerManagerTest, checkSchedulerProfileIdOnQueue) {
  auto portHandle = saiManagerTable->portManager().getPortHandle(PortID(1));
  PortSaiId portSaiId = portHandle->port->adapterKey();
  std::vector<uint8_t> queueIds = {1, 2};
  uint64_t minPps = 12000;
  uint64_t maxPps = 88000;
  auto queueRateType = cfg::PortQueueRate::Type::pktsPerSec;
  auto queueConfig = makeQueueConfig(
      {queueIds},
      cfg::StreamType::UNICAST,
      cfg::QueueScheduling::STRICT_PRIORITY,
      0,
      minPps,
      maxPps);
  auto queueSaiIds = getPortQueueSaiIds(portHandle);
  auto queueHandles =
      saiManagerTable->queueManager().loadQueues(portSaiId, queueSaiIds);
  for (const auto& queueHandle : queueHandles) {
    auto scheduler = queueHandle.second->scheduler;
    if (!scheduler) {
      continue;
    }
    auto schedulerProfileId = saiApiTable->queueApi().getAttribute(
        queueHandle.second->queue->adapterKey(),
        SaiQueueTraits::Attributes::SchedulerProfileId{});
    EXPECT_EQ(scheduler->adapterKey(), schedulerProfileId);
  }
}
