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
#include <typeindex>
#include <utility>

#include "fboss/agent/hw/sai/api/MacsecApi.h"
#include "fboss/agent/hw/sai/tracer/MacsecApiTracer.h"
#include "fboss/agent/hw/sai/tracer/Utils.h"

using folly::to;

namespace {
std::map<int32_t, std::pair<std::string, std::size_t>> _MacsecMap{
    SAI_ATTR_MAP(Macsec, Direction),
    SAI_ATTR_MAP(Macsec, PhysicalBypass),
};

std::map<int32_t, std::pair<std::string, std::size_t>> _MacsecPortMap{
    SAI_ATTR_MAP(MacsecPort, PortID),
    SAI_ATTR_MAP(MacsecPort, MacsecDirection),
};

std::map<int32_t, std::pair<std::string, std::size_t>> _MacsecSAMap {
  SAI_ATTR_MAP(MacsecSA, AssocNum), SAI_ATTR_MAP(MacsecSA, AuthKey),
      SAI_ATTR_MAP(MacsecSA, MacsecDirection),
#if SAI_API_VERSION >= SAI_VERSION(1, 7, 1)
      SAI_ATTR_MAP(MacsecSA, SSCI),
#endif
      SAI_ATTR_MAP(MacsecSA, MinimumXpn), SAI_ATTR_MAP(MacsecSA, SAK),
      SAI_ATTR_MAP(MacsecSA, Salt), SAI_ATTR_MAP(MacsecSA, SCID),
#if SAI_API_VERSION >= SAI_VERSION(1, 8, 1)
      SAI_ATTR_MAP(MacsecSA, CurrentXpn),
#endif
};

std::map<int32_t, std::pair<std::string, std::size_t>> _MacsecSCMap {
  SAI_ATTR_MAP(MacsecSC, SCI), SAI_ATTR_MAP(MacsecSC, MacsecDirection),
      SAI_ATTR_MAP(MacsecSC, ActiveEgressSAID), SAI_ATTR_MAP(MacsecSC, FlowID),
#if SAI_API_VERSION >= SAI_VERSION(1, 7, 1)
      SAI_ATTR_MAP(MacsecSC, CipherSuite),
#endif
      SAI_ATTR_MAP(MacsecSC, SCIEnable),
      SAI_ATTR_MAP(MacsecSC, ReplayProtectionEnable),
      SAI_ATTR_MAP(MacsecSC, ReplayProtectionWindow),
      SAI_ATTR_MAP(MacsecSC, SectagOffset),
};

std::map<int32_t, std::pair<std::string, std::size_t>> _MacsecFlowMap{
    SAI_ATTR_MAP(MacsecFlow, MacsecDirection),
};

} // namespace

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

SET_SAI_ATTRIBUTES(Macsec)
SET_SAI_ATTRIBUTES(MacsecPort)
SET_SAI_ATTRIBUTES(MacsecSA)
SET_SAI_ATTRIBUTES(MacsecSC)
SET_SAI_ATTRIBUTES(MacsecFlow)

} // namespace facebook::fboss
