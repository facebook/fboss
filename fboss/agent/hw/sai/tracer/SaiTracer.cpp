/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include <tuple>

#include "fboss/agent/SysError.h"
#include "fboss/agent/hw/sai/tracer/AclApiTracer.h"
#include "fboss/agent/hw/sai/tracer/BridgeApiTracer.h"
#include "fboss/agent/hw/sai/tracer/PortApiTracer.h"
#include "fboss/agent/hw/sai/tracer/SaiTracer.h"
#include "fboss/agent/hw/sai/tracer/VlanApiTracer.h"

#include <folly/FileUtil.h>
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
    case (SAI_API_ACL):
      SaiTracer::getInstance()->aclApi_ =
          static_cast<sai_acl_api_t*>(*api_method_table);
      *api_method_table = facebook::fboss::wrapAclApi();
      break;
    case (SAI_API_BRIDGE):
      SaiTracer::getInstance()->bridgeApi_ =
          static_cast<sai_bridge_api_t*>(*api_method_table);
      *api_method_table = facebook::fboss::wrapBridgeApi();
      break;
    case (SAI_API_PORT):
      SaiTracer::getInstance()->portApi_ =
          static_cast<sai_port_api_t*>(*api_method_table);
      *api_method_table = facebook::fboss::wrapPortApi();
      break;
    case (SAI_API_VLAN):
      SaiTracer::getInstance()->vlanApi_ =
          static_cast<sai_vlan_api_t*>(*api_method_table);
      *api_method_table = facebook::fboss::wrapVlanApi();
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

}

namespace facebook::fboss {

SaiTracer::SaiTracer() {
  saiLogFile_ = folly::File(FLAGS_sai_log.c_str(), O_RDWR | O_CREAT | O_TRUNC);
  setupGlobals();
  initVarCounts();
}

SaiTracer::~SaiTracer() {
  fsync(saiLogFile_.wlock()->fd());
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

  auto bytesWritten = saiLogFile_.withWLock([&](auto& lockedFile) {
    return folly::writeFull(lockedFile.fd(), lines.c_str(), lines.size());
  });
  if (bytesWritten < 0) {
    throw SysError(
        errno,
        "error writing ",
        lines.size(),
        " bytes to SAI Replayer log file");
  }
}

void SaiTracer::logCreateFn(
    const string& fn_name,
    sai_object_id_t* create_object_id,
    sai_object_id_t switch_id,
    uint32_t attr_count,
    const sai_attribute_t* attr_list,
    sai_object_type_t object_type) {
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

  // Make the function call & write to file
  lines.push_back(createFnCall(
      fn_name,
      varName,
      getVariable(switch_id, SAI_OBJECT_TYPE_SWITCH),
      attr_count,
      object_type));

  writeToFile(lines);
}

void SaiTracer::logRemoveFn(
    const string& fn_name,
    sai_object_id_t remove_object_id,
    sai_object_type_t object_type) {
  if (!FLAGS_enable_replayer) {
    return;
  }

  // Directly make remove call
  writeToFile({to<string>(
      folly::get_or_throw(
          fnPrefix_, object_type, "Unsupported Sai Object type in Sai Tracer"),
      fn_name,
      "(",
      getVariable(remove_object_id, object_type),
      ")")});
}

void SaiTracer::logSetAttrFn(
    const string& fn_name,
    sai_object_id_t set_object_id,
    const sai_attribute_t* attr,
    sai_object_type_t object_type) {
  if (!FLAGS_enable_replayer) {
    return;
  }

  // Setup one attribute
  vector<string> lines = setAttrList(attr, 1, object_type);

  // Make setAttribute call
  lines.push_back(to<string>(
      folly::get_or_throw(
          fnPrefix_, object_type, "Unsupported Sai Object type in Sai Tracer"),
      fn_name,
      "(",
      getVariable(set_object_id, object_type),
      ", sai_attributes)"));
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
    sai_object_type_t object_type) {
  if (!FLAGS_enable_replayer) {
    return "";
  }

  // Check variable map to get object_id -> variable name mapping
  return folly::get_or_throw(
             variables_,
             object_type,
             "Unsupported Sai Object type in Sai Tracer")
      .withRLock([&](auto& vars) {
        return folly::get_default(vars, object_id, to<string>(object_id));
      });
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
    case SAI_OBJECT_TYPE_PORT:
      setPortAttributes(attr_list, attr_count, attrLines);
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
      "sai_attribute_t *sai_attributes = malloc(sizeof(sai_attribute_t) * ",
      FLAGS_default_list_size,
      ")")};
  for (int i = 0; i < FLAGS_default_list_count; i++) {
    globalVar.push_back(
        to<string>("int list_", i, "[", FLAGS_default_list_size, "]"));
  }
  writeToFile(globalVar);

  maxAttrCount_ = FLAGS_default_list_size;
  maxListCount_ = FLAGS_default_list_count;
}

void SaiTracer::initVarCounts() {
  varCounts_.emplace(SAI_OBJECT_TYPE_ACL_ENTRY, 0);
  varCounts_.emplace(SAI_OBJECT_TYPE_ACL_TABLE, 0);
  varCounts_.emplace(SAI_OBJECT_TYPE_ACL_TABLE_GROUP, 0);
  varCounts_.emplace(SAI_OBJECT_TYPE_ACL_TABLE_GROUP_MEMBER, 0);
  varCounts_.emplace(SAI_OBJECT_TYPE_BRIDGE, 0);
  varCounts_.emplace(SAI_OBJECT_TYPE_BRIDGE_PORT, 0);
  varCounts_.emplace(SAI_OBJECT_TYPE_PORT, 0);
  varCounts_.emplace(SAI_OBJECT_TYPE_QOS_MAP, 0);
  varCounts_.emplace(SAI_OBJECT_TYPE_QUEUE, 0);
  varCounts_.emplace(SAI_OBJECT_TYPE_SWITCH, 0);
  varCounts_.emplace(SAI_OBJECT_TYPE_VLAN, 0);
  varCounts_.emplace(SAI_OBJECT_TYPE_VLAN_MEMBER, 0);
}

} // namespace facebook::fboss
