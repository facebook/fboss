/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/sai/tracer/VlanApiTracer.h"
#include <typeindex>
#include <utility>

#include "fboss/agent/hw/sai/api/VlanApi.h"
#include "fboss/agent/hw/sai/tracer/Utils.h"

using folly::to;

namespace {
std::map<int32_t, std::pair<std::string, std::size_t>> _VlanMap{
    SAI_ATTR_MAP(Vlan, VlanId),
    SAI_ATTR_MAP(Vlan, MemberList),
};

std::map<int32_t, std::pair<std::string, std::size_t>> _VlanMemberMap{
    SAI_ATTR_MAP(VlanMember, BridgePortId),
    SAI_ATTR_MAP(VlanMember, VlanId),
};

} // namespace

namespace facebook::fboss {

WRAP_CREATE_FUNC(vlan, SAI_OBJECT_TYPE_VLAN, vlan);
WRAP_REMOVE_FUNC(vlan, SAI_OBJECT_TYPE_VLAN, vlan);
WRAP_SET_ATTR_FUNC(vlan, SAI_OBJECT_TYPE_VLAN, vlan);
WRAP_GET_ATTR_FUNC(vlan, SAI_OBJECT_TYPE_VLAN, vlan);

WRAP_CREATE_FUNC(vlan_member, SAI_OBJECT_TYPE_VLAN_MEMBER, vlan);
WRAP_REMOVE_FUNC(vlan_member, SAI_OBJECT_TYPE_VLAN_MEMBER, vlan);
WRAP_SET_ATTR_FUNC(vlan_member, SAI_OBJECT_TYPE_VLAN_MEMBER, vlan);
WRAP_GET_ATTR_FUNC(vlan_member, SAI_OBJECT_TYPE_VLAN_MEMBER, vlan);

sai_vlan_api_t* wrappedVlanApi() {
  static sai_vlan_api_t vlanWrappers;

  vlanWrappers.create_vlan = &wrap_create_vlan;
  vlanWrappers.remove_vlan = &wrap_remove_vlan;
  vlanWrappers.set_vlan_attribute = &wrap_set_vlan_attribute;
  vlanWrappers.get_vlan_attribute = &wrap_get_vlan_attribute;
  vlanWrappers.create_vlan_member = &wrap_create_vlan_member;
  vlanWrappers.remove_vlan_member = &wrap_remove_vlan_member;
  vlanWrappers.set_vlan_member_attribute = &wrap_set_vlan_member_attribute;
  vlanWrappers.get_vlan_member_attribute = &wrap_get_vlan_member_attribute;

  return &vlanWrappers;
}

SET_SAI_ATTRIBUTES(Vlan)
SET_SAI_ATTRIBUTES(VlanMember)

} // namespace facebook::fboss
