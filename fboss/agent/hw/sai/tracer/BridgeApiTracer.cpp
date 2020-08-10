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

sai_status_t wrap_create_bridge(
    sai_object_id_t* bridge_id,
    sai_object_id_t switch_id,
    uint32_t attr_count,
    const sai_attribute_t* attr_list) {
  auto rv = SaiTracer::getInstance()->bridgeApi_->create_bridge(
      bridge_id, switch_id, attr_count, attr_list);

  SaiTracer::getInstance()->logCreateFn(
      "create_bridge",
      bridge_id,
      switch_id,
      attr_count,
      attr_list,
      SAI_OBJECT_TYPE_BRIDGE,
      rv);
  return rv;
}

sai_status_t wrap_remove_bridge(sai_object_id_t bridge_id) {
  auto rv = SaiTracer::getInstance()->bridgeApi_->remove_bridge(bridge_id);

  SaiTracer::getInstance()->logRemoveFn(
      "remove_bridge", bridge_id, SAI_OBJECT_TYPE_BRIDGE, rv);
  return rv;
}

sai_status_t wrap_set_bridge_attribute(
    sai_object_id_t bridge_id,
    const sai_attribute_t* attr) {
  auto rv = SaiTracer::getInstance()->bridgeApi_->set_bridge_attribute(
      bridge_id, attr);

  SaiTracer::getInstance()->logSetAttrFn(
      "set_bridge_attribute", bridge_id, attr, SAI_OBJECT_TYPE_BRIDGE, rv);
  return rv;
}

sai_status_t wrap_get_bridge_attribute(
    sai_object_id_t bridge_id,
    uint32_t attr_count,
    sai_attribute_t* attr_list) {
  return SaiTracer::getInstance()->bridgeApi_->get_bridge_attribute(
      bridge_id, attr_count, attr_list);
}

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

sai_status_t wrap_create_bridge_port(
    sai_object_id_t* bridge_port_id,
    sai_object_id_t switch_id,
    uint32_t attr_count,
    const sai_attribute_t* attr_list) {
  auto rv = SaiTracer::getInstance()->bridgeApi_->create_bridge_port(
      bridge_port_id, switch_id, attr_count, attr_list);

  SaiTracer::getInstance()->logCreateFn(
      "create_bridge_port",
      bridge_port_id,
      switch_id,
      attr_count,
      attr_list,
      SAI_OBJECT_TYPE_BRIDGE_PORT,
      rv);
  return rv;
}

sai_status_t wrap_remove_bridge_port(sai_object_id_t bridge_port_id) {
  auto rv =
      SaiTracer::getInstance()->bridgeApi_->remove_bridge_port(bridge_port_id);

  SaiTracer::getInstance()->logRemoveFn(
      "remove_bridge_port", bridge_port_id, SAI_OBJECT_TYPE_BRIDGE_PORT, rv);
  return rv;
}

sai_status_t wrap_set_bridge_port_attribute(
    sai_object_id_t bridge_port_id,
    const sai_attribute_t* attr) {
  auto rv = SaiTracer::getInstance()->bridgeApi_->set_bridge_port_attribute(
      bridge_port_id, attr);

  SaiTracer::getInstance()->logSetAttrFn(
      "set_bridge_port_attribute",
      bridge_port_id,
      attr,
      SAI_OBJECT_TYPE_BRIDGE_PORT,
      rv);
  return rv;
}

sai_status_t wrap_get_bridge_port_attribute(
    sai_object_id_t bridge_port_id,
    uint32_t attr_count,
    sai_attribute_t* attr_list) {
  return SaiTracer::getInstance()->bridgeApi_->get_bridge_port_attribute(
      bridge_port_id, attr_count, attr_list);
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
        // TODO(zecheng): Better check for newly added attributes (T69350100)
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
        // TODO(zecheng): Better check for newly added attributes (T69350100)
        break;
    }
  }
}

} // namespace facebook::fboss
