/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/sai/tracer/SchedulerApiTracer.h"
#include <typeindex>
#include <utility>

#include "fboss/agent/hw/sai/api/SchedulerApi.h"
#include "fboss/agent/hw/sai/tracer/Utils.h"

using folly::to;

namespace {
std::map<int32_t, std::pair<std::string, std::size_t>> _SchedulerMap{
    SAI_ATTR_MAP(Scheduler, SchedulingType),
    SAI_ATTR_MAP(Scheduler, SchedulingWeight),
    SAI_ATTR_MAP(Scheduler, MeterType),
    SAI_ATTR_MAP(Scheduler, MinBandwidthRate),
    SAI_ATTR_MAP(Scheduler, MinBandwidthBurstRate),
    SAI_ATTR_MAP(Scheduler, MaxBandwidthRate),
    SAI_ATTR_MAP(Scheduler, MaxBandwidthBurstRate),
};
} // namespace

namespace facebook::fboss {

WRAP_CREATE_FUNC(scheduler, SAI_OBJECT_TYPE_SCHEDULER, scheduler);
WRAP_REMOVE_FUNC(scheduler, SAI_OBJECT_TYPE_SCHEDULER, scheduler);
WRAP_SET_ATTR_FUNC(scheduler, SAI_OBJECT_TYPE_SCHEDULER, scheduler);
WRAP_GET_ATTR_FUNC(scheduler, SAI_OBJECT_TYPE_SCHEDULER, scheduler);

sai_scheduler_api_t* wrappedSchedulerApi() {
  static sai_scheduler_api_t schedulerWrappers;

  schedulerWrappers.create_scheduler = &wrap_create_scheduler;
  schedulerWrappers.remove_scheduler = &wrap_remove_scheduler;
  schedulerWrappers.set_scheduler_attribute = &wrap_set_scheduler_attribute;
  schedulerWrappers.get_scheduler_attribute = &wrap_get_scheduler_attribute;

  return &schedulerWrappers;
}

SET_SAI_ATTRIBUTES(Scheduler)

} // namespace facebook::fboss
