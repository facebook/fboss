/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/sai/tracer/RouterInterfaceApiTracer.h"
#include <typeindex>
#include <utility>

#include "fboss/agent/hw/sai/api/RouterInterfaceApi.h"
#include "fboss/agent/hw/sai/tracer/Utils.h"

using folly::to;

namespace {
std::map<int32_t, std::pair<std::string, std::size_t>> _RouterInterfaceMap{
    SAI_ATTR_MAP(VlanRouterInterface, SrcMac),
    SAI_ATTR_MAP(VlanRouterInterface, Type),
    SAI_ATTR_MAP(VlanRouterInterface, VirtualRouterId),
    SAI_ATTR_MAP(VlanRouterInterface, VlanId),
    SAI_ATTR_MAP(VlanRouterInterface, Mtu),
    SAI_ATTR_MAP(PortRouterInterface, SrcMac),
    SAI_ATTR_MAP(PortRouterInterface, Type),
    SAI_ATTR_MAP(PortRouterInterface, VirtualRouterId),
    SAI_ATTR_MAP(PortRouterInterface, PortId),
    SAI_ATTR_MAP(PortRouterInterface, Mtu),
};
} // namespace

namespace facebook::fboss {

WRAP_CREATE_FUNC(
    router_interface,
    SAI_OBJECT_TYPE_ROUTER_INTERFACE,
    routerInterface);
WRAP_REMOVE_FUNC(
    router_interface,
    SAI_OBJECT_TYPE_ROUTER_INTERFACE,
    routerInterface);
WRAP_SET_ATTR_FUNC(
    router_interface,
    SAI_OBJECT_TYPE_ROUTER_INTERFACE,
    routerInterface);
WRAP_GET_ATTR_FUNC(
    router_interface,
    SAI_OBJECT_TYPE_ROUTER_INTERFACE,
    routerInterface);

sai_status_t wrap_get_router_interface_stats(
    sai_object_id_t router_interface_id,
    uint32_t number_of_counters,
    const sai_stat_id_t* counter_ids,
    uint64_t* counters) {
  return SaiTracer::getInstance()
      ->routerInterfaceApi_->get_router_interface_stats(
          router_interface_id, number_of_counters, counter_ids, counters);
}

sai_status_t wrap_get_router_interface_stats_ext(
    sai_object_id_t router_interface_id,
    uint32_t number_of_counters,
    const sai_stat_id_t* counter_ids,
    sai_stats_mode_t mode,
    uint64_t* counters) {
  return SaiTracer::getInstance()
      ->routerInterfaceApi_->get_router_interface_stats_ext(
          router_interface_id, number_of_counters, counter_ids, mode, counters);
}

sai_status_t wrap_clear_router_interface_stats(
    sai_object_id_t router_interface_id,
    uint32_t number_of_counters,
    const sai_stat_id_t* counter_ids) {
  return SaiTracer::getInstance()
      ->routerInterfaceApi_->clear_router_interface_stats(
          router_interface_id, number_of_counters, counter_ids);
}

sai_router_interface_api_t* wrappedRouterInterfaceApi() {
  static sai_router_interface_api_t routerInterfaceWrappers;

  routerInterfaceWrappers.create_router_interface =
      &wrap_create_router_interface;
  routerInterfaceWrappers.remove_router_interface =
      &wrap_remove_router_interface;
  routerInterfaceWrappers.set_router_interface_attribute =
      &wrap_set_router_interface_attribute;
  routerInterfaceWrappers.get_router_interface_attribute =
      &wrap_get_router_interface_attribute;
  routerInterfaceWrappers.get_router_interface_stats =
      &wrap_get_router_interface_stats;
  routerInterfaceWrappers.get_router_interface_stats_ext =
      &wrap_get_router_interface_stats_ext;
  routerInterfaceWrappers.clear_router_interface_stats =
      &wrap_clear_router_interface_stats;

  return &routerInterfaceWrappers;
}

SET_SAI_ATTRIBUTES(RouterInterface)

} // namespace facebook::fboss
