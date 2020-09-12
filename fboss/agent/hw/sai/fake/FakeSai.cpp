/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "FakeSai.h"

#include <folly/FileUtil.h>
#include <folly/Singleton.h>

#include <folly/logging/xlog.h>

namespace {
struct singleton_tag_type {};
} // namespace

using facebook::fboss::FakeSai;
static folly::Singleton<FakeSai, singleton_tag_type> fakeSaiSingleton{};
std::shared_ptr<FakeSai> FakeSai::getInstance() {
  return fakeSaiSingleton.try_get();
}

void FakeSai::clear() {
  auto fs = FakeSai::getInstance();

  fs->aclTableGroupManager.clearWithMembers();
  fs->aclEntryManager.clear();
  fs->aclCounterManager.clear();
  fs->aclTableManager.clear();
  fs->bridgeManager.clearWithMembers();
  fs->debugCounterManager.clear();
  fs->fdbManager.clear();
  fs->hashManager.clear();
  fs->hostIfTrapManager.clear();
  fs->hostifTrapGroupManager.clear();
  fs->inSegEntryManager.clear();
  fs->neighborManager.clear();
  fs->nextHopManager.clear();
  fs->nextHopGroupManager.clearWithMembers();
  fs->portManager.clear();
  fs->qosMapManager.clear();
  fs->queueManager.clear();
  fs->routeManager.clear();
  fs->routeInterfaceManager.clear();
  fs->scheduleManager.clear();
  fs->switchManager.clear();
  fs->virtualRouteManager.clear();
  fs->vlanManager.clearWithMembers();
  fs->wredManager.clear();
}

sai_object_id_t FakeSai::getCpuPort() {
  return cpuPortId;
}

void sai_create_cpu_port() {
  // Create the CPU port
  auto fs = FakeSai::getInstance();
  std::vector<uint32_t> cpuPortLanes{};
  uint32_t cpuPortSpeed = 0;
  sai_object_id_t portId = fs->portManager.create(cpuPortLanes, cpuPortSpeed);
  auto& port = fs->portManager.get(portId);
  for (uint8_t queueId = 0; queueId < 8; queueId++) {
    auto saiQueueId =
        fs->queueManager.create(SAI_QUEUE_TYPE_ALL, portId, queueId, portId);
    port.queueIdList.push_back(saiQueueId);
  }
  fs->cpuPortId = portId;
}

sai_status_t sai_api_initialize(
    uint64_t /* flags */,
    const sai_service_method_table_t* /* services */) {
  auto fs = FakeSai::getInstance();
  if (fs->initialized) {
    return SAI_STATUS_FAILURE;
  }
  // Create the default switch per the SAI spec
  fs->switchManager.create();
  // Create the default 1Q bridge per the SAI spec
  fs->bridgeManager.create();
  // Create the default virtual router per the SAI spec
  fs->virtualRouteManager.create();

  // Create the CPU port
  sai_create_cpu_port();

  fs->initialized = true;
  return SAI_STATUS_SUCCESS;
}

sai_status_t sai_api_query(sai_api_t sai_api_id, void** api_method_table) {
  auto fs = FakeSai::getInstance();
  if (!fs->initialized) {
    return SAI_STATUS_FAILURE;
  }
  sai_status_t res;
  switch (sai_api_id) {
    case SAI_API_ACL:
      facebook::fboss::populate_acl_api((sai_acl_api_t**)api_method_table);
      res = SAI_STATUS_SUCCESS;
      break;
    case SAI_API_BRIDGE:
      facebook::fboss::populate_bridge_api(
          (sai_bridge_api_t**)api_method_table);
      res = SAI_STATUS_SUCCESS;
      break;
    case SAI_API_BUFFER:
      facebook::fboss::populate_buffer_api(
          (sai_buffer_api_t**)api_method_table);
      res = SAI_STATUS_SUCCESS;
      break;
    case SAI_API_DEBUG_COUNTER:
      facebook::fboss::populate_debug_counter_api(
          (sai_debug_counter_api_t**)api_method_table);
      res = SAI_STATUS_SUCCESS;
      break;
    case SAI_API_FDB:
      facebook::fboss::populate_fdb_api((sai_fdb_api_t**)api_method_table);
      res = SAI_STATUS_SUCCESS;
      break;
    case SAI_API_HASH:
      facebook::fboss::populate_hash_api((sai_hash_api_t**)api_method_table);
      res = SAI_STATUS_SUCCESS;
      break;
    case SAI_API_HOSTIF:
      facebook::fboss::populate_hostif_api(
          (sai_hostif_api_t**)api_method_table);
      res = SAI_STATUS_SUCCESS;
      break;
    case SAI_API_MPLS:
      facebook::fboss::populate_mpls_api((sai_mpls_api_t**)api_method_table);
      res = SAI_STATUS_SUCCESS;
      break;
    case SAI_API_NEIGHBOR:
      facebook::fboss::populate_neighbor_api(
          (sai_neighbor_api_t**)api_method_table);
      res = SAI_STATUS_SUCCESS;
      break;
    case SAI_API_NEXT_HOP:
      facebook::fboss::populate_next_hop_api(
          (sai_next_hop_api_t**)api_method_table);
      res = SAI_STATUS_SUCCESS;
      break;
    case SAI_API_NEXT_HOP_GROUP:
      facebook::fboss::populate_next_hop_group_api(
          (sai_next_hop_group_api_t**)api_method_table);
      res = SAI_STATUS_SUCCESS;
      break;
    case SAI_API_PORT:
      facebook::fboss::populate_port_api((sai_port_api_t**)api_method_table);
      res = SAI_STATUS_SUCCESS;
      break;
    case SAI_API_QOS_MAP:
      facebook::fboss::populate_qos_map_api(
          (sai_qos_map_api_t**)api_method_table);
      res = SAI_STATUS_SUCCESS;
      break;
    case SAI_API_QUEUE:
      facebook::fboss::populate_queue_api((sai_queue_api_t**)api_method_table);
      res = SAI_STATUS_SUCCESS;
      break;
    case SAI_API_ROUTE:
      facebook::fboss::populate_route_api((sai_route_api_t**)api_method_table);
      res = SAI_STATUS_SUCCESS;
      break;
    case SAI_API_ROUTER_INTERFACE:
      facebook::fboss::populate_router_interface_api(
          (sai_router_interface_api_t**)api_method_table);
      res = SAI_STATUS_SUCCESS;
      break;
    case SAI_API_SCHEDULER:
      facebook::fboss::populate_scheduler_api(
          (sai_scheduler_api_t**)api_method_table);
      res = SAI_STATUS_SUCCESS;
      break;
    case SAI_API_SWITCH:
      facebook::fboss::populate_switch_api(
          (sai_switch_api_t**)api_method_table);
      res = SAI_STATUS_SUCCESS;
      break;
    case SAI_API_VIRTUAL_ROUTER:
      facebook::fboss::populate_virtual_router_api(
          (sai_virtual_router_api_t**)api_method_table);
      res = SAI_STATUS_SUCCESS;
      break;
    case SAI_API_VLAN:
      facebook::fboss::populate_vlan_api((sai_vlan_api_t**)api_method_table);
      res = SAI_STATUS_SUCCESS;
      break;
    case SAI_API_WRED:
      facebook::fboss::populate_wred_api((sai_wred_api_t**)api_method_table);
      res = SAI_STATUS_SUCCESS;
      break;
    default:
      res = SAI_STATUS_INVALID_PARAMETER;
      break;
  }
  return res;
}

sai_status_t sai_log_set(sai_api_t /*api*/, sai_log_level_t /*log_level*/) {
  return SAI_STATUS_SUCCESS;
}

sai_status_t sai_dbg_generate_dump(const char* dump_file_name) {
  folly::writeFile(
      std::string("Hello from land of FakeSai. All hunky dory here"),
      dump_file_name);
  return SAI_STATUS_SUCCESS;
}
