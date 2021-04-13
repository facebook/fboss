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

WRAP_CREATE_FUNC(virtual_router, SAI_OBJECT_TYPE_VIRTUAL_ROUTER, virtualRouter);
WRAP_REMOVE_FUNC(virtual_router, SAI_OBJECT_TYPE_VIRTUAL_ROUTER, virtualRouter);
WRAP_SET_ATTR_FUNC(
    virtual_router,
    SAI_OBJECT_TYPE_VIRTUAL_ROUTER,
    virtualRouter);
WRAP_GET_ATTR_FUNC(
    virtual_router,
    SAI_OBJECT_TYPE_VIRTUAL_ROUTER,
    virtualRouter);

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
        break;
    }
  }
}

} // namespace facebook::fboss
