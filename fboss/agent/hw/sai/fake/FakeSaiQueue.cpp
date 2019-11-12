/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/sai/fake/FakeSaiQueue.h"
#include "fboss/agent/hw/sai/fake/FakeSai.h"

#include <folly/logging/xlog.h>
#include <optional>

using facebook::fboss::FakeQueue;
using facebook::fboss::FakeSai;

sai_status_t create_queue_fn(
    sai_object_id_t* queue_id,
    sai_object_id_t /* switch_id */,
    uint32_t attr_count,
    const sai_attribute_t* attr_list) {
  auto fs = FakeSai::getInstance();
  std::optional<sai_object_id_t> port;
  std::optional<sai_object_id_t> parentScheduler;
  std::optional<uint8_t> index = 0;
  std::optional<sai_queue_type_t> type;
  for (int i = 0; i < attr_count; ++i) {
    switch (attr_list[i].id) {
      case SAI_QUEUE_ATTR_TYPE:
        type = static_cast<sai_queue_type_t>(attr_list[i].value.s32);
        break;
      case SAI_QUEUE_ATTR_PORT:
        port = attr_list[i].value.oid;
        break;
      case SAI_QUEUE_ATTR_INDEX:
        index = attr_list[i].value.u8;
        break;
      case SAI_QUEUE_ATTR_PARENT_SCHEDULER_NODE:
        parentScheduler = attr_list[i].value.oid;
        break;
      default:
        return SAI_STATUS_INVALID_PARAMETER;
    }
  }
  if (!type || !port || !index || !parentScheduler) {
    return SAI_STATUS_INVALID_PARAMETER;
  }
  *queue_id = fs->qm.create(
      type.value(), port.value(), index.value(), parentScheduler.value());
  return SAI_STATUS_SUCCESS;
}

sai_status_t remove_queue_fn(sai_object_id_t queue_id) {
  auto fs = FakeSai::getInstance();
  fs->qm.remove(queue_id);
  return SAI_STATUS_SUCCESS;
}

sai_status_t set_queue_attribute_fn(
    sai_object_id_t queue_id,
    const sai_attribute_t* attr) {
  auto fs = FakeSai::getInstance();
  auto& queue = fs->qm.get(queue_id);
  sai_status_t res;
  if (!attr) {
    return SAI_STATUS_INVALID_PARAMETER;
  }
  switch (attr->id) {
    case SAI_QUEUE_ATTR_PARENT_SCHEDULER_NODE:
      queue.parentScheduler = attr->value.oid;
      res = SAI_STATUS_SUCCESS;
      break;
    case SAI_QUEUE_ATTR_WRED_PROFILE_ID:
      queue.wredProfileId = attr->value.oid;
      res = SAI_STATUS_SUCCESS;
      break;
    case SAI_QUEUE_ATTR_BUFFER_PROFILE_ID:
      queue.bufferProfileId = attr->value.oid;
      res = SAI_STATUS_SUCCESS;
      break;
    case SAI_QUEUE_ATTR_SCHEDULER_PROFILE_ID:
      queue.schedulerProfileId = attr->value.oid;
      res = SAI_STATUS_SUCCESS;
      break;
    default:
      res = SAI_STATUS_INVALID_PARAMETER;
      break;
  }
  return res;
}

sai_status_t get_queue_attribute_fn(
    sai_object_id_t queue_id,
    uint32_t attr_count,
    sai_attribute_t* attr) {
  auto fs = FakeSai::getInstance();
  auto queue = fs->qm.get(queue_id);
  for (int i = 0; i < attr_count; ++i) {
    switch (attr[i].id) {
      case SAI_QUEUE_ATTR_TYPE:
        attr[i].value.s32 = queue.type;
        break;
      case SAI_QUEUE_ATTR_PORT:
        attr[i].value.oid = queue.port;
        break;
      case SAI_QUEUE_ATTR_INDEX:
        attr[i].value.u8 = queue.index;
        break;
      case SAI_QUEUE_ATTR_PARENT_SCHEDULER_NODE:
        attr[i].value.oid = queue.parentScheduler;
        break;
      case SAI_QUEUE_ATTR_WRED_PROFILE_ID:
        attr[i].value.oid = queue.wredProfileId;
        break;
      case SAI_QUEUE_ATTR_BUFFER_PROFILE_ID:
        attr[i].value.oid = queue.bufferProfileId;
        break;
      case SAI_QUEUE_ATTR_SCHEDULER_PROFILE_ID:
        attr[i].value.oid = queue.schedulerProfileId;
        break;
      default:
        return SAI_STATUS_INVALID_PARAMETER;
    }
  }
  return SAI_STATUS_SUCCESS;
}

namespace facebook::fboss {

static sai_queue_api_t _queue_api;

void populate_queue_api(sai_queue_api_t** queue_api) {
  _queue_api.create_queue = &create_queue_fn;
  _queue_api.remove_queue = &remove_queue_fn;
  _queue_api.set_queue_attribute = &set_queue_attribute_fn;
  _queue_api.get_queue_attribute = &get_queue_attribute_fn;
  *queue_api = &_queue_api;
}

} // namespace facebook::fboss
