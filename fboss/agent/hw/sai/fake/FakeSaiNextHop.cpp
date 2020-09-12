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
#include "fboss/agent/hw/sai/api/SaiVersion.h"

#include <folly/logging/xlog.h>
#include <optional>

using facebook::fboss::FakePort;
using facebook::fboss::FakeSai;

sai_status_t create_next_hop_fn(
    sai_object_id_t* next_hop_id,
    sai_object_id_t /* switch_id */,
    uint32_t attr_count,
    const sai_attribute_t* attr_list) {
  auto fs = FakeSai::getInstance();
  std::optional<sai_next_hop_type_t> type;
  std::optional<folly::IPAddress> ip;
  std::optional<sai_object_id_t> routerInterfaceId;
  std::vector<sai_uint32_t> labelStack;
  bool disableTtlDecrement{false};
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
      case SAI_NEXT_HOP_ATTR_LABELSTACK:
        if (type != SAI_NEXT_HOP_TYPE_MPLS) {
          return SAI_STATUS_INVALID_PARAMETER;
        }
        labelStack.resize(attr_list[i].value.u32list.count);
        for (auto j = 0; j < attr_list[i].value.u32list.count; j++) {
          labelStack[j] = attr_list[i].value.u32list.list[j];
        }
        break;
      case SAI_NEXT_HOP_ATTR_DECREMENT_TTL:
        disableTtlDecrement = attr_list[i].value.booldata;
        break;
      default:
        return SAI_STATUS_INVALID_PARAMETER;
    }
  }
  if (!type || !ip || !routerInterfaceId) {
    return SAI_STATUS_INVALID_PARAMETER;
  }
  *next_hop_id = fs->nextHopManager.create(
      type.value(),
      ip.value(),
      routerInterfaceId.value(),
      labelStack,
      disableTtlDecrement);

  return SAI_STATUS_SUCCESS;
}

sai_status_t remove_next_hop_fn(sai_object_id_t next_hop_id) {
  auto fs = FakeSai::getInstance();
  fs->nextHopManager.remove(next_hop_id);
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
  const auto& nextHop = fs->nextHopManager.get(next_hop_id);
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
      case SAI_NEXT_HOP_ATTR_LABELSTACK:
        if (static_cast<int32_t>(nextHop.type) != SAI_NEXT_HOP_TYPE_MPLS) {
          return SAI_STATUS_INVALID_PARAMETER;
        }
        if (attr[i].value.u32list.count < nextHop.labelStack.size()) {
          attr[i].value.u32list.count = nextHop.labelStack.size();
          return SAI_STATUS_BUFFER_OVERFLOW;
        }
        for (auto j = 0; j < attr[i].value.u32list.count; j++) {
          attr[i].value.u32list.list[j] = nextHop.labelStack[j];
        }
        break;
      case SAI_NEXT_HOP_ATTR_DECREMENT_TTL:
        attr[i].value.booldata = nextHop.disableTtlDecrement;
        break;
      default:
        return SAI_STATUS_INVALID_PARAMETER;
    }
  }
  return SAI_STATUS_SUCCESS;
}

namespace facebook::fboss {

static sai_next_hop_api_t _next_hop_api;

void populate_next_hop_api(sai_next_hop_api_t** next_hop_api) {
  _next_hop_api.create_next_hop = &create_next_hop_fn;
  _next_hop_api.remove_next_hop = &remove_next_hop_fn;
  _next_hop_api.set_next_hop_attribute = &set_next_hop_attribute_fn;
  _next_hop_api.get_next_hop_attribute = &get_next_hop_attribute_fn;
  *next_hop_api = &_next_hop_api;
}

} // namespace facebook::fboss
