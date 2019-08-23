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

namespace facebook {
namespace fboss {

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
  using AdapterHostKey = std::set<typename SaiQueueTraits::AdapterHostKey>;
  using CreateAttributes = std::tuple<
      std::optional<Attributes::SchedulingType>,
      std::optional<Attributes::SchedulingWeight>,
      std::optional<Attributes::MaxBandwidthRate>>;
};

struct SchedulerApiParameters {
  static constexpr sai_api_t ApiType = SAI_API_SCHEDULER;
  struct Attributes {
    using EnumType = sai_scheduler_attr_t;
    using SchedulingType =
        SaiAttribute<EnumType, SAI_SCHEDULER_ATTR_SCHEDULING_TYPE, sai_int32_t>;
    using SchedulingWeight = SaiAttribute<
        EnumType,
        SAI_SCHEDULER_ATTR_SCHEDULING_WEIGHT,
        sai_uint8_t>;
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

    using CreateAttributes = SaiAttributeTuple<
        SaiAttributeOptional<SchedulingType>,
        SaiAttributeOptional<SchedulingWeight>,
        SaiAttributeOptional<MaxBandwidthRate>>;
    Attributes(const CreateAttributes& attrs) {
      std::tie(schedulingType, schedulingWeight, maxBandwidthRate) =
          attrs.value();
    }
    CreateAttributes attrs() const {
      return {schedulingType, schedulingWeight, maxBandwidthRate};
    }
    bool operator==(const Attributes& other) const {
      return attrs() == other.attrs();
    }
    bool operator!=(const Attributes& other) const {
      return !(*this == other);
    }
    folly::Optional<typename SchedulingType::ValueType> schedulingType;
    folly::Optional<typename SchedulingWeight::ValueType> schedulingWeight;
    folly::Optional<typename MinBandwidthRate::ValueType> minBandwidthRate;
    folly::Optional<typename MinBandwidthBurstRate::ValueType>
        minBandwidthBurstRate;
    folly::Optional<typename MaxBandwidthRate::ValueType> maxBandwidthRate;
    folly::Optional<typename MaxBandwidthBurstRate::ValueType>
        maxBandwidthBurstRate;
  };
};

class SchedulerApi : public SaiApi<SchedulerApi, SchedulerApiParameters> {
 public:
  SchedulerApi() {
    sai_status_t status =
        sai_api_query(SAI_API_SCHEDULER, reinterpret_cast<void**>(&api_));
    saiApiCheckError(
        status,
        SchedulerApiParameters::ApiType,
        "Failed to query for scheduler api");
  }

 private:
  sai_status_t _create(
      sai_object_id_t* scheduler_id,
      sai_attribute_t* attr_list,
      size_t count,
      sai_object_id_t switch_id) {
    return api_->create_scheduler(scheduler_id, switch_id, count, attr_list);
  }
  sai_status_t _remove(sai_object_id_t scheduler_id) {
    return api_->remove_scheduler(scheduler_id);
  }
  sai_status_t _getAttr(sai_attribute_t* attr, sai_object_id_t scheduler_id)
      const {
    return api_->get_scheduler_attribute(scheduler_id, 1, attr);
  }
  sai_status_t _setAttr(
      const sai_attribute_t* attr,
      sai_object_id_t scheduler_id) {
    return api_->set_scheduler_attribute(scheduler_id, attr);
  }
  sai_scheduler_api_t* api_;
  friend class SaiApi<SchedulerApi, SchedulerApiParameters>;
};

} // namespace fboss
} // namespace facebook
