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

namespace facebook::fboss {

class FakeNextHopGroupMember {
 public:
  FakeNextHopGroupMember(
      sai_object_id_t nextHopGroupId,
      sai_object_id_t nextHopId,
      std::optional<sai_uint32_t> weight)
      : nextHopGroupId(nextHopGroupId), nextHopId(nextHopId), weight(weight) {}
  sai_object_id_t nextHopGroupId;
  sai_object_id_t nextHopId;
  sai_object_id_t id;
  std::optional<sai_uint32_t> weight;
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

} // namespace facebook::fboss
