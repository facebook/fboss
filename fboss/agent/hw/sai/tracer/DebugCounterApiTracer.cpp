/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/sai/tracer/DebugCounterApiTracer.h"
#include "fboss/agent/hw/sai/tracer/Utils.h"

namespace facebook::fboss {

WRAP_CREATE_FUNC(debug_counter, SAI_OBJECT_TYPE_DEBUG_COUNTER, debugCounter);
WRAP_REMOVE_FUNC(debug_counter, SAI_OBJECT_TYPE_DEBUG_COUNTER, debugCounter);
WRAP_SET_ATTR_FUNC(debug_counter, SAI_OBJECT_TYPE_DEBUG_COUNTER, debugCounter);
WRAP_GET_ATTR_FUNC(debug_counter, SAI_OBJECT_TYPE_DEBUG_COUNTER, debugCounter);

sai_debug_counter_api_t* wrappedDebugCounterApi() {
  static sai_debug_counter_api_t debugCounterWrappers;

  debugCounterWrappers.create_debug_counter = &wrap_create_debug_counter;
  debugCounterWrappers.remove_debug_counter = &wrap_remove_debug_counter;
  debugCounterWrappers.set_debug_counter_attribute =
      &wrap_set_debug_counter_attribute;
  debugCounterWrappers.get_debug_counter_attribute =
      &wrap_get_debug_counter_attribute;

  return &debugCounterWrappers;
}

void setDebugCounterAttributes(
    const sai_attribute_t* attr_list,
    uint32_t attr_count,
    std::vector<std::string>& attrLines) {
  uint32_t listCount = 0;

  for (int i = 0; i < attr_count; ++i) {
    switch (attr_list[i].id) {
      case SAI_DEBUG_COUNTER_ATTR_TYPE:
      case SAI_DEBUG_COUNTER_ATTR_BIND_METHOD:
        attrLines.push_back(s32Attr(attr_list, i));
        break;
      case SAI_DEBUG_COUNTER_ATTR_INDEX:
        attrLines.push_back(s32Attr(attr_list, i));
        break;
      case SAI_DEBUG_COUNTER_ATTR_IN_DROP_REASON_LIST:
        s32ListAttr(attr_list, i, listCount++, attrLines);
        break;
      default:
        break;
    }
  }
}

} // namespace facebook::fboss
