/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/sai/api/AddressUtil.h"
#include "fboss/agent/hw/sai/api/SaiVersion.h"
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
      // All ports excluding CPU port
      *count = fs->portManager.map().size() - 1;
      break;
    case SAI_OBJECT_TYPE_PORT_SERDES:
      *count = fs->portSerdesManager.map().size();
      break;
    case SAI_OBJECT_TYPE_VIRTUAL_ROUTER:
      *count = fs->virtualRouteManager.map().size();
      break;
    case SAI_OBJECT_TYPE_NEXT_HOP:
      *count = fs->nextHopManager.map().size();
      break;
    case SAI_OBJECT_TYPE_NEXT_HOP_GROUP:
      *count = fs->nextHopGroupManager.map().size();
      break;
    case SAI_OBJECT_TYPE_NEXT_HOP_GROUP_MEMBER: {
      for (const auto& nhg : fs->nextHopGroupManager.map()) {
        *count += nhg.second.fm().map().size();
      }
      break;
    }
    case SAI_OBJECT_TYPE_ROUTER_INTERFACE:
      *count = fs->routeInterfaceManager.map().size();
      break;
    case SAI_OBJECT_TYPE_HASH:
      *count = fs->hashManager.map().size();
      break;
    case SAI_OBJECT_TYPE_HOSTIF_TRAP_GROUP:
      *count = fs->hostifTrapGroupManager.map().size();
      break;
    case SAI_OBJECT_TYPE_FDB_ENTRY:
      *count = fs->fdbManager.map().size();
      break;
    case SAI_OBJECT_TYPE_SWITCH:
      *count = fs->switchManager.map().size();
      break;
    case SAI_OBJECT_TYPE_HOSTIF_TRAP:
      *count = fs->hostIfTrapManager.map().size();
      break;
    case SAI_OBJECT_TYPE_NEIGHBOR_ENTRY:
      *count = fs->neighborManager.map().size();
      break;
    case SAI_OBJECT_TYPE_ROUTE_ENTRY:
      *count = fs->routeManager.map().size();
      break;
    case SAI_OBJECT_TYPE_VLAN:
      *count = fs->vlanManager.map().size();
      break;
    case SAI_OBJECT_TYPE_VLAN_MEMBER: {
      for (const auto& v : fs->vlanManager.map()) {
        *count += v.second.fm().map().size();
      }
      break;
    }
    case SAI_OBJECT_TYPE_BRIDGE:
      *count = fs->bridgeManager.map().size();
      break;
    case SAI_OBJECT_TYPE_BUFFER_POOL:
      *count = fs->bufferPoolManager.map().size();
      break;
    case SAI_OBJECT_TYPE_BUFFER_PROFILE:
      *count = fs->bufferProfileManager.map().size();
      break;
    case SAI_OBJECT_TYPE_BRIDGE_PORT: {
      for (const auto& br : fs->bridgeManager.map()) {
        *count += br.second.fm().map().size();
      }
      break;
    }
    case SAI_OBJECT_TYPE_QOS_MAP:
      *count = fs->qosMapManager.map().size();
      break;
    case SAI_OBJECT_TYPE_QUEUE:
      *count = fs->queueManager.map().size();
      break;
    case SAI_OBJECT_TYPE_SCHEDULER:
      *count = fs->scheduleManager.map().size();
      break;
    case SAI_OBJECT_TYPE_INSEG_ENTRY:
      *count = fs->inSegEntryManager.map().size();
      break;
    case SAI_OBJECT_TYPE_ACL_TABLE_GROUP:
      *count = fs->aclTableGroupManager.map().size();
      break;
    case SAI_OBJECT_TYPE_ACL_TABLE_GROUP_MEMBER: {
      for (const auto& aclTableGroupMember : fs->aclTableGroupManager.map()) {
        *count += aclTableGroupMember.second.fm().map().size();
      }
      break;
    }
    case SAI_OBJECT_TYPE_ACL_TABLE:
      *count = fs->aclTableManager.map().size();
      break;
    case SAI_OBJECT_TYPE_ACL_ENTRY: {
      *count = fs->aclEntryManager.map().size();
      break;
    }
    case SAI_OBJECT_TYPE_ACL_COUNTER:
      *count = fs->aclCounterManager.map().size();
      break;
    case SAI_OBJECT_TYPE_DEBUG_COUNTER:
      *count = fs->debugCounterManager.map().size();
      break;
    case SAI_OBJECT_TYPE_WRED:
      *count = fs->wredManager.map().size();
      break;
    case SAI_OBJECT_TYPE_TAM_REPORT:
      *count = fs->tamReportManager.map().size();
      break;
    case SAI_OBJECT_TYPE_TAM_EVENT_ACTION:
      *count = fs->tamEventActionManager.map().size();
      break;
    case SAI_OBJECT_TYPE_TAM_EVENT:
      *count = fs->tamEventManager.map().size();
      break;
    case SAI_OBJECT_TYPE_TAM:
      *count = fs->tamManager.map().size();
      break;
    default:
      return SAI_STATUS_INVALID_PARAMETER;
  }
  return SAI_STATUS_SUCCESS;
}

sai_status_t sai_get_object_key(
    sai_object_id_t switch_id,
    sai_object_type_t object_type,
    uint32_t* count,
    sai_object_key_t* object_list) {
  auto fs = facebook::fboss::FakeSai::getInstance();
  uint32_t c = 0;
  sai_get_object_count(switch_id, object_type, &c);
  if (c > *count) {
    // TODO(borisb): how should we best handle this overflow?
    // feels a little weird to not signal back.
    return SAI_STATUS_BUFFER_OVERFLOW;
  }
  int i = 0;
  switch (object_type) {
    case SAI_OBJECT_TYPE_PORT: {
      for (const auto& p : fs->portManager.map()) {
        if (fs->cpuPortId == p.second.id) {
          continue;
        }
        object_list[i++].key.object_id = p.second.id;
      }
      break;
    }
    case SAI_OBJECT_TYPE_PORT_SERDES: {
      for (const auto& serdes : fs->portSerdesManager.map()) {
        object_list[i++].key.object_id = serdes.second.id;
      }
      break;
    }
    case SAI_OBJECT_TYPE_VIRTUAL_ROUTER: {
      for (const auto& vr : fs->virtualRouteManager.map()) {
        object_list[i++].key.object_id = vr.second.id;
      }
      break;
    }
    case SAI_OBJECT_TYPE_NEXT_HOP: {
      for (const auto& nh : fs->nextHopManager.map()) {
        object_list[i++].key.object_id = nh.second.id;
      }
      break;
    }
    case SAI_OBJECT_TYPE_NEXT_HOP_GROUP: {
      for (const auto& nhg : fs->nextHopGroupManager.map()) {
        object_list[i++].key.object_id = nhg.second.id;
      }
      break;
    }
    case SAI_OBJECT_TYPE_NEXT_HOP_GROUP_MEMBER: {
      for (const auto& nhg : fs->nextHopGroupManager.map()) {
        for (const auto& nextHopGroupManager : nhg.second.fm().map()) {
          object_list[i++].key.object_id = nextHopGroupManager.second.id;
        }
      }
      break;
    }
    case SAI_OBJECT_TYPE_ROUTER_INTERFACE: {
      for (const auto& rif : fs->routeInterfaceManager.map()) {
        object_list[i++].key.object_id = rif.second.id;
      }
      break;
    }
    case SAI_OBJECT_TYPE_HOSTIF_TRAP_GROUP: {
      for (const auto& htg : fs->hostifTrapGroupManager.map()) {
        object_list[i++].key.object_id = htg.second.id;
      }
      break;
    }
    case SAI_OBJECT_TYPE_HASH: {
      *count = fs->hashManager.map().size();
      for (const auto& hash : fs->hashManager.map()) {
        object_list[i++].key.object_id = hash.first;
      }
      break;
    }
    case SAI_OBJECT_TYPE_FDB_ENTRY: {
      for (const auto& fdbe : fs->fdbManager.map()) {
        object_list[i].key.fdb_entry.switch_id = std::get<0>(fdbe.first);
        object_list[i].key.fdb_entry.bv_id = std::get<1>(fdbe.first);
        facebook::fboss::toSaiMacAddress(
            std::get<2>(fdbe.first), object_list[i].key.fdb_entry.mac_address);
        ++i;
      }
      break;
    }
    case SAI_OBJECT_TYPE_SWITCH: {
      for (const auto& sw : fs->switchManager.map()) {
        object_list[i++].key.object_id = sw.second.id;
      }
      break;
    }
    case SAI_OBJECT_TYPE_HOSTIF_TRAP: {
      for (const auto& ht : fs->hostIfTrapManager.map()) {
        object_list[i++].key.object_id = ht.second.id;
      }
      break;
    }
    case SAI_OBJECT_TYPE_NEIGHBOR_ENTRY: {
      for (const auto& nbr : fs->neighborManager.map()) {
        object_list[i].key.neighbor_entry.switch_id = std::get<0>(nbr.first);
        object_list[i].key.neighbor_entry.rif_id = std::get<1>(nbr.first);
        object_list[i].key.neighbor_entry.ip_address =
            facebook::fboss::toSaiIpAddress(std::get<2>(nbr.first));
        ++i;
      }
      break;
    }
    case SAI_OBJECT_TYPE_ROUTE_ENTRY: {
      for (const auto& route : fs->routeManager.map()) {
        object_list[i].key.route_entry.switch_id = std::get<0>(route.first);
        object_list[i].key.route_entry.vr_id = std::get<1>(route.first);
        object_list[i].key.route_entry.destination =
            facebook::fboss::toSaiIpPrefix(std::get<2>(route.first));
        ++i;
      }
      break;
    }
    case SAI_OBJECT_TYPE_VLAN: {
      for (const auto& v : fs->vlanManager.map()) {
        object_list[i++].key.object_id = v.second.id;
      }
      break;
    }
    case SAI_OBJECT_TYPE_VLAN_MEMBER: {
      for (const auto& v : fs->vlanManager.map()) {
        for (const auto& vlanManager : v.second.fm().map()) {
          object_list[i++].key.object_id = vlanManager.second.id;
        }
      }
      break;
    }
    case SAI_OBJECT_TYPE_BRIDGE: {
      for (const auto& b : fs->bridgeManager.map()) {
        object_list[i++].key.object_id = b.second.id;
      }
      break;
    }
    case SAI_OBJECT_TYPE_BRIDGE_PORT: {
      for (const auto& br : fs->bridgeManager.map()) {
        for (const auto& brp : br.second.fm().map()) {
          object_list[i++].key.object_id = brp.second.id;
        }
      }
      break;
    }
    case SAI_OBJECT_TYPE_BUFFER_POOL: {
      for (const auto& b : fs->bufferPoolManager.map()) {
        object_list[i++].key.object_id = b.second.id;
      }
      break;
    }
    case SAI_OBJECT_TYPE_BUFFER_PROFILE: {
      for (const auto& b : fs->bufferProfileManager.map()) {
        object_list[i++].key.object_id = b.second.id;
      }
      break;
    }
    case SAI_OBJECT_TYPE_QOS_MAP: {
      for (const auto& qm : fs->qosMapManager.map()) {
        object_list[i++].key.object_id = qm.second.id;
      }
      break;
    }
    case SAI_OBJECT_TYPE_QUEUE: {
      for (const auto& q : fs->queueManager.map()) {
        object_list[i++].key.object_id = q.second.id;
      }
      break;
    }
    case SAI_OBJECT_TYPE_SCHEDULER: {
      for (const auto& sc : fs->scheduleManager.map()) {
        object_list[i++].key.object_id = sc.second.id;
      }
      break;
    }
    case SAI_OBJECT_TYPE_INSEG_ENTRY: {
      for (const auto& entry : fs->inSegEntryManager.map()) {
        object_list[i++].key.inseg_entry = entry.first.sai_inseg_entry;
      }
      break;
    }
    case SAI_OBJECT_TYPE_ACL_TABLE_GROUP: {
      for (const auto& aclTableGroup : fs->aclTableGroupManager.map()) {
        object_list[i++].key.object_id = aclTableGroup.second.id;
      }
      break;
    }
    case SAI_OBJECT_TYPE_ACL_TABLE_GROUP_MEMBER: {
      for (const auto& aclTableGroup : fs->aclTableGroupManager.map()) {
        for (const auto& aclTableGroupMember :
             aclTableGroup.second.fm().map()) {
          object_list[i++].key.object_id = aclTableGroupMember.second.id;
        }
      }
      break;
    }
    case SAI_OBJECT_TYPE_ACL_TABLE: {
      for (const auto& aclTable : fs->aclTableManager.map()) {
        object_list[i++].key.object_id = aclTable.second.id;
      }
      break;
    }
    case SAI_OBJECT_TYPE_ACL_ENTRY: {
      for (const auto& aclEntry : fs->aclEntryManager.map()) {
        object_list[i++].key.object_id = aclEntry.second.id;
      }
      break;
    }
    case SAI_OBJECT_TYPE_ACL_COUNTER: {
      for (const auto& aclCounter : fs->aclCounterManager.map()) {
        object_list[i++].key.object_id = aclCounter.second.id;
      }
      break;
    }
    case SAI_OBJECT_TYPE_DEBUG_COUNTER: {
      *count = fs->debugCounterManager.map().size();
      for (const auto& debugCounter : fs->debugCounterManager.map()) {
        object_list[i++].key.object_id = debugCounter.first;
      }
      break;
    }
    case SAI_OBJECT_TYPE_WRED: {
      for (const auto& wred : fs->wredManager.map()) {
        object_list[i++].key.object_id = wred.second.id;
      }
      break;
    }
    case SAI_OBJECT_TYPE_TAM_REPORT: {
      for (const auto& ob : fs->tamReportManager.map()) {
        object_list[i++].key.object_id = ob.second.id;
      }
      break;
    }
    case SAI_OBJECT_TYPE_TAM_EVENT_ACTION: {
      for (const auto& ob : fs->tamEventActionManager.map()) {
        object_list[i++].key.object_id = ob.second.id;
      }
      break;
    }
    case SAI_OBJECT_TYPE_TAM_EVENT: {
      for (const auto& ob : fs->tamEventManager.map()) {
        object_list[i++].key.object_id = ob.second.id;
      }
      break;
    }
    case SAI_OBJECT_TYPE_TAM: {
      for (const auto& ob : fs->tamManager.map()) {
        object_list[i++].key.object_id = ob.second.id;
      }
      break;
    }
    default:
      return SAI_STATUS_INVALID_PARAMETER;
  }
  return SAI_STATUS_SUCCESS;
}
