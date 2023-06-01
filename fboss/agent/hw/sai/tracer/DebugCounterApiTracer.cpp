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
#include <typeindex>
#include <utility>

#include "fboss/agent/hw/sai/api/DebugCounterApi.h"
#include "fboss/agent/hw/sai/tracer/Utils.h"

using folly::to;

namespace {
std::map<int32_t, std::pair<std::string, std::size_t>> _DebugCounterMap{
    SAI_ATTR_MAP(DebugCounter, Index),
    SAI_ATTR_MAP(DebugCounter, Type),
    SAI_ATTR_MAP(DebugCounter, BindMethod),
    SAI_ATTR_MAP(DebugCounter, InDropReasons),
};
} // namespace

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

SET_SAI_ATTRIBUTES(DebugCounter)

} // namespace facebook::fboss
