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

WRAP_CREATE_FUNC(next_hop_group, SAI_OBJECT_TYPE_NEXT_HOP_GROUP, nextHopGroup);
WRAP_REMOVE_FUNC(next_hop_group, SAI_OBJECT_TYPE_NEXT_HOP_GROUP, nextHopGroup);
WRAP_SET_ATTR_FUNC(
    next_hop_group,
    SAI_OBJECT_TYPE_NEXT_HOP_GROUP,
    nextHopGroup);
WRAP_GET_ATTR_FUNC(
    next_hop_group,
    SAI_OBJECT_TYPE_NEXT_HOP_GROUP,
    nextHopGroup);

WRAP_CREATE_FUNC(
    next_hop_group_member,
    SAI_OBJECT_TYPE_NEXT_HOP_GROUP_MEMBER,
    nextHopGroup);
WRAP_REMOVE_FUNC(
    next_hop_group_member,
    SAI_OBJECT_TYPE_NEXT_HOP_GROUP_MEMBER,
    nextHopGroup);
WRAP_SET_ATTR_FUNC(
    next_hop_group_member,
    SAI_OBJECT_TYPE_NEXT_HOP_GROUP_MEMBER,
    nextHopGroup);
WRAP_GET_ATTR_FUNC(
    next_hop_group_member,
    SAI_OBJECT_TYPE_NEXT_HOP_GROUP_MEMBER,
    nextHopGroup);

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
        break;
    }
  }
}

} // namespace facebook::fboss
