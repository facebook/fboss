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
#include "FakeSaiPort.h"
#include "fboss/agent/hw/sai/api/AddressUtil.h"

#include <folly/logging/xlog.h>
#include <folly/Optional.h>

using facebook::fboss::FakeSai;
using facebook::fboss::FakePort;

sai_status_t create_next_hop_fn(
    sai_object_id_t* next_hop_id,
    sai_object_id_t /* switch_id */,
    uint32_t attr_count,
    const sai_attribute_t* attr_list) {
  auto fs = FakeSai::getInstance();
  folly::Optional<sai_next_hop_type_t> type;
  folly::Optional<folly::IPAddress> ip;
  folly::Optional<sai_object_id_t> routerInterfaceId;
  for (int i = 0; i < attr_count; ++i) {
    switch (attr_list[i].id) {
      case SAI_NEXT_HOP_ATTR_TYPE:
        type = static_cast<sai_next_hop_type_t>(attr_list[i].value.s32);
        break;
      case SAI_NEXT_HOP_ATTR_IP:
        ip = facebook::fboss::fromSaiIpAddress(attr_list[i].value.ipaddr);
        break;
      case SAI_NEXT_HOP_ATTR_ROUTER_INTERFACE_ID:
        routerInterfaceId = attr_list[i].value.oid;
        break;
      default:
        return SAI_STATUS_INVALID_PARAMETER;
    }
  }
  if (!type || !ip || !routerInterfaceId) {
    return SAI_STATUS_INVALID_PARAMETER;
  }
  *next_hop_id =
      fs->nhm.create(type.value(), ip.value(), routerInterfaceId.value());
  return SAI_STATUS_SUCCESS;
}

sai_status_t remove_next_hop_fn(sai_object_id_t next_hop_id) {
  auto fs = FakeSai::getInstance();
  fs->nhm.remove(next_hop_id);
  return SAI_STATUS_SUCCESS;
}

sai_status_t set_next_hop_attribute_fn(
    sai_object_id_t /* next_hop_id */,
    const sai_attribute_t* attr) {
  switch (attr->id) {
    default:
      return SAI_STATUS_INVALID_PARAMETER;
  }
  return SAI_STATUS_SUCCESS;
}

sai_status_t get_next_hop_attribute_fn(
    sai_object_id_t next_hop_id,
    uint32_t attr_count,
    sai_attribute_t* attr) {
  auto fs = FakeSai::getInstance();
  const auto& nextHop = fs->nhm.get(next_hop_id);
  for (int i = 0; i < attr_count; ++i) {
    switch (attr[i].id) {
      case SAI_NEXT_HOP_ATTR_TYPE:
        attr[i].value.s32 = static_cast<int32_t>(nextHop.type);
        break;
      case SAI_NEXT_HOP_ATTR_IP:
        attr[i].value.ipaddr = facebook::fboss::toSaiIpAddress(nextHop.ip);
        break;
      case SAI_NEXT_HOP_ATTR_ROUTER_INTERFACE_ID:
        attr[i].value.oid = nextHop.routerInterfaceId;
        break;
      default:
        return SAI_STATUS_INVALID_PARAMETER;
    }
  }
  return SAI_STATUS_SUCCESS;
}

namespace facebook {
namespace fboss {

static sai_next_hop_api_t _next_hop_api;

void populate_next_hop_api(sai_next_hop_api_t** next_hop_api) {
  _next_hop_api.create_next_hop = &create_next_hop_fn;
  _next_hop_api.remove_next_hop = &remove_next_hop_fn;
  _next_hop_api.set_next_hop_attribute = &set_next_hop_attribute_fn;
  _next_hop_api.get_next_hop_attribute = &get_next_hop_attribute_fn;
  *next_hop_api = &_next_hop_api;
}

} // namespace fboss
} // namespace facebook
