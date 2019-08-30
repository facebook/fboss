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

sai_status_t sai_get_object_key(
    sai_object_id_t switch_id,
    sai_object_type_t object_type,
    uint32_t count,
    sai_object_key_t* object_list) {
  auto fs = facebook::fboss::FakeSai::getInstance();
  uint32_t c = 0;
  sai_get_object_count(switch_id, object_type, &c);
  if (c > count) {
    // TODO(borisb): how should we best handle this overflow?
    // feels a little weird to not signal back.
    return SAI_STATUS_BUFFER_OVERFLOW;
  }
  int i = 0;
  switch (object_type) {
    case SAI_OBJECT_TYPE_PORT: {
      for (const auto& p : fs->pm.map()) {
        object_list[i++].key.object_id = p.second.id;
      }
      break;
    }
    case SAI_OBJECT_TYPE_VIRTUAL_ROUTER: {
      for (const auto& vr : fs->vrm.map()) {
        object_list[i++].key.object_id = vr.second.id;
      }
      break;
    }
    case SAI_OBJECT_TYPE_NEXT_HOP: {
      for (const auto& nh : fs->nhm.map()) {
        object_list[i++].key.object_id = nh.second.id;
      }
      break;
    }
    case SAI_OBJECT_TYPE_NEXT_HOP_GROUP: {
      for (const auto& nhg : fs->nhgm.map()) {
        object_list[i++].key.object_id = nhg.second.id;
      }
      break;
    }
    case SAI_OBJECT_TYPE_NEXT_HOP_GROUP_MEMBER: {
      for (const auto& nhg : fs->nhgm.map()) {
        for (const auto& nhgm : nhg.second.fm().map()) {
          object_list[i++].key.object_id = nhgm.second.id;
        }
      }
      break;
    }
    case SAI_OBJECT_TYPE_ROUTER_INTERFACE: {
      for (const auto& rif : fs->rim.map()) {
        object_list[i++].key.object_id = rif.second.id;
      }
      break;
    }
    case SAI_OBJECT_TYPE_HOSTIF_TRAP_GROUP: {
      for (const auto& htg : fs->htgm.map()) {
        object_list[i++].key.object_id = htg.second.id;
      }
      break;
    }
    case SAI_OBJECT_TYPE_FDB_ENTRY: {
      for (const auto& fdbe : fs->fdbm.map()) {
        object_list[i].key.fdb_entry.switch_id = std::get<0>(fdbe.first);
        object_list[i].key.fdb_entry.bv_id = std::get<1>(fdbe.first);
        facebook::fboss::toSaiMacAddress(
            std::get<2>(fdbe.first), object_list[i].key.fdb_entry.mac_address);
        ++i;
      }
      break;
    }
    case SAI_OBJECT_TYPE_SWITCH: {
      for (const auto& sw : fs->swm.map()) {
        object_list[i++].key.object_id = sw.second.id;
      }
      break;
    }
    case SAI_OBJECT_TYPE_HOSTIF_TRAP: {
      for (const auto& ht : fs->htm.map()) {
        object_list[i++].key.object_id = ht.second.id;
      }
      break;
    }
    case SAI_OBJECT_TYPE_NEIGHBOR_ENTRY: {
      for (const auto& nbr : fs->nm.map()) {
        object_list[i].key.neighbor_entry.switch_id = std::get<0>(nbr.first);
        object_list[i].key.neighbor_entry.rif_id = std::get<1>(nbr.first);
        object_list[i].key.neighbor_entry.ip_address =
            facebook::fboss::toSaiIpAddress(std::get<2>(nbr.first));
        ++i;
      }
      break;
    }
    case SAI_OBJECT_TYPE_ROUTE_ENTRY: {
      for (const auto& route : fs->rm.map()) {
        object_list[i].key.route_entry.switch_id = std::get<0>(route.first);
        object_list[i].key.route_entry.vr_id = std::get<1>(route.first);
        object_list[i].key.route_entry.destination =
            facebook::fboss::toSaiIpPrefix(std::get<2>(route.first));
        ++i;
      }
      break;
    }
    case SAI_OBJECT_TYPE_VLAN: {
      for (const auto& v : fs->vm.map()) {
        object_list[i++].key.object_id = v.second.id;
      }
      break;
    }
    case SAI_OBJECT_TYPE_VLAN_MEMBER: {
      for (const auto& v : fs->vm.map()) {
        for (const auto& vm : v.second.fm().map()) {
          object_list[i++].key.object_id = vm.second.id;
        }
      }
      break;
    }
    case SAI_OBJECT_TYPE_BRIDGE: {
      for (const auto& b : fs->brm.map()) {
        object_list[i++].key.object_id = b.second.id;
      }
      break;
    }
    case SAI_OBJECT_TYPE_BRIDGE_PORT: {
      for (const auto& br : fs->brm.map()) {
        for (const auto& brp : br.second.fm().map()) {
          object_list[i++].key.object_id = brp.second.id;
        }
      }
      break;
    }
    default:
      return SAI_STATUS_INVALID_PARAMETER;
  }
  return SAI_STATUS_SUCCESS;
}
