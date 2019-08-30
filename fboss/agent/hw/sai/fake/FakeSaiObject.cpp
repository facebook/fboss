/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/sai/fake/FakeSaiObject.h"
#include "fboss/agent/hw/sai/api/AddressUtil.h"
#include "fboss/agent/hw/sai/fake/FakeSai.h"

#include <folly/logging/xlog.h>

sai_status_t sai_get_object_count(
    sai_object_id_t /* switch_id */,
    sai_object_type_t object_type,
    uint32_t* count) {
  *count = 0;
  auto fs = facebook::fboss::FakeSai::getInstance();
  switch (object_type) {
    case SAI_OBJECT_TYPE_PORT:
      *count = fs->pm.map().size();
      break;
    case SAI_OBJECT_TYPE_VIRTUAL_ROUTER:
      *count = fs->vrm.map().size();
      break;
    case SAI_OBJECT_TYPE_NEXT_HOP:
      *count = fs->nhm.map().size();
      break;
    case SAI_OBJECT_TYPE_NEXT_HOP_GROUP:
      *count = fs->nhgm.map().size();
      break;
    case SAI_OBJECT_TYPE_NEXT_HOP_GROUP_MEMBER: {
      for (const auto& nhg : fs->nhgm.map()) {
        *count += nhg.second.fm().map().size();
      }
      break;
    }
    case SAI_OBJECT_TYPE_ROUTER_INTERFACE:
      *count = fs->rim.map().size();
      break;
    case SAI_OBJECT_TYPE_HOSTIF_TRAP_GROUP:
      *count = fs->htgm.map().size();
      break;
    case SAI_OBJECT_TYPE_FDB_ENTRY:
      *count = fs->fdbm.map().size();
      break;
    case SAI_OBJECT_TYPE_SWITCH:
      *count = fs->swm.map().size();
      break;
    case SAI_OBJECT_TYPE_HOSTIF_TRAP:
      *count = fs->htm.map().size();
      break;
    case SAI_OBJECT_TYPE_NEIGHBOR_ENTRY:
      *count = fs->nm.map().size();
      break;
    case SAI_OBJECT_TYPE_ROUTE_ENTRY:
      *count = fs->rm.map().size();
      break;
    case SAI_OBJECT_TYPE_VLAN:
      *count = fs->vm.map().size();
      break;
    case SAI_OBJECT_TYPE_VLAN_MEMBER: {
      for (const auto& v : fs->vm.map()) {
        *count += v.second.fm().map().size();
      }
      break;
    }
    case SAI_OBJECT_TYPE_BRIDGE:
      *count = fs->brm.map().size();
      break;
    case SAI_OBJECT_TYPE_BRIDGE_PORT: {
      for (const auto& br : fs->brm.map()) {
        *count += br.second.fm().map().size();
      }
      break;
    }
    default:
      return SAI_STATUS_INVALID_PARAMETER;
  }
  return SAI_STATUS_SUCCESS;
}
