/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/sai/tracer/VirtualRouterApiTracer.h"
#include "fboss/agent/hw/sai/tracer/Utils.h"

namespace facebook::fboss {

sai_status_t wrap_create_virtual_router(
    sai_object_id_t* virtual_router_id,
    sai_object_id_t switch_id,
    uint32_t attr_count,
    const sai_attribute_t* attr_list) {
  auto rv = SaiTracer::getInstance()->virtualRouterApi_->create_virtual_router(
      virtual_router_id, switch_id, attr_count, attr_list);

  SaiTracer::getInstance()->logCreateFn(
      "create_virtual_router",
      virtual_router_id,
      switch_id,
      attr_count,
      attr_list,
      SAI_OBJECT_TYPE_VIRTUAL_ROUTER,
      rv);
  return rv;
}

sai_status_t wrap_remove_virtual_router(sai_object_id_t virtual_router_id) {
  auto rv = SaiTracer::getInstance()->virtualRouterApi_->remove_virtual_router(
      virtual_router_id);

  SaiTracer::getInstance()->logRemoveFn(
      "remove_virtual_router",
      virtual_router_id,
      SAI_OBJECT_TYPE_VIRTUAL_ROUTER,
      rv);
  return rv;
}

sai_status_t wrap_set_virtual_router_attribute(
    sai_object_id_t virtual_router_id,
    const sai_attribute_t* attr) {
  auto rv =
      SaiTracer::getInstance()->virtualRouterApi_->set_virtual_router_attribute(
          virtual_router_id, attr);

  SaiTracer::getInstance()->logSetAttrFn(
      "set_virtual_router_attribute",
      virtual_router_id,
      attr,
      SAI_OBJECT_TYPE_VIRTUAL_ROUTER,
      rv);
  return rv;
}

sai_status_t wrap_get_virtual_router_attribute(
    sai_object_id_t virtual_router_id,
    uint32_t attr_count,
    sai_attribute_t* attr_list) {
  // TODO(zecheng): Log get functions as well
  return SaiTracer::getInstance()
      ->virtualRouterApi_->get_virtual_router_attribute(
          virtual_router_id, attr_count, attr_list);
}

sai_virtual_router_api_t* wrappedVirtualRouterApi() {
  static sai_virtual_router_api_t virtualRouterWrappers;

  virtualRouterWrappers.create_virtual_router = &wrap_create_virtual_router;
  virtualRouterWrappers.remove_virtual_router = &wrap_remove_virtual_router;
  virtualRouterWrappers.set_virtual_router_attribute =
      &wrap_set_virtual_router_attribute;
  virtualRouterWrappers.get_virtual_router_attribute =
      &wrap_get_virtual_router_attribute;

  return &virtualRouterWrappers;
}

void setVirtualRouterAttributes(
    const sai_attribute_t* attr_list,
    uint32_t attr_count,
    std::vector<std::string>& attrLines) {
  for (int i = 0; i < attr_count; ++i) {
    switch (attr_list[i].id) {
      case SAI_VIRTUAL_ROUTER_ATTR_SRC_MAC_ADDRESS:
        macAddressAttr(attr_list, i, attrLines);
        break;
      default:
        // TODO(zecheng): Better check for newly added attributes (T69350100)
        break;
    }
  }
}

} // namespace facebook::fboss
