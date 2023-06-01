/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/sai/tracer/CounterApiTracer.h"
#include <typeindex>
#include <utility>

#include "fboss/agent/hw/sai/api/CounterApi.h"
#include "fboss/agent/hw/sai/tracer/Utils.h"
#include "folly/Conv.h"

using folly::to;

namespace {
std::map<int32_t, std::pair<std::string, std::size_t>> _CounterMap {
  SAI_ATTR_MAP(Counter, Type),
#if SAI_API_VERSION >= SAI_VERSION(1, 10, 0)
      SAI_ATTR_MAP(Counter, Label),
#endif
};
} // namespace

namespace facebook::fboss {

WRAP_CREATE_FUNC(counter, SAI_OBJECT_TYPE_COUNTER, counter);
WRAP_REMOVE_FUNC(counter, SAI_OBJECT_TYPE_COUNTER, counter);
WRAP_SET_ATTR_FUNC(counter, SAI_OBJECT_TYPE_COUNTER, counter);
WRAP_GET_ATTR_FUNC(counter, SAI_OBJECT_TYPE_COUNTER, counter);

sai_status_t wrap_get_counter_stats(
    sai_object_id_t counter_id,
    uint32_t number_of_counters,
    const sai_stat_id_t* counter_ids,
    uint64_t* counters) {
  return SaiTracer::getInstance()->counterApi_->get_counter_stats(
      counter_id, number_of_counters, counter_ids, counters);
}

sai_status_t wrap_get_counter_stats_ext(
    sai_object_id_t counter_id,
    uint32_t number_of_counters,
    const sai_stat_id_t* counter_ids,
    sai_stats_mode_t mode,
    uint64_t* counters) {
  return SaiTracer::getInstance()->counterApi_->get_counter_stats_ext(
      counter_id, number_of_counters, counter_ids, mode, counters);
}

sai_status_t wrap_clear_counter_stats(
    sai_object_id_t counter_id,
    uint32_t number_of_counters,
    const sai_stat_id_t* counter_ids) {
  return SaiTracer::getInstance()->counterApi_->clear_counter_stats(
      counter_id, number_of_counters, counter_ids);
}

sai_counter_api_t* wrappedCounterApi() {
  static sai_counter_api_t counterWrappers;

  counterWrappers.create_counter = &wrap_create_counter;
  counterWrappers.remove_counter = &wrap_remove_counter;
  counterWrappers.set_counter_attribute = &wrap_set_counter_attribute;
  counterWrappers.get_counter_attribute = &wrap_get_counter_attribute;
  counterWrappers.get_counter_stats = &wrap_get_counter_stats;
  counterWrappers.get_counter_stats_ext = &wrap_get_counter_stats_ext;
  counterWrappers.clear_counter_stats = &wrap_clear_counter_stats;

  return &counterWrappers;
}

SET_SAI_ATTRIBUTES(Counter)

} // namespace facebook::fboss
