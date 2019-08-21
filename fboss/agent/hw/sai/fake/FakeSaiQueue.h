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

#include "fboss/agent/hw/sai/fake/FakeManager.h"

#include <unordered_map>
#include <vector>

extern "C" {
#include <sai.h>
}

sai_status_t create_queue_fn(
    sai_object_id_t* queue_id,
    sai_object_id_t switch_id,
    uint32_t attr_count,
    const sai_attribute_t* attr_list);

sai_status_t remove_queue_fn(sai_object_id_t queue_id);

sai_status_t set_queue_attribute_fn(
    sai_object_id_t queue_id,
    const sai_attribute_t* attr);

sai_status_t get_queue_attribute_fn(
    sai_object_id_t queue_id,
    uint32_t attr_count,
    sai_attribute_t* attr);

namespace facebook {
namespace fboss {

class FakeQueue {
 public:
  FakeQueue(
      sai_queue_type_t type,
      sai_object_id_t port,
      uint8_t index,
      sai_object_id_t parentScheduler)
      : type(type),
        port(port),
        index(index),
        parentScheduler(parentScheduler) {}
  sai_queue_type_t type;
  sai_object_id_t port;
  uint8_t index;
  sai_object_id_t parentScheduler;
  sai_object_id_t wredProfileId;
  sai_object_id_t bufferProfileId;
  sai_object_id_t schedulerProfileId;
  sai_object_id_t id;
};

using FakeQueueManager = FakeManager<sai_object_id_t, FakeQueue>;

void populate_queue_api(sai_queue_api_t** queue_api);
} // namespace fboss
} // namespace facebook
