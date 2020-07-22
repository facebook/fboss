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

#include <memory>

#include <gflags/gflags.h>

#include <map>
#include <tuple>

#include <folly/File.h>
#include <folly/String.h>
#include <folly/Synchronized.h>

extern "C" {
#include <sai.h>
}

DECLARE_bool(enable_replayer);

namespace facebook::fboss {

class SaiTracer {
 public:
  explicit SaiTracer();
  ~SaiTracer();

  static std::shared_ptr<SaiTracer> getInstance();

  void logSwitchCreateFn(
      sai_object_id_t* switch_id,
      uint32_t attr_count,
      const sai_attribute_t* attr_list,
      sai_status_t rv);

  void logRouteEntryCreateFn(
      const sai_route_entry_t* route_entry,
      uint32_t attr_count,
      const sai_attribute_t* attr_list,
      sai_status_t rv);

  void logNeighborEntryCreateFn(
      const sai_neighbor_entry_t* neighbor_entry,
      uint32_t attr_count,
      const sai_attribute_t* attr_list,
      sai_status_t rv);

  void logFdbEntryCreateFn(
      const sai_fdb_entry_t* fdb_entry,
      uint32_t attr_count,
      const sai_attribute_t* attr_list,
      sai_status_t rv);

  void logCreateFn(
      const std::string& fn_name,
      sai_object_id_t* create_object_id,
      sai_object_id_t switch_id,
      uint32_t attr_count,
      const sai_attribute_t* attr_list,
      sai_object_type_t object_type,
      sai_status_t rv);

  void logRouteEntryRemoveFn(
      const sai_route_entry_t* route_entry,
      sai_status_t rv);

  void logNeighborEntryRemoveFn(
      const sai_neighbor_entry_t* neighbor_entry,
      sai_status_t rv);

  void logFdbEntryRemoveFn(const sai_fdb_entry_t* fdb_entry, sai_status_t rv);

  void logRemoveFn(
      const std::string& fn_name,
      sai_object_id_t remove_object_id,
      sai_object_type_t object_type,
      sai_status_t rv);

  void logRouteEntrySetAttrFn(
      const sai_route_entry_t* route_entry,
      const sai_attribute_t* attr,
      sai_status_t rv);

  void logNeighborEntrySetAttrFn(
      const sai_neighbor_entry_t* neighbor_entry,
      const sai_attribute_t* attr,
      sai_status_t rv);

  void logFdbEntrySetAttrFn(
      const sai_fdb_entry_t* fdb_entry,
      const sai_attribute_t* attr,
      sai_status_t rv);

  void logSetAttrFn(
      const std::string& fn_name,
      sai_object_id_t set_object_id,
      const sai_attribute_t* attr,
      sai_object_type_t object_type,
      sai_status_t rv);

  std::string getVariable(
      sai_object_id_t object_id,
      std::vector<sai_object_type_t> object_types);

  uint32_t
  checkListCount(uint32_t list_count, uint32_t elem_size, uint32_t elem_count);

  sai_acl_api_t* aclApi_;
  sai_bridge_api_t* bridgeApi_;
  sai_buffer_api_t* bufferApi_;
  sai_fdb_api_t* fdbApi_;
  sai_hash_api_t* hashApi_;
  sai_neighbor_api_t* neighborApi_;
  sai_next_hop_api_t* nextHopApi_;
  sai_next_hop_group_api_t* nextHopGroupApi_;
  sai_port_api_t* portApi_;
  sai_queue_api_t* queueApi_;
  sai_route_api_t* routeApi_;
  sai_router_interface_api_t* routerInterfaceApi_;
  sai_scheduler_api_t* schedulerApi_;
  sai_switch_api_t* switchApi_;
  sai_virtual_router_api_t* virtualRouterApi_;
  sai_vlan_api_t* vlanApi_;

 private:
  void writeToFile(const std::vector<std::string>& strVec);

  // Helper methods for variables and attribute list
  std::tuple<std::string, std::string> declareVariable(
      sai_object_id_t* object_id,
      sai_object_type_t object_type);

  std::vector<std::string> setAttrList(
      const sai_attribute_t* attr_list,
      uint32_t attr_count,
      sai_object_type_t object_type);

  std::string createFnCall(
      const std::string& fn_name,
      const std::string& var1,
      const std::string& var2,
      uint32_t attr_count,
      sai_object_type_t object_type);

  void setFdbEntry(
      const sai_fdb_entry_t* fdb_entry,
      std::vector<std::string>& lines);

  void setNeighborEntry(
      const sai_neighbor_entry_t* neighbor_entry,
      std::vector<std::string>& lines);

  void setRouteEntry(
      const sai_route_entry_t* route_entry,
      std::vector<std::string>& lines);

  std::string rvCheck(sai_status_t rv);

  std::string logTimeAndId(sai_object_id_t object_id, sai_status_t rv);

  void checkAttrCount(uint32_t attr_count);

  // Init functions
  void setupGlobals();
  void initVarCounts();

  uint32_t maxAttrCount_;
  uint32_t maxListCount_;
  uint32_t numCalls_;
  folly::Synchronized<folly::File> saiLogFile_;

  // Variables mappings in generated C code
  std::map<sai_object_type_t, std::atomic<uint32_t>> varCounts_;

  std::map<sai_object_type_t, std::string> varNames_{
      {SAI_OBJECT_TYPE_ACL_ENTRY, "aclEntry_"},
      {SAI_OBJECT_TYPE_ACL_TABLE, "aclTable_"},
      {SAI_OBJECT_TYPE_ACL_TABLE_GROUP, "aclTableGroup_"},
      {SAI_OBJECT_TYPE_ACL_TABLE_GROUP_MEMBER, "aclTableGroupMember_"},
      {SAI_OBJECT_TYPE_BRIDGE, "bridge_"},
      {SAI_OBJECT_TYPE_BRIDGE_PORT, "bridgePort_"},
      {SAI_OBJECT_TYPE_BUFFER_POOL, "bufferPool_"},
      {SAI_OBJECT_TYPE_BUFFER_PROFILE, "bufferProfile_"},
      {SAI_OBJECT_TYPE_HASH, "hash_"},
      {SAI_OBJECT_TYPE_NEXT_HOP, "nextHop_"},
      {SAI_OBJECT_TYPE_NEXT_HOP_GROUP, "nextHopGroup_"},
      {SAI_OBJECT_TYPE_NEXT_HOP_GROUP_MEMBER, "nextHopGroupMember_"},
      {SAI_OBJECT_TYPE_PORT, "port_"},
      {SAI_OBJECT_TYPE_QOS_MAP, "qosMap_"},
      {SAI_OBJECT_TYPE_QUEUE, "queue_"},
      {SAI_OBJECT_TYPE_ROUTER_INTERFACE, "routerInterface_"},
      {SAI_OBJECT_TYPE_SCHEDULER, "scheduler_"},
      {SAI_OBJECT_TYPE_SCHEDULER_GROUP, "schedulerGroup_"},
      {SAI_OBJECT_TYPE_SWITCH, "switch_"},
      {SAI_OBJECT_TYPE_UDF_GROUP, "udfGroup_"},
      {SAI_OBJECT_TYPE_VIRTUAL_ROUTER, "virtualRouter_"},
      {SAI_OBJECT_TYPE_VLAN, "vlan_"},
      {SAI_OBJECT_TYPE_VLAN_MEMBER, "vlanMember_"},
      {SAI_OBJECT_TYPE_WRED, "wred_"}};

  folly::Synchronized<std::map<sai_object_id_t, std::string>> aclEntryMap_;
  folly::Synchronized<std::map<sai_object_id_t, std::string>> aclTableMap_;
  folly::Synchronized<std::map<sai_object_id_t, std::string>> aclTableGroupMap_;
  folly::Synchronized<std::map<sai_object_id_t, std::string>>
      aclTableGroupMemberMap_;
  folly::Synchronized<std::map<sai_object_id_t, std::string>> bridgeMap_;
  folly::Synchronized<std::map<sai_object_id_t, std::string>> bridgePortMap_;
  folly::Synchronized<std::map<sai_object_id_t, std::string>> bufferPoolMap_;
  folly::Synchronized<std::map<sai_object_id_t, std::string>> bufferProfileMap_;
  folly::Synchronized<std::map<sai_object_id_t, std::string>> hashMap_;
  folly::Synchronized<std::map<sai_object_id_t, std::string>> nextHopMap_;
  folly::Synchronized<std::map<sai_object_id_t, std::string>> nextHopGroupMap_;
  folly::Synchronized<std::map<sai_object_id_t, std::string>>
      nextHopGroupMemberMap_;
  folly::Synchronized<std::map<sai_object_id_t, std::string>> portMap_;
  folly::Synchronized<std::map<sai_object_id_t, std::string>> qosMap_;
  folly::Synchronized<std::map<sai_object_id_t, std::string>> queueMap_;
  folly::Synchronized<std::map<sai_object_id_t, std::string>>
      routerInterfaceMap_;
  folly::Synchronized<std::map<sai_object_id_t, std::string>> switchMap_;
  folly::Synchronized<std::map<sai_object_id_t, std::string>> schedulerMap_;
  folly::Synchronized<std::map<sai_object_id_t, std::string>>
      schedulerGroupMap_;
  folly::Synchronized<std::map<sai_object_id_t, std::string>> udfGroupMap_;
  folly::Synchronized<std::map<sai_object_id_t, std::string>> virtualRouterMap_;
  folly::Synchronized<std::map<sai_object_id_t, std::string>> vlanMap_;
  folly::Synchronized<std::map<sai_object_id_t, std::string>> vlanMemberMap_;
  folly::Synchronized<std::map<sai_object_id_t, std::string>> wredMap_;

  std::map<
      sai_object_type_t,
      folly::Synchronized<std::map<sai_object_id_t, std::string>>>
      variables_{
          {SAI_OBJECT_TYPE_ACL_ENTRY, aclEntryMap_},
          {SAI_OBJECT_TYPE_ACL_TABLE, aclTableMap_},
          {SAI_OBJECT_TYPE_ACL_TABLE_GROUP, aclTableGroupMap_},
          {SAI_OBJECT_TYPE_ACL_TABLE_GROUP_MEMBER, aclTableGroupMemberMap_},
          {SAI_OBJECT_TYPE_BRIDGE, bridgeMap_},
          {SAI_OBJECT_TYPE_BRIDGE_PORT, bridgePortMap_},
          {SAI_OBJECT_TYPE_BUFFER_POOL, bufferPoolMap_},
          {SAI_OBJECT_TYPE_BUFFER_PROFILE, bufferProfileMap_},
          {SAI_OBJECT_TYPE_HASH, hashMap_},
          {SAI_OBJECT_TYPE_NEXT_HOP, nextHopMap_},
          {SAI_OBJECT_TYPE_NEXT_HOP_GROUP, nextHopGroupMap_},
          {SAI_OBJECT_TYPE_NEXT_HOP_GROUP_MEMBER, nextHopGroupMemberMap_},
          {SAI_OBJECT_TYPE_PORT, portMap_},
          {SAI_OBJECT_TYPE_QOS_MAP, qosMap_},
          {SAI_OBJECT_TYPE_QUEUE, queueMap_},
          {SAI_OBJECT_TYPE_ROUTER_INTERFACE, routerInterfaceMap_},
          {SAI_OBJECT_TYPE_SCHEDULER, schedulerMap_},
          {SAI_OBJECT_TYPE_SCHEDULER_GROUP, schedulerGroupMap_},
          {SAI_OBJECT_TYPE_SWITCH, switchMap_},
          {SAI_OBJECT_TYPE_UDF_GROUP, udfGroupMap_},
          {SAI_OBJECT_TYPE_VIRTUAL_ROUTER, virtualRouterMap_},
          {SAI_OBJECT_TYPE_VLAN, vlanMap_},
          {SAI_OBJECT_TYPE_VLAN_MEMBER, vlanMemberMap_},
          {SAI_OBJECT_TYPE_WRED, wredMap_}};

  std::map<sai_object_type_t, std::string> fnPrefix_{
      {SAI_OBJECT_TYPE_ACL_ENTRY, "acl_api->"},
      {SAI_OBJECT_TYPE_ACL_TABLE, "acl_api->"},
      {SAI_OBJECT_TYPE_ACL_TABLE_GROUP, "acl_api->"},
      {SAI_OBJECT_TYPE_ACL_TABLE_GROUP_MEMBER, "acl_api->"},
      {SAI_OBJECT_TYPE_BRIDGE, "bridge_api->"},
      {SAI_OBJECT_TYPE_BRIDGE_PORT, "bridge_api->"},
      {SAI_OBJECT_TYPE_BUFFER_POOL, "buffer_api->"},
      {SAI_OBJECT_TYPE_BUFFER_PROFILE, "buffer_api->"},
      {SAI_OBJECT_TYPE_FDB_ENTRY, "fdb_api->"},
      {SAI_OBJECT_TYPE_HASH, "hash_api->"},
      {SAI_OBJECT_TYPE_NEIGHBOR_ENTRY, "neighbor_api->"},
      {SAI_OBJECT_TYPE_NEXT_HOP, "next_hop_api->"},
      {SAI_OBJECT_TYPE_NEXT_HOP_GROUP, "next_hop_group_api->"},
      {SAI_OBJECT_TYPE_NEXT_HOP_GROUP_MEMBER, "next_hop_group_api->"},
      {SAI_OBJECT_TYPE_PORT, "port_api->"},
      {SAI_OBJECT_TYPE_ROUTE_ENTRY, "route_api->"},
      {SAI_OBJECT_TYPE_ROUTER_INTERFACE, "router_interface_api->"},
      {SAI_OBJECT_TYPE_QOS_MAP, "qosMap_api->"},
      {SAI_OBJECT_TYPE_QUEUE, "queue_api->"},
      {SAI_OBJECT_TYPE_SCHEDULER, "scheduler_api->"},
      {SAI_OBJECT_TYPE_SCHEDULER_GROUP, "scheduler_group_api->"},
      {SAI_OBJECT_TYPE_SWITCH, "switch_api->"},
      {SAI_OBJECT_TYPE_VIRTUAL_ROUTER, "virtual_router_api->"},
      {SAI_OBJECT_TYPE_VLAN, "vlan_api->"},
      {SAI_OBJECT_TYPE_VLAN_MEMBER, "vlan_api->"}};
};

} // namespace facebook::fboss
