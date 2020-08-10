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

sai_status_t wrap_create_vlan(
    sai_object_id_t* vlan_id,
    sai_object_id_t switch_id,
    uint32_t attr_count,
    const sai_attribute_t* attr_list) {
  auto rv = SaiTracer::getInstance()->vlanApi_->create_vlan(
      vlan_id, switch_id, attr_count, attr_list);

  SaiTracer::getInstance()->logCreateFn(
      "create_vlan",
      vlan_id,
      switch_id,
      attr_count,
      attr_list,
      SAI_OBJECT_TYPE_VLAN,
      rv);
  return rv;
}

sai_status_t wrap_remove_vlan(sai_object_id_t vlan_id) {
  auto rv = SaiTracer::getInstance()->vlanApi_->remove_vlan(vlan_id);

  SaiTracer::getInstance()->logRemoveFn(
      "remove_vlan", vlan_id, SAI_OBJECT_TYPE_VLAN, rv);
  return rv;
}

sai_status_t wrap_set_vlan_attribute(
    sai_object_id_t vlan_id,
    const sai_attribute_t* attr) {
  auto rv =
      SaiTracer::getInstance()->vlanApi_->set_vlan_attribute(vlan_id, attr);

  SaiTracer::getInstance()->logSetAttrFn(
      "set_vlan_attribute", vlan_id, attr, SAI_OBJECT_TYPE_VLAN, rv);
  return rv;
}

sai_status_t wrap_get_vlan_attribute(
    sai_object_id_t vlan_id,
    uint32_t attr_count,
    sai_attribute_t* attr_list) {
  return SaiTracer::getInstance()->vlanApi_->get_vlan_attribute(
      vlan_id, attr_count, attr_list);
}

sai_status_t wrap_create_vlan_member(
    sai_object_id_t* vlan_member_id,
    sai_object_id_t switch_id,
    uint32_t attr_count,
    const sai_attribute_t* attr_list) {
  auto rv = SaiTracer::getInstance()->vlanApi_->create_vlan_member(
      vlan_member_id, switch_id, attr_count, attr_list);

  SaiTracer::getInstance()->logCreateFn(
      "create_vlan_member",
      vlan_member_id,
      switch_id,
      attr_count,
      attr_list,
      SAI_OBJECT_TYPE_VLAN_MEMBER,
      rv);
  return rv;
}

sai_status_t wrap_remove_vlan_member(sai_object_id_t vlan_member_id) {
  auto rv =
      SaiTracer::getInstance()->vlanApi_->remove_vlan_member(vlan_member_id);

  SaiTracer::getInstance()->logRemoveFn(
      "remove_vlan_member", vlan_member_id, SAI_OBJECT_TYPE_VLAN_MEMBER, rv);
  return rv;
}

sai_status_t wrap_set_vlan_member_attribute(
    sai_object_id_t vlan_member_id,
    const sai_attribute_t* attr) {
  auto rv = SaiTracer::getInstance()->vlanApi_->set_vlan_member_attribute(
      vlan_member_id, attr);

  SaiTracer::getInstance()->logSetAttrFn(
      "set_vlan_member_attribute",
      vlan_member_id,
      attr,
      SAI_OBJECT_TYPE_VLAN_MEMBER,
      rv);
  return rv;
}

sai_status_t wrap_get_vlan_member_attribute(
    sai_object_id_t vlan_member_id,
    uint32_t attr_count,
    sai_attribute_t* attr_list) {
  return SaiTracer::getInstance()->vlanApi_->get_vlan_member_attribute(
      vlan_member_id, attr_count, attr_list);
}

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
        // TODO(zecheng): Better check for newly added attributes (T69350100)
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
        // TODO(zecheng): Better check for newly added attributes (T69350100)
        break;
    }
  }
}

} // namespace facebook::fboss
