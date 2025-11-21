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
#include <typeindex>
#include <utility>

#include "fboss/agent/hw/sai/api/BufferApi.h"
#include "fboss/agent/hw/sai/tracer/Utils.h"

using folly::to;

namespace {
std::map<int32_t, std::pair<std::string, std::size_t>> _BufferPoolMap{
    SAI_ATTR_MAP(BufferPool, Type),
    SAI_ATTR_MAP(BufferPool, Size),
    SAI_ATTR_MAP(BufferPool, ThresholdMode),
    SAI_ATTR_MAP(BufferPool, XoffSize),
};

std::map<int32_t, std::pair<std::string, std::size_t>> _BufferProfileMap{
    SAI_ATTR_MAP(StaticBufferProfile, PoolId),
    SAI_ATTR_MAP(StaticBufferProfile, ReservedBytes),
    SAI_ATTR_MAP(StaticBufferProfile, ThresholdMode),
    SAI_ATTR_MAP(StaticBufferProfile, SharedStaticThreshold),
    SAI_ATTR_MAP(StaticBufferProfile, XoffTh),
    SAI_ATTR_MAP(StaticBufferProfile, XonTh),
    SAI_ATTR_MAP(StaticBufferProfile, XonOffsetTh),
    SAI_ATTR_MAP(DynamicBufferProfile, SharedDynamicThreshold),
};

std::map<int32_t, std::pair<std::string, std::size_t>> _IngressPriorityGroupMap{
    SAI_ATTR_MAP(IngressPriorityGroup, Port),
    SAI_ATTR_MAP(IngressPriorityGroup, Index),
    SAI_ATTR_MAP(IngressPriorityGroup, BufferProfile),
};

void handleExtensionAttributes() {
  SAI_EXT_ATTR_MAP_2(BufferProfile, StaticBufferProfile, SharedFadtMaxTh)
  SAI_EXT_ATTR_MAP_2(BufferProfile, StaticBufferProfile, SharedFadtMinTh)
  SAI_EXT_ATTR_MAP_2(BufferProfile, StaticBufferProfile, SramFadtMaxTh)
  SAI_EXT_ATTR_MAP_2(BufferProfile, StaticBufferProfile, SramFadtMinTh)
  SAI_EXT_ATTR_MAP_2(BufferProfile, StaticBufferProfile, SramFadtXonOffset)
  SAI_EXT_ATTR_MAP_2(BufferProfile, StaticBufferProfile, SramDynamicTh)
  SAI_EXT_ATTR_MAP_2(BufferProfile, StaticBufferProfile, PgPipelineLatencyBytes)

  SAI_EXT_ATTR_MAP_2(BufferProfile, DynamicBufferProfile, SharedFadtMaxTh)
  SAI_EXT_ATTR_MAP_2(BufferProfile, DynamicBufferProfile, SharedFadtMinTh)
  SAI_EXT_ATTR_MAP_2(BufferProfile, DynamicBufferProfile, SramFadtMaxTh)
  SAI_EXT_ATTR_MAP_2(BufferProfile, DynamicBufferProfile, SramFadtMinTh)
  SAI_EXT_ATTR_MAP_2(BufferProfile, DynamicBufferProfile, SramFadtXonOffset)
  SAI_EXT_ATTR_MAP_2(BufferProfile, DynamicBufferProfile, SramDynamicTh)
  SAI_EXT_ATTR_MAP_2(
      BufferProfile, DynamicBufferProfile, PgPipelineLatencyBytes)

  SAI_EXT_ATTR_MAP(IngressPriorityGroup, LosslessEnable)
}

} // namespace

namespace facebook::fboss {

WRAP_CREATE_FUNC(buffer_pool, SAI_OBJECT_TYPE_BUFFER_POOL, buffer);
WRAP_REMOVE_FUNC(buffer_pool, SAI_OBJECT_TYPE_BUFFER_POOL, buffer);
WRAP_SET_ATTR_FUNC(buffer_pool, SAI_OBJECT_TYPE_BUFFER_POOL, buffer);
WRAP_GET_ATTR_FUNC(buffer_pool, SAI_OBJECT_TYPE_BUFFER_POOL, buffer);
WRAP_GET_STATS_FUNC(buffer_pool, SAI_OBJECT_TYPE_BUFFER_POOL, buffer);
WRAP_GET_STATS_EXT_FUNC(buffer_pool, SAI_OBJECT_TYPE_BUFFER_POOL, buffer);
WRAP_CLEAR_STATS_FUNC(buffer_pool, SAI_OBJECT_TYPE_BUFFER_POOL, buffer);

WRAP_CREATE_FUNC(buffer_profile, SAI_OBJECT_TYPE_BUFFER_PROFILE, buffer);
WRAP_REMOVE_FUNC(buffer_profile, SAI_OBJECT_TYPE_BUFFER_PROFILE, buffer);
WRAP_SET_ATTR_FUNC(buffer_profile, SAI_OBJECT_TYPE_BUFFER_PROFILE, buffer);
WRAP_GET_ATTR_FUNC(buffer_profile, SAI_OBJECT_TYPE_BUFFER_PROFILE, buffer);

WRAP_CREATE_FUNC(
    ingress_priority_group,
    SAI_OBJECT_TYPE_INGRESS_PRIORITY_GROUP,
    buffer);
WRAP_REMOVE_FUNC(
    ingress_priority_group,
    SAI_OBJECT_TYPE_INGRESS_PRIORITY_GROUP,
    buffer);
WRAP_SET_ATTR_FUNC(
    ingress_priority_group,
    SAI_OBJECT_TYPE_INGRESS_PRIORITY_GROUP,
    buffer);
WRAP_GET_ATTR_FUNC(
    ingress_priority_group,
    SAI_OBJECT_TYPE_INGRESS_PRIORITY_GROUP,
    buffer);
WRAP_GET_STATS_FUNC(
    ingress_priority_group,
    SAI_OBJECT_TYPE_INGRESS_PRIORITY_GROUP,
    buffer);
WRAP_GET_STATS_EXT_FUNC(
    ingress_priority_group,
    SAI_OBJECT_TYPE_INGRESS_PRIORITY_GROUP,
    buffer);
WRAP_CLEAR_STATS_FUNC(
    ingress_priority_group,
    SAI_OBJECT_TYPE_INGRESS_PRIORITY_GROUP,
    buffer);

sai_buffer_api_t* wrappedBufferApi() {
  handleExtensionAttributes();
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
  bufferWrappers.create_ingress_priority_group =
      &wrap_create_ingress_priority_group;
  bufferWrappers.remove_ingress_priority_group =
      &wrap_remove_ingress_priority_group;
  bufferWrappers.set_ingress_priority_group_attribute =
      &wrap_set_ingress_priority_group_attribute;
  bufferWrappers.get_ingress_priority_group_attribute =
      &wrap_get_ingress_priority_group_attribute;
  bufferWrappers.get_ingress_priority_group_stats =
      &wrap_get_ingress_priority_group_stats;
  bufferWrappers.get_ingress_priority_group_stats_ext =
      &wrap_get_ingress_priority_group_stats_ext;
  bufferWrappers.clear_ingress_priority_group_stats =
      &wrap_clear_ingress_priority_group_stats;

  return &bufferWrappers;
}

SET_SAI_ATTRIBUTES(BufferPool)
SET_SAI_ATTRIBUTES(BufferProfile)
SET_SAI_ATTRIBUTES(IngressPriorityGroup)

} // namespace facebook::fboss
