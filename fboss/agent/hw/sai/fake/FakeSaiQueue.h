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
namespace facebook::fboss {

class FakeQueue {
 public:
  FakeQueue(
      sai_queue_type_t type,
      sai_object_id_t port,
      uint8_t index,
      sai_object_id_t parentScheduler,
      sai_object_id_t schedulerProfileId = SAI_NULL_OBJECT_ID,
      sai_object_id_t wredProfileId = SAI_NULL_OBJECT_ID,
      sai_object_id_t bufferProfileId = SAI_NULL_OBJECT_ID,
      bool enablePfcDldr = false)
      : type(type),
        port(port),
        index(index),
        parentScheduler(parentScheduler),
        schedulerProfileId(schedulerProfileId),
        wredProfileId(wredProfileId),
        bufferProfileId(bufferProfileId),
        enablePfcDldr(enablePfcDldr) {}
  sai_queue_type_t type;
  sai_object_id_t port;
  uint8_t index;
  sai_object_id_t parentScheduler;
  sai_object_id_t schedulerProfileId;
  sai_object_id_t wredProfileId;
  sai_object_id_t bufferProfileId;
  sai_object_id_t id;
  bool enablePfcDldr;
};

using FakeQueueManager = FakeManager<sai_object_id_t, FakeQueue>;

void populate_queue_api(sai_queue_api_t** queue_api);

} // namespace facebook::fboss
