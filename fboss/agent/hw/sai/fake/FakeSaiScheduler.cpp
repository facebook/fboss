/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/sai/fake/FakeSaiScheduler.h"
#include "fboss/agent/hw/sai/fake/FakeSai.h"

#include <folly/logging/xlog.h>
#include <optional>

using facebook::fboss::FakeSai;
using facebook::fboss::FakeScheduler;

sai_status_t create_scheduler_fn(
    sai_object_id_t* scheduler_id,
    sai_object_id_t /* switch_id */,
    uint32_t attr_count,
    const sai_attribute_t* attr_list) {
  std::optional<sai_scheduling_type_t> schedulingType;
  std::optional<sai_uint8_t> weight;
  std::optional<sai_meter_type_t> meterType;
  std::optional<sai_uint64_t> minBandwidthRate;
  std::optional<sai_uint64_t> minBandwidthBurstRate;
  std::optional<sai_uint64_t> maxBandwidthRate;
  std::optional<sai_uint64_t> maxBandwidthBurstRate;
  auto fs = FakeSai::getInstance();
  for (int i = 0; i < attr_count; ++i) {
    switch (attr_list[i].id) {
      case SAI_SCHEDULER_ATTR_SCHEDULING_TYPE:
        schedulingType =
            static_cast<sai_scheduling_type_t>(attr_list[i].value.s32);
        break;
      case SAI_SCHEDULER_ATTR_SCHEDULING_WEIGHT:
        weight = attr_list[i].value.u8;
        break;
      case SAI_SCHEDULER_ATTR_METER_TYPE:
        meterType = static_cast<sai_meter_type_t>(attr_list[i].value.s32);
        break;
      case SAI_SCHEDULER_ATTR_MIN_BANDWIDTH_RATE:
        minBandwidthRate = attr_list[i].value.s32;
        break;
      case SAI_SCHEDULER_ATTR_MIN_BANDWIDTH_BURST_RATE:
        minBandwidthBurstRate = attr_list[i].value.s32;
        break;
      case SAI_SCHEDULER_ATTR_MAX_BANDWIDTH_RATE:
        maxBandwidthRate = attr_list[i].value.s32;
        break;
      case SAI_SCHEDULER_ATTR_MAX_BANDWIDTH_BURST_RATE:
        maxBandwidthBurstRate = attr_list[i].value.s32;
        break;
      default:
        return SAI_STATUS_INVALID_PARAMETER;
    }
  }
  *scheduler_id = fs->scheduleManager.create();
  auto& scheduler = fs->scheduleManager.get(*scheduler_id);
  if (schedulingType) {
    scheduler.schedulingType = schedulingType.value();
  }
  if (weight) {
    scheduler.weight = weight.value();
  }
  if (meterType) {
    scheduler.meterType = meterType.value();
  }
  if (minBandwidthRate) {
    scheduler.minBandwidthRate = minBandwidthRate.value();
  }
  if (minBandwidthBurstRate) {
    scheduler.minBandwidthBurstRate = minBandwidthBurstRate.value();
  }
  if (maxBandwidthRate) {
    scheduler.maxBandwidthRate = maxBandwidthRate.value();
  }
  if (maxBandwidthBurstRate) {
    scheduler.maxBandwidthBurstRate = maxBandwidthBurstRate.value();
  }
  return SAI_STATUS_SUCCESS;
}

sai_status_t remove_scheduler_fn(sai_object_id_t scheduler_id) {
  auto fs = FakeSai::getInstance();
  fs->scheduleManager.remove(scheduler_id);
  return SAI_STATUS_SUCCESS;
}

sai_status_t set_scheduler_attribute_fn(
    sai_object_id_t scheduler_id,
    const sai_attribute_t* attr) {
  auto fs = FakeSai::getInstance();
  auto& scheduler = fs->scheduleManager.get(scheduler_id);
  sai_status_t res = SAI_STATUS_SUCCESS;
  if (!attr) {
    return SAI_STATUS_INVALID_PARAMETER;
  }
  switch (attr->id) {
    case SAI_SCHEDULER_ATTR_SCHEDULING_TYPE:
      scheduler.schedulingType =
          static_cast<sai_scheduling_type_t>(attr->value.s32);
      break;
    case SAI_SCHEDULER_ATTR_SCHEDULING_WEIGHT:
      scheduler.weight = attr->value.u8;
      break;
    case SAI_SCHEDULER_ATTR_METER_TYPE:
      scheduler.meterType = static_cast<sai_meter_type_t>(attr->value.s32);
      break;
    case SAI_SCHEDULER_ATTR_MIN_BANDWIDTH_RATE:
      scheduler.minBandwidthRate = attr->value.u64;
      break;
    case SAI_SCHEDULER_ATTR_MIN_BANDWIDTH_BURST_RATE:
      scheduler.minBandwidthBurstRate = attr->value.u64;
      break;
    case SAI_SCHEDULER_ATTR_MAX_BANDWIDTH_RATE:
      scheduler.maxBandwidthRate = attr->value.u64;
      break;
    case SAI_SCHEDULER_ATTR_MAX_BANDWIDTH_BURST_RATE:
      scheduler.maxBandwidthBurstRate = attr->value.u64;
      break;
    default:
      res = SAI_STATUS_INVALID_PARAMETER;
      break;
  }
  return res;
}

sai_status_t get_scheduler_attribute_fn(
    sai_object_id_t scheduler_id,
    uint32_t attr_count,
    sai_attribute_t* attr) {
  auto fs = FakeSai::getInstance();
  auto scheduler = fs->scheduleManager.get(scheduler_id);
  for (int i = 0; i < attr_count; ++i) {
    switch (attr[i].id) {
      case SAI_SCHEDULER_ATTR_SCHEDULING_TYPE:
        attr[i].value.s32 = static_cast<sai_int32_t>(scheduler.schedulingType);
        break;
      case SAI_SCHEDULER_ATTR_SCHEDULING_WEIGHT:
        attr[i].value.u8 = scheduler.weight;
        break;
      case SAI_SCHEDULER_ATTR_METER_TYPE:
        attr[i].value.s32 = static_cast<sai_int32_t>(scheduler.meterType);
        break;
      case SAI_SCHEDULER_ATTR_MIN_BANDWIDTH_RATE:
        attr[i].value.u64 = scheduler.minBandwidthRate;
        break;
      case SAI_SCHEDULER_ATTR_MAX_BANDWIDTH_RATE:
        attr[i].value.u64 = scheduler.maxBandwidthRate;
        break;
      case SAI_SCHEDULER_ATTR_MIN_BANDWIDTH_BURST_RATE:
        attr[i].value.u64 = scheduler.minBandwidthBurstRate;
        break;
      case SAI_SCHEDULER_ATTR_MAX_BANDWIDTH_BURST_RATE:
        attr[i].value.u64 = scheduler.maxBandwidthBurstRate;
        break;
      default:
        return SAI_STATUS_INVALID_PARAMETER;
    }
  }
  return SAI_STATUS_SUCCESS;
}

namespace facebook::fboss {

static sai_scheduler_api_t _scheduler_api;

void populate_scheduler_api(sai_scheduler_api_t** scheduler_api) {
  _scheduler_api.create_scheduler = &create_scheduler_fn;
  _scheduler_api.remove_scheduler = &remove_scheduler_fn;
  _scheduler_api.set_scheduler_attribute = &set_scheduler_attribute_fn;
  _scheduler_api.get_scheduler_attribute = &get_scheduler_attribute_fn;
  *scheduler_api = &_scheduler_api;
}

} // namespace facebook::fboss
