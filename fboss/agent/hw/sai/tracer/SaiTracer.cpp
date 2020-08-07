/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include <chrono>
#include <ctime>
#include <iomanip>
#include <ostream>
#include <tuple>

#include "fboss/agent/SysError.h"
#include "fboss/agent/hw/sai/tracer/AclApiTracer.h"
#include "fboss/agent/hw/sai/tracer/BridgeApiTracer.h"
#include "fboss/agent/hw/sai/tracer/BufferApiTracer.h"
#include "fboss/agent/hw/sai/tracer/FdbApiTracer.h"
#include "fboss/agent/hw/sai/tracer/HashApiTracer.h"
#include "fboss/agent/hw/sai/tracer/HostifApiTracer.h"
#include "fboss/agent/hw/sai/tracer/MplsApiTracer.h"
#include "fboss/agent/hw/sai/tracer/NeighborApiTracer.h"
#include "fboss/agent/hw/sai/tracer/NextHopApiTracer.h"
#include "fboss/agent/hw/sai/tracer/NextHopGroupApiTracer.h"
#include "fboss/agent/hw/sai/tracer/PortApiTracer.h"
#include "fboss/agent/hw/sai/tracer/QosMapApiTracer.h"
#include "fboss/agent/hw/sai/tracer/QueueApiTracer.h"
#include "fboss/agent/hw/sai/tracer/RouteApiTracer.h"
#include "fboss/agent/hw/sai/tracer/RouterInterfaceApiTracer.h"
#include "fboss/agent/hw/sai/tracer/SaiTracer.h"
#include "fboss/agent/hw/sai/tracer/SchedulerApiTracer.h"
#include "fboss/agent/hw/sai/tracer/SwitchApiTracer.h"
#include "fboss/agent/hw/sai/tracer/VirtualRouterApiTracer.h"
#include "fboss/agent/hw/sai/tracer/VlanApiTracer.h"

#include <folly/FileUtil.h>
#include <folly/MacAddress.h>
#include <folly/MapUtil.h>
#include <folly/Singleton.h>
#include <folly/String.h>

extern "C" {
#include <sai.h>
}

DEFINE_bool(
    enable_replayer,
    false,
    "Flag to indicate whether function wrapper and logger is enabled.");

DEFINE_bool(
    enable_packet_log,
    false,
    "Flag to indicate whether to log the send_hostif_packet function."
    "At runtime, it should be disabled to reduce logging overhead."
    "However, it's needed for testing e.g. HwL4PortBlackHolingTest");

DEFINE_string(sai_log, "/tmp/sai_log.c", "File path to the SAI Replayer logs");

DEFINE_int32(
    default_list_size,
    128,
    "Default size of the lists initialzied by SAI replayer");

DEFINE_int32(
    default_list_count,
    6,
    "Default number of the lists initialzied by SAI replayer");

using facebook::fboss::SaiTracer;
using folly::to;
using std::string;
using std::vector;

extern "C" {

// Real functions for Sai APIs
sai_status_t __real_sai_api_query(
    sai_api_t sai_api_id,
    void** api_method_table);

// Wrap function for Sai APIs
sai_status_t __wrap_sai_api_query(
    sai_api_t sai_api_id,
    void** api_method_table) {
  // Functions defined in sai.h can be properly wrapped using the linker trick
  // e.g. sai_api_query(), sai_api_initialize() ...
  // However, for other functions such as create_*, set_*, and get_*,
  // their function pointers are returned to the api_method_table in this call.
  // To 'wrap' those function pointers as well, we store the real
  // function pointers and return the 'wrapped' function pointers defined in
  // this tracer directory, adding an redirection layer for logging purpose.
  // (See 'AclApiTracer.h' for example).

  sai_status_t rv = __real_sai_api_query(sai_api_id, api_method_table);

  switch (sai_api_id) {
    case SAI_API_ACL:
      SaiTracer::getInstance()->aclApi_ =
          static_cast<sai_acl_api_t*>(*api_method_table);
      *api_method_table = facebook::fboss::wrapAclApi();
      SaiTracer::getInstance()->logApiQuery(sai_api_id, "acl_api");
      break;
    case SAI_API_BRIDGE:
      SaiTracer::getInstance()->bridgeApi_ =
          static_cast<sai_bridge_api_t*>(*api_method_table);
      *api_method_table = facebook::fboss::wrapBridgeApi();
      SaiTracer::getInstance()->logApiQuery(sai_api_id, "bridge_api");
      break;
    case SAI_API_BUFFER:
      SaiTracer::getInstance()->bufferApi_ =
          static_cast<sai_buffer_api_t*>(*api_method_table);
      *api_method_table = facebook::fboss::wrapBufferApi();
      SaiTracer::getInstance()->logApiQuery(sai_api_id, "buffer_api");
      break;
    case SAI_API_FDB:
      SaiTracer::getInstance()->fdbApi_ =
          static_cast<sai_fdb_api_t*>(*api_method_table);
      *api_method_table = facebook::fboss::wrapFdbApi();
      SaiTracer::getInstance()->logApiQuery(sai_api_id, "fdb_api");
      break;
    case SAI_API_HASH:
      SaiTracer::getInstance()->hashApi_ =
          static_cast<sai_hash_api_t*>(*api_method_table);
      *api_method_table = facebook::fboss::wrapHashApi();
      SaiTracer::getInstance()->logApiQuery(sai_api_id, "hash_api");
      break;
    case SAI_API_HOSTIF:
      SaiTracer::getInstance()->hostifApi_ =
          static_cast<sai_hostif_api_t*>(*api_method_table);
      *api_method_table = facebook::fboss::wrapHostifApi();
      SaiTracer::getInstance()->logApiQuery(sai_api_id, "hostif_api");
      break;
    case SAI_API_MPLS:
      SaiTracer::getInstance()->mplsApi_ =
          static_cast<sai_mpls_api_t*>(*api_method_table);
      *api_method_table = facebook::fboss::wrapMplsApi();
      SaiTracer::getInstance()->logApiQuery(sai_api_id, "mpls_api");
      break;
    case SAI_API_NEIGHBOR:
      SaiTracer::getInstance()->neighborApi_ =
          static_cast<sai_neighbor_api_t*>(*api_method_table);
      *api_method_table = facebook::fboss::wrapNeighborApi();
      SaiTracer::getInstance()->logApiQuery(sai_api_id, "neighbor_api");
      break;
    case SAI_API_NEXT_HOP:
      SaiTracer::getInstance()->nextHopApi_ =
          static_cast<sai_next_hop_api_t*>(*api_method_table);
      *api_method_table = facebook::fboss::wrapNextHopApi();
      SaiTracer::getInstance()->logApiQuery(sai_api_id, "next_hop_api");
      break;
    case SAI_API_NEXT_HOP_GROUP:
      SaiTracer::getInstance()->nextHopGroupApi_ =
          static_cast<sai_next_hop_group_api_t*>(*api_method_table);
      *api_method_table = facebook::fboss::wrapNextHopGroupApi();
      SaiTracer::getInstance()->logApiQuery(sai_api_id, "next_hop_group_api");
      break;
    case SAI_API_PORT:
      SaiTracer::getInstance()->portApi_ =
          static_cast<sai_port_api_t*>(*api_method_table);
      *api_method_table = facebook::fboss::wrapPortApi();
      SaiTracer::getInstance()->logApiQuery(sai_api_id, "port_api");
      break;
    case SAI_API_QOS_MAP:
      SaiTracer::getInstance()->qosMapApi_ =
          static_cast<sai_qos_map_api_t*>(*api_method_table);
      *api_method_table = facebook::fboss::wrapQosMapApi();
      SaiTracer::getInstance()->logApiQuery(sai_api_id, "qos_map_api");
      break;
    case SAI_API_QUEUE:
      SaiTracer::getInstance()->queueApi_ =
          static_cast<sai_queue_api_t*>(*api_method_table);
      *api_method_table = facebook::fboss::wrapQueueApi();
      SaiTracer::getInstance()->logApiQuery(sai_api_id, "queue_api");
      break;
    case SAI_API_ROUTE:
      SaiTracer::getInstance()->routeApi_ =
          static_cast<sai_route_api_t*>(*api_method_table);
      *api_method_table = facebook::fboss::wrapRouteApi();
      SaiTracer::getInstance()->logApiQuery(sai_api_id, "route_api");
      break;
    case SAI_API_ROUTER_INTERFACE:
      SaiTracer::getInstance()->routerInterfaceApi_ =
          static_cast<sai_router_interface_api_t*>(*api_method_table);
      *api_method_table = facebook::fboss::wrapRouterInterfaceApi();
      SaiTracer::getInstance()->logApiQuery(sai_api_id, "router_interface_api");
      break;
    case SAI_API_SCHEDULER:
      SaiTracer::getInstance()->schedulerApi_ =
          static_cast<sai_scheduler_api_t*>(*api_method_table);
      *api_method_table = facebook::fboss::wrapSchedulerApi();
      SaiTracer::getInstance()->logApiQuery(sai_api_id, "scheduler_api");
      break;
    case SAI_API_SWITCH:
      SaiTracer::getInstance()->switchApi_ =
          static_cast<sai_switch_api_t*>(*api_method_table);
      *api_method_table = facebook::fboss::wrapSwitchApi();
      SaiTracer::getInstance()->logApiQuery(sai_api_id, "switch_api");
      break;
    case SAI_API_VIRTUAL_ROUTER:
      SaiTracer::getInstance()->virtualRouterApi_ =
          static_cast<sai_virtual_router_api_t*>(*api_method_table);
      *api_method_table = facebook::fboss::wrapVirtualRouterApi();
      SaiTracer::getInstance()->logApiQuery(sai_api_id, "virtual_router_api");
      break;
    case SAI_API_VLAN:
      SaiTracer::getInstance()->vlanApi_ =
          static_cast<sai_vlan_api_t*>(*api_method_table);
      *api_method_table = facebook::fboss::wrapVlanApi();
      SaiTracer::getInstance()->logApiQuery(sai_api_id, "vlan_api");
      break;
    default:
      // TODO: For other APIs, create new API wrappers and invoke wrapApi()
      // funtion here
      break;
  }
  return rv;
}

} // extern "C"

namespace {

folly::Singleton<facebook::fboss::SaiTracer> _saiTracer;

inline void printHex(std::ostringstream& outStringStream, uint8_t u8) {
  outStringStream << "0x" << std::setfill('0') << std::setw(2) << std::hex
                  << static_cast<int>(u8);
}

inline int flushToFile(
    folly::Synchronized<folly::File>& logFile,
    const char* content,
    size_t length) {
  return logFile.withWLock([&](auto& lockedFile) {
    return folly::writeFull(lockedFile.fd(), content, length);
  });
}

} // namespace

namespace facebook::fboss {

SaiTracer::SaiTracer() {
  if (FLAGS_enable_replayer) {
    saiLogFile_ =
        folly::File(FLAGS_sai_log.c_str(), O_RDWR | O_CREAT | O_TRUNC);

    if (flushToFile(saiLogFile_, cpp_header_, strlen(cpp_header_)) < 0) {
      throw SysError(
          errno,
          "error writing ",
          strlen(cpp_header_),
          " bytes to SAI Replayer log file");
    }

    setupGlobals();
    initVarCounts();
  }
}

SaiTracer::~SaiTracer() {
  if (FLAGS_enable_replayer) {
    writeFooter();
    fsync(saiLogFile_.wlock()->fd());
  }
}

std::shared_ptr<SaiTracer> SaiTracer::getInstance() {
  return _saiTracer.try_get();
}

void SaiTracer::writeToFile(const vector<string>& strVec) {
  if (!FLAGS_enable_replayer) {
    return;
  }

  auto constexpr lineEnd = ";\n";
  auto lines = folly::join(lineEnd, strVec) + lineEnd + "\n";

  if (flushToFile(saiLogFile_, lines.c_str(), lines.size()) < 0) {
    throw SysError(
        errno,
        "error writing ",
        lines.size(),
        " bytes to SAI Replayer log file");
  }
}

void SaiTracer::logApiQuery(sai_api_t api_id, const std::string& api_var) {
  // If replayer is not enabled or api is already initialized
  if (!FLAGS_enable_replayer || init_api_.find(api_id) != init_api_.end()) {
    return;
  }

  init_api_.emplace(api_id, api_var);

  writeToFile(
      {to<string>("sai_", api_var, "_t* ", api_var),
       to<string>(
           "sai_api_query((sai_api_t)", api_id, ", (void**)&", api_var, ")")});
}

void SaiTracer::logSwitchCreateFn(
    sai_object_id_t* switch_id,
    uint32_t attr_count,
    const sai_attribute_t* attr_list,
    sai_status_t rv) {
  if (!FLAGS_enable_replayer) {
    return;
  }

  // First fill in attribute list
  vector<string> lines =
      setAttrList(attr_list, attr_count, SAI_OBJECT_TYPE_SWITCH);

  // Then create new variable - declareVariable returns
  // 1. Declaration of the new variable
  // 2. name of the new variable
  auto [declaration, varName] =
      declareVariable(switch_id, SAI_OBJECT_TYPE_SWITCH);
  lines.push_back(declaration);

  // Log current timestamp, object id and return value
  lines.push_back(logTimeAndId(*switch_id, rv));

  // Make the function call
  lines.push_back(to<string>(
      "status = ",
      folly::get_or_throw(
          fnPrefix_,
          SAI_OBJECT_TYPE_SWITCH,
          "Unsupported Sai Object type in Sai Tracer"),
      "create_switch",
      "(&",
      varName,
      ", ",
      attr_count,
      ", sai_attributes)"));

  // Check return value to be the same as the original run
  lines.push_back(rvCheck(rv));

  writeToFile(lines);
}

void SaiTracer::logRouteEntryCreateFn(
    const sai_route_entry_t* route_entry,
    uint32_t attr_count,
    const sai_attribute_t* attr_list,
    sai_status_t rv) {
  if (!FLAGS_enable_replayer) {
    return;
  }

  // First fill in attribute list
  vector<string> lines =
      setAttrList(attr_list, attr_count, SAI_OBJECT_TYPE_ROUTE_ENTRY);

  // Then setup route entry (switch, virtual router and destination)
  setRouteEntry(route_entry, lines);

  // TODO(zecheng): Log current timestamp, object id and return value

  // Make the function call
  lines.push_back(to<string>(
      "status = ",
      folly::get_or_throw(
          fnPrefix_,
          SAI_OBJECT_TYPE_ROUTE_ENTRY,
          "Unsupported Sai Object type in Sai Tracer"),
      "create_route_entry(&route_entry, ",
      attr_count,
      ", sai_attributes)"));

  // Check return value to be the same as the original run
  lines.push_back(rvCheck(rv));

  writeToFile(lines);
}

void SaiTracer::logNeighborEntryCreateFn(
    const sai_neighbor_entry_t* neighbor_entry,
    uint32_t attr_count,
    const sai_attribute_t* attr_list,
    sai_status_t rv) {
  if (!FLAGS_enable_replayer) {
    return;
  }

  // First fill in attribute list
  vector<string> lines =
      setAttrList(attr_list, attr_count, SAI_OBJECT_TYPE_NEIGHBOR_ENTRY);

  // Then setup neighbor entry (switch, router interface and ip address)
  setNeighborEntry(neighbor_entry, lines);

  // TODO(zecheng): Log current timestamp, object id and return value

  // Make the function call
  lines.push_back(to<string>(
      "status = ",
      folly::get_or_throw(
          fnPrefix_,
          SAI_OBJECT_TYPE_NEIGHBOR_ENTRY,
          "Unsupported Sai Object type in Sai Tracer"),
      "create_neighbor_entry(&neighbor_entry, ",
      attr_count,
      ", sai_attributes)"));

  // Check return value to be the same as the original run
  lines.push_back(rvCheck(rv));

  writeToFile(lines);
}

void SaiTracer::logFdbEntryCreateFn(
    const sai_fdb_entry_t* fdb_entry,
    uint32_t attr_count,
    const sai_attribute_t* attr_list,
    sai_status_t rv) {
  if (!FLAGS_enable_replayer) {
    return;
  }

  // First fill in attribute list
  vector<string> lines =
      setAttrList(attr_list, attr_count, SAI_OBJECT_TYPE_FDB_ENTRY);

  // Then setup fdb entry (switch, router interface and ip address)
  setFdbEntry(fdb_entry, lines);

  // TODO(zecheng): Log current timestamp, object id and return value

  // Make the function call
  lines.push_back(to<string>(
      "status = ",
      folly::get_or_throw(
          fnPrefix_,
          SAI_OBJECT_TYPE_FDB_ENTRY,
          "Unsupported Sai Object type in Sai Tracer"),
      "create_fdb_entry(&fdb_entry, ",
      attr_count,
      ", sai_attributes)"));

  // Check return value to be the same as the original run
  lines.push_back(rvCheck(rv));

  writeToFile(lines);
}

void SaiTracer::logInsegEntryCreateFn(
    const sai_inseg_entry_t* inseg_entry,
    uint32_t attr_count,
    const sai_attribute_t* attr_list,
    sai_status_t rv) {
  if (!FLAGS_enable_replayer) {
    return;
  }

  // First fill in attribute list
  vector<string> lines =
      setAttrList(attr_list, attr_count, SAI_OBJECT_TYPE_INSEG_ENTRY);

  // Then setup inseg entry (switch and label)
  setInsegEntry(inseg_entry, lines);

  // TODO(zecheng): Log current timestamp, object id and return value

  // Make the function call
  lines.push_back(to<string>(
      "status = ",
      folly::get_or_throw(
          fnPrefix_,
          SAI_OBJECT_TYPE_INSEG_ENTRY,
          "Unsupported Sai Object type in Sai Tracer"),
      "create_inseg_entry(&inseg_entry, ",
      attr_count,
      ", sai_attributes)"));

  // Check return value to be the same as the original run
  lines.push_back(rvCheck(rv));

  writeToFile(lines);
}

void SaiTracer::logCreateFn(
    const string& fn_name,
    sai_object_id_t* create_object_id,
    sai_object_id_t switch_id,
    uint32_t attr_count,
    const sai_attribute_t* attr_list,
    sai_object_type_t object_type,
    sai_status_t rv) {
  if (!FLAGS_enable_replayer) {
    return;
  }

  // First fill in attribute list
  vector<string> lines = setAttrList(attr_list, attr_count, object_type);

  // Then create new variable - declareVariable returns
  // 1. Declaration of the new variable
  // 2. name of the new variable
  auto [declaration, varName] = declareVariable(create_object_id, object_type);
  lines.push_back(declaration);

  // Log current timestamp, object id and return value
  lines.push_back(logTimeAndId(*create_object_id, rv));

  // Make the function call & write to file
  lines.push_back(createFnCall(
      fn_name,
      varName,
      getVariable(switch_id, {SAI_OBJECT_TYPE_SWITCH}),
      attr_count,
      object_type));

  // Check return value to be the same as the original run
  lines.push_back(rvCheck(rv));

  writeToFile(lines);
}

void SaiTracer::logRouteEntryRemoveFn(
    const sai_route_entry_t* route_entry,
    sai_status_t rv) {
  if (!FLAGS_enable_replayer) {
    return;
  }

  vector<string> lines{};
  setRouteEntry(route_entry, lines);

  // TODO(zecheng): Log current timestamp, object id and return value

  lines.push_back(to<string>(
      "status = ",
      folly::get_or_throw(
          fnPrefix_,
          SAI_OBJECT_TYPE_ROUTE_ENTRY,
          "Unsupported Sai Object type in Sai Tracer"),
      "remove_route_entry(&route_entry)"));

  // Check return value to be the same as the original run
  lines.push_back(rvCheck(rv));

  writeToFile(lines);
}

void SaiTracer::logNeighborEntryRemoveFn(
    const sai_neighbor_entry_t* neighbor_entry,
    sai_status_t rv) {
  if (!FLAGS_enable_replayer) {
    return;
  }

  vector<string> lines{};
  setNeighborEntry(neighbor_entry, lines);

  // TODO(zecheng): Log current timestamp, object id and return value

  lines.push_back(to<string>(
      "status = ",
      folly::get_or_throw(
          fnPrefix_,
          SAI_OBJECT_TYPE_NEIGHBOR_ENTRY,
          "Unsupported Sai Object type in Sai Tracer"),
      "remove_neighbor_entry(&neighbor_entry)"));

  // Check return value to be the same as the original run
  lines.push_back(rvCheck(rv));

  writeToFile(lines);
}

void SaiTracer::logFdbEntryRemoveFn(
    const sai_fdb_entry_t* fdb_entry,
    sai_status_t rv) {
  if (!FLAGS_enable_replayer) {
    return;
  }

  vector<string> lines{};
  setFdbEntry(fdb_entry, lines);

  // TODO(zecheng): Log current timestamp, object id and return value

  lines.push_back(to<string>(
      "status = ",
      folly::get_or_throw(
          fnPrefix_,
          SAI_OBJECT_TYPE_FDB_ENTRY,
          "Unsupported Sai Object type in Sai Tracer"),
      "remove_fdb_entry(&fdb_entry)"));

  // Check return value to be the same as the original run
  lines.push_back(rvCheck(rv));

  writeToFile(lines);
}

void SaiTracer::logInsegEntryRemoveFn(
    const sai_inseg_entry_t* inseg_entry,
    sai_status_t rv) {
  if (!FLAGS_enable_replayer) {
    return;
  }

  vector<string> lines{};
  setInsegEntry(inseg_entry, lines);

  // TODO(zecheng): Log current timestamp, object id and return value

  lines.push_back(to<string>(
      "status = ",
      folly::get_or_throw(
          fnPrefix_,
          SAI_OBJECT_TYPE_INSEG_ENTRY,
          "Unsupported Sai Object type in Sai Tracer"),
      "remove_inseg_entry(&inseg_entry)"));

  // Check return value to be the same as the original run
  lines.push_back(rvCheck(rv));

  writeToFile(lines);
}

void SaiTracer::logRemoveFn(
    const string& fn_name,
    sai_object_id_t remove_object_id,
    sai_object_type_t object_type,
    sai_status_t rv) {
  if (!FLAGS_enable_replayer) {
    return;
  }

  vector<string> lines{};

  // Log current timestamp, object id and return value
  lines.push_back(logTimeAndId(remove_object_id, rv));

  // Make the remove call
  lines.push_back(to<string>(
      "status = ",
      folly::get_or_throw(
          fnPrefix_, object_type, "Unsupported Sai Object type in Sai Tracer"),
      fn_name,
      "(",
      getVariable(remove_object_id, {object_type}),
      ")"));

  // Check return value to be the same as the original run
  lines.push_back(rvCheck(rv));

  writeToFile(lines);

  // Remove object from variables_
  folly::get_or_throw(
      variables_, object_type, "Unsupported Sai Object type in Sai Tracer")
      .withWLock([&](auto& vars) { return vars.erase(remove_object_id); });
}

void SaiTracer::logRouteEntrySetAttrFn(
    const sai_route_entry_t* route_entry,
    const sai_attribute_t* attr,
    sai_status_t rv) {
  if (!FLAGS_enable_replayer) {
    return;
  }

  // Setup one attribute
  vector<string> lines = setAttrList(attr, 1, SAI_OBJECT_TYPE_ROUTE_ENTRY);

  setRouteEntry(route_entry, lines);

  // TODO(zecheg): Log current timestamp, object id and return value

  // Make setAttribute call
  lines.push_back(to<string>(
      "status = ",
      folly::get_or_throw(
          fnPrefix_,
          SAI_OBJECT_TYPE_ROUTE_ENTRY,
          "Unsupported Sai Object type in Sai Tracer"),
      "set_route_entry_attribute(&route_entry, sai_attributes)"));

  // Check return value to be the same as the original run
  lines.push_back(rvCheck(rv));

  writeToFile(lines);
}

void SaiTracer::logNeighborEntrySetAttrFn(
    const sai_neighbor_entry_t* neighbor_entry,
    const sai_attribute_t* attr,
    sai_status_t rv) {
  if (!FLAGS_enable_replayer) {
    return;
  }

  // Setup one attribute
  vector<string> lines = setAttrList(attr, 1, SAI_OBJECT_TYPE_NEIGHBOR_ENTRY);

  setNeighborEntry(neighbor_entry, lines);

  // TODO(zecheg): Log current timestamp, object id and return value

  // Make setAttribute call
  lines.push_back(to<string>(
      "status = ",
      folly::get_or_throw(
          fnPrefix_,
          SAI_OBJECT_TYPE_NEIGHBOR_ENTRY,
          "Unsupported Sai Object type in Sai Tracer"),
      "set_neighbor_entry_attribute(&neighbor_entry, sai_attributes)"));

  // Check return value to be the same as the original run
  lines.push_back(rvCheck(rv));

  writeToFile(lines);
}

void SaiTracer::logFdbEntrySetAttrFn(
    const sai_fdb_entry_t* fdb_entry,
    const sai_attribute_t* attr,
    sai_status_t rv) {
  if (!FLAGS_enable_replayer) {
    return;
  }

  // Setup one attribute
  vector<string> lines = setAttrList(attr, 1, SAI_OBJECT_TYPE_FDB_ENTRY);

  setFdbEntry(fdb_entry, lines);

  // TODO(zecheg): Log current timestamp, object id and return value

  // Make setAttribute call
  lines.push_back(to<string>(
      "status = ",
      folly::get_or_throw(
          fnPrefix_,
          SAI_OBJECT_TYPE_FDB_ENTRY,
          "Unsupported Sai Object type in Sai Tracer"),
      "set_fdb_entry_attribute(&fdb_entry, sai_attributes)"));

  // Check return value to be the same as the original run
  lines.push_back(rvCheck(rv));

  writeToFile(lines);
}

void SaiTracer::logInsegEntrySetAttrFn(
    const sai_inseg_entry_t* inseg_entry,
    const sai_attribute_t* attr,
    sai_status_t rv) {
  if (!FLAGS_enable_replayer) {
    return;
  }

  // Setup one attribute
  vector<string> lines = setAttrList(attr, 1, SAI_OBJECT_TYPE_INSEG_ENTRY);

  setInsegEntry(inseg_entry, lines);

  // TODO(zecheg): Log current timestamp, object id and return value

  // Make setAttribute call
  lines.push_back(to<string>(
      "status = ",
      folly::get_or_throw(
          fnPrefix_,
          SAI_OBJECT_TYPE_INSEG_ENTRY,
          "Unsupported Sai Object type in Sai Tracer"),
      "set_inseg_entry_attribute(&inseg_entry, sai_attributes)"));

  // Check return value to be the same as the original run
  lines.push_back(rvCheck(rv));

  writeToFile(lines);
}

void SaiTracer::logSetAttrFn(
    const string& fn_name,
    sai_object_id_t set_object_id,
    const sai_attribute_t* attr,
    sai_object_type_t object_type,
    sai_status_t rv) {
  if (!FLAGS_enable_replayer) {
    return;
  }

  // Setup one attribute
  vector<string> lines = setAttrList(attr, 1, object_type);

  // Log current timestamp, object id and return value
  lines.push_back(logTimeAndId(set_object_id, rv));

  // Make setAttribute call
  lines.push_back(to<string>(
      "status = ",
      folly::get_or_throw(
          fnPrefix_, object_type, "Unsupported Sai Object type in Sai Tracer"),
      fn_name,
      "(",
      getVariable(set_object_id, {object_type}),
      ", sai_attributes)"));

  // Check return value to be the same as the original run
  lines.push_back(rvCheck(rv));

  writeToFile(lines);
}

void SaiTracer::logSendHostifPacketFn(
    sai_object_id_t hostif_id,
    sai_size_t buffer_size,
    const uint8_t* buffer,
    uint32_t attr_count,
    const sai_attribute_t* attr_list,
    sai_status_t rv) {
  if (!FLAGS_enable_replayer || !FLAGS_enable_packet_log) {
    return;
  }

  vector<string> lines =
      setAttrList(attr_list, attr_count, SAI_OBJECT_TYPE_HOSTIF_PACKET);

  // packet_buffer is declared in local space for each send packet call
  // Therefore, we open the bracket here and then declare the buffer
  lines.push_back({"{"});

  std::ostringstream outStringStream;
  outStringStream << "uint8_t packet_buffer[" << buffer_size << "] = {";
  for (int i = 0; i < buffer_size - 1; i += 32) {
    outStringStream << "\n";
    uint64_t j_end = i + 32;

    for (int j = i; j < std::min(j_end, buffer_size - 1); ++j) {
      printHex(outStringStream, buffer[j]);
      outStringStream << ", ";
    }
  }
  printHex(outStringStream, buffer[buffer_size - 1]);
  outStringStream << "}";
  lines.push_back(outStringStream.str());

  lines.push_back(to<string>(
      "status = ",
      folly::get_or_throw(
          fnPrefix_,
          SAI_OBJECT_TYPE_HOSTIF,
          "Unsupported Sai Object type in Sai Tracer"),
      "send_hostif_packet(",
      getVariable(hostif_id, {SAI_OBJECT_TYPE_HOSTIF, SAI_OBJECT_TYPE_SWITCH}),
      ", ",
      buffer_size,
      ", packet_buffer, ",
      attr_count,
      ", sai_attributes)"));

  // Close bracket for local scope
  lines.push_back({"}"});

  lines.push_back(rvCheck(rv));
  writeToFile(lines);
}

std::tuple<string, string> SaiTracer::declareVariable(
    sai_object_id_t* object_id,
    sai_object_type_t object_type) {
  if (!FLAGS_enable_replayer) {
    return std::make_tuple("", "");
  }

  auto constexpr varType = "sai_object_id_t ";

  // Get the prefix name for pbject type
  string varPrefix = folly::get_or_throw(
      varNames_, object_type, "Unsupported Sai Object type in Sai Tracer");

  // Get the variable count for declaration (e.g. aclTable_1)
  uint num = folly::get_or_throw(
      varCounts_, object_type, "Unsupported Sai Object type in Sai Tracer")++;
  string varName = to<string>(varPrefix, num);

  // Add this variable to the variable map
  folly::get_or_throw(
      variables_, object_type, "Unsupported Sai Object type in Sai Tracer")
      .wlock()
      ->emplace(*object_id, varName);
  return std::make_tuple(to<string>(varType, varName), varName);
}

string SaiTracer::getVariable(
    sai_object_id_t object_id,
    vector<sai_object_type_t> object_types) {
  if (!FLAGS_enable_replayer) {
    return "";
  }

  // Check variable map to get object_id -> variable name mapping
  for (auto object_type : object_types) {
    auto varName = folly::get_or_throw(
                       variables_,
                       object_type,
                       "Unsupported Sai Object type in Sai Tracer")
                       .withRLock([&](auto& vars) {
                         return folly::get_optional(vars, object_id);
                       });
    if (varName) {
      return *varName;
    }
  }

  return to<string>(object_id);
}

vector<string> SaiTracer::setAttrList(
    const sai_attribute_t* attr_list,
    uint32_t attr_count,
    sai_object_type_t object_type) {
  if (!FLAGS_enable_replayer) {
    return {};
  }

  checkAttrCount(attr_count);

  auto constexpr sai_attribute = "sai_attributes";
  vector<string> attrLines;

  attrLines.push_back(to<string>(
      "memset(sai_attributes, 0, sizeof(sai_attribute_t) * ",
      maxAttrCount_,
      ")"));

  // Setup ids
  for (int i = 0; i < attr_count; ++i) {
    attrLines.push_back(
        to<string>(sai_attribute, "[", i, "].id = ", attr_list[i].id));
  }

  // Call functions defined in *ApiTracer.h to serialize attributes
  // that are specific to each Sai object type
  switch (object_type) {
    case SAI_OBJECT_TYPE_ACL_ENTRY:
      setAclEntryAttributes(attr_list, attr_count, attrLines);
      break;
    case SAI_OBJECT_TYPE_ACL_TABLE:
      setAclTableAttributes(attr_list, attr_count, attrLines);
      break;
    case SAI_OBJECT_TYPE_ACL_TABLE_GROUP:
      setAclTableGroupAttributes(attr_list, attr_count, attrLines);
      break;
    case SAI_OBJECT_TYPE_ACL_TABLE_GROUP_MEMBER:
      setAclTableGroupMemberAttributes(attr_list, attr_count, attrLines);
      break;
    case SAI_OBJECT_TYPE_BRIDGE:
      setBridgeAttributes(attr_list, attr_count, attrLines);
      break;
    case SAI_OBJECT_TYPE_BRIDGE_PORT:
      setBridgePortAttributes(attr_list, attr_count, attrLines);
      break;
    case SAI_OBJECT_TYPE_BUFFER_POOL:
      setBufferPoolAttributes(attr_list, attr_count, attrLines);
      break;
    case SAI_OBJECT_TYPE_BUFFER_PROFILE:
      setBufferProfileAttributes(attr_list, attr_count, attrLines);
      break;
    case SAI_OBJECT_TYPE_FDB_ENTRY:
      setFdbEntryAttributes(attr_list, attr_count, attrLines);
      break;
    case SAI_OBJECT_TYPE_HASH:
      setHashAttributes(attr_list, attr_count, attrLines);
      break;
    case SAI_OBJECT_TYPE_HOSTIF_PACKET:
      setHostifPacketAttributes(attr_list, attr_count, attrLines);
      break;
    case SAI_OBJECT_TYPE_HOSTIF_TRAP:
      setHostifTrapAttributes(attr_list, attr_count, attrLines);
      break;
    case SAI_OBJECT_TYPE_HOSTIF_TRAP_GROUP:
      setHostifTrapGroupAttributes(attr_list, attr_count, attrLines);
      break;
    case SAI_OBJECT_TYPE_INSEG_ENTRY:
      setInsegEntryAttributes(attr_list, attr_count, attrLines);
      break;
    case SAI_OBJECT_TYPE_NEIGHBOR_ENTRY:
      setNeighborEntryAttributes(attr_list, attr_count, attrLines);
      break;
    case SAI_OBJECT_TYPE_NEXT_HOP:
      setNextHopAttributes(attr_list, attr_count, attrLines);
      break;
    case SAI_OBJECT_TYPE_NEXT_HOP_GROUP:
      setNextHopGroupAttributes(attr_list, attr_count, attrLines);
      break;
    case SAI_OBJECT_TYPE_NEXT_HOP_GROUP_MEMBER:
      setNextHopGroupMemberAttributes(attr_list, attr_count, attrLines);
      break;
    case SAI_OBJECT_TYPE_PORT:
      setPortAttributes(attr_list, attr_count, attrLines);
      break;
    case SAI_OBJECT_TYPE_QOS_MAP:
      setQosMapAttributes(attr_list, attr_count, attrLines);
      break;
    case SAI_OBJECT_TYPE_QUEUE:
      setQueueAttributes(attr_list, attr_count, attrLines);
      break;
    case SAI_OBJECT_TYPE_ROUTE_ENTRY:
      setRouteEntryAttributes(attr_list, attr_count, attrLines);
      break;
    case SAI_OBJECT_TYPE_ROUTER_INTERFACE:
      setRouterInterfaceAttributes(attr_list, attr_count, attrLines);
      break;
    case SAI_OBJECT_TYPE_SCHEDULER:
      setSchedulerAttributes(attr_list, attr_count, attrLines);
      break;
    case SAI_OBJECT_TYPE_SWITCH:
      setSwitchAttributes(attr_list, attr_count, attrLines);
      break;
    case SAI_OBJECT_TYPE_VIRTUAL_ROUTER:
      setVirtualRouterAttributes(attr_list, attr_count, attrLines);
      break;
    case SAI_OBJECT_TYPE_VLAN:
      setVlanAttributes(attr_list, attr_count, attrLines);
      break;
    case SAI_OBJECT_TYPE_VLAN_MEMBER:
      setVlanMemberAttributes(attr_list, attr_count, attrLines);
      break;
    default:
      // TODO: For other APIs, create new API wrappers and invoke
      // setAttributes() function here
      break;
  }

  return attrLines;
}

string SaiTracer::createFnCall(
    const string& fn_name,
    const string& var1,
    const string& var2,
    uint32_t attr_count,
    sai_object_type_t object_type) {
  // This helper method produces the create call. For example -
  // bridge_api->create_bridge_port(&bridgePort_1, switch_0, 4, sai_attributes);
  return to<string>(
      "status = ",
      folly::get_or_throw(
          fnPrefix_, object_type, "Unsupported Sai Object type in Sai Tracer"),
      fn_name,
      "(&",
      var1,
      ", ",
      var2,
      ", ",
      attr_count,
      ", sai_attributes)");
}

void SaiTracer::setFdbEntry(
    const sai_fdb_entry_t* fdb_entry,
    std::vector<std::string>& lines) {
  lines.push_back(to<string>(
      "fdb_entry.switch_id = ",
      getVariable(fdb_entry->switch_id, {SAI_OBJECT_TYPE_SWITCH})));
  lines.push_back(to<string>(
      "fdb_entry.bv_id = ",
      getVariable(
          fdb_entry->bv_id, {SAI_OBJECT_TYPE_BRIDGE, SAI_OBJECT_TYPE_VLAN})));

  // The underlying type of sai_mac_t is uint8_t[6]
  for (int i = 0; i < 6; ++i) {
    lines.push_back(to<string>(
        "fdb_entry.mac_address[", i, "] = ", fdb_entry->mac_address[i]));
  }
}

void SaiTracer::setInsegEntry(
    const sai_inseg_entry_t* inseg_entry,
    std::vector<std::string>& lines) {
  lines.push_back(to<string>(
      "inseg_entry.switch_id = ",
      getVariable(inseg_entry->switch_id, {SAI_OBJECT_TYPE_SWITCH})));
  lines.push_back(to<string>("inseg_entry.label = ", inseg_entry->label));
}

void SaiTracer::setNeighborEntry(
    const sai_neighbor_entry_t* neighbor_entry,
    vector<string>& lines) {
  lines.push_back(to<string>(
      "neighbor_entry.switch_id = ",
      getVariable(neighbor_entry->switch_id, {SAI_OBJECT_TYPE_SWITCH})));
  lines.push_back(to<string>(
      "neighbor_entry.rif_id = ",
      getVariable(neighbor_entry->rif_id, {SAI_OBJECT_TYPE_ROUTER_INTERFACE})));

  if (neighbor_entry->ip_address.addr_family == SAI_IP_ADDR_FAMILY_IPV4) {
    lines.push_back(
        "neighbor_entry.ip_address.addr_family = SAI_IP_ADDR_FAMILY_IPV4");
    lines.push_back(to<string>(
        "neighbor_entry.ip_address.addr.ip4 = ",
        neighbor_entry->ip_address.addr.ip4));

  } else if (
      neighbor_entry->ip_address.addr_family == SAI_IP_ADDR_FAMILY_IPV6) {
    lines.push_back(
        "neighbor_entry.ip_address.addr_family = SAI_IP_ADDR_FAMILY_IPV6");

    // Underlying type of sai_ip6_t is uint8_t[16]
    for (int i = 0; i < 16; ++i) {
      lines.push_back(to<string>(
          "neighbor_entry.ip_address.addr.ip6[",
          i,
          "] = ",
          neighbor_entry->ip_address.addr.ip6[i]));
    }
  }
}

void SaiTracer::setRouteEntry(
    const sai_route_entry_t* route_entry,
    vector<string>& lines) {
  lines.push_back(to<string>(
      "route_entry.switch_id = ",
      getVariable(route_entry->switch_id, {SAI_OBJECT_TYPE_SWITCH})));
  lines.push_back(to<string>(
      "route_entry.vr_id = ",
      getVariable(route_entry->vr_id, {SAI_OBJECT_TYPE_VIRTUAL_ROUTER})));

  if (route_entry->destination.addr_family == SAI_IP_ADDR_FAMILY_IPV4) {
    lines.push_back(
        "route_entry.destination.addr_family = SAI_IP_ADDR_FAMILY_IPV4");
    lines.push_back(to<string>(
        "route_entry.destination.addr.ip4 = ",
        route_entry->destination.addr.ip4));
    lines.push_back(to<string>(
        "route_entry.destination.mask.ip4 = ",
        route_entry->destination.mask.ip4));
  } else if (route_entry->destination.addr_family == SAI_IP_ADDR_FAMILY_IPV6) {
    lines.push_back(
        "route_entry.destination.addr_family = SAI_IP_ADDR_FAMILY_IPV6");
    // Underlying type of sai_ip6_t is uint8_t[16]
    for (int i = 0; i < 16; ++i) {
      lines.push_back(to<string>(
          "route_entry.destination.addr.ip6[",
          i,
          "] = ",
          route_entry->destination.addr.ip6[i]));
    }
    for (int i = 0; i < 16; ++i) {
      lines.push_back(to<string>(
          "route_entry.destination.mask.ip6[",
          i,
          "] = ",
          route_entry->destination.mask.ip6[i]));
    }
  }
}

string SaiTracer::rvCheck(sai_status_t rv) {
  return to<string>(
      "if (status != ",
      rv,
      ") printf(\"Unexpected rv at ",
      numCalls_++,
      " with status %d \\n\", status)");
}

string SaiTracer::logTimeAndId(sai_object_id_t object_id, sai_status_t rv) {
  auto now = std::chrono::system_clock::now();
  auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                    now.time_since_epoch()) %
      1000;
  auto timer = std::chrono::system_clock::to_time_t(now);
  std::tm tm;
  localtime_r(&timer, &tm);

  std::ostringstream outStringStream;
  outStringStream << "// " << std::put_time(&tm, "%Y-%m-%d %T");
  outStringStream << "." << std::setfill('0') << std::setw(3) << now_ms.count();
  outStringStream << " object_id: " << object_id << " (0x";
  outStringStream << std::hex << object_id;
  outStringStream << ") rv: " << rv;

  return outStringStream.str();
}

void SaiTracer::checkAttrCount(uint32_t attr_count) {
  // If any object has more than the current sai_attribute list has (128 by
  // default), sai_attributes will be reallocated to have enough space for all
  // attributes
  if (attr_count > maxAttrCount_) {
    maxAttrCount_ = attr_count;
    writeToFile({to<string>(
        "sai_attributes = (sai_attribute_t*)realloc(sai_attributes, sizeof(sai_attribute_t) * ",
        maxAttrCount_,
        ")")});
  }
}

uint32_t SaiTracer::checkListCount(
    uint32_t list_count,
    uint32_t elem_size,
    uint32_t elem_count) {
  // If any object uses more than the current number of lists (6 by default),
  // sai replayer will initialize more lists on stack
  if (list_count > maxListCount_) {
    writeToFile({to<string>("int list_", maxListCount_, "[128]")});
    maxListCount_ = list_count;
  }

  // TODO(zecheng): Handle list size that's larger than 512 bytes.
  if (elem_size * elem_count > FLAGS_default_list_size * sizeof(int)) {
    writeToFile({to<string>(
        "printf(\"[ERROR] The replayed program is using more than ",
        FLAGS_default_list_size * sizeof(int),
        " bytes in a list attribute. Excess bytes will not be included.\")")});
  }

  // Return the maximum number of elements can be written into the list
  return FLAGS_default_list_size * sizeof(int) / elem_size;
}

void SaiTracer::setupGlobals() {
  // TODO(zecheng): Handle list size that's larger than 512 bytes.
  vector<string> globalVar = {to<string>(
      "sai_attribute_t *sai_attributes = (sai_attribute_t*)malloc(sizeof(sai_attribute_t) * ",
      FLAGS_default_list_size,
      ")")};

  for (int i = 0; i < FLAGS_default_list_count; i++) {
    globalVar.push_back(
        to<string>("int list_", i, "[", FLAGS_default_list_size, "]"));
  }

  globalVar.push_back("uint8_t* mac");
  globalVar.push_back("sai_status_t status");
  globalVar.push_back("sai_route_entry_t route_entry");
  globalVar.push_back("sai_neighbor_entry_t neighbor_entry");
  globalVar.push_back("sai_fdb_entry_t fdb_entry");
  globalVar.push_back("sai_inseg_entry_t inseg_entry");
  writeToFile(globalVar);

  maxAttrCount_ = FLAGS_default_list_size;
  maxListCount_ = FLAGS_default_list_count;
  numCalls_ = 0;
}

void SaiTracer::writeFooter() {
  string footer = "free(sai_attributes);\n}\n} // namespace facebook::fboss";

  if (flushToFile(saiLogFile_, footer.c_str(), footer.size()) < 0) {
    throw SysError(
        errno,
        "error writing ",
        footer.size(),
        " bytes to SAI Replayer log file");
  }
}

void SaiTracer::initVarCounts() {
  varCounts_.emplace(SAI_OBJECT_TYPE_ACL_ENTRY, 0);
  varCounts_.emplace(SAI_OBJECT_TYPE_ACL_TABLE, 0);
  varCounts_.emplace(SAI_OBJECT_TYPE_ACL_TABLE_GROUP, 0);
  varCounts_.emplace(SAI_OBJECT_TYPE_ACL_TABLE_GROUP_MEMBER, 0);
  varCounts_.emplace(SAI_OBJECT_TYPE_BRIDGE, 0);
  varCounts_.emplace(SAI_OBJECT_TYPE_BRIDGE_PORT, 0);
  varCounts_.emplace(SAI_OBJECT_TYPE_BUFFER_POOL, 0);
  varCounts_.emplace(SAI_OBJECT_TYPE_BUFFER_PROFILE, 0);
  varCounts_.emplace(SAI_OBJECT_TYPE_HASH, 0);
  varCounts_.emplace(SAI_OBJECT_TYPE_HOSTIF, 0);
  varCounts_.emplace(SAI_OBJECT_TYPE_HOSTIF_TRAP, 0);
  varCounts_.emplace(SAI_OBJECT_TYPE_HOSTIF_TRAP_GROUP, 0);
  varCounts_.emplace(SAI_OBJECT_TYPE_NEIGHBOR_ENTRY, 0);
  varCounts_.emplace(SAI_OBJECT_TYPE_NEXT_HOP, 0);
  varCounts_.emplace(SAI_OBJECT_TYPE_NEXT_HOP_GROUP, 0);
  varCounts_.emplace(SAI_OBJECT_TYPE_NEXT_HOP_GROUP_MEMBER, 0);
  varCounts_.emplace(SAI_OBJECT_TYPE_POLICER, 0);
  varCounts_.emplace(SAI_OBJECT_TYPE_PORT, 0);
  varCounts_.emplace(SAI_OBJECT_TYPE_QOS_MAP, 0);
  varCounts_.emplace(SAI_OBJECT_TYPE_QUEUE, 0);
  varCounts_.emplace(SAI_OBJECT_TYPE_ROUTER_INTERFACE, 0);
  varCounts_.emplace(SAI_OBJECT_TYPE_SCHEDULER, 0);
  varCounts_.emplace(SAI_OBJECT_TYPE_SCHEDULER_GROUP, 0);
  varCounts_.emplace(SAI_OBJECT_TYPE_SWITCH, 0);
  varCounts_.emplace(SAI_OBJECT_TYPE_UDF_GROUP, 0);
  varCounts_.emplace(SAI_OBJECT_TYPE_VIRTUAL_ROUTER, 0);
  varCounts_.emplace(SAI_OBJECT_TYPE_VLAN, 0);
  varCounts_.emplace(SAI_OBJECT_TYPE_VLAN_MEMBER, 0);
  varCounts_.emplace(SAI_OBJECT_TYPE_WRED, 0);
}

} // namespace facebook::fboss
