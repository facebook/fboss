/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "FakeSaiRoute.h"
#include "fboss/agent/hw/sai/fake/FakeSai.h"

#include "fboss/agent/hw/sai/api/AddressUtil.h"

#include <folly/logging/xlog.h>

using facebook::fboss::FakeRoute;
using facebook::fboss::FakeSai;

sai_status_t set_route_entry_attribute_fn(
    const sai_route_entry_t* route_entry,
    const sai_attribute_t* attr) {
  auto fs = FakeSai::getInstance();
  auto re = std::make_tuple(
      route_entry->switch_id,
      route_entry->vr_id,
      facebook::fboss::fromSaiIpPrefix(route_entry->destination));
  auto& fr = fs->routeManager.get(re);
  switch (attr->id) {
    case SAI_ROUTE_ENTRY_ATTR_PACKET_ACTION:
      fr.packetAction = attr->value.s32;
      break;
    case SAI_ROUTE_ENTRY_ATTR_NEXT_HOP_ID:
      fr.nextHopId = attr->value.oid;
      break;
    case SAI_ROUTE_ENTRY_ATTR_META_DATA:
      fr.metadata = attr->value.u32;
      break;
    default:
      return SAI_STATUS_INVALID_PARAMETER;
  }
  return SAI_STATUS_SUCCESS;
}

sai_status_t create_route_entry_fn(
    const sai_route_entry_t* route_entry,
    uint32_t attr_count,
    const sai_attribute_t* attr_list) {
  auto fs = FakeSai::getInstance();
  auto re = std::make_tuple(
      route_entry->switch_id,
      route_entry->vr_id,
      facebook::fboss::fromSaiIpPrefix(route_entry->destination));
  fs->routeManager.create(re);
  for (int i = 0; i < attr_count; ++i) {
    set_route_entry_attribute_fn(route_entry, &attr_list[i]);
  }
  return SAI_STATUS_SUCCESS;
}

sai_status_t remove_route_entry_fn(const sai_route_entry_t* route_entry) {
  auto fs = FakeSai::getInstance();
  auto re = std::make_tuple(
      route_entry->switch_id,
      route_entry->vr_id,
      facebook::fboss::fromSaiIpPrefix(route_entry->destination));
  if (fs->routeManager.remove(re) == 0) {
    return SAI_STATUS_FAILURE;
  }
  return SAI_STATUS_SUCCESS;
}

sai_status_t get_route_entry_attribute_fn(
    const sai_route_entry_t* route_entry,
    uint32_t attr_count,
    sai_attribute_t* attr_list) {
  auto fs = FakeSai::getInstance();
  auto re = std::make_tuple(
      route_entry->switch_id,
      route_entry->vr_id,
      facebook::fboss::fromSaiIpPrefix(route_entry->destination));
  const auto& fr = fs->routeManager.get(re);
  for (int i = 0; i < attr_count; ++i) {
    switch (attr_list[i].id) {
      case SAI_ROUTE_ENTRY_ATTR_PACKET_ACTION:
        attr_list[i].value.s32 = fr.packetAction;
        break;
      case SAI_ROUTE_ENTRY_ATTR_NEXT_HOP_ID:
        attr_list[i].value.oid = fr.nextHopId;
        break;
      case SAI_ROUTE_ENTRY_ATTR_META_DATA:
        attr_list[i].value.u32 = fr.metadata;
        break;
      default:
        return SAI_STATUS_INVALID_PARAMETER;
    }
  }
  return SAI_STATUS_SUCCESS;
}

namespace facebook::fboss {

static sai_route_api_t _route_api;

void populate_route_api(sai_route_api_t** route_api) {
  _route_api.create_route_entry = &create_route_entry_fn;
  _route_api.remove_route_entry = &remove_route_entry_fn;
  _route_api.set_route_entry_attribute = &set_route_entry_attribute_fn;
  _route_api.get_route_entry_attribute = &get_route_entry_attribute_fn;
  *route_api = &_route_api;
}

} // namespace facebook::fboss
