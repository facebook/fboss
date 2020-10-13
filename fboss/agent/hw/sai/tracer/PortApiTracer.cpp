/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/sai/tracer/PortApiTracer.h"
#include "fboss/agent/hw/sai/tracer/Utils.h"

namespace facebook::fboss {

sai_status_t wrap_create_port(
    sai_object_id_t* port_id,
    sai_object_id_t switch_id,
    uint32_t attr_count,
    const sai_attribute_t* attr_list) {
  auto rv = SaiTracer::getInstance()->portApi_->create_port(
      port_id, switch_id, attr_count, attr_list);

  SaiTracer::getInstance()->logCreateFn(
      "create_port",
      port_id,
      switch_id,
      attr_count,
      attr_list,
      SAI_OBJECT_TYPE_PORT,
      rv);
  return rv;
}

sai_status_t wrap_remove_port(sai_object_id_t port_id) {
  auto rv = SaiTracer::getInstance()->portApi_->remove_port(port_id);

  SaiTracer::getInstance()->logRemoveFn(
      "remove_port", port_id, SAI_OBJECT_TYPE_PORT, rv);
  return rv;
}

sai_status_t wrap_set_port_attribute(
    sai_object_id_t port_id,
    const sai_attribute_t* attr) {
  auto rv =
      SaiTracer::getInstance()->portApi_->set_port_attribute(port_id, attr);

  SaiTracer::getInstance()->logSetAttrFn(
      "set_port_attribute", port_id, attr, SAI_OBJECT_TYPE_PORT, rv);
  return rv;
}

sai_status_t wrap_get_port_attribute(
    sai_object_id_t port_id,
    uint32_t attr_count,
    sai_attribute_t* attr_list) {
  return SaiTracer::getInstance()->portApi_->get_port_attribute(
      port_id, attr_count, attr_list);
}

sai_status_t wrap_get_port_stats(
    sai_object_id_t port_id,
    uint32_t number_of_counters,
    const sai_stat_id_t* counter_ids,
    uint64_t* counters) {
  return SaiTracer::getInstance()->portApi_->get_port_stats(
      port_id, number_of_counters, counter_ids, counters);
}

sai_status_t wrap_get_port_stats_ext(
    sai_object_id_t port_id,
    uint32_t number_of_counters,
    const sai_stat_id_t* counter_ids,
    sai_stats_mode_t mode,
    uint64_t* counters) {
  return SaiTracer::getInstance()->portApi_->get_port_stats_ext(
      port_id, number_of_counters, counter_ids, mode, counters);
}

sai_status_t wrap_clear_port_stats(
    sai_object_id_t port_id,
    uint32_t number_of_counters,
    const sai_stat_id_t* counter_ids) {
  return SaiTracer::getInstance()->portApi_->clear_port_stats(
      port_id, number_of_counters, counter_ids);
}

sai_status_t wrap_clear_port_all_stats(sai_object_id_t port_id) {
  return SaiTracer::getInstance()->portApi_->clear_port_all_stats(port_id);
}

sai_status_t wrap_create_port_pool(
    sai_object_id_t* port_pool_id,
    sai_object_id_t switch_id,
    uint32_t attr_count,
    const sai_attribute_t* attr_list) {
  return SaiTracer::getInstance()->portApi_->create_port_pool(
      port_pool_id, switch_id, attr_count, attr_list);
}

sai_status_t wrap_remove_port_pool(sai_object_id_t port_pool_id) {
  return SaiTracer::getInstance()->portApi_->remove_port_pool(port_pool_id);
}

sai_status_t wrap_set_port_pool_attribute(
    sai_object_id_t port_pool_id,
    const sai_attribute_t* attr) {
  return SaiTracer::getInstance()->portApi_->set_port_pool_attribute(
      port_pool_id, attr);
}

sai_status_t wrap_get_port_pool_attribute(
    sai_object_id_t port_pool_id,
    uint32_t attr_count,
    sai_attribute_t* attr_list) {
  return SaiTracer::getInstance()->portApi_->get_port_pool_attribute(
      port_pool_id, attr_count, attr_list);
}

sai_status_t wrap_get_port_pool_stats(
    sai_object_id_t port_pool_id,
    uint32_t number_of_counters,
    const sai_stat_id_t* counter_ids,
    uint64_t* counters) {
  return SaiTracer::getInstance()->portApi_->get_port_pool_stats(
      port_pool_id, number_of_counters, counter_ids, counters);
}

sai_status_t wrap_get_port_pool_stats_ext(
    sai_object_id_t port_pool_id,
    uint32_t number_of_counters,
    const sai_stat_id_t* counter_ids,
    sai_stats_mode_t mode,
    uint64_t* counters) {
  return SaiTracer::getInstance()->portApi_->get_port_pool_stats_ext(
      port_pool_id, number_of_counters, counter_ids, mode, counters);
}

sai_status_t wrap_clear_port_pool_stats(
    sai_object_id_t port_pool_id,
    uint32_t number_of_counters,
    const sai_stat_id_t* counter_ids) {
  return SaiTracer::getInstance()->portApi_->clear_port_pool_stats(
      port_pool_id, number_of_counters, counter_ids);
}

sai_status_t wrap_create_port_serdes(
    sai_object_id_t* port_serdes_id,
    sai_object_id_t switch_id,
    uint32_t attr_count,
    const sai_attribute_t* attr_list) {
  auto rv = SaiTracer::getInstance()->portApi_->create_port_serdes(
      port_serdes_id, switch_id, attr_count, attr_list);
  SaiTracer::getInstance()->logCreateFn(
      "create_port_serdes",
      port_serdes_id,
      switch_id,
      attr_count,
      attr_list,
      SAI_OBJECT_TYPE_PORT_SERDES,
      rv);
  return rv;
}

sai_status_t wrap_remove_port_serdes(sai_object_id_t port_serdes_id) {
  auto rv =
      SaiTracer::getInstance()->portApi_->remove_port_serdes(port_serdes_id);
  SaiTracer::getInstance()->logRemoveFn(
      "remove_port_serdes", port_serdes_id, SAI_OBJECT_TYPE_PORT_SERDES, rv);
  return rv;
}

sai_status_t wrap_set_port_serdes_attribute(
    sai_object_id_t port_serdes_id,
    const sai_attribute_t* attr) {
  auto rv = SaiTracer::getInstance()->portApi_->set_port_serdes_attribute(
      port_serdes_id, attr);
  SaiTracer::getInstance()->logSetAttrFn(
      "set_port_serdes_attribute",
      port_serdes_id,
      attr,
      SAI_OBJECT_TYPE_PORT_SERDES,
      rv);
  return rv;
}

sai_status_t wrap_get_port_serdes_attribute(
    sai_object_id_t port_serdes_id,
    uint32_t attr_count,
    sai_attribute_t* attr_list) {
  // TODO: log get port serdes attribute
  return SaiTracer::getInstance()->portApi_->get_port_serdes_attribute(
      port_serdes_id, attr_count, attr_list);
}

sai_port_api_t* wrappedPortApi() {
  static sai_port_api_t portWrappers;

  portWrappers.create_port = &wrap_create_port;
  portWrappers.remove_port = &wrap_remove_port;
  portWrappers.set_port_attribute = &wrap_set_port_attribute;
  portWrappers.get_port_attribute = &wrap_get_port_attribute;
  portWrappers.get_port_stats = &wrap_get_port_stats;
  portWrappers.get_port_stats_ext = &wrap_get_port_stats_ext;
  portWrappers.clear_port_stats = &wrap_clear_port_stats;
  portWrappers.clear_port_all_stats = &wrap_clear_port_all_stats;
  portWrappers.create_port_serdes = &wrap_create_port_serdes;
  portWrappers.remove_port_serdes = &wrap_remove_port_serdes;
  portWrappers.set_port_serdes_attribute = &wrap_set_port_serdes_attribute;
  portWrappers.get_port_serdes_attribute = &wrap_get_port_serdes_attribute;
  return &portWrappers;
}

void setPortAttributes(
    const sai_attribute_t* attr_list,
    uint32_t attr_count,
    std::vector<std::string>& attrLines) {
  uint32_t listCount = 0;

  for (int i = 0; i < attr_count; ++i) {
    switch (attr_list[i].id) {
      case SAI_PORT_ATTR_ADMIN_STATE:
        attrLines.push_back(boolAttr(attr_list, i));
        break;
      case SAI_PORT_ATTR_HW_LANE_LIST:
      case SAI_PORT_ATTR_SERDES_PREEMPHASIS:
        u32ListAttr(attr_list, i, listCount++, attrLines);
        break;
      case SAI_PORT_ATTR_SPEED:
      case SAI_PORT_ATTR_QOS_NUMBER_OF_QUEUES:
      case SAI_PORT_ATTR_MTU:
        attrLines.push_back(u32Attr(attr_list, i));
        break;
      case SAI_PORT_ATTR_QOS_QUEUE_LIST:
        oidListAttr(attr_list, i, listCount++, attrLines);
        break;
      case SAI_PORT_ATTR_TYPE:
      case SAI_PORT_ATTR_FEC_MODE:
      case SAI_PORT_ATTR_OPER_STATUS:
      case SAI_PORT_ATTR_INTERNAL_LOOPBACK_MODE:
      case SAI_PORT_ATTR_MEDIA_TYPE:
      case SAI_PORT_ATTR_GLOBAL_FLOW_CONTROL_MODE:
        attrLines.push_back(s32Attr(attr_list, i));
        break;
      case SAI_PORT_ATTR_PORT_VLAN_ID:
        attrLines.push_back(u16Attr(attr_list, i));
        break;
      case SAI_PORT_ATTR_QOS_DSCP_TO_TC_MAP:
      case SAI_PORT_ATTR_QOS_TC_TO_QUEUE_MAP:
        attrLines.push_back(oidAttr(attr_list, i));
        break;
      default:
        // TODO(zecheng): Better check for newly added attributes (T69350100)
        break;
    }
  }
}

} // namespace facebook::fboss
