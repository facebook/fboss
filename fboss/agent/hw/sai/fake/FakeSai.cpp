/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "FakeSai.h"

#include <folly/Singleton.h>

#include <folly/logging/xlog.h>

namespace {
  struct singleton_tag_type {};
}

using facebook::fboss::FakeSai;
static folly::Singleton<FakeSai, singleton_tag_type> fakeSaiSingleton{};
std::shared_ptr<FakeSai> FakeSai::getInstance() {
  return fakeSaiSingleton.try_get();
}

sai_status_t sai_api_initialize(
    uint64_t /* flags */,
    const sai_service_method_table_t* /* services */) {
  auto fs = FakeSai::getInstance();
  if (fs->initialized) {
    return SAI_STATUS_FAILURE;
  }

  // Create the default switch per the SAI spec
  fs->swm.create();
  // Create the default 1Q bridge per the SAI spec
  fs->brm.create();
  // Create the default virtual router per the SAI spec
  fs->vrm.create();

  fs->initialized = true;
  return SAI_STATUS_SUCCESS;
}

sai_status_t sai_api_query(sai_api_t sai_api_id, void** api_method_table) {
  auto fs = FakeSai::getInstance();
  if (!fs->initialized) {
    return SAI_STATUS_FAILURE;
  }
  sai_status_t res;
  switch (sai_api_id) {
    case SAI_API_BRIDGE:
      facebook::fboss::populate_bridge_api(
          (sai_bridge_api_t**)api_method_table);
      res = SAI_STATUS_SUCCESS;
      break;
      case SAI_API_LAG:
        facebook::fboss::populate_lag_api(
            (sai_lag_api_t**)api_method_table);
        res = SAI_STATUS_SUCCESS;
        break;
    case SAI_API_NEIGHBOR:
      facebook::fboss::populate_neighbor_api(
          (sai_neighbor_api_t**)api_method_table);
      res = SAI_STATUS_SUCCESS;
      break;
    case SAI_API_NEXT_HOP:
      facebook::fboss::populate_next_hop_api(
          (sai_next_hop_api_t**)api_method_table);
      res = SAI_STATUS_SUCCESS;
      break;
    case SAI_API_PORT:
      facebook::fboss::populate_port_api((sai_port_api_t**)api_method_table);
      res = SAI_STATUS_SUCCESS;
      break;
    case SAI_API_ROUTE:
      facebook::fboss::populate_route_api((sai_route_api_t**)api_method_table);
      res = SAI_STATUS_SUCCESS;
      break;
    case SAI_API_ROUTER_INTERFACE:
      facebook::fboss::populate_router_interface_api(
          (sai_router_interface_api_t**)api_method_table);
      res = SAI_STATUS_SUCCESS;
      break;
    case SAI_API_SWITCH:
      facebook::fboss::populate_switch_api(
          (sai_switch_api_t**)api_method_table);
      res = SAI_STATUS_SUCCESS;
      break;
    case SAI_API_VIRTUAL_ROUTER:
      facebook::fboss::populate_virtual_router_api(
          (sai_virtual_router_api_t**)api_method_table);
      res = SAI_STATUS_SUCCESS;
      break;
    case SAI_API_VLAN:
      facebook::fboss::populate_vlan_api(
          (sai_vlan_api_t**)api_method_table);
      res = SAI_STATUS_SUCCESS;
      break;
    default:
      res = SAI_STATUS_INVALID_PARAMETER;
      break;
  }
  return res;
}
