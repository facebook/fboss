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

#include "SaiApi.h"
#include "fboss/agent/hw/sai/api/SaiAttribute.h"
#include "fboss/agent/hw/sai/api/SaiAttributeDataTypes.h"
#include "fboss/agent/hw/sai/api/Types.h"

#include <folly/logging/xlog.h>

#include <optional>
#include <tuple>

extern "C" {
#include <sai.h>
}

namespace facebook {
namespace fboss {

class BufferApi;

struct SaiBufferPoolTraits {
  static constexpr sai_api_t ApiType = SAI_API_BUFFER;
  static constexpr sai_object_type_t ObjectType = SAI_OBJECT_TYPE_BUFFER_POOL;
  using SaiApiT = BufferApi;
  struct Attributes {
    using EnumType = sai_buffer_pool_attr_t;
    using Type = SaiAttribute<EnumType, SAI_BUFFER_POOL_ATTR_TYPE, sai_int32_t>;
    using Size =
        SaiAttribute<EnumType, SAI_BUFFER_POOL_ATTR_SIZE, sai_uint64_t>;
    using ThresholdMode = SaiAttribute<
        EnumType,
        SAI_BUFFER_POOL_ATTR_THRESHOLD_MODE,
        sai_int32_t>;
  };
  using AdapterKey = BufferPoolSaiId;
  using AdapterHostKey = Attributes::Type;
  using CreateAttributes =
      std::tuple<Attributes::Type, Attributes::Size, Attributes::ThresholdMode>;

  static constexpr std::array<sai_stat_id_t, 0> CounterIdsToReadAndClear = {};
  static constexpr std::array<sai_stat_id_t, 1> CounterIdsToRead = {
      SAI_BUFFER_POOL_STAT_WATERMARK_BYTES,
  };
};

SAI_ATTRIBUTE_NAME(BufferPool, Type);
SAI_ATTRIBUTE_NAME(BufferPool, Size);
SAI_ATTRIBUTE_NAME(BufferPool, ThresholdMode);

template <>
struct SaiObjectHasStats<SaiBufferPoolTraits> : public std::true_type {};

struct SaiBufferProfileTraits {
  static constexpr sai_api_t ApiType = SAI_API_BUFFER;
  static constexpr sai_object_type_t ObjectType =
      SAI_OBJECT_TYPE_BUFFER_PROFILE;
  using SaiApiT = BufferApi;
  struct Attributes {
    using EnumType = sai_buffer_profile_attr_t;
    using PoolId = SaiAttribute<
        EnumType,
        SAI_BUFFER_PROFILE_ATTR_POOL_ID,
        sai_object_id_t>;
    using ReservedBytes = SaiAttribute<
        EnumType,
        SAI_BUFFER_PROFILE_ATTR_RESERVED_BUFFER_SIZE,
        sai_uint64_t>;
    using ThresholdMode = SaiAttribute<
        EnumType,
        SAI_BUFFER_PROFILE_ATTR_THRESHOLD_MODE,
        sai_int32_t>;
    using SharedDynamicThreshold = SaiAttribute<
        EnumType,
        SAI_BUFFER_PROFILE_ATTR_SHARED_DYNAMIC_TH,
        sai_int8_t>;
    using SharedStaticThreshold = SaiAttribute<
        EnumType,
        SAI_BUFFER_PROFILE_ATTR_SHARED_STATIC_TH,
        sai_int8_t>;
  };
  using AdapterKey = BufferProfileSaiId;
  using CreateAttributes = std::tuple<
      Attributes::PoolId,
      Attributes::ReservedBytes,
      Attributes::ThresholdMode,
      Attributes::SharedDynamicThreshold,
      Attributes::SharedStaticThreshold>;
  using AdapterHostKey = CreateAttributes;
};

SAI_ATTRIBUTE_NAME(BufferProfile, PoolId);
SAI_ATTRIBUTE_NAME(BufferProfile, ReservedBytes);
SAI_ATTRIBUTE_NAME(BufferProfile, ThresholdMode);
SAI_ATTRIBUTE_NAME(BufferProfile, SharedDynamicThreshold);
SAI_ATTRIBUTE_NAME(BufferProfile, SharedStaticThreshold);

class BufferApi : public SaiApi<BufferApi> {
 public:
  static constexpr sai_api_t ApiType = SAI_API_BUFFER;
  BufferApi() {
    sai_status_t status =
        sai_api_query(ApiType, reinterpret_cast<void**>(&api_));
    saiApiCheckError(status, ApiType, "Failed to query for buffer api");
  }
  BufferApi(const BufferApi& other) = delete;
  BufferApi& operator=(const BufferApi& other) = delete;

 private:
  sai_status_t _create(
      BufferPoolSaiId* id,
      sai_object_id_t switch_id,
      size_t count,
      sai_attribute_t* attr_list) {
    return api_->create_buffer_pool(rawSaiId(id), switch_id, count, attr_list);
  }
  sai_status_t _remove(BufferPoolSaiId id) {
    return api_->remove_buffer_pool(id);
  }
  sai_status_t _getAttribute(BufferPoolSaiId key, sai_attribute_t* attr) const {
    return api_->get_buffer_pool_attribute(key, 1, attr);
  }
  sai_status_t _setAttribute(BufferPoolSaiId key, const sai_attribute_t* attr) {
    return api_->set_buffer_pool_attribute(key, attr);
  }

  sai_status_t _getStats(
      BufferPoolSaiId key,
      uint32_t num_of_counters,
      const sai_stat_id_t* counter_ids,
      sai_stats_mode_t mode,
      uint64_t* counters) const {
    return mode == SAI_STATS_MODE_READ
        ? api_->get_buffer_pool_stats(
              key, num_of_counters, counter_ids, counters)
        : api_->get_buffer_pool_stats_ext(
              key, num_of_counters, counter_ids, mode, counters);
  }

  sai_status_t _clearStats(
      BufferPoolSaiId key,
      uint32_t num_of_counters,
      const sai_stat_id_t* counter_ids) const {
    return api_->clear_buffer_pool_stats(key, num_of_counters, counter_ids);
  }

  sai_status_t _create(
      BufferProfileSaiId* id,
      sai_object_id_t switch_id,
      size_t count,
      sai_attribute_t* attr_list) {
    return api_->create_buffer_profile(
        rawSaiId(id), switch_id, count, attr_list);
  }
  sai_status_t _remove(BufferProfileSaiId id) {
    return api_->remove_buffer_profile(id);
  }
  sai_status_t _getAttribute(BufferProfileSaiId key, sai_attribute_t* attr)
      const {
    return api_->get_buffer_profile_attribute(key, 1, attr);
  }
  sai_status_t _setAttribute(
      BufferProfileSaiId key,
      const sai_attribute_t* attr) {
    return api_->set_buffer_profile_attribute(key, attr);
  }
  sai_buffer_api_t* api_;
  friend class SaiApi<BufferApi>;
};

} // namespace fboss
} // namespace facebook
