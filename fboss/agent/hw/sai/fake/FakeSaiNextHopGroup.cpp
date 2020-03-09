/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "FakeSaiNextHopGroup.h"
#include "FakeSai.h"

#include <folly/logging/xlog.h>
#include <optional>

using facebook::fboss::FakeNextHopGroup;
using facebook::fboss::FakeNextHopGroupMember;
using facebook::fboss::FakeSai;

sai_status_t create_next_hop_group_fn(
    sai_object_id_t* next_hop_group_id,
    sai_object_id_t /* switch_id */,
    uint32_t attr_count,
    const sai_attribute_t* attr_list) {
  auto fs = FakeSai::getInstance();
  std::optional<int32_t> type;
  for (int i = 0; i < attr_count; ++i) {
    switch (attr_list[i].id) {
      case SAI_NEXT_HOP_GROUP_ATTR_TYPE:
        type = attr_list[i].value.s32;
        break;
      default:
        return SAI_STATUS_NOT_SUPPORTED;
        break;
    }
  }
  if (!type) {
    return SAI_STATUS_INVALID_PARAMETER;
  }
  if (type.value() != SAI_NEXT_HOP_GROUP_TYPE_ECMP) {
    return SAI_STATUS_INVALID_PARAMETER;
  }
  *next_hop_group_id = fs->nextHopGroupManager.create(type.value());
  return SAI_STATUS_SUCCESS;
}

sai_status_t remove_next_hop_group_fn(sai_object_id_t next_hop_group_id) {
  auto fs = FakeSai::getInstance();
  fs->nextHopGroupManager.remove(next_hop_group_id);
  return SAI_STATUS_SUCCESS;
}

sai_status_t get_next_hop_group_attribute_fn(
    sai_object_id_t next_hop_group_id,
    uint32_t attr_count,
    sai_attribute_t* attr) {
  auto fs = FakeSai::getInstance();
  const auto& nextHopGroup = fs->nextHopGroupManager.get(next_hop_group_id);
  for (int i = 0; i < attr_count; ++i) {
    switch (attr[i].id) {
      case SAI_NEXT_HOP_GROUP_ATTR_TYPE:
        attr[i].value.s32 = nextHopGroup.id;
        break;
      case SAI_NEXT_HOP_GROUP_ATTR_NEXT_HOP_MEMBER_LIST: {
        const auto& nextHopGroupMemberMap =
            fs->nextHopGroupManager.get(next_hop_group_id).fm().map();
        if (nextHopGroupMemberMap.size() > attr[i].value.objlist.count) {
          attr[i].value.objlist.count = nextHopGroupMemberMap.size();
          return SAI_STATUS_BUFFER_OVERFLOW;
        }
        attr[i].value.objlist.count = nextHopGroupMemberMap.size();
        int j = 0;
        for (const auto& m : nextHopGroupMemberMap) {
          attr[i].value.objlist.list[j++] = m.first;
        }
      } break;
      default:
        return SAI_STATUS_NOT_SUPPORTED;
    }
  }
  return SAI_STATUS_SUCCESS;
}

sai_status_t set_next_hop_group_attribute_fn(
    sai_object_id_t /* next_hop_group_id */,
    const sai_attribute_t* attr) {
  switch (attr->id) {
    default:
      return SAI_STATUS_NOT_SUPPORTED;
  }
  return SAI_STATUS_SUCCESS;
}

sai_status_t create_next_hop_group_member_fn(
    sai_object_id_t* next_hop_group_member_id,
    sai_object_id_t /* switch_id */,
    uint32_t attr_count,
    const sai_attribute_t* attr_list) {
  auto fs = FakeSai::getInstance();
  std::optional<sai_object_id_t> nextHopGroupId;
  std::optional<sai_object_id_t> nextHopId;
  std::optional<sai_uint32_t> weight = std::nullopt;
  for (int i = 0; i < attr_count; ++i) {
    switch (attr_list[i].id) {
      case SAI_NEXT_HOP_GROUP_MEMBER_ATTR_NEXT_HOP_GROUP_ID:
        nextHopGroupId = attr_list[i].value.oid;
        break;
      case SAI_NEXT_HOP_GROUP_MEMBER_ATTR_NEXT_HOP_ID:
        nextHopId = attr_list[i].value.oid;
        break;
      case SAI_NEXT_HOP_GROUP_MEMBER_ATTR_WEIGHT:
        weight = attr_list[i].value.u64;
        break;
      default:
        return SAI_STATUS_NOT_SUPPORTED;
    }
  }
  if (!nextHopGroupId || !nextHopId) {
    return SAI_STATUS_INVALID_PARAMETER;
  }
  *next_hop_group_member_id = fs->nextHopGroupManager.createMember(
      nextHopGroupId.value(),
      nextHopGroupId.value(),
      nextHopId.value(),
      weight);
  return SAI_STATUS_SUCCESS;
}

sai_status_t remove_next_hop_group_member_fn(
    sai_object_id_t next_hop_group_member_id) {
  auto fs = FakeSai::getInstance();
  fs->nextHopGroupManager.removeMember(next_hop_group_member_id);
  return SAI_STATUS_SUCCESS;
}

sai_status_t get_next_hop_group_member_attribute_fn(
    sai_object_id_t next_hop_group_member_id,
    uint32_t attr_count,
    sai_attribute_t* attr) {
  auto fs = FakeSai::getInstance();
  auto& nextHopGroupMember =
      fs->nextHopGroupManager.getMember(next_hop_group_member_id);
  for (int i = 0; i < attr_count; ++i) {
    switch (attr[i].id) {
      case SAI_NEXT_HOP_GROUP_MEMBER_ATTR_NEXT_HOP_GROUP_ID:
        attr[i].value.oid = nextHopGroupMember.nextHopGroupId;
        break;
      case SAI_NEXT_HOP_GROUP_MEMBER_ATTR_NEXT_HOP_ID:
        attr[i].value.oid = nextHopGroupMember.nextHopId;
        break;
      case SAI_NEXT_HOP_GROUP_MEMBER_ATTR_WEIGHT:
        if (auto weight = nextHopGroupMember.weight) {
          attr[i].value.u32 = *weight;
        }
        break;
      default:
        return SAI_STATUS_NOT_SUPPORTED;
    }
  }
  return SAI_STATUS_SUCCESS;
}

sai_status_t set_next_hop_group_member_attribute_fn(
    sai_object_id_t next_hop_group_member_id,
    const sai_attribute_t* attr) {
  switch (attr->id) {
    default:
      return SAI_STATUS_NOT_SUPPORTED;
  }
  return SAI_STATUS_SUCCESS;
}

namespace facebook::fboss {

static sai_next_hop_group_api_t _next_hop_group_api;

void populate_next_hop_group_api(
    sai_next_hop_group_api_t** next_hop_group_api) {
  _next_hop_group_api.create_next_hop_group = &create_next_hop_group_fn;
  _next_hop_group_api.remove_next_hop_group = &remove_next_hop_group_fn;
  _next_hop_group_api.set_next_hop_group_attribute =
      &set_next_hop_group_attribute_fn;
  _next_hop_group_api.get_next_hop_group_attribute =
      &get_next_hop_group_attribute_fn;
  _next_hop_group_api.create_next_hop_group_member =
      &create_next_hop_group_member_fn;
  _next_hop_group_api.remove_next_hop_group_member =
      &remove_next_hop_group_member_fn;
  _next_hop_group_api.set_next_hop_group_member_attribute =
      &set_next_hop_group_member_attribute_fn;
  _next_hop_group_api.get_next_hop_group_member_attribute =
      &get_next_hop_group_member_attribute_fn;
  *next_hop_group_api = &_next_hop_group_api;
}

} // namespace facebook::fboss
