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
#include "fboss/agent/hw/sai/api/SaiVersion.h"
#include "fboss/agent/hw/sai/tracer/Utils.h"

namespace facebook::fboss {

sai_status_t wrap_create_next_hop(
    sai_object_id_t* next_hop_id,
    sai_object_id_t switch_id,
    uint32_t attr_count,
    const sai_attribute_t* attr_list) {
  auto rv = SaiTracer::getInstance()->nextHopApi_->create_next_hop(
      next_hop_id, switch_id, attr_count, attr_list);

  SaiTracer::getInstance()->logCreateFn(
      "create_next_hop",
      next_hop_id,
      switch_id,
      attr_count,
      attr_list,
      SAI_OBJECT_TYPE_NEXT_HOP,
      rv);
  return rv;
}

sai_status_t wrap_remove_next_hop(sai_object_id_t next_hop_id) {
  auto rv = SaiTracer::getInstance()->nextHopApi_->remove_next_hop(next_hop_id);

  SaiTracer::getInstance()->logRemoveFn(
      "remove_next_hop", next_hop_id, SAI_OBJECT_TYPE_NEXT_HOP, rv);
  return rv;
}

sai_status_t wrap_set_next_hop_attribute(
    sai_object_id_t next_hop_id,
    const sai_attribute_t* attr) {
  auto rv = SaiTracer::getInstance()->nextHopApi_->set_next_hop_attribute(
      next_hop_id, attr);

  SaiTracer::getInstance()->logSetAttrFn(
      "set_next_hop_attribute",
      next_hop_id,
      attr,
      SAI_OBJECT_TYPE_NEXT_HOP,
      rv);
  return rv;
}

sai_status_t wrap_get_next_hop_attribute(
    sai_object_id_t next_hop_id,
    uint32_t attr_count,
    sai_attribute_t* attr_list) {
  // TODO(zecheng): Log get functions as well
  return SaiTracer::getInstance()->nextHopApi_->get_next_hop_attribute(
      next_hop_id, attr_count, attr_list);
}

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
      case SAI_NEXT_HOP_ATTR_DECREMENT_TTL:
        attrLines.push_back(boolAttr(attr_list, i));
        break;
      default:
        // TODO(zecheng): Better check for newly added attributes (T69350100)
        break;
    }
  }
}

} // namespace facebook::fboss
