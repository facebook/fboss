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
namespace facebook::fboss {

class FakeScheduler {
 public:
  FakeScheduler() {}
  sai_object_id_t id;
  sai_scheduling_type_t schedulingType{SAI_SCHEDULING_TYPE_WRR};
  sai_uint8_t weight{1};
  sai_meter_type_t meterType{SAI_METER_TYPE_BYTES};
  sai_uint64_t minBandwidthRate{0};
  sai_uint64_t minBandwidthBurstRate{0};
  sai_uint64_t maxBandwidthRate{0};
  sai_uint64_t maxBandwidthBurstRate{0};
};

using FakeSchedulerManager = FakeManager<sai_object_id_t, FakeScheduler>;

void populate_scheduler_api(sai_scheduler_api_t** scheduler_api);

} // namespace facebook::fboss
