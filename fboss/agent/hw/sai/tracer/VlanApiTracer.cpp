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
#include "fboss/agent/hw/sai/tracer/Utils.h"

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

void setVlanAttributes(
    const sai_attribute_t* attr_list,
    uint32_t attr_count,
    std::vector<std::string>& attrLines) {
  uint32_t listCount = 0;

  for (int i = 0; i < attr_count; ++i) {
    switch (attr_list[i].id) {
      case SAI_VLAN_ATTR_VLAN_ID:
        attrLines.push_back(u16Attr(attr_list, i));
        break;
      case SAI_VLAN_ATTR_MEMBER_LIST:
        oidListAttr(attr_list, i, listCount++, attrLines);
        break;
      default:
        break;
    }
  }
}

void setVlanMemberAttributes(
    const sai_attribute_t* attr_list,
    uint32_t attr_count,
    std::vector<std::string>& attrLines) {
  for (int i = 0; i < attr_count; ++i) {
    switch (attr_list[i].id) {
      case SAI_VLAN_MEMBER_ATTR_BRIDGE_PORT_ID:
      case SAI_VLAN_MEMBER_ATTR_VLAN_ID:
        attrLines.push_back(oidAttr(attr_list, i));
        break;
      default:
        break;
    }
  }
}

} // namespace facebook::fboss
