/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/sai/tracer/NextHopApiTracer.h"
#include "fboss/agent/hw/sai/tracer/Utils.h"

namespace facebook::fboss {

WRAP_CREATE_FUNC(next_hop, SAI_OBJECT_TYPE_NEXT_HOP, nextHop);
WRAP_REMOVE_FUNC(next_hop, SAI_OBJECT_TYPE_NEXT_HOP, nextHop);
WRAP_SET_ATTR_FUNC(next_hop, SAI_OBJECT_TYPE_NEXT_HOP, nextHop);
WRAP_GET_ATTR_FUNC(next_hop, SAI_OBJECT_TYPE_NEXT_HOP, nextHop);

sai_next_hop_api_t* wrappedNextHopApi() {
  static sai_next_hop_api_t nextHopWrappers;

  nextHopWrappers.create_next_hop = &wrap_create_next_hop;
  nextHopWrappers.remove_next_hop = &wrap_remove_next_hop;
  nextHopWrappers.set_next_hop_attribute = &wrap_set_next_hop_attribute;
  nextHopWrappers.get_next_hop_attribute = &wrap_get_next_hop_attribute;

  return &nextHopWrappers;
}

void setNextHopAttributes(
    const sai_attribute_t* attr_list,
    uint32_t attr_count,
    std::vector<std::string>& attrLines) {
  uint32_t listCount = 0;

  for (int i = 0; i < attr_count; ++i) {
    switch (attr_list[i].id) {
      case SAI_NEXT_HOP_ATTR_IP:
        ipAttr(attr_list, i, attrLines);
        break;
      case SAI_NEXT_HOP_ATTR_ROUTER_INTERFACE_ID:
        attrLines.push_back(oidAttr(attr_list, i));
        break;
      case SAI_NEXT_HOP_ATTR_LABELSTACK:
        u32ListAttr(attr_list, i, listCount++, attrLines);
        break;
      case SAI_NEXT_HOP_ATTR_TYPE:
        attrLines.push_back(s32Attr(attr_list, i));
        break;
#if SAI_API_VERSION >= SAI_VERSION(1, 7, 0)
      case SAI_NEXT_HOP_ATTR_DISABLE_DECREMENT_TTL:
#else
      case SAI_NEXT_HOP_ATTR_DECREMENT_TTL:
#endif
        attrLines.push_back(boolAttr(attr_list, i));
        break;
      default:
        break;
    }
  }
}

} // namespace facebook::fboss
