/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/sai/tracer/SwitchApiTracer.h"
#include "fboss/agent/hw/sai/api/SwitchApi.h"
#include "fboss/agent/hw/sai/tracer/Utils.h"

namespace facebook::fboss {

sai_status_t wrap_create_switch(
    sai_object_id_t* switch_id,
    uint32_t attr_count,
    const sai_attribute_t* attr_list) {
  auto rv = SaiTracer::getInstance()->switchApi_->create_switch(
      switch_id, attr_count, attr_list);

  SaiTracer::getInstance()->logSwitchCreateFn(
      switch_id, attr_count, attr_list, rv);
  return rv;
}

sai_status_t wrap_remove_switch(sai_object_id_t switch_id) {
  auto rv = SaiTracer::getInstance()->switchApi_->remove_switch(switch_id);

  SaiTracer::getInstance()->logRemoveFn(
      "remove_switch", switch_id, SAI_OBJECT_TYPE_SWITCH, rv);
  return rv;
}

sai_status_t wrap_set_switch_attribute(
    sai_object_id_t switch_id,
    const sai_attribute_t* attr) {
  sai_status_t rv{0};
  if (attr->id == SAI_SWITCH_ATTR_SWITCH_SHELL_ENABLE) {
    // this blocks forever, can't hold singleton or tracer must be leaky
    // singleton
    auto* tracer = SaiTracer::getInstance().get();
    rv = tracer->switchApi_->set_switch_attribute(switch_id, attr);
  } else {
    rv = SaiTracer::getInstance()->switchApi_->set_switch_attribute(
        switch_id, attr);
    SaiTracer::getInstance()->logSetAttrFn(
        "set_switch_attribute", switch_id, attr, SAI_OBJECT_TYPE_SWITCH, rv);
  }
  return rv;
}

sai_status_t wrap_get_switch_attribute(
    sai_object_id_t switch_id,
    uint32_t attr_count,
    sai_attribute_t* attr_list) {
  return SaiTracer::getInstance()->switchApi_->get_switch_attribute(
      switch_id, attr_count, attr_list);
}

sai_status_t wrap_get_switch_stats(
    sai_object_id_t switch_id,
    uint32_t number_of_counters,
    const sai_stat_id_t* counter_ids,
    uint64_t* counters) {
  return SaiTracer::getInstance()->switchApi_->get_switch_stats(
      switch_id, number_of_counters, counter_ids, counters);
}

sai_status_t wrap_get_switch_stats_ext(
    sai_object_id_t switch_id,
    uint32_t number_of_counters,
    const sai_stat_id_t* counter_ids,
    sai_stats_mode_t mode,
    uint64_t* counters) {
  return SaiTracer::getInstance()->switchApi_->get_switch_stats_ext(
      switch_id, number_of_counters, counter_ids, mode, counters);
}

sai_status_t wrap_clear_switch_stats(
    sai_object_id_t switch_id,
    uint32_t number_of_counters,
    const sai_stat_id_t* counter_ids) {
  return SaiTracer::getInstance()->switchApi_->clear_switch_stats(
      switch_id, number_of_counters, counter_ids);
}

sai_switch_api_t* wrappedSwitchApi() {
  static sai_switch_api_t switchWrappers;

  switchWrappers.create_switch = &wrap_create_switch;
  switchWrappers.remove_switch = &wrap_remove_switch;
  switchWrappers.set_switch_attribute = &wrap_set_switch_attribute;
  switchWrappers.get_switch_attribute = &wrap_get_switch_attribute;
  switchWrappers.get_switch_stats = &wrap_get_switch_stats;
  switchWrappers.get_switch_stats_ext = &wrap_get_switch_stats_ext;
  switchWrappers.clear_switch_stats = &wrap_clear_switch_stats;

  return &switchWrappers;
}

void setSwitchAttributes(
    const sai_attribute_t* attr_list,
    uint32_t attr_count,
    std::vector<std::string>& attrLines) {
  uint32_t listCount = 0;

  for (int i = 0; i < attr_count; ++i) {
    switch (attr_list[i].id) {
      case SAI_SWITCH_ATTR_CPU_PORT:
      case SAI_SWITCH_ATTR_DEFAULT_VIRTUAL_ROUTER_ID:
      case SAI_SWITCH_ATTR_DEFAULT_VLAN_ID:
      case SAI_SWITCH_ATTR_DEFAULT_1Q_BRIDGE_ID:
      case SAI_SWITCH_ATTR_ECMP_HASH:
      case SAI_SWITCH_ATTR_LAG_HASH:
      case SAI_SWITCH_ATTR_ECMP_HASH_IPV4:
      case SAI_SWITCH_ATTR_ECMP_HASH_IPV6:
      case SAI_SWITCH_ATTR_LAG_HASH_IPV4:
      case SAI_SWITCH_ATTR_LAG_HASH_IPV6:
      case SAI_SWITCH_ATTR_QOS_DSCP_TO_TC_MAP:
      case SAI_SWITCH_ATTR_QOS_TC_TO_QUEUE_MAP:
      case SAI_SWITCH_ATTR_INGRESS_ACL:
        attrLines.push_back(oidAttr(attr_list, i));
        break;
      case SAI_SWITCH_ATTR_PORT_LIST:
      case SAI_SWITCH_ATTR_TAM_OBJECT_ID:
      case SAI_SWITCH_ATTR_PORT_CONNECTOR_LIST:
      case SAI_SWITCH_ATTR_SYSTEM_PORT_CONFIG_LIST:
        oidListAttr(attr_list, i, listCount++, attrLines);
        break;
      case SAI_SWITCH_ATTR_SRC_MAC_ADDRESS:
        macAddressAttr(attr_list, i, attrLines);
        break;
      case SAI_SWITCH_ATTR_SWITCH_HARDWARE_INFO:
      case SAI_SWITCH_ATTR_FIRMWARE_PATH_NAME:
        s8ListAttr(attr_list, i, listCount++, attrLines, true);
        break;
      case SAI_SWITCH_ATTR_INIT_SWITCH:
      case SAI_SWITCH_ATTR_SWITCH_SHELL_ENABLE:
      case SAI_SWITCH_ATTR_RESTART_WARM:
      case SAI_SWITCH_ATTR_ECN_ECT_THRESHOLD_ENABLE:
      case SAI_SWITCH_ATTR_FIRMWARE_STATUS:
        attrLines.push_back(boolAttr(attr_list, i));
        break;
      case SAI_SWITCH_ATTR_PORT_NUMBER:
      case SAI_SWITCH_ATTR_NUMBER_OF_UNICAST_QUEUES:
      case SAI_SWITCH_ATTR_NUMBER_OF_MULTICAST_QUEUES:
      case SAI_SWITCH_ATTR_NUMBER_OF_QUEUES:
      case SAI_SWITCH_ATTR_NUMBER_OF_CPU_QUEUES:
      case SAI_SWITCH_ATTR_MAX_NUMBER_OF_SUPPORTED_PORTS:
      case SAI_SWITCH_ATTR_ECMP_DEFAULT_HASH_SEED:
      case SAI_SWITCH_ATTR_LAG_DEFAULT_HASH_SEED:
      case SAI_SWITCH_ATTR_ACL_ENTRY_MAXIMUM_PRIORITY:
      case SAI_SWITCH_ATTR_ACL_ENTRY_MINIMUM_PRIORITY:
      case SAI_SWITCH_ATTR_AVAILABLE_IPV4_NEIGHBOR_ENTRY:
      case SAI_SWITCH_ATTR_AVAILABLE_IPV4_NEXTHOP_ENTRY:
      case SAI_SWITCH_ATTR_AVAILABLE_IPV4_ROUTE_ENTRY:
      case SAI_SWITCH_ATTR_AVAILABLE_IPV6_NEIGHBOR_ENTRY:
      case SAI_SWITCH_ATTR_AVAILABLE_IPV6_NEXTHOP_ENTRY:
      case SAI_SWITCH_ATTR_AVAILABLE_IPV6_ROUTE_ENTRY:
      case SAI_SWITCH_ATTR_AVAILABLE_NEXT_HOP_GROUP_ENTRY:
      case SAI_SWITCH_ATTR_AVAILABLE_NEXT_HOP_GROUP_MEMBER_ENTRY:
      case SAI_SWITCH_ATTR_COUNTER_REFRESH_INTERVAL:
      case SAI_SWITCH_ATTR_FDB_AGING_TIME:
      case SAI_SWITCH_ATTR_FIRMWARE_MAJOR_VERSION:
      case SAI_SWITCH_ATTR_FIRMWARE_MINOR_VERSION:
      case SAI_SWITCH_ATTR_MAX_SYSTEM_CORES:
      case SAI_SWITCH_ATTR_SWITCH_ID:
      case SAI_SWITCH_ATTR_SWITCH_PROFILE_ID:
      case SAI_SWITCH_ATTR_TYPE:
        attrLines.push_back(u32Attr(attr_list, i));
        break;
      case SAI_SWITCH_ATTR_ECMP_DEFAULT_HASH_ALGORITHM:
      case SAI_SWITCH_ATTR_LAG_DEFAULT_HASH_ALGORITHM:
      case SAI_SWITCH_ATTR_FIRMWARE_LOAD_METHOD:
      case SAI_SWITCH_ATTR_FIRMWARE_LOAD_TYPE:
      case SAI_SWITCH_ATTR_HARDWARE_ACCESS_BUS:
        attrLines.push_back(s32Attr(attr_list, i));
        break;
      case SAI_SWITCH_ATTR_FDB_DST_USER_META_DATA_RANGE:
      case SAI_SWITCH_ATTR_NEIGHBOR_DST_USER_META_DATA_RANGE:
      case SAI_SWITCH_ATTR_ROUTE_DST_USER_META_DATA_RANGE:
        u32RangeAttr(attr_list, i, attrLines);
        break;
      case SAI_SWITCH_ATTR_PLATFROM_CONTEXT:
        attrLines.push_back(u64Attr(attr_list, i));
        break;
      default:
        // Handle extension attributes
        auto ledId = facebook::fboss::SaiSwitchTraits::Attributes::Led::
            optionalExtensionAttributeId();
        auto ledResetId = facebook::fboss::SaiSwitchTraits::Attributes::
            LedReset::optionalExtensionAttributeId();
        auto aclFieldListId = facebook::fboss::SaiSwitchTraits::Attributes::
            AclFieldList::optionalExtensionAttributeId();
        auto egressPoolAvailableSizeId =
            facebook::fboss::SaiSwitchTraits::Attributes::
                EgressPoolAvaialableSize::optionalExtensionAttributeId();
        auto hwECCErrorInitiateWrapperId = facebook::fboss::SaiSwitchTraits::
            Attributes::HwEccErrorInitiate::optionalExtensionAttributeId();
        if (ledId.has_value() && attr_list[i].id == ledId.value()) {
          u32ListAttr(attr_list, i, listCount++, attrLines);
        } else if (
            ledResetId.has_value() && attr_list[i].id == ledResetId.value()) {
          u32ListAttr(attr_list, i, listCount++, attrLines);
        } else if (
            aclFieldListId.has_value() &&
            attr_list[i].id == aclFieldListId.value()) {
          s32ListAttr(attr_list, i, listCount++, attrLines);
        } else if (
            egressPoolAvailableSizeId.has_value() &&
            attr_list[i].id == egressPoolAvailableSizeId.value()) {
          attrLines.push_back(u32Attr(attr_list, i));
        } else if (
            hwECCErrorInitiateWrapperId.has_value() &&
            attr_list[i].id == hwECCErrorInitiateWrapperId.value()) {
          attrLines.push_back(u16Attr(attr_list, i));
        }
        break;
    }
  }
}

} // namespace facebook::fboss
