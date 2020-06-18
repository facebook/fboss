/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "FakeSaiVlan.h"
#include "FakeSai.h"

#include <folly/logging/xlog.h>
#include <optional>

using facebook::fboss::FakeSai;
using facebook::fboss::FakeVlan;
using facebook::fboss::FakeVlanMember;

sai_status_t create_vlan_fn(
    sai_object_id_t* vlan_id,
    sai_object_id_t /* switch_id */,
    uint32_t attr_count,
    const sai_attribute_t* attr_list) {
  auto fs = FakeSai::getInstance();
  std::optional<uint16_t> vlanId;
  for (int i = 0; i < attr_count; ++i) {
    switch (attr_list[i].id) {
      case SAI_VLAN_ATTR_VLAN_ID:
        vlanId = attr_list[i].value.u16;
        break;
      default:
        return SAI_STATUS_INVALID_PARAMETER;
        break;
    }
  }
  if (!vlanId) {
    return SAI_STATUS_INVALID_PARAMETER;
  }
  *vlan_id = fs->vlanManager.create(vlanId.value());
  return SAI_STATUS_SUCCESS;
}

sai_status_t remove_vlan_fn(sai_object_id_t vlan_id) {
  auto fs = FakeSai::getInstance();
  fs->vlanManager.remove(vlan_id);
  return SAI_STATUS_SUCCESS;
}

sai_status_t get_vlan_attribute_fn(
    sai_object_id_t vlan_id,
    uint32_t attr_count,
    sai_attribute_t* attr) {
  auto fs = FakeSai::getInstance();
  const auto& vlan = fs->vlanManager.get(vlan_id);
  for (int i = 0; i < attr_count; ++i) {
    switch (attr[i].id) {
      case SAI_VLAN_ATTR_MEMBER_LIST: {
        const auto& vlanMemberMap = fs->vlanManager.get(vlan_id).fm().map();
        if (vlanMemberMap.size() > attr[i].value.objlist.count) {
          attr[i].value.objlist.count = vlanMemberMap.size();
          return SAI_STATUS_BUFFER_OVERFLOW;
        }
        attr[i].value.objlist.count = vlanMemberMap.size();
        int j = 0;
        for (const auto& m : vlanMemberMap) {
          attr[i].value.objlist.list[j++] = m.first;
        }
      } break;
      case SAI_VLAN_ATTR_VLAN_ID:
        attr[i].value.u16 = vlan.vlanId;
        break;
      default:
        return SAI_STATUS_NOT_SUPPORTED;
    }
  }
  return SAI_STATUS_SUCCESS;
}

sai_status_t set_vlan_attribute_fn(
    sai_object_id_t /* vlan_id */,
    const sai_attribute_t* attr) {
  switch (attr->id) {
    default:
      return SAI_STATUS_NOT_SUPPORTED;
  }
  return SAI_STATUS_SUCCESS;
}

sai_status_t set_vlan_member_attribute_fn(
    sai_object_id_t /* vlan_member_id */,
    const sai_attribute_t* attr) {
  sai_status_t res;
  if (!attr) {
    return SAI_STATUS_INVALID_PARAMETER;
  }
  switch (attr->id) {
    default:
      res = SAI_STATUS_NOT_SUPPORTED;
      break;
  }
  return res;
}

sai_status_t create_vlan_member_fn(
    sai_object_id_t* vlan_member_id,
    sai_object_id_t /* switch_id */,
    uint32_t attr_count,
    const sai_attribute_t* attr_list) {
  auto fs = FakeSai::getInstance();
  std::optional<sai_object_id_t> vlanId;
  for (int i = 0; i < attr_count; ++i) {
    if (attr_list[i].id == SAI_VLAN_MEMBER_ATTR_VLAN_ID) {
      vlanId = attr_list[i].value.oid;
      break;
    }
  }
  if (!vlanId) {
    return SAI_STATUS_INVALID_PARAMETER;
  }
  *vlan_member_id =
      fs->vlanManager.createMember(vlanId.value(), vlanId.value());
  auto& vlanMember = fs->vlanManager.getMember(*vlan_member_id);

  for (int i = 0; i < attr_count; ++i) {
    switch (attr_list[i].id) {
      case SAI_VLAN_MEMBER_ATTR_VLAN_ID:
        vlanMember.vlanId = attr_list[i].value.oid;
        break;
      case SAI_VLAN_MEMBER_ATTR_BRIDGE_PORT_ID:
        vlanMember.bridgePortId = attr_list[i].value.oid;
        break;
      default:
        return SAI_STATUS_INVALID_PARAMETER;
    }
  }
  return SAI_STATUS_SUCCESS;
}

sai_status_t remove_vlan_member_fn(sai_object_id_t vlan_member_id) {
  auto fs = FakeSai::getInstance();
  fs->vlanManager.removeMember(vlan_member_id);
  return SAI_STATUS_SUCCESS;
}

sai_status_t get_vlan_member_attribute_fn(
    sai_object_id_t vlan_member_id,
    uint32_t attr_count,
    sai_attribute_t* attr) {
  auto fs = FakeSai::getInstance();
  auto& vlanMember = fs->vlanManager.getMember(vlan_member_id);
  for (int i = 0; i < attr_count; ++i) {
    switch (attr[i].id) {
      case SAI_VLAN_MEMBER_ATTR_VLAN_ID:
        attr[i].value.oid = vlanMember.vlanId;
        break;
      case SAI_VLAN_MEMBER_ATTR_BRIDGE_PORT_ID:
        attr[i].value.oid = vlanMember.bridgePortId;
        break;
      default:
        return SAI_STATUS_NOT_SUPPORTED;
    }
  }
  return SAI_STATUS_SUCCESS;
}

namespace facebook::fboss {

static sai_vlan_api_t _vlan_api;

void populate_vlan_api(sai_vlan_api_t** vlan_api) {
  _vlan_api.create_vlan = &create_vlan_fn;
  _vlan_api.remove_vlan = &remove_vlan_fn;
  _vlan_api.set_vlan_attribute = &set_vlan_attribute_fn;
  _vlan_api.get_vlan_attribute = &get_vlan_attribute_fn;
  _vlan_api.create_vlan_member = &create_vlan_member_fn;
  _vlan_api.remove_vlan_member = &remove_vlan_member_fn;
  _vlan_api.set_vlan_member_attribute = &set_vlan_member_attribute_fn;
  _vlan_api.get_vlan_member_attribute = &get_vlan_member_attribute_fn;
  *vlan_api = &_vlan_api;
}

} // namespace facebook::fboss
