/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#pragma once

#include "fboss/agent/hw/sai/fake/FakeManager.h"

extern "C" {
#include <sai.h>
}

sai_status_t create_next_hop_group_fn(
    sai_object_id_t* next_hop_group_id,
    sai_object_id_t switch_id,
    uint32_t attr_count,
    const sai_attribute_t* attr_list);

sai_status_t remove_next_hop_group_fn(sai_object_id_t next_hop_group_id);

sai_status_t get_next_hop_group_attribute_fn(
    sai_object_id_t next_hop_group_id,
    uint32_t attr_count,
    sai_attribute_t* attr);

sai_status_t set_next_hop_group_attribute_fn(
    sai_object_id_t next_hop_group_id,
    const sai_attribute_t* attr);

sai_status_t create_next_hop_group_member_fn(
    sai_object_id_t* next_hop_group_member_id,
    sai_object_id_t switch_id,
    uint32_t attr_count,
    const sai_attribute_t* attr_list);

sai_status_t remove_next_hop_group_member_fn(
    sai_object_id_t next_hop_group_member_id);

sai_status_t get_next_hop_group_member_attribute_fn(
    sai_object_id_t next_hop_group_id,
    uint32_t attr_count,
    sai_attribute_t* attr);

sai_status_t set_next_hop_group_member_attribute_fn(
    sai_object_id_t next_hop_group_member_id,
    const sai_attribute_t* attr);

namespace facebook {
namespace fboss {

class FakeNextHopGroupMember {
 public:
  FakeNextHopGroupMember(
      sai_object_id_t nextHopGroupId,
      sai_object_id_t nextHopId)
      : nextHopGroupId(nextHopGroupId), nextHopId(nextHopId) {}
  sai_object_id_t nextHopGroupId;
  sai_object_id_t nextHopId;
  sai_object_id_t id;
};

class FakeNextHopGroup {
 public:
  FakeNextHopGroup(int32_t type) : type(type) {}
  int32_t type;
  sai_object_id_t id;
  FakeManager<sai_object_id_t, FakeNextHopGroupMember>& fm() {
    return fm_;
  }
  const FakeManager<sai_object_id_t, FakeNextHopGroupMember>& fm() const {
    return fm_;
  }

 private:
  FakeManager<sai_object_id_t, FakeNextHopGroupMember> fm_;
};

using FakeNextHopGroupManager =
    FakeManagerWithMembers<FakeNextHopGroup, FakeNextHopGroupMember>;

void populate_next_hop_group_api(sai_next_hop_group_api_t** next_hop_group_api);
} // namespace fboss
} // namespace facebook
