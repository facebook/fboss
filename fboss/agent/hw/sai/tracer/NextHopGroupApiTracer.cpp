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
#include <typeindex>
#include <utility>

#include "fboss/agent/hw/sai/api/NextHopGroupApi.h"
#include "fboss/agent/hw/sai/tracer/Utils.h"

using folly::to;

namespace {
std::map<int32_t, std::pair<std::string, std::size_t>> _NextHopGroupMap{
    SAI_ATTR_MAP(NextHopGroup, NextHopMemberList),
    SAI_ATTR_MAP(NextHopGroup, Type),
};

std::map<int32_t, std::pair<std::string, std::size_t>> _NextHopGroupMemberMap{
    SAI_ATTR_MAP(NextHopGroupMember, NextHopGroupId),
    SAI_ATTR_MAP(NextHopGroupMember, NextHopId),
    SAI_ATTR_MAP(NextHopGroupMember, Weight),
};

} // namespace

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
#if SAI_API_VERSION >= SAI_VERSION(1, 10, 0)
WRAP_BULK_SET_ATTR_FUNC(
    next_hop_group_member,
    SAI_OBJECT_TYPE_NEXT_HOP_GROUP_MEMBER,
    nextHopGroup);
#endif
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
#if SAI_API_VERSION >= SAI_VERSION(1, 10, 0)
  nextHopGroupWrappers.set_next_hop_group_members_attribute =
      &wrap_set_next_hop_group_members_attribute;
#endif
  nextHopGroupWrappers.get_next_hop_group_member_attribute =
      &wrap_get_next_hop_group_member_attribute;

  return &nextHopGroupWrappers;
}

SET_SAI_ATTRIBUTES(NextHopGroup)
SET_SAI_ATTRIBUTES(NextHopGroupMember)

} // namespace facebook::fboss
