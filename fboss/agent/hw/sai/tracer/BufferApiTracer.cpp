/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/sai/tracer/BufferApiTracer.h"
#include "fboss/agent/hw/sai/tracer/Utils.h"

namespace facebook::fboss {

WRAP_CREATE_FUNC(buffer_pool, SAI_OBJECT_TYPE_BUFFER_POOL, buffer);
WRAP_REMOVE_FUNC(buffer_pool, SAI_OBJECT_TYPE_BUFFER_POOL, buffer);
WRAP_SET_ATTR_FUNC(buffer_pool, SAI_OBJECT_TYPE_BUFFER_POOL, buffer);
WRAP_GET_ATTR_FUNC(buffer_pool, SAI_OBJECT_TYPE_BUFFER_POOL, buffer);

WRAP_CREATE_FUNC(buffer_profile, SAI_OBJECT_TYPE_BUFFER_PROFILE, buffer);
WRAP_REMOVE_FUNC(buffer_profile, SAI_OBJECT_TYPE_BUFFER_PROFILE, buffer);
WRAP_SET_ATTR_FUNC(buffer_profile, SAI_OBJECT_TYPE_BUFFER_PROFILE, buffer);
WRAP_GET_ATTR_FUNC(buffer_profile, SAI_OBJECT_TYPE_BUFFER_PROFILE, buffer);

sai_status_t wrap_get_buffer_pool_stats(
    sai_object_id_t buffer_pool_id,
    uint32_t number_of_counters,
    const sai_stat_id_t* counter_ids,
    uint64_t* counters) {
  return SaiTracer::getInstance()->bufferApi_->get_buffer_pool_stats(
      buffer_pool_id, number_of_counters, counter_ids, counters);
}

sai_status_t wrap_get_buffer_pool_stats_ext(
    sai_object_id_t buffer_pool_id,
    uint32_t number_of_counters,
    const sai_stat_id_t* counter_ids,
    sai_stats_mode_t mode,
    uint64_t* counters) {
  return SaiTracer::getInstance()->bufferApi_->get_buffer_pool_stats_ext(
      buffer_pool_id, number_of_counters, counter_ids, mode, counters);
}

sai_status_t wrap_clear_buffer_pool_stats(
    sai_object_id_t buffer_pool_id,
    uint32_t number_of_counters,
    const sai_stat_id_t* counter_ids) {
  return SaiTracer::getInstance()->bufferApi_->clear_buffer_pool_stats(
      buffer_pool_id, number_of_counters, counter_ids);
}

sai_buffer_api_t* wrappedBufferApi() {
  static sai_buffer_api_t bufferWrappers;

  bufferWrappers.create_buffer_pool = &wrap_create_buffer_pool;
  bufferWrappers.remove_buffer_pool = &wrap_remove_buffer_pool;
  bufferWrappers.set_buffer_pool_attribute = &wrap_set_buffer_pool_attribute;
  bufferWrappers.get_buffer_pool_attribute = &wrap_get_buffer_pool_attribute;
  bufferWrappers.get_buffer_pool_stats = &wrap_get_buffer_pool_stats;
  bufferWrappers.get_buffer_pool_stats_ext = &wrap_get_buffer_pool_stats_ext;
  bufferWrappers.clear_buffer_pool_stats = &wrap_clear_buffer_pool_stats;
  bufferWrappers.create_buffer_profile = &wrap_create_buffer_profile;
  bufferWrappers.remove_buffer_profile = &wrap_remove_buffer_profile;
  bufferWrappers.set_buffer_profile_attribute =
      &wrap_set_buffer_profile_attribute;
  bufferWrappers.get_buffer_profile_attribute =
      &wrap_get_buffer_profile_attribute;

  return &bufferWrappers;
}

void setBufferPoolAttributes(
    const sai_attribute_t* attr_list,
    uint32_t attr_count,
    std::vector<std::string>& attrLines) {
  for (int i = 0; i < attr_count; ++i) {
    switch (attr_list[i].id) {
      case SAI_BUFFER_POOL_ATTR_TYPE:
      case SAI_BUFFER_POOL_ATTR_THRESHOLD_MODE:
        attrLines.push_back(s32Attr(attr_list, i));
        break;
      case SAI_BUFFER_POOL_ATTR_SIZE:
        attrLines.push_back(u64Attr(attr_list, i));
        break;
      default:
        break;
    }
  }
}

void setBufferProfileAttributes(
    const sai_attribute_t* attr_list,
    uint32_t attr_count,
    std::vector<std::string>& attrLines) {
  for (int i = 0; i < attr_count; ++i) {
    switch (attr_list[i].id) {
      case SAI_BUFFER_PROFILE_ATTR_POOL_ID:
        attrLines.push_back(oidAttr(attr_list, i));
        break;
      case SAI_BUFFER_PROFILE_ATTR_RESERVED_BUFFER_SIZE:
        attrLines.push_back(u64Attr(attr_list, i));
        break;
      case SAI_BUFFER_PROFILE_ATTR_THRESHOLD_MODE:
        attrLines.push_back(s32Attr(attr_list, i));
        break;
      case SAI_BUFFER_PROFILE_ATTR_SHARED_DYNAMIC_TH:
      case SAI_BUFFER_PROFILE_ATTR_SHARED_STATIC_TH:
        attrLines.push_back(s8Attr(attr_list, i));
        break;
      default:
        break;
    }
  }
}

} // namespace facebook::fboss
