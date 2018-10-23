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

#include <unordered_map>

extern "C" {
  #include <sai.h>
}

sai_status_t create_vlan_fn(
    sai_object_id_t* vlan_id,
    sai_object_id_t switch_id,
    uint32_t attr_count,
    const sai_attribute_t* attr_list);

sai_status_t remove_vlan_fn(sai_object_id_t vlan_id);

sai_status_t get_vlan_attribute_fn(
    sai_object_id_t vlan_id,
    uint32_t attr_count,
    sai_attribute_t* attr);

sai_status_t set_vlan_attribute_fn(
    sai_object_id_t vlan_id,
    const sai_attribute_t* attr);

sai_status_t create_vlan_member_fn(
    sai_object_id_t* vlan_member_id,
    sai_object_id_t switch_id,
    uint32_t attr_count,
    const sai_attribute_t* attr_list);

sai_status_t remove_vlan_member_fn(sai_object_id_t vlan_member_id);

sai_status_t get_vlan_member_attribute_fn(
    sai_object_id_t vlan_id,
    uint32_t attr_count,
    sai_attribute_t* attr);

sai_status_t set_vlan_member_attribute_fn(
    sai_object_id_t vlan_member_id,
    const sai_attribute_t* attr);

namespace facebook {
namespace fboss {

class FakeVlanMember {
 public:
  sai_object_id_t bridgePortId;
  sai_object_id_t vlanId;
  sai_object_id_t id;
};

class FakeVlan {
 public:
  sai_object_id_t id;
  std::unordered_map<sai_object_id_t, FakeVlanMember>& memberMap() {
    return memberMap_;
  }
  const std::unordered_map<sai_object_id_t, FakeVlanMember>& memberMap() const {
    return memberMap_;
  }

 private:
  std::unordered_map<sai_object_id_t, FakeVlanMember> memberMap_;
};

class FakeVlanManager {
 public:
  sai_object_id_t addVlan(const FakeVlan& b) {
    auto ins = vlanMap_.insert({++vlanId, b});
    ins.first->second.id = vlanId;
    return vlanId;
  }
  void deleteVlan(sai_object_id_t vlanId) {
    size_t erased = vlanMap_.erase(vlanId);
    if (!erased) {
      //TODO
    }
  }
  FakeVlan& getVlan(const sai_object_id_t& vlanId) {
    return vlanMap_.at(vlanId);
  }
  const FakeVlan& getVlan(const sai_object_id_t& vlanId) const {
    return vlanMap_.at(vlanId);
  }
  sai_object_id_t addVlanMember(
      sai_object_id_t vlanId,
      const FakeVlanMember& bp) {
    auto& vlanMemberMap = getVlan(vlanId).memberMap();
    auto ins = vlanMemberMap.insert({++vlanMemberId, bp});
    ins.first->second.id = vlanMemberId;
    vlanMemberToVlanMap_.insert({vlanMemberId, vlanId});
    return vlanMemberId;
  }
  void deleteVlanMember(sai_object_id_t vlanMemberId) {
    auto vlanId = vlanMemberToVlanMap_.at(vlanMemberId);
    vlanMemberToVlanMap_.erase(vlanMemberId);
    auto& vlanMemberMap = getVlan(vlanId).memberMap();
    size_t erased = vlanMemberMap.erase(vlanMemberId);
    if (!erased) {
      //TODO
    }
  }

  FakeVlanMember& getVlanMember(const sai_object_id_t& vlanMemberId) {
    auto vlanId = vlanMemberToVlanMap_.at(vlanMemberId);
    auto& vlanMemberMap = getVlan(vlanId).memberMap();
    return vlanMemberMap.at(vlanMemberId);
  }

  const FakeVlanMember& getVlanMember(
      const sai_object_id_t& vlanMemberId) const {
    auto vlanId = vlanMemberToVlanMap_.at(vlanMemberId);
    auto vlanMemberMap = getVlan(vlanId).memberMap();
    return vlanMemberMap.at(vlanMemberId);
  }

 private:
  size_t vlanId = 0;
  size_t vlanMemberId = 0;
  std::unordered_map<sai_object_id_t, sai_object_id_t> vlanMemberToVlanMap_;
  std::unordered_map<sai_object_id_t, FakeVlan> vlanMap_;
};

void populate_vlan_api(sai_vlan_api_t** vlan_api);
} // namespace fboss
} // namespace facebook
