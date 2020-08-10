/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/sai/tracer/NextHopGroupApiTracer.h"
#include "fboss/agent/hw/sai/tracer/Utils.h"

namespace facebook::fboss {

sai_status_t wrap_create_next_hop_group(
    sai_object_id_t* next_hop_group_id,
    sai_object_id_t switch_id,
    uint32_t attr_count,
    const sai_attribute_t* attr_list) {
  auto rv = SaiTracer::getInstance()->nextHopGroupApi_->create_next_hop_group(
      next_hop_group_id, switch_id, attr_count, attr_list);

  SaiTracer::getInstance()->logCreateFn(
      "create_next_hop_group",
      next_hop_group_id,
      switch_id,
      attr_count,
      attr_list,
      SAI_OBJECT_TYPE_NEXT_HOP_GROUP,
      rv);
  return rv;
}

sai_status_t wrap_remove_next_hop_group(sai_object_id_t next_hop_group_id) {
  auto rv = SaiTracer::getInstance()->nextHopGroupApi_->remove_next_hop_group(
      next_hop_group_id);

  SaiTracer::getInstance()->logRemoveFn(
      "remove_next_hop_group",
      next_hop_group_id,
      SAI_OBJECT_TYPE_NEXT_HOP_GROUP,
      rv);
  return rv;
}

sai_status_t wrap_set_next_hop_group_attribute(
    sai_object_id_t next_hop_group_id,
    const sai_attribute_t* attr) {
  auto rv =
      SaiTracer::getInstance()->nextHopGroupApi_->set_next_hop_group_attribute(
          next_hop_group_id, attr);

  SaiTracer::getInstance()->logSetAttrFn(
      "set_next_hop_group_attribute",
      next_hop_group_id,
      attr,
      SAI_OBJECT_TYPE_NEXT_HOP_GROUP,
      rv);
  return rv;
}

sai_status_t wrap_get_next_hop_group_attribute(
    sai_object_id_t next_hop_group_id,
    uint32_t attr_count,
    sai_attribute_t* attr_list) {
  // TODO(zecheng): Log get functions as well
  return SaiTracer::getInstance()
      ->nextHopGroupApi_->get_next_hop_group_attribute(
          next_hop_group_id, attr_count, attr_list);
}

sai_status_t wrap_create_next_hop_group_member(
    sai_object_id_t* next_hop_group_member_id,
    sai_object_id_t switch_id,
    uint32_t attr_count,
    const sai_attribute_t* attr_list) {
  auto rv =
      SaiTracer::getInstance()->nextHopGroupApi_->create_next_hop_group_member(
          next_hop_group_member_id, switch_id, attr_count, attr_list);

  SaiTracer::getInstance()->logCreateFn(
      "create_next_hop_group_member",
      next_hop_group_member_id,
      switch_id,
      attr_count,
      attr_list,
      SAI_OBJECT_TYPE_NEXT_HOP_GROUP_MEMBER,
      rv);
  return rv;
}

sai_status_t wrap_remove_next_hop_group_member(
    sai_object_id_t next_hop_group_member_id) {
  auto rv =
      SaiTracer::getInstance()->nextHopGroupApi_->remove_next_hop_group_member(
          next_hop_group_member_id);

  SaiTracer::getInstance()->logRemoveFn(
      "remove_next_hop_group_member",
      next_hop_group_member_id,
      SAI_OBJECT_TYPE_NEXT_HOP_GROUP_MEMBER,
      rv);
  return rv;
}

sai_status_t wrap_set_next_hop_group_member_attribute(
    sai_object_id_t next_hop_group_member_id,
    const sai_attribute_t* attr) {
  auto rv = SaiTracer::getInstance()
                ->nextHopGroupApi_->set_next_hop_group_member_attribute(
                    next_hop_group_member_id, attr);

  SaiTracer::getInstance()->logSetAttrFn(
      "set_next_hop_group_member_attribute",
      next_hop_group_member_id,
      attr,
      SAI_OBJECT_TYPE_NEXT_HOP_GROUP_MEMBER,
      rv);
  return rv;
}

sai_status_t wrap_get_next_hop_group_member_attribute(
    sai_object_id_t next_hop_group_member_id,
    uint32_t attr_count,
    sai_attribute_t* attr_list) {
  // TODO(zecheng): Log get functions as well
  return SaiTracer::getInstance()
      ->nextHopGroupApi_->get_next_hop_group_member_attribute(
          next_hop_group_member_id, attr_count, attr_list);
}

sai_next_hop_group_api_t* wrappedNextHopGroupApi() {
  static sai_next_hop_group_api_t nextHopGroupWrappers;

  nextHopGroupWrappers.create_next_hop_group = &wrap_create_next_hop_group;
  nextHopGroupWrappers.remove_next_hop_group = &wrap_remove_next_hop_group;
  nextHopGroupWrappers.set_next_hop_group_attribute =
      &wrap_set_next_hop_group_attribute;
  nextHopGroupWrappers.get_next_hop_group_attribute =
      &wrap_get_next_hop_group_attribute;
  nextHopGroupWrappers.create_next_hop_group_member =
      &wrap_create_next_hop_group_member;
  nextHopGroupWrappers.remove_next_hop_group_member =
      &wrap_remove_next_hop_group_member;
  nextHopGroupWrappers.set_next_hop_group_member_attribute =
      &wrap_set_next_hop_group_member_attribute;
  nextHopGroupWrappers.get_next_hop_group_member_attribute =
      &wrap_get_next_hop_group_member_attribute;

  return &nextHopGroupWrappers;
}

void setNextHopGroupAttributes(
    const sai_attribute_t* attr_list,
    uint32_t attr_count,
    std::vector<std::string>& attrLines) {
  uint32_t listCount = 0;

  for (int i = 0; i < attr_count; ++i) {
    switch (attr_list[i].id) {
      case SAI_NEXT_HOP_GROUP_ATTR_NEXT_HOP_MEMBER_LIST:
        oidListAttr(attr_list, i, listCount++, attrLines);
        break;
      case SAI_NEXT_HOP_GROUP_ATTR_TYPE:
        attrLines.push_back(s32Attr(attr_list, i));
        break;
      default:
        // TODO(zecheng): Better check for newly added attributes (T69350100)
        break;
    }
  }
}

void setNextHopGroupMemberAttributes(
    const sai_attribute_t* attr_list,
    uint32_t attr_count,
    std::vector<std::string>& attrLines) {
  for (int i = 0; i < attr_count; ++i) {
    switch (attr_list[i].id) {
      case SAI_NEXT_HOP_GROUP_MEMBER_ATTR_NEXT_HOP_ID:
      case SAI_NEXT_HOP_GROUP_MEMBER_ATTR_NEXT_HOP_GROUP_ID:
        attrLines.push_back(oidAttr(attr_list, i));
        break;
      case SAI_NEXT_HOP_GROUP_MEMBER_ATTR_WEIGHT:
        attrLines.push_back(u32Attr(attr_list, i));
        break;
      default:
        // TODO(zecheng): Better check for newly added attributes (T69350100)
        break;
    }
  }
}

} // namespace facebook::fboss
