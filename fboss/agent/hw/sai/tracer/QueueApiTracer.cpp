/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/sai/tracer/QueueApiTracer.h"
#include <typeindex>
#include <utility>

#include "fboss/agent/hw/sai/api/QueueApi.h"
#include "fboss/agent/hw/sai/tracer/Utils.h"

using folly::to;

namespace {
std::map<int32_t, std::pair<std::string, std::size_t>> _QueueMap{
    SAI_ATTR_MAP(Queue, Type),
    SAI_ATTR_MAP(Queue, Port),
    SAI_ATTR_MAP(Queue, Index),
    SAI_ATTR_MAP(Queue, ParentSchedulerNode),
    SAI_ATTR_MAP(Queue, WredProfileId),
    SAI_ATTR_MAP(Queue, BufferProfileId),
    SAI_ATTR_MAP(Queue, SchedulerProfileId),
};
} // namespace

namespace facebook::fboss {

WRAP_CREATE_FUNC(queue, SAI_OBJECT_TYPE_QUEUE, queue);
WRAP_REMOVE_FUNC(queue, SAI_OBJECT_TYPE_QUEUE, queue);
WRAP_SET_ATTR_FUNC(queue, SAI_OBJECT_TYPE_QUEUE, queue);
WRAP_GET_ATTR_FUNC(queue, SAI_OBJECT_TYPE_QUEUE, queue);
WRAP_GET_STATS_FUNC(queue, SAI_OBJECT_TYPE_QUEUE, queue);
WRAP_GET_STATS_EXT_FUNC(queue, SAI_OBJECT_TYPE_QUEUE, queue);
WRAP_CLEAR_STATS_FUNC(queue, SAI_OBJECT_TYPE_QUEUE, queue);

sai_queue_api_t* wrappedQueueApi() {
  static sai_queue_api_t queueWrappers;

  queueWrappers.create_queue = &wrap_create_queue;
  queueWrappers.remove_queue = &wrap_remove_queue;
  queueWrappers.set_queue_attribute = &wrap_set_queue_attribute;
  queueWrappers.get_queue_attribute = &wrap_get_queue_attribute;
  queueWrappers.get_queue_stats = &wrap_get_queue_stats;
  queueWrappers.get_queue_stats_ext = &wrap_get_queue_stats_ext;
  queueWrappers.clear_queue_stats = &wrap_clear_queue_stats;

  return &queueWrappers;
}

SET_SAI_ATTRIBUTES(Queue)

} // namespace facebook::fboss
