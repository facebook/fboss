/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#pragma once

#include "fboss/agent/hw/sai/fake/FakeManager.h"

#include <vector>

extern "C" {
#include <sai.h>
}

sai_status_t create_scheduler_fn(
    sai_object_id_t* scheduler_id,
    sai_object_id_t switch_id,
    uint32_t attr_count,
    const sai_attribute_t* attr_list);

sai_status_t remove_scheduler_fn(sai_object_id_t scheduler_id);

sai_status_t set_scheduler_attribute_fn(
    sai_object_id_t scheduler_id,
    const sai_attribute_t* attr);

sai_status_t get_scheduler_attribute_fn(
    sai_object_id_t scheduler_id,
    uint32_t attr_count,
    sai_attribute_t* attr);

namespace facebook {
namespace fboss {

class FakeScheduler {
 public:
  FakeScheduler() {}
  sai_object_id_t id;
  sai_scheduling_type_t schedulingType{SAI_SCHEDULING_TYPE_WRR};
  sai_uint8_t weight{1};
  sai_uint64_t minBandwidthRate{0};
  sai_uint64_t minBandwidthBurstRate{0};
  sai_uint64_t maxBandwidthRate{0};
  sai_uint64_t maxBandwidthBurstRate{0};
};

using FakeSchedulerManager = FakeManager<sai_object_id_t, FakeScheduler>;

void populate_scheduler_api(sai_scheduler_api_t** scheduler_api);
} // namespace fboss
} // namespace facebook
