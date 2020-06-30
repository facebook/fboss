/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/sai/switch/SaiSchedulerManager.h"
#include "fboss/agent/hw/sai/store/SaiStore.h"
#include "fboss/agent/hw/sai/switch/SaiManagerTable.h"
#include "fboss/agent/hw/sai/switch/SaiSwitchManager.h"
#include "fboss/lib/TupleUtils.h"

using facebook::fboss::PortQueue;
using facebook::fboss::SaiApiTable;
using facebook::fboss::SaiSchedulerTraits;
using facebook::fboss::cfg::PortQueueRate;
using facebook::fboss::cfg::QueueScheduling;

namespace {

SaiSchedulerTraits::CreateAttributes makeSchedulerAttributes(
    const PortQueue& portQueue) {
  sai_scheduling_type_t type = SAI_SCHEDULING_TYPE_STRICT;
  uint8_t weight = 0;
  if (portQueue.getScheduling() == QueueScheduling::WEIGHTED_ROUND_ROBIN) {
    type = SAI_SCHEDULING_TYPE_WRR;
    weight = portQueue.getWeight();
  }
  uint64_t minBwRate = 0, maxBwRate = 0;
  int32_t meterType = SAI_METER_TYPE_BYTES;
  if (portQueue.getPortQueueRate().has_value()) {
    auto portQueueRate = portQueue.getPortQueueRate().value();
    switch (portQueueRate.getType()) {
      case PortQueueRate::Type::pktsPerSec:
        meterType = SAI_METER_TYPE_PACKETS;
        minBwRate = *portQueueRate.get_pktsPerSec().minimum_ref();
        maxBwRate = *portQueueRate.get_pktsPerSec().maximum_ref();
        break;
      case PortQueueRate::Type::kbitsPerSec:
        /*
         * config sets the rate in kbps (kilo bits per second) whereas SAI
         * expects it in bytes per second.
         */
        meterType = SAI_METER_TYPE_BYTES;
        minBwRate = static_cast<uint64_t>(
                        *portQueueRate.get_kbitsPerSec().minimum_ref()) *
            1000 / 8;
        maxBwRate = static_cast<uint64_t>(
                        *portQueueRate.get_kbitsPerSec().maximum_ref()) *
            1000 / 8;
        break;
      default:
        break;
    }
  }
  // weight 0 is invalid
  return SaiSchedulerTraits::CreateAttributes(
      {type, !weight ? 1 : weight, meterType, minBwRate, maxBwRate});
}

} // namespace

namespace facebook::fboss {

SaiSchedulerManager::SaiSchedulerManager(
    SaiManagerTable* managerTable,
    const SaiPlatform* platform)
    : managerTable_(managerTable), platform_(platform) {}

std::shared_ptr<SaiScheduler> SaiSchedulerManager::createScheduler(
    const PortQueue& portQueue) {
  auto attributes = makeSchedulerAttributes(portQueue);
  SaiSchedulerTraits::AdapterHostKey k = tupleProjection<
      SaiSchedulerTraits::CreateAttributes,
      SaiSchedulerTraits::AdapterHostKey>(attributes);
  auto& store = SaiStore::getInstance()->get<SaiSchedulerTraits>();
  return store.setObject(k, attributes);
}

void SaiSchedulerManager::fillSchedulerSettings(
    const SaiScheduler* scheduler,
    PortQueue* portQueue) const {
  if (!scheduler) {
    return;
  }
  auto weight =
      GET_OPT_ATTR(Scheduler, SchedulingWeight, scheduler->attributes());
  auto schedulingType =
      GET_OPT_ATTR(Scheduler, SchedulingType, scheduler->attributes());
  auto meterType = GET_OPT_ATTR(Scheduler, MeterType, scheduler->attributes());
  auto minBwRate =
      GET_OPT_ATTR(Scheduler, MinBandwidthRate, scheduler->attributes());
  auto maxBwRate =
      GET_OPT_ATTR(Scheduler, MaxBandwidthRate, scheduler->attributes());
  portQueue->setWeight(weight);
  portQueue->setScheduling(
      schedulingType == SAI_SCHEDULING_TYPE_WRR
          ? QueueScheduling::WEIGHTED_ROUND_ROBIN
          : QueueScheduling::STRICT_PRIORITY);
  cfg::Range range;
  range.minimum_ref() = minBwRate;
  range.maximum_ref() = maxBwRate;
  cfg::PortQueueRate portQueueRate;
  if (meterType == SAI_METER_TYPE_BYTES) {
    portQueueRate.set_kbitsPerSec(range);
  } else {
    portQueueRate.set_pktsPerSec(range);
  }
  portQueue->setPortQueueRate(portQueueRate);
}

} // namespace facebook::fboss
