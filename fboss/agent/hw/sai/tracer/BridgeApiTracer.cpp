/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/sai/tracer/BridgeApiTracer.h"
#include "fboss/agent/hw/sai/tracer/Utils.h"

namespace facebook::fboss {

WRAP_CREATE_FUNC(bridge, SAI_OBJECT_TYPE_BRIDGE, bridge);
WRAP_REMOVE_FUNC(bridge, SAI_OBJECT_TYPE_BRIDGE, bridge);
WRAP_SET_ATTR_FUNC(bridge, SAI_OBJECT_TYPE_BRIDGE, bridge);
WRAP_GET_ATTR_FUNC(bridge, SAI_OBJECT_TYPE_BRIDGE, bridge);

WRAP_CREATE_FUNC(bridge_port, SAI_OBJECT_TYPE_BRIDGE_PORT, bridge);
WRAP_REMOVE_FUNC(bridge_port, SAI_OBJECT_TYPE_BRIDGE_PORT, bridge);
WRAP_SET_ATTR_FUNC(bridge_port, SAI_OBJECT_TYPE_BRIDGE_PORT, bridge);
WRAP_GET_ATTR_FUNC(bridge_port, SAI_OBJECT_TYPE_BRIDGE_PORT, bridge);

sai_status_t wrap_get_bridge_stats(
    sai_object_id_t bridge_id,
    uint32_t number_of_counters,
    const sai_stat_id_t* counter_ids,
    uint64_t* counters) {
  return SaiTracer::getInstance()->bridgeApi_->get_bridge_stats(
      bridge_id, number_of_counters, counter_ids, counters);
}

sai_status_t wrap_get_bridge_stats_ext(
    sai_object_id_t bridge_id,
    uint32_t number_of_counters,
    const sai_stat_id_t* counter_ids,
    sai_stats_mode_t mode,
    uint64_t* counters) {
  return SaiTracer::getInstance()->bridgeApi_->get_bridge_stats_ext(
      bridge_id, number_of_counters, counter_ids, mode, counters);
}

sai_status_t wrap_clear_bridge_stats(
    sai_object_id_t bridge_id,
    uint32_t number_of_counters,
    const sai_stat_id_t* counter_ids) {
  return SaiTracer::getInstance()->bridgeApi_->clear_bridge_stats(
      bridge_id, number_of_counters, counter_ids);
}

sai_status_t wrap_get_bridge_port_stats(
    sai_object_id_t bridge_port_id,
    uint32_t number_of_counters,
    const sai_stat_id_t* counter_ids,
    uint64_t* counters) {
  return SaiTracer::getInstance()->bridgeApi_->get_bridge_port_stats(
      bridge_port_id, number_of_counters, counter_ids, counters);
}

sai_status_t wrap_get_bridge_port_stats_ext(
    sai_object_id_t bridge_port_id,
    uint32_t number_of_counters,
    const sai_stat_id_t* counter_ids,
    sai_stats_mode_t mode,
    uint64_t* counters) {
  return SaiTracer::getInstance()->bridgeApi_->get_bridge_port_stats_ext(
      bridge_port_id, number_of_counters, counter_ids, mode, counters);
}

sai_status_t wrap_clear_bridge_port_stats(
    sai_object_id_t bridge_port_id,
    uint32_t number_of_counters,
    const sai_stat_id_t* counter_ids) {
  return SaiTracer::getInstance()->bridgeApi_->clear_bridge_port_stats(
      bridge_port_id, number_of_counters, counter_ids);
}

sai_bridge_api_t* wrappedBridgeApi() {
  static sai_bridge_api_t bridgeWrappers;

  bridgeWrappers.create_bridge = &wrap_create_bridge;
  bridgeWrappers.remove_bridge = &wrap_remove_bridge;
  bridgeWrappers.set_bridge_attribute = &wrap_set_bridge_attribute;
  bridgeWrappers.get_bridge_attribute = &wrap_get_bridge_attribute;
  bridgeWrappers.get_bridge_stats = &wrap_get_bridge_stats;
  bridgeWrappers.get_bridge_stats_ext = &wrap_get_bridge_stats_ext;
  bridgeWrappers.clear_bridge_stats = &wrap_clear_bridge_stats;
  bridgeWrappers.create_bridge_port = &wrap_create_bridge_port;
  bridgeWrappers.remove_bridge_port = &wrap_remove_bridge_port;
  bridgeWrappers.set_bridge_port_attribute = &wrap_set_bridge_port_attribute;
  bridgeWrappers.get_bridge_port_attribute = &wrap_get_bridge_port_attribute;
  bridgeWrappers.get_bridge_port_stats = &wrap_get_bridge_port_stats;
  bridgeWrappers.get_bridge_port_stats_ext = &wrap_get_bridge_port_stats_ext;
  bridgeWrappers.clear_bridge_port_stats = &wrap_clear_bridge_port_stats;

  return &bridgeWrappers;
}

void setBridgePortAttributes(
    const sai_attribute_t* attr_list,
    uint32_t attr_count,
    std::vector<std::string>& attrLines) {
  for (int i = 0; i < attr_count; ++i) {
    switch (attr_list[i].id) {
      case SAI_BRIDGE_PORT_ATTR_BRIDGE_ID:
      case SAI_BRIDGE_PORT_ATTR_PORT_ID:
        attrLines.push_back(oidAttr(attr_list, i));
        break;
      case SAI_BRIDGE_PORT_ATTR_TYPE:
      case SAI_BRIDGE_PORT_ATTR_FDB_LEARNING_MODE:
        attrLines.push_back(s32Attr(attr_list, i));
        break;
      case SAI_BRIDGE_PORT_ATTR_ADMIN_STATE:
        attrLines.push_back(boolAttr(attr_list, i));
        break;
      default:
        break;
    }
  }
}

void setBridgeAttributes(
    const sai_attribute_t* attr_list,
    uint32_t attr_count,
    std::vector<std::string>& attrLines) {
  uint32_t listCount = 0;

  for (int i = 0; i < attr_count; ++i) {
    switch (attr_list[i].id) {
      case SAI_BRIDGE_ATTR_PORT_LIST:
        oidListAttr(attr_list, i, listCount++, attrLines);
        break;
      case SAI_BRIDGE_ATTR_TYPE:
        attrLines.push_back(s32Attr(attr_list, i));
        break;
      default:
        break;
    }
  }
}

} // namespace facebook::fboss
