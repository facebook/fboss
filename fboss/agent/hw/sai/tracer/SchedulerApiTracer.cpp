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
#include "fboss/agent/hw/sai/tracer/Utils.h"

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

void setSchedulerAttributes(
    const sai_attribute_t* attr_list,
    uint32_t attr_count,
    std::vector<std::string>& attrLines) {
  for (int i = 0; i < attr_count; ++i) {
    switch (attr_list[i].id) {
      case SAI_SCHEDULER_ATTR_SCHEDULING_TYPE:
      case SAI_SCHEDULER_ATTR_METER_TYPE:
        attrLines.push_back(s32Attr(attr_list, i));
        break;
      case SAI_SCHEDULER_ATTR_SCHEDULING_WEIGHT:
        attrLines.push_back(u8Attr(attr_list, i));
        break;
      case SAI_SCHEDULER_ATTR_MIN_BANDWIDTH_RATE:
      case SAI_SCHEDULER_ATTR_MIN_BANDWIDTH_BURST_RATE:
      case SAI_SCHEDULER_ATTR_MAX_BANDWIDTH_RATE:
      case SAI_SCHEDULER_ATTR_MAX_BANDWIDTH_BURST_RATE:
        attrLines.push_back(u64Attr(attr_list, i));
        break;
      default:
        break;
    }
  }
}

} // namespace facebook::fboss
