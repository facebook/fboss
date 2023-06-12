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
std::map<int32_t, std::pair<std::string, std::size_t>> _BufferPoolMap {
  SAI_ATTR_MAP(BufferPool, Type), SAI_ATTR_MAP(BufferPool, Size),
      SAI_ATTR_MAP(BufferPool, ThresholdMode),
#if defined(TAJO_SDK) || defined(SAI_VERSION_8_2_0_0_ODP) || \
    defined(SAI_VERSION_8_2_0_0_DNX_ODP) ||                  \
    defined(SAI_VERSION_9_2_0_0_ODP) ||                      \
    defined(SAI_VERSION_8_2_0_0_SIM_ODP) ||                  \
    defined(SAI_VERSION_9_0_EA_SIM_ODP) ||                   \
    defined(SAI_VERSION_10_0_EA_DNX_SIM_ODP) ||              \
    defined(SAI_VERSION_10_0_EA_DNX_ODP)
      SAI_ATTR_MAP(BufferPool, XoffSize),
#endif
};

std::map<int32_t, std::pair<std::string, std::size_t>> _BufferProfileMap{
    SAI_ATTR_MAP(BufferProfile, PoolId),
    SAI_ATTR_MAP(BufferProfile, ReservedBytes),
    SAI_ATTR_MAP(BufferProfile, ThresholdMode),
    SAI_ATTR_MAP(BufferProfile, SharedDynamicThreshold),
    SAI_ATTR_MAP(BufferProfile, XoffTh),
    SAI_ATTR_MAP(BufferProfile, XonTh),
    SAI_ATTR_MAP(BufferProfile, XonOffsetTh),
};

std::map<int32_t, std::pair<std::string, std::size_t>> _IngressPriorityGroupMap{
    SAI_ATTR_MAP(IngressPriorityGroup, Port),
    SAI_ATTR_MAP(IngressPriorityGroup, Index),
    SAI_ATTR_MAP(IngressPriorityGroup, BufferProfile),
};
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
