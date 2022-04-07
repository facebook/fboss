/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/sai/fake/FakeSaiCounter.h"

#include "fboss/agent/hw/sai/fake/FakeSai.h"

using facebook::fboss::FakeCounter;
using facebook::fboss::FakeSai;

sai_status_t set_counter_attribute_fn(
    sai_object_id_t id,
    const sai_attribute_t* attr) {
  if (!attr) {
    return SAI_STATUS_INVALID_PARAMETER;
  }
  auto& counterManager = FakeSai::getInstance()->counterManager;
  auto& counter = counterManager.get(id);
  switch (attr->id) {
    case SAI_COUNTER_ATTR_TYPE:
      counter.setType(static_cast<sai_counter_type_t>(attr->value.s32));
      break;
#if SAI_API_VERSION >= SAI_VERSION(1, 10, 0)
    case SAI_COUNTER_ATTR_LABEL:
      counter.setLabel(attr);
      break;
#endif
    default:
      return SAI_STATUS_INVALID_PARAMETER;
  }
  return SAI_STATUS_SUCCESS;
}

sai_status_t get_counter_attribute_fn(
    sai_object_id_t counter_id,
    uint32_t attr_count,
    sai_attribute_t* attr) {
  auto fs = FakeSai::getInstance();
  const auto& counter = fs->counterManager.get(counter_id);
  for (int i = 0; i < attr_count; ++i) {
    switch (attr[i].id) {
      case SAI_COUNTER_ATTR_TYPE:
        attr[i].value.u32 = counter.getType();
        break;
#if SAI_API_VERSION >= SAI_VERSION(1, 10, 0)
      case SAI_COUNTER_ATTR_LABEL:
        counter.getLabel(&attr[i]);
        break;
#endif
      default:
        return SAI_STATUS_INVALID_PARAMETER;
    }
  }
  return SAI_STATUS_SUCCESS;
}

sai_status_t create_counter_fn(
    sai_object_id_t* id,
    sai_object_id_t /*switch_id*/,
    uint32_t attr_count,
    const sai_attribute_t* attr_list) {
  auto fs = FakeSai::getInstance();
  *id = fs->counterManager.create();
  for (int i = 0; i < attr_count; ++i) {
    set_counter_attribute_fn(*id, &attr_list[i]);
  }
  return SAI_STATUS_SUCCESS;
}

sai_status_t remove_counter_fn(sai_object_id_t counter_id) {
  auto fs = FakeSai::getInstance();
  fs->counterManager.remove(counter_id);
  return SAI_STATUS_SUCCESS;
}

namespace facebook::fboss {

FakeCounter::FakeCounter() {}

static sai_counter_api_t _counter_api;

void populate_counter_api(sai_counter_api_t** counter_api) {
  _counter_api.create_counter = &create_counter_fn;
  _counter_api.remove_counter = &remove_counter_fn;
  _counter_api.set_counter_attribute = &set_counter_attribute_fn;
  _counter_api.get_counter_attribute = &get_counter_attribute_fn;
  *counter_api = &_counter_api;
}
} // namespace facebook::fboss
