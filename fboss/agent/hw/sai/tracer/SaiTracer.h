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

#include <map>
#include <memory>
#include <tuple>

#include "fboss/agent/AsyncLogger.h"

#include <folly/File.h>
#include <folly/String.h>
#include <folly/Synchronized.h>
#include <gflags/gflags.h>

extern "C" {
#include <sai.h>
}

DECLARE_bool(enable_replayer);
DECLARE_bool(enable_packet_log);

namespace facebook::fboss {

class SaiTracer {
 public:
  explicit SaiTracer();
  ~SaiTracer();

  static std::shared_ptr<SaiTracer> getInstance();

  void logApiInitialize(const char** variables, const char** values, int size);

  void logApiQuery(sai_api_t api_id, const std::string& api_var);

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

  void logInsegEntryCreateFn(
      const sai_inseg_entry_t* inseg_entry,
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

  void logInsegEntryRemoveFn(
      const sai_inseg_entry_t* inseg_entry,
      sai_status_t rv);

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

  void logInsegEntrySetAttrFn(
      const sai_inseg_entry_t* inseg_entry,
      const sai_attribute_t* attr,
      sai_status_t rv);

  void logSetAttrFn(
      const std::string& fn_name,
      sai_object_id_t set_object_id,
      const sai_attribute_t* attr,
      sai_object_type_t object_type,
      sai_status_t rv);

  void logSendHostifPacketFn(
      sai_object_id_t hostif_id,
      sai_size_t buffer_size,
      const uint8_t* buffer,
      uint32_t attr_count,
      const sai_attribute_t* attr_list,
      sai_status_t rv);

  std::string getVariable(sai_object_id_t object_id);

  uint32_t
  checkListCount(uint32_t list_count, uint32_t elem_size, uint32_t elem_count);

  sai_acl_api_t* aclApi_;
  sai_bridge_api_t* bridgeApi_;
  sai_buffer_api_t* bufferApi_;
  sai_fdb_api_t* fdbApi_;
  sai_hash_api_t* hashApi_;
  sai_hostif_api_t* hostifApi_;
  sai_neighbor_api_t* neighborApi_;
  sai_next_hop_api_t* nextHopApi_;
  sai_next_hop_group_api_t* nextHopGroupApi_;
  sai_mpls_api_t* mplsApi_;
  sai_port_api_t* portApi_;
  sai_queue_api_t* queueApi_;
  sai_qos_map_api_t* qosMapApi_;
  sai_route_api_t* routeApi_;
  sai_router_interface_api_t* routerInterfaceApi_;
  sai_scheduler_api_t* schedulerApi_;
  sai_switch_api_t* switchApi_;
  sai_virtual_router_api_t* virtualRouterApi_;
  sai_vlan_api_t* vlanApi_;
  sai_tam_api_t* tamApi_;

  std::map<sai_api_t, std::string> init_api_;

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

  void setInsegEntry(
      const sai_inseg_entry_t* inseg_entry,
      std::vector<std::string>& lines);

  void setNeighborEntry(
      const sai_neighbor_entry_t* neighbor_entry,
      std::vector<std::string>& lines);

  void setRouteEntry(
      const sai_route_entry_t* route_entry,
      std::vector<std::string>& lines);

  std::string rvCheck(sai_status_t rv);

  std::string logTimeAndRv(
      sai_status_t rv,
      sai_object_id_t object_id = SAI_NULL_OBJECT_ID);

  void checkAttrCount(uint32_t attr_count);

  // Init functions
  void setupGlobals();
  void initVarCounts();

  void writeFooter();

  uint32_t maxAttrCount_;
  uint32_t maxListCount_;
  uint32_t numCalls_;
  std::unique_ptr<AsyncLogger> asyncLogger_;

  // Variables mappings in generated C code
  // varCounts map from object type to the current counter
  std::map<sai_object_type_t, std::atomic<uint32_t>> varCounts_;
  // variables_ map from object id to its variable name
  folly::Synchronized<std::map<sai_object_id_t, std::string>> variables_;

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
      {SAI_OBJECT_TYPE_HOSTIF, "hostif_"},
      {SAI_OBJECT_TYPE_HOSTIF_TRAP, "hostifTrap_"},
      {SAI_OBJECT_TYPE_HOSTIF_TRAP_GROUP, "hostifTrapGroup_"},
      {SAI_OBJECT_TYPE_NEXT_HOP, "nextHop_"},
      {SAI_OBJECT_TYPE_NEXT_HOP_GROUP, "nextHopGroup_"},
      {SAI_OBJECT_TYPE_NEXT_HOP_GROUP_MEMBER, "nextHopGroupMember_"},
      {SAI_OBJECT_TYPE_POLICER, "policier_"},
      {SAI_OBJECT_TYPE_PORT, "port_"},
      {SAI_OBJECT_TYPE_PORT_SERDES, "portSerdes_"},
      {SAI_OBJECT_TYPE_QOS_MAP, "qosMap_"},
      {SAI_OBJECT_TYPE_QUEUE, "queue_"},
      {SAI_OBJECT_TYPE_ROUTER_INTERFACE, "routerInterface_"},
      {SAI_OBJECT_TYPE_SCHEDULER, "scheduler_"},
      {SAI_OBJECT_TYPE_SCHEDULER_GROUP, "schedulerGroup_"},
      {SAI_OBJECT_TYPE_SWITCH, "switch_"},
      {SAI_OBJECT_TYPE_TAM_REPORT, "tamReport_"},
      {SAI_OBJECT_TYPE_TAM_EVENT_ACTION, "tamEventAction_"},
      {SAI_OBJECT_TYPE_TAM_EVENT, "tamEvent_"},
      {SAI_OBJECT_TYPE_TAM, "tam_"},
      {SAI_OBJECT_TYPE_UDF_GROUP, "udfGroup_"},
      {SAI_OBJECT_TYPE_VIRTUAL_ROUTER, "virtualRouter_"},
      {SAI_OBJECT_TYPE_VLAN, "vlan_"},
      {SAI_OBJECT_TYPE_VLAN_MEMBER, "vlanMember_"},
      {SAI_OBJECT_TYPE_WRED, "wred_"}};

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
      {SAI_OBJECT_TYPE_HOSTIF, "hostif_api->"},
      {SAI_OBJECT_TYPE_HOSTIF_TRAP, "hostif_api->"},
      {SAI_OBJECT_TYPE_HOSTIF_TRAP_GROUP, "hostif_api->"},
      {SAI_OBJECT_TYPE_INSEG_ENTRY, "mpls_api->"},
      {SAI_OBJECT_TYPE_NEIGHBOR_ENTRY, "neighbor_api->"},
      {SAI_OBJECT_TYPE_NEXT_HOP, "next_hop_api->"},
      {SAI_OBJECT_TYPE_NEXT_HOP_GROUP, "next_hop_group_api->"},
      {SAI_OBJECT_TYPE_NEXT_HOP_GROUP_MEMBER, "next_hop_group_api->"},
      {SAI_OBJECT_TYPE_PORT, "port_api->"},
      {SAI_OBJECT_TYPE_PORT_SERDES, "port_api->"},
      {SAI_OBJECT_TYPE_ROUTE_ENTRY, "route_api->"},
      {SAI_OBJECT_TYPE_ROUTER_INTERFACE, "router_interface_api->"},
      {SAI_OBJECT_TYPE_QOS_MAP, "qos_map_api->"},
      {SAI_OBJECT_TYPE_QUEUE, "queue_api->"},
      {SAI_OBJECT_TYPE_SCHEDULER, "scheduler_api->"},
      {SAI_OBJECT_TYPE_SCHEDULER_GROUP, "scheduler_group_api->"},
      {SAI_OBJECT_TYPE_SWITCH, "switch_api->"},
      {SAI_OBJECT_TYPE_TAM_REPORT, "tam_api->"},
      {SAI_OBJECT_TYPE_TAM_EVENT_ACTION, "tam_api->"},
      {SAI_OBJECT_TYPE_TAM_EVENT, "tam_api->"},
      {SAI_OBJECT_TYPE_TAM, "tam_api->"},
      {SAI_OBJECT_TYPE_VIRTUAL_ROUTER, "virtual_router_api->"},
      {SAI_OBJECT_TYPE_VLAN, "vlan_api->"},
      {SAI_OBJECT_TYPE_VLAN_MEMBER, "vlan_api->"}};

  const char* cpp_header_ =
      "/*\n"
      " *  Copyright (c) 2004-present, Facebook, Inc.\n"
      " *  All rights reserved.\n"
      " *\n"
      " *  This source code is licensed under the BSD-style license found in the\n"
      " *  LICENSE file in the root directory of this source tree. An additional grant\n"
      " *  of patent rights can be found in the PATENTS file in the same directory.\n"
      " *\n"
      " */\n"
      "\n"
      "#include <string>\n"
      "#include <unordered_map>\n"
      "\n"
      "#include \"fboss/agent/hw/sai/tracer/run/SaiLog.h\"\n"
      "\n"
      "#define ATTR_SIZE sizeof(sai_attribute_t)\n"
      "\n"
      "namespace {\n"
      "\n"
      "static std::unordered_map<std::string, std::string> kSaiProfileValues;\n"
      "\n"
      "const char* saiProfileGetValue(\n"
      "    sai_switch_profile_id_t /*profile_id*/,\n"
      "    const char* variable) {\n"
      "  auto saiProfileValItr = kSaiProfileValues.find(variable);\n"
      "  return saiProfileValItr != kSaiProfileValues.end()\n"
      "      ? saiProfileValItr->second.c_str()\n"
      "      : nullptr;\n"
      "}\n"
      "\n"
      "int saiProfileGetNextValue(\n"
      "    sai_switch_profile_id_t /* profile_id */,\n"
      "    const char** variable,\n"
      "    const char** value) {\n"
      "  static auto saiProfileValItr = kSaiProfileValues.begin();\n"
      "  if (!value) {\n"
      "    saiProfileValItr = kSaiProfileValues.begin();\n"
      "    return 0;\n"
      "  }\n"
      "  if (saiProfileValItr == kSaiProfileValues.end()) {\n"
      "    return -1;\n"
      "  }\n"
      "  *variable = saiProfileValItr->first.c_str();\n"
      "  *value = saiProfileValItr->second.c_str();\n"
      "  ++saiProfileValItr;\n"
      "  return 0;\n"
      "}\n"
      "\n"
      "sai_service_method_table_t kSaiServiceMethodTable = {\n"
      "    .profile_get_value = saiProfileGetValue,\n"
      "    .profile_get_next_value = saiProfileGetNextValue,\n"
      "};\n"
      "\n"
      "inline void rvCheck(int rv, int expected, int count) {\n"
      "  if (rv != expected) printf(\"Unexpected rv at %d with status %d\\n\", count, rv);\n"
      "}\n"
      "\n"
      "} //namespace\n"
      "\n"
      "namespace facebook::fboss {\n"
      "\n"
      "void run_trace() {\n";
};

} // namespace facebook::fboss
