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
#include "fboss/agent/hw/sai/tracer/Utils.h"

namespace facebook::fboss {

sai_status_t wrap_create_queue(
    sai_object_id_t* queue_id,
    sai_object_id_t switch_id,
    uint32_t attr_count,
    const sai_attribute_t* attr_list) {
  auto rv = SaiTracer::getInstance()->queueApi_->create_queue(
      queue_id, switch_id, attr_count, attr_list);

  SaiTracer::getInstance()->logCreateFn(
      "create_queue",
      queue_id,
      switch_id,
      attr_count,
      attr_list,
      SAI_OBJECT_TYPE_QUEUE,
      rv);
  return rv;
}

sai_status_t wrap_remove_queue(sai_object_id_t queue_id) {
  auto rv = SaiTracer::getInstance()->queueApi_->remove_queue(queue_id);

  SaiTracer::getInstance()->logRemoveFn(
      "remove_queue", queue_id, SAI_OBJECT_TYPE_QUEUE, rv);
  return rv;
}

sai_status_t wrap_set_queue_attribute(
    sai_object_id_t queue_id,
    const sai_attribute_t* attr) {
  auto rv =
      SaiTracer::getInstance()->queueApi_->set_queue_attribute(queue_id, attr);

  SaiTracer::getInstance()->logSetAttrFn(
      "set_queue_attribute", queue_id, attr, SAI_OBJECT_TYPE_QUEUE, rv);
  return rv;
}

sai_status_t wrap_get_queue_attribute(
    sai_object_id_t queue_id,
    uint32_t attr_count,
    sai_attribute_t* attr_list) {
  // TODO(zecheng): Log get functions as well
  return SaiTracer::getInstance()->queueApi_->get_queue_attribute(
      queue_id, attr_count, attr_list);
}

sai_status_t wrap_get_queue_stats(
    sai_object_id_t queue_id,
    uint32_t number_of_counters,
    const sai_stat_id_t* counter_ids,
    uint64_t* counters) {
  return SaiTracer::getInstance()->queueApi_->get_queue_stats(
      queue_id, number_of_counters, counter_ids, counters);
}

sai_status_t wrap_get_queue_stats_ext(
    sai_object_id_t queue_id,
    uint32_t number_of_counters,
    const sai_stat_id_t* counter_ids,
    sai_stats_mode_t mode,
    uint64_t* counters) {
  return SaiTracer::getInstance()->queueApi_->get_queue_stats_ext(
      queue_id, number_of_counters, counter_ids, mode, counters);
}

sai_status_t wrap_clear_queue_stats(
    sai_object_id_t queue_id,
    uint32_t number_of_counters,
    const sai_stat_id_t* counter_ids) {
  return SaiTracer::getInstance()->queueApi_->clear_queue_stats(
      queue_id, number_of_counters, counter_ids);
}

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

void setQueueAttributes(
    const sai_attribute_t* attr_list,
    uint32_t attr_count,
    std::vector<std::string>& attrLines) {
  for (int i = 0; i < attr_count; ++i) {
    switch (attr_list[i].id) {
      case SAI_QUEUE_ATTR_TYPE:
        attrLines.push_back(s32Attr(attr_list, i));
        break;
      case SAI_QUEUE_ATTR_INDEX:
        attrLines.push_back(u8Attr(attr_list, i));
        break;
      case SAI_QUEUE_ATTR_PORT:
      case SAI_QUEUE_ATTR_PARENT_SCHEDULER_NODE:
      case SAI_QUEUE_ATTR_WRED_PROFILE_ID:
      case SAI_QUEUE_ATTR_BUFFER_PROFILE_ID:
      case SAI_QUEUE_ATTR_SCHEDULER_PROFILE_ID:
        attrLines.push_back(oidAttr(attr_list, i));
        break;
      default:
        // TODO(zecheng): Better check for newly added attributes (T69350100)
        break;
    }
  }
}

} // namespace facebook::fboss
