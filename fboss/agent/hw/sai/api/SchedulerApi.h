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

#include "fboss/agent/hw/sai/api/QueueApi.h"
#include "fboss/agent/hw/sai/api/SaiApi.h"
#include "fboss/agent/hw/sai/api/SaiAttribute.h"
#include "fboss/agent/hw/sai/api/SaiAttributeDataTypes.h"
#include "fboss/agent/hw/sai/api/Types.h"

#include <folly/logging/xlog.h>

#include <boost/variant.hpp>

extern "C" {
#include <sai.h>
}

namespace facebook::fboss {

class SchedulerApi;

struct SaiSchedulerTraits {
  static constexpr sai_object_type_t ObjectType = SAI_OBJECT_TYPE_SCHEDULER;
  using SaiApiT = SchedulerApi;
  struct Attributes {
    using EnumType = sai_scheduler_attr_t;
    using SchedulingType =
        SaiAttribute<EnumType, SAI_SCHEDULER_ATTR_SCHEDULING_TYPE, sai_int32_t>;
    using SchedulingWeight = SaiAttribute<
        EnumType,
        SAI_SCHEDULER_ATTR_SCHEDULING_WEIGHT,
        sai_uint8_t>;
    using MeterType =
        SaiAttribute<EnumType, SAI_SCHEDULER_ATTR_METER_TYPE, sai_int32_t>;
    using MinBandwidthRate = SaiAttribute<
        EnumType,
        SAI_SCHEDULER_ATTR_MIN_BANDWIDTH_RATE,
        sai_uint64_t>;
    using MinBandwidthBurstRate = SaiAttribute<
        EnumType,
        SAI_SCHEDULER_ATTR_MIN_BANDWIDTH_BURST_RATE,
        sai_uint64_t>;
    using MaxBandwidthRate = SaiAttribute<
        EnumType,
        SAI_SCHEDULER_ATTR_MAX_BANDWIDTH_RATE,
        sai_uint64_t>;
    using MaxBandwidthBurstRate = SaiAttribute<
        EnumType,
        SAI_SCHEDULER_ATTR_MAX_BANDWIDTH_BURST_RATE,
        sai_uint64_t>;
  };
  using AdapterKey = SchedulerSaiId;
  using AdapterHostKey = std::tuple<
      std::optional<Attributes::SchedulingType>,
      std::optional<Attributes::SchedulingWeight>,
      std::optional<Attributes::MeterType>,
      std::optional<Attributes::MinBandwidthRate>,
      std::optional<Attributes::MaxBandwidthRate>>;
  using CreateAttributes = AdapterHostKey;
};

SAI_ATTRIBUTE_NAME(Scheduler, SchedulingType)
SAI_ATTRIBUTE_NAME(Scheduler, SchedulingWeight)
SAI_ATTRIBUTE_NAME(Scheduler, MeterType)
SAI_ATTRIBUTE_NAME(Scheduler, MinBandwidthRate)
SAI_ATTRIBUTE_NAME(Scheduler, MinBandwidthBurstRate)
SAI_ATTRIBUTE_NAME(Scheduler, MaxBandwidthRate)
SAI_ATTRIBUTE_NAME(Scheduler, MaxBandwidthBurstRate)

class SchedulerApi : public SaiApi<SchedulerApi> {
 public:
  static constexpr sai_api_t ApiType = SAI_API_SCHEDULER;
  SchedulerApi() {
    sai_status_t status =
        sai_api_query(ApiType, reinterpret_cast<void**>(&api_));
    saiApiCheckError(status, ApiType, "Failed to query for scheduler api");
  }

 private:
  sai_status_t _create(
      SchedulerSaiId* id,
      sai_object_id_t switch_id,
      size_t count,
      sai_attribute_t* attr_list) {
    return api_->create_scheduler(rawSaiId(id), switch_id, count, attr_list);
  }
  sai_status_t _remove(SchedulerSaiId id) {
    return api_->remove_scheduler(id);
  }
  sai_status_t _getAttribute(SchedulerSaiId id, sai_attribute_t* attr) const {
    return api_->get_scheduler_attribute(id, 1, attr);
  }
  sai_status_t _setAttribute(SchedulerSaiId id, const sai_attribute_t* attr) {
    return api_->set_scheduler_attribute(id, attr);
  }
  sai_scheduler_api_t* api_;
  friend class SaiApi<SchedulerApi>;
};

} // namespace facebook::fboss
