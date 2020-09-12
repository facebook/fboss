/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/sai/fake/FakeSaiDebugCounter.h"

#include "fboss/agent/hw/sai/fake/FakeSai.h"

using facebook::fboss::FakeDebugCounter;
using facebook::fboss::FakeSai;

sai_status_t set_debug_counter_attribute_fn(
    sai_object_id_t id,
    const sai_attribute_t* attr) {
  if (!attr) {
    return SAI_STATUS_INVALID_PARAMETER;
  }
  auto& debugCounterManager = FakeSai::getInstance()->debugCounterManager;
  auto& debugCounter = debugCounterManager.get(id);
  switch (attr->id) {
    case SAI_DEBUG_COUNTER_ATTR_TYPE:
      debugCounter.setType(
          static_cast<sai_debug_counter_type_t>(attr->value.s32));
      break;
    case SAI_DEBUG_COUNTER_ATTR_BIND_METHOD:
      debugCounter.setBindMethod(
          static_cast<sai_debug_counter_bind_method_t>(attr->value.s32));
      break;
    case SAI_DEBUG_COUNTER_ATTR_IN_DROP_REASON_LIST: {
      FakeDebugCounter::InDropReasons inDropReasons(attr->value.s32list.count);
      for (auto i = 0; i < attr->value.s32list.count; ++i) {
        inDropReasons[i] = attr->value.s32list.list[i];
      }
      debugCounter.setInDropReasons(inDropReasons);
    } break;
    case SAI_DEBUG_COUNTER_ATTR_OUT_DROP_REASON_LIST: {
      FakeDebugCounter::OutDropReasons outDropReasons(
          attr->value.s32list.count);
      for (auto i = 0; i < attr->value.s32list.count; ++i) {
        outDropReasons[i] = attr->value.s32list.list[i];
      }
      debugCounter.setOutDropReasons(outDropReasons);
    } break;
    default:
      return SAI_STATUS_INVALID_PARAMETER;
  }
  return SAI_STATUS_SUCCESS;
}

sai_status_t get_debug_counter_attribute_fn(
    sai_object_id_t debug_counter_id,
    uint32_t attr_count,
    sai_attribute_t* attr) {
  auto fs = FakeSai::getInstance();
  const auto& debugCounter = fs->debugCounterManager.get(debug_counter_id);
  for (int i = 0; i < attr_count; ++i) {
    switch (attr[i].id) {
      case SAI_DEBUG_COUNTER_ATTR_TYPE:
        attr[i].value.u32 = debugCounter.getType();
        break;
      case SAI_DEBUG_COUNTER_ATTR_BIND_METHOD:
        attr[i].value.u32 = debugCounter.getBindMethod();
        break;
      case SAI_DEBUG_COUNTER_ATTR_INDEX:
        attr[i].value.u32 = debugCounter.getIndex();
        break;
      case SAI_DEBUG_COUNTER_ATTR_IN_DROP_REASON_LIST: {
        const auto& inDropReasons = debugCounter.getInDropReasons();
        if (inDropReasons.size() > attr[i].value.s32list.count) {
          attr[i].value.s32list.count = inDropReasons.size();
          return SAI_STATUS_BUFFER_OVERFLOW;
        }
        attr[i].value.s32list.count = inDropReasons.size();
        int j = 0;
        for (const auto& reason : inDropReasons) {
          attr[i].value.s32list.list[j++] = reason;
        }
      } break;
      case SAI_DEBUG_COUNTER_ATTR_OUT_DROP_REASON_LIST: {
        const auto& outDropReasons = debugCounter.getOutDropReasons();
        if (outDropReasons.size() > attr[i].value.s32list.count) {
          attr[i].value.s32list.count = outDropReasons.size();
          return SAI_STATUS_BUFFER_OVERFLOW;
        }
        attr[i].value.s32list.count = outDropReasons.size();
        int j = 0;
        for (const auto& reason : outDropReasons) {
          attr[i].value.s32list.list[j++] = reason;
        }
      } break;
      default:
        return SAI_STATUS_INVALID_PARAMETER;
    }
  }
  return SAI_STATUS_SUCCESS;
}

sai_status_t create_debug_counter_fn(
    sai_object_id_t* id,
    sai_object_id_t /*switch_id*/,
    uint32_t attr_count,
    const sai_attribute_t* attr_list) {
  auto fs = FakeSai::getInstance();
  *id = fs->debugCounterManager.create();
  for (int i = 0; i < attr_count; ++i) {
    set_debug_counter_attribute_fn(*id, &attr_list[i]);
  }
  return SAI_STATUS_SUCCESS;
}

sai_status_t remove_debug_counter_fn(sai_object_id_t debug_counter_id) {
  auto fs = FakeSai::getInstance();
  fs->debugCounterManager.remove(debug_counter_id);
  return SAI_STATUS_SUCCESS;
}

namespace facebook::fboss {

FakeDebugCounter::FakeDebugCounter() {
  static sai_uint32_t curIndex = 0;
  index_ = ++curIndex;
}

static sai_debug_counter_api_t _debug_counter_api;

void populate_debug_counter_api(sai_debug_counter_api_t** debug_counter_api) {
  _debug_counter_api.create_debug_counter = &create_debug_counter_fn;
  _debug_counter_api.remove_debug_counter = &remove_debug_counter_fn;
  _debug_counter_api.set_debug_counter_attribute =
      &set_debug_counter_attribute_fn;
  _debug_counter_api.get_debug_counter_attribute =
      &get_debug_counter_attribute_fn;
  *debug_counter_api = &_debug_counter_api;
}
} // namespace facebook::fboss
