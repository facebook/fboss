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
#include "fboss/agent/hw/sai/api/PortApi.h"
#include "fboss/agent/hw/sai/tracer/Utils.h"

namespace facebook::fboss {

WRAP_CREATE_FUNC(port, SAI_OBJECT_TYPE_PORT, port);
WRAP_REMOVE_FUNC(port, SAI_OBJECT_TYPE_PORT, port);
WRAP_SET_ATTR_FUNC(port, SAI_OBJECT_TYPE_PORT, port);
WRAP_GET_ATTR_FUNC(port, SAI_OBJECT_TYPE_PORT, port);

WRAP_CREATE_FUNC(port_serdes, SAI_OBJECT_TYPE_PORT_SERDES, port);
WRAP_REMOVE_FUNC(port_serdes, SAI_OBJECT_TYPE_PORT_SERDES, port);
WRAP_SET_ATTR_FUNC(port_serdes, SAI_OBJECT_TYPE_PORT_SERDES, port);
WRAP_GET_ATTR_FUNC(port_serdes, SAI_OBJECT_TYPE_PORT_SERDES, port);

WRAP_CREATE_FUNC(port_connector, SAI_OBJECT_TYPE_PORT_CONNECTOR, port);
WRAP_REMOVE_FUNC(port_connector, SAI_OBJECT_TYPE_PORT_CONNECTOR, port);
WRAP_SET_ATTR_FUNC(port_connector, SAI_OBJECT_TYPE_PORT_CONNECTOR, port);
WRAP_GET_ATTR_FUNC(port_connector, SAI_OBJECT_TYPE_PORT_CONNECTOR, port);

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
  portWrappers.create_port_connector = &wrap_create_port_connector;
  portWrappers.remove_port_connector = &wrap_remove_port_connector;
  portWrappers.set_port_connector_attribute =
      &wrap_set_port_connector_attribute;
  portWrappers.get_port_connector_attribute =
      &wrap_get_port_connector_attribute;
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
      case SAI_PORT_ATTR_PKT_TX_ENABLE:
#if SAI_API_VERSION >= SAI_VERSION(1, 7, 0)
      case SAI_PORT_ATTR_DISABLE_DECREMENT_TTL:
#else
      case SAI_PORT_ATTR_DECREMENT_TTL:
#endif
        attrLines.push_back(boolAttr(attr_list, i));
        break;
      case SAI_PORT_ATTR_HW_LANE_LIST:
      case SAI_PORT_ATTR_SERDES_PREEMPHASIS:
        u32ListAttr(attr_list, i, listCount++, attrLines);
        break;
      case SAI_PORT_ATTR_SPEED:
      case SAI_PORT_ATTR_QOS_NUMBER_OF_QUEUES:
      case SAI_PORT_ATTR_MTU:
      case SAI_PORT_ATTR_PRBS_POLYNOMIAL:
        attrLines.push_back(u32Attr(attr_list, i));
        break;
      case SAI_PORT_ATTR_QOS_QUEUE_LIST:
      case SAI_PORT_ATTR_EGRESS_MIRROR_SESSION:
      case SAI_PORT_ATTR_INGRESS_MIRROR_SESSION:
#if SAI_API_VERSION >= SAI_VERSION(1, 7, 0)
      case SAI_PORT_ATTR_EGRESS_SAMPLE_MIRROR_SESSION:
      case SAI_PORT_ATTR_INGRESS_SAMPLE_MIRROR_SESSION:
#endif
        oidListAttr(attr_list, i, listCount++, attrLines);
        break;
      case SAI_PORT_ATTR_TYPE:
      case SAI_PORT_ATTR_FEC_MODE:
      case SAI_PORT_ATTR_OPER_STATUS:
      case SAI_PORT_ATTR_INTERNAL_LOOPBACK_MODE:
      case SAI_PORT_ATTR_MEDIA_TYPE:
      case SAI_PORT_ATTR_INTERFACE_TYPE:
      case SAI_PORT_ATTR_GLOBAL_FLOW_CONTROL_MODE:
      case SAI_PORT_ATTR_PRBS_CONFIG:
      case SAI_PORT_ATTR_PTP_MODE:
        attrLines.push_back(s32Attr(attr_list, i));
        break;
      case SAI_PORT_ATTR_PORT_VLAN_ID:
        attrLines.push_back(u16Attr(attr_list, i));
        break;
      case SAI_PORT_ATTR_QOS_DSCP_TO_TC_MAP:
      case SAI_PORT_ATTR_QOS_TC_TO_QUEUE_MAP:
      case SAI_PORT_ATTR_PORT_SERDES_ID:
      case SAI_PORT_ATTR_EGRESS_MACSEC_ACL:
      case SAI_PORT_ATTR_EGRESS_SAMPLEPACKET_ENABLE:
      case SAI_PORT_ATTR_INGRESS_MACSEC_ACL:
      case SAI_PORT_ATTR_INGRESS_SAMPLEPACKET_ENABLE:
        attrLines.push_back(oidAttr(attr_list, i));
        break;
      default:
        // Handle extension attributes
        auto systemPortId = facebook::fboss::SaiPortTraits::Attributes::
            SystemPortId::optionalExtensionAttributeId();
        if (systemPortId.has_value() &&
            attr_list[i].id == systemPortId.value()) {
          attrLines.push_back(u16Attr(attr_list, i));
        }
        break;
    }
  }
}

void setPortSerdesAttributes(
    const sai_attribute_t* attr_list,
    uint32_t attr_count,
    std::vector<std::string>& attrLines) {
  uint32_t listCount = 0;
  std::unordered_set<sai_attr_id_t> s32ExtensionAttr;
  auto rxCtleCodeId = facebook::fboss::SaiPortSerdesTraits::Attributes::
      RxCtleCode::optionalExtensionAttributeId();
  auto rxDspModeId = facebook::fboss::SaiPortSerdesTraits::Attributes::
      RxDspMode::optionalExtensionAttributeId();
  auto rxAfeTrimId = facebook::fboss::SaiPortSerdesTraits::Attributes::
      RxAfeTrim::optionalExtensionAttributeId();
  auto rxAcCouplingByPassId = facebook::fboss::SaiPortSerdesTraits::Attributes::
      RxAcCouplingByPass::optionalExtensionAttributeId();
  auto rxAfeAdaptiveEnableId = facebook::fboss::SaiPortSerdesTraits::
      Attributes::RxAfeAdaptiveEnable::optionalExtensionAttributeId();
  if (rxCtleCodeId.has_value()) {
    s32ExtensionAttr.insert(rxCtleCodeId.value());
  }
  if (rxDspModeId.has_value()) {
    s32ExtensionAttr.insert(rxDspModeId.value());
  }
  if (rxAfeTrimId.has_value()) {
    s32ExtensionAttr.insert(rxAfeTrimId.value());
  }
  if (rxAcCouplingByPassId.has_value()) {
    s32ExtensionAttr.insert(rxAcCouplingByPassId.value());
  }
  if (rxAfeAdaptiveEnableId.has_value()) {
    s32ExtensionAttr.insert(rxAfeAdaptiveEnableId.value());
  }

  for (int i = 0; i < attr_count; ++i) {
    switch (attr_list[i].id) {
      case SAI_PORT_SERDES_ATTR_PORT_ID:
        attrLines.push_back(oidAttr(attr_list, i));
        break;
      case SAI_PORT_SERDES_ATTR_PREEMPHASIS:
      case SAI_PORT_SERDES_ATTR_IDRIVER:
      case SAI_PORT_SERDES_ATTR_TX_FIR_PRE1:
      case SAI_PORT_SERDES_ATTR_TX_FIR_PRE2:
      case SAI_PORT_SERDES_ATTR_TX_FIR_MAIN:
      case SAI_PORT_SERDES_ATTR_TX_FIR_POST1:
      case SAI_PORT_SERDES_ATTR_TX_FIR_POST2:
      case SAI_PORT_SERDES_ATTR_TX_FIR_POST3:
        u32ListAttr(attr_list, i, listCount++, attrLines);
        break;
      default:
        // Handle extension attributes
        if (s32ExtensionAttr.find(attr_list[i].id) != s32ExtensionAttr.end()) {
          s32ListAttr(attr_list, i, listCount++, attrLines);
        }
        break;
    }
  }
}

void setPortConnectorAttributes(
    const sai_attribute_t* attr_list,
    uint32_t attr_count,
    std::vector<std::string>& attrLines) {
  for (int i = 0; i < attr_count; ++i) {
    switch (attr_list[i].id) {
      case SAI_PORT_CONNECTOR_ATTR_LINE_SIDE_PORT_ID:
      case SAI_PORT_CONNECTOR_ATTR_SYSTEM_SIDE_PORT_ID:
        attrLines.push_back(oidAttr(attr_list, i));
        break;
      default:
        break;
    }
  }
}

} // namespace facebook::fboss
