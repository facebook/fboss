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

#include "fboss/agent/hw/sai/api/SaiApi.h"
#include "fboss/agent/hw/sai/api/SaiAttribute.h"
#include "fboss/agent/hw/sai/api/SaiAttributeDataTypes.h"
#include "fboss/agent/hw/sai/api/SaiVersion.h"
#include "fboss/agent/hw/sai/api/Types.h"

#include <folly/logging/xlog.h>

#include <optional>
#include <tuple>

extern "C" {
#include <sai.h>
}

namespace facebook::fboss {

class QueueApi;

struct SaiQueueTraits {
  static constexpr sai_object_type_t ObjectType = SAI_OBJECT_TYPE_QUEUE;
  using SaiApiT = QueueApi;
  struct Attributes {
    using EnumType = sai_queue_attr_t;
    using Type = SaiAttribute<EnumType, SAI_QUEUE_ATTR_TYPE, sai_int32_t>;
    using Port = SaiAttribute<EnumType, SAI_QUEUE_ATTR_PORT, SaiObjectIdT>;
    using Index = SaiAttribute<EnumType, SAI_QUEUE_ATTR_INDEX, sai_uint8_t>;

    using ParentSchedulerNode = SaiAttribute<
        EnumType,
        SAI_QUEUE_ATTR_PARENT_SCHEDULER_NODE,
        SaiObjectIdT,
        SaiObjectIdDefault>;
    using WredProfileId = SaiAttribute<
        EnumType,
        SAI_QUEUE_ATTR_WRED_PROFILE_ID,
        SaiObjectIdT,
        SaiObjectIdDefault>;
    using BufferProfileId = SaiAttribute<
        EnumType,
        SAI_QUEUE_ATTR_BUFFER_PROFILE_ID,
        SaiObjectIdT,
        SaiObjectIdDefault>;
    using SchedulerProfileId = SaiAttribute<
        EnumType,
        SAI_QUEUE_ATTR_SCHEDULER_PROFILE_ID,
        SaiObjectIdT,
        SaiObjectIdDefault>;
  };
  using AdapterKey = QueueSaiId;
  using AdapterHostKey =
      std::tuple<Attributes::Type, Attributes::Port, Attributes::Index>;
  using CreateAttributes = std::tuple<
      Attributes::Type,
      Attributes::Port,
      Attributes::Index,
      std::optional<Attributes::ParentSchedulerNode>,
      std::optional<Attributes::SchedulerProfileId>,
      std::optional<Attributes::WredProfileId>,
      std::optional<Attributes::BufferProfileId>>;
  static constexpr std::array<sai_stat_id_t, 4> CounterIdsToRead = {
      SAI_QUEUE_STAT_PACKETS,
      SAI_QUEUE_STAT_BYTES,
      SAI_QUEUE_STAT_DROPPED_PACKETS,
      SAI_QUEUE_STAT_DROPPED_BYTES,
  };
  static constexpr std::array<sai_stat_id_t, 2> VoqCounterIdsToRead = {
      SAI_QUEUE_STAT_BYTES,
      SAI_QUEUE_STAT_DROPPED_BYTES,
  };

#if SAI_API_VERSION >= SAI_VERSION(1, 12, 0)
  static constexpr std::array<sai_stat_id_t, 1>
      VoqWatchDogDeleteCounterIdsToRead = {
          SAI_QUEUE_STAT_CREDIT_WD_DELETED_PACKETS};
#else
  static constexpr std::array<sai_stat_id_t, 0>
      VoqWatchDogDeleteCounterIdsToRead = {};
#endif

  static constexpr std::array<sai_stat_id_t, 1> WredCounterIdsToRead = {
      SAI_QUEUE_STAT_WRED_DROPPED_PACKETS,
  };

  static constexpr std::array<sai_stat_id_t, 1> EcnCounterIdsToRead = {
      SAI_QUEUE_STAT_WRED_ECN_MARKED_PACKETS,
  };

  static constexpr std::array<sai_stat_id_t, 2> CounterIdsToReadAndClear = {
      SAI_QUEUE_STAT_WATERMARK_BYTES,
      SAI_QUEUE_STAT_WATERMARK_LEVEL,
  };
  // Non watermark stats
  static constexpr auto NonWatermarkCounterIdsToRead = CounterIdsToRead;
  static constexpr std::array<sai_stat_id_t, 0>
      NonWatermarkCounterIdsToReadAndClear = {};
  static constexpr auto VoqNonWatermarkCounterIdsToRead = VoqCounterIdsToRead;
  static constexpr std::array<sai_stat_id_t, 0>
      VoqNonWatermarkCounterIdsToReadAndClear = {};
  static constexpr auto NonWatermarkWredCounterIdsToRead = WredCounterIdsToRead;
  static constexpr auto NonWatermarkEcnCounterIdsToRead = EcnCounterIdsToRead;
  // Watermark stats
  static constexpr std::array<sai_stat_id_t, 1>
      WatermarkByteCounterIdsToReadAndClear = {
          SAI_QUEUE_STAT_WATERMARK_BYTES,
  };
  static constexpr std::array<sai_stat_id_t, 1>
      WatermarkLevelCounterIdsToReadAndClear = {
          SAI_QUEUE_STAT_WATERMARK_LEVEL,
  };
};

SAI_ATTRIBUTE_NAME(Queue, Type)
SAI_ATTRIBUTE_NAME(Queue, Port)
SAI_ATTRIBUTE_NAME(Queue, Index)
SAI_ATTRIBUTE_NAME(Queue, ParentSchedulerNode)
SAI_ATTRIBUTE_NAME(Queue, WredProfileId)
SAI_ATTRIBUTE_NAME(Queue, BufferProfileId)
SAI_ATTRIBUTE_NAME(Queue, SchedulerProfileId)

template <>
struct IsSaiObjectOwnedByAdapter<SaiQueueTraits> : public std::true_type {};

template <>
struct SaiObjectHasStats<SaiQueueTraits> : public std::true_type {};

class QueueApi : public SaiApi<QueueApi> {
 public:
  static constexpr sai_api_t ApiType = SAI_API_QUEUE;
  QueueApi() {
    sai_status_t status =
        sai_api_query(ApiType, reinterpret_cast<void**>(&api_));
    saiApiCheckError(status, ApiType, "Failed to query for queue api");
  }

 private:
  sai_status_t _create(
      QueueSaiId* id,
      sai_object_id_t switch_id,
      size_t count,
      sai_attribute_t* attr_list) const {
    return api_->create_queue(rawSaiId(id), switch_id, count, attr_list);
  }
  sai_status_t _remove(QueueSaiId id) const {
    return api_->remove_queue(id);
  }
  sai_status_t _getAttribute(QueueSaiId id, sai_attribute_t* attr) const {
    return api_->get_queue_attribute(id, 1, attr);
  }
  sai_status_t _setAttribute(QueueSaiId id, const sai_attribute_t* attr) const {
    return api_->set_queue_attribute(id, attr);
  }

  sai_status_t _getStats(
      QueueSaiId key,
      uint32_t num_of_counters,
      const sai_stat_id_t* counter_ids,
      sai_stats_mode_t mode,
      uint64_t* counters) const {
    /*
     * Unfortunately not all vendors implement the ext stats api.
     * ext stats api matter only for modes other than the (default)
     * SAI_STATS_MODE_READ. So play defensive and call ext mode only
     * when called with something other than default
     */
    return mode == SAI_STATS_MODE_READ
        ? api_->get_queue_stats(key, num_of_counters, counter_ids, counters)
        : api_->get_queue_stats_ext(
              key, num_of_counters, counter_ids, mode, counters);
  }

  sai_status_t _clearStats(
      QueueSaiId key,
      uint32_t num_of_counters,
      const sai_stat_id_t* counter_ids) const {
    return api_->clear_queue_stats(key, num_of_counters, counter_ids);
  }

  sai_queue_api_t* api_;
  friend class SaiApi<QueueApi>;
};

} // namespace facebook::fboss
