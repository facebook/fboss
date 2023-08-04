/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/sai/tracer/RouteApiTracer.h"
#include <typeindex>
#include <utility>

#include "fboss/agent/hw/sai/api/RouteApi.h"
#include "fboss/agent/hw/sai/tracer/Utils.h"

using folly::to;

namespace {
std::map<int32_t, std::pair<std::string, std::size_t>> _RouteEntryMap {
  SAI_ATTR_MAP(Route, PacketAction), SAI_ATTR_MAP(Route, NextHopId),
      SAI_ATTR_MAP(Route, Metadata),
#if SAI_API_VERSION >= SAI_VERSION(1, 10, 0)
      SAI_ATTR_MAP(Route, CounterID),
#endif
};
} // namespace

namespace facebook::fboss {

sai_status_t wrap_create_route_entry(
    const sai_route_entry_t* route_entry,
    uint32_t attr_count,
    const sai_attribute_t* attr_list) {
  SaiTracer::getInstance()->logRouteEntryCreateFn(
      route_entry, attr_count, attr_list);
  auto begin = std::chrono::system_clock::now();
  auto rv = SaiTracer::getInstance()->routeApi_->create_route_entry(
      route_entry, attr_count, attr_list);
  SaiTracer::getInstance()->logPostInvocation(rv, SAI_NULL_OBJECT_ID, begin);
  return rv;
}

sai_status_t wrap_remove_route_entry(const sai_route_entry_t* route_entry) {
  SaiTracer::getInstance()->logRouteEntryRemoveFn(route_entry);
  auto begin = std::chrono::system_clock::now();
  auto rv =
      SaiTracer::getInstance()->routeApi_->remove_route_entry(route_entry);
  SaiTracer::getInstance()->logPostInvocation(rv, SAI_NULL_OBJECT_ID, begin);
  return rv;
}

sai_status_t wrap_set_route_entry_attribute(
    const sai_route_entry_t* route_entry,
    const sai_attribute_t* attr) {
  SaiTracer::getInstance()->logRouteEntrySetAttrFn(route_entry, attr);
  auto begin = std::chrono::system_clock::now();
  auto rv = SaiTracer::getInstance()->routeApi_->set_route_entry_attribute(
      route_entry, attr);
  SaiTracer::getInstance()->logPostInvocation(rv, SAI_NULL_OBJECT_ID, begin);
  return rv;
}

sai_status_t wrap_get_route_entry_attribute(
    const sai_route_entry_t* route_entry,
    uint32_t attr_count,
    sai_attribute_t* attr_list) {
  // TODO(zecheng): Log get functions as well
  return SaiTracer::getInstance()->routeApi_->get_route_entry_attribute(
      route_entry, attr_count, attr_list);
}

sai_route_api_t* wrappedRouteApi() {
  static sai_route_api_t routeWrappers;

  routeWrappers.create_route_entry = &wrap_create_route_entry;
  routeWrappers.remove_route_entry = &wrap_remove_route_entry;
  routeWrappers.set_route_entry_attribute = &wrap_set_route_entry_attribute;
  routeWrappers.get_route_entry_attribute = &wrap_get_route_entry_attribute;

  return &routeWrappers;
}

SET_SAI_ATTRIBUTES(RouteEntry)

} // namespace facebook::fboss
