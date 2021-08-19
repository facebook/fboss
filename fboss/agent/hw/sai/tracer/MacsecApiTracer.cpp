/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/sai/tracer/MacsecApiTracer.h"
#include "fboss/agent/hw/sai/tracer/Utils.h"

namespace facebook::fboss {

WRAP_CREATE_FUNC(macsec, SAI_OBJECT_TYPE_MACSEC, macsec);
WRAP_REMOVE_FUNC(macsec, SAI_OBJECT_TYPE_MACSEC, macsec);
WRAP_SET_ATTR_FUNC(macsec, SAI_OBJECT_TYPE_MACSEC, macsec);
WRAP_GET_ATTR_FUNC(macsec, SAI_OBJECT_TYPE_MACSEC, macsec);

WRAP_CREATE_FUNC(macsec_port, SAI_OBJECT_TYPE_MACSEC_PORT, macsec);
WRAP_REMOVE_FUNC(macsec_port, SAI_OBJECT_TYPE_MACSEC_PORT, macsec);
WRAP_SET_ATTR_FUNC(macsec_port, SAI_OBJECT_TYPE_MACSEC_PORT, macsec);
WRAP_GET_ATTR_FUNC(macsec_port, SAI_OBJECT_TYPE_MACSEC_PORT, macsec);

WRAP_CREATE_FUNC(macsec_sa, SAI_OBJECT_TYPE_MACSEC_SA, macsec);
WRAP_REMOVE_FUNC(macsec_sa, SAI_OBJECT_TYPE_MACSEC_SA, macsec);
WRAP_SET_ATTR_FUNC(macsec_sa, SAI_OBJECT_TYPE_MACSEC_SA, macsec);
WRAP_GET_ATTR_FUNC(macsec_sa, SAI_OBJECT_TYPE_MACSEC_SA, macsec);

WRAP_CREATE_FUNC(macsec_sc, SAI_OBJECT_TYPE_MACSEC_SC, macsec);
WRAP_REMOVE_FUNC(macsec_sc, SAI_OBJECT_TYPE_MACSEC_SC, macsec);
WRAP_SET_ATTR_FUNC(macsec_sc, SAI_OBJECT_TYPE_MACSEC_SC, macsec);
WRAP_GET_ATTR_FUNC(macsec_sc, SAI_OBJECT_TYPE_MACSEC_SC, macsec);

WRAP_CREATE_FUNC(macsec_flow, SAI_OBJECT_TYPE_MACSEC_FLOW, macsec);
WRAP_REMOVE_FUNC(macsec_flow, SAI_OBJECT_TYPE_MACSEC_FLOW, macsec);
WRAP_SET_ATTR_FUNC(macsec_flow, SAI_OBJECT_TYPE_MACSEC_FLOW, macsec);
WRAP_GET_ATTR_FUNC(macsec_flow, SAI_OBJECT_TYPE_MACSEC_FLOW, macsec);

sai_status_t wrap_get_macsec_port_stats(
    sai_object_id_t macsec_port_id,
    uint32_t number_of_counters,
    const sai_stat_id_t* counter_ids,
    uint64_t* counters) {
  return SaiTracer::getInstance()->macsecApi_->get_macsec_port_stats(
      macsec_port_id, number_of_counters, counter_ids, counters);
}

sai_status_t wrap_get_macsec_sa_stats(
    sai_object_id_t macsec_sa_id,
    uint32_t number_of_counters,
    const sai_stat_id_t* counter_ids,
    uint64_t* counters) {
  return SaiTracer::getInstance()->macsecApi_->get_macsec_sa_stats(
      macsec_sa_id, number_of_counters, counter_ids, counters);
}

sai_status_t wrap_get_macsec_flow_stats(
    sai_object_id_t macsec_flow_id,
    uint32_t number_of_counters,
    const sai_stat_id_t* counter_ids,
    uint64_t* counters) {
  return SaiTracer::getInstance()->macsecApi_->get_macsec_flow_stats(
      macsec_flow_id, number_of_counters, counter_ids, counters);
}

sai_macsec_api_t* wrappedMacsecApi() {
  static sai_macsec_api_t macsecWrappers;

  macsecWrappers.create_macsec = &wrap_create_macsec;
  macsecWrappers.remove_macsec = &wrap_remove_macsec;
  macsecWrappers.set_macsec_attribute = &wrap_set_macsec_attribute;
  macsecWrappers.get_macsec_attribute = &wrap_get_macsec_attribute;

  macsecWrappers.create_macsec_port = &wrap_create_macsec_port;
  macsecWrappers.remove_macsec_port = &wrap_remove_macsec_port;
  macsecWrappers.set_macsec_port_attribute = &wrap_set_macsec_port_attribute;
  macsecWrappers.get_macsec_port_attribute = &wrap_get_macsec_port_attribute;
  macsecWrappers.get_macsec_port_stats = &wrap_get_macsec_port_stats;

  macsecWrappers.create_macsec_sa = &wrap_create_macsec_sa;
  macsecWrappers.remove_macsec_sa = &wrap_remove_macsec_sa;
  macsecWrappers.set_macsec_sa_attribute = &wrap_set_macsec_sa_attribute;
  macsecWrappers.get_macsec_sa_attribute = &wrap_get_macsec_sa_attribute;
  macsecWrappers.get_macsec_sa_stats = &wrap_get_macsec_sa_stats;

  macsecWrappers.create_macsec_sc = &wrap_create_macsec_sc;
  macsecWrappers.remove_macsec_sc = &wrap_remove_macsec_sc;
  macsecWrappers.set_macsec_sc_attribute = &wrap_set_macsec_sc_attribute;
  macsecWrappers.get_macsec_sc_attribute = &wrap_get_macsec_sc_attribute;

  macsecWrappers.create_macsec_flow = &wrap_create_macsec_flow;
  macsecWrappers.remove_macsec_flow = &wrap_remove_macsec_flow;
  macsecWrappers.set_macsec_flow_attribute = &wrap_set_macsec_flow_attribute;
  macsecWrappers.get_macsec_flow_attribute = &wrap_get_macsec_flow_attribute;
  macsecWrappers.get_macsec_flow_stats = &wrap_get_macsec_flow_stats;

  return &macsecWrappers;
}

void setMacsecAttributes(
    const sai_attribute_t* attr_list,
    uint32_t attr_count,
    std::vector<std::string>& attrLines) {
  for (int i = 0; i < attr_count; ++i) {
    switch (attr_list[i].id) {
      case SAI_MACSEC_ATTR_DIRECTION:
        attrLines.push_back(s32Attr(attr_list, i));
        break;
      case SAI_MACSEC_ATTR_PHYSICAL_BYPASS_ENABLE:
        attrLines.push_back(boolAttr(attr_list, i));
        break;
      default:
        break;
    }
  }
}

void setMacsecPortAttributes(
    const sai_attribute_t* attr_list,
    uint32_t attr_count,
    std::vector<std::string>& attrLines) {
  for (int i = 0; i < attr_count; ++i) {
    switch (attr_list[i].id) {
      case SAI_MACSEC_PORT_ATTR_MACSEC_DIRECTION:
        attrLines.push_back(s32Attr(attr_list, i));
        break;
      case SAI_MACSEC_PORT_ATTR_PORT_ID:
        attrLines.push_back(oidAttr(attr_list, i));
        break;
      default:
        break;
    }
  }
}

void setMacsecSaAttributes(
    const sai_attribute_t* attr_list,
    uint32_t attr_count,
    std::vector<std::string>& attrLines) {
  for (int i = 0; i < attr_count; ++i) {
    switch (attr_list[i].id) {
      case SAI_MACSEC_SA_ATTR_AN:
        attrLines.push_back(u8Attr(attr_list, i));
        break;
      case SAI_MACSEC_SA_ATTR_MACSEC_DIRECTION:
        attrLines.push_back(s32Attr(attr_list, i));
        break;
#if SAI_API_VERSION >= SAI_VERSION(1, 7, 1)
      case SAI_MACSEC_SA_ATTR_MACSEC_SSCI:
        attrLines.push_back(u32Attr(attr_list, i));
        break;
#endif
      case SAI_MACSEC_SA_ATTR_MINIMUM_XPN:
        attrLines.push_back(u64Attr(attr_list, i));
        break;
      case SAI_MACSEC_SA_ATTR_SC_ID:
        attrLines.push_back(oidAttr(attr_list, i));
        break;
      case SAI_MACSEC_SA_ATTR_AUTH_KEY:
        u8ArrGenericAttr(
            static_cast<const uint8_t*>(attr_list[i].value.macsecauthkey),
            16,
            i,
            attrLines,
            "macsecauthkey");
        break;
      case SAI_MACSEC_SA_ATTR_SAK:
        u8ArrGenericAttr(
            static_cast<const uint8_t*>(attr_list[i].value.macsecsak),
            32,
            i,
            attrLines,
            "macsecsak");
        break;
      case SAI_MACSEC_SA_ATTR_SALT:
        u8ArrGenericAttr(
            static_cast<const uint8_t*>(attr_list[i].value.macsecsalt),
            12,
            i,
            attrLines,
            "macsecsalt");
        break;
      default:
        break;
    }
  }
}

void setMacsecScAttributes(
    const sai_attribute_t* attr_list,
    uint32_t attr_count,
    std::vector<std::string>& attrLines) {
  for (int i = 0; i < attr_count; ++i) {
    switch (attr_list[i].id) {
      case SAI_MACSEC_SC_ATTR_ACTIVE_EGRESS_SA_ID:
      case SAI_MACSEC_SC_ATTR_FLOW_ID:
        attrLines.push_back(oidAttr(attr_list, i));
        break;
#if SAI_API_VERSION >= SAI_VERSION(1, 7, 1)
      case SAI_MACSEC_SC_ATTR_MACSEC_CIPHER_SUITE:
#endif
      case SAI_MACSEC_SC_ATTR_MACSEC_DIRECTION:
      case SAI_MACSEC_SC_ATTR_MACSEC_REPLAY_PROTECTION_WINDOW:
        attrLines.push_back(s32Attr(attr_list, i));
        break;
      case SAI_MACSEC_SC_ATTR_MACSEC_SCI:
        attrLines.push_back(u64Attr(attr_list, i));
        break;
      case SAI_MACSEC_SC_ATTR_MACSEC_EXPLICIT_SCI_ENABLE:
      case SAI_MACSEC_SC_ATTR_MACSEC_REPLAY_PROTECTION_ENABLE:
        attrLines.push_back(boolAttr(attr_list, i));
        break;
      case SAI_MACSEC_SC_ATTR_MACSEC_SECTAG_OFFSET:
        attrLines.push_back(u8Attr(attr_list, i));
        break;
      default:
        break;
    }
  }
}

void setMacsecFlowAttributes(
    const sai_attribute_t* attr_list,
    uint32_t attr_count,
    std::vector<std::string>& attrLines) {
  for (int i = 0; i < attr_count; ++i) {
    switch (attr_list[i].id) {
      case SAI_MACSEC_FLOW_ATTR_MACSEC_DIRECTION:
        attrLines.push_back(s32Attr(attr_list, i));
        break;
      default:
        break;
    }
  }
}

} // namespace facebook::fboss
