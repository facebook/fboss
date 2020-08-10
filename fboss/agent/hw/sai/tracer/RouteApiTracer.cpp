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
#include "fboss/agent/hw/sai/tracer/Utils.h"

namespace facebook::fboss {

sai_status_t wrap_create_route_entry(
    const sai_route_entry_t* route_entry,
    uint32_t attr_count,
    const sai_attribute_t* attr_list) {
  auto rv = SaiTracer::getInstance()->routeApi_->create_route_entry(
      route_entry, attr_count, attr_list);

  SaiTracer::getInstance()->logRouteEntryCreateFn(
      route_entry, attr_count, attr_list, rv);
  return rv;
}

sai_status_t wrap_remove_route_entry(const sai_route_entry_t* route_entry) {
  auto rv =
      SaiTracer::getInstance()->routeApi_->remove_route_entry(route_entry);

  SaiTracer::getInstance()->logRouteEntryRemoveFn(route_entry, rv);
  return rv;
}

sai_status_t wrap_set_route_entry_attribute(
    const sai_route_entry_t* route_entry,
    const sai_attribute_t* attr) {
  auto rv = SaiTracer::getInstance()->routeApi_->set_route_entry_attribute(
      route_entry, attr);

  SaiTracer::getInstance()->logRouteEntrySetAttrFn(route_entry, attr, rv);
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

void setRouteEntryAttributes(
    const sai_attribute_t* attr_list,
    uint32_t attr_count,
    std::vector<std::string>& attrLines) {
  for (int i = 0; i < attr_count; ++i) {
    switch (attr_list[i].id) {
      case SAI_ROUTE_ENTRY_ATTR_NEXT_HOP_ID:
        attrLines.push_back(oidAttr(attr_list, i));
        break;
      case SAI_ROUTE_ENTRY_ATTR_PACKET_ACTION:
        attrLines.push_back(s32Attr(attr_list, i));
        break;
      case SAI_ROUTE_ENTRY_ATTR_META_DATA:
        attrLines.push_back(u32Attr(attr_list, i));
        break;
      default:
        // TODO(zecheng): Better check for newly added attributes (T69350100)
        break;
    }
  }
}

} // namespace facebook::fboss
