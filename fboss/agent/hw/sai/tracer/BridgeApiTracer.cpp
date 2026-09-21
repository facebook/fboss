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
#include <typeindex>
#include <utility>

#include "fboss/agent/hw/sai/api/BridgeApi.h"
#include "fboss/agent/hw/sai/tracer/Utils.h"

using folly::to;

namespace {
std::map<int32_t, std::pair<std::string, std::size_t>> _BridgeMap{
    SAI_ATTR_MAP(Bridge, PortList),
    SAI_ATTR_MAP(Bridge, Type),
};

std::map<int32_t, std::pair<std::string, std::size_t>> _BridgePortMap{
    SAI_ATTR_MAP(BridgePort, BridgeId),
    SAI_ATTR_MAP(BridgePort, PortId),
    SAI_ATTR_MAP(BridgePort, Type),
    SAI_ATTR_MAP(BridgePort, FdbLearningMode),
    SAI_ATTR_MAP(BridgePort, AdminState),
};

} // namespace

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

SET_SAI_ATTRIBUTES(Bridge)
SET_SAI_ATTRIBUTES(BridgePort)

} // namespace facebook::fboss
