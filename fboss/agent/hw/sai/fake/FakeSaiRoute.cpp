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
#include "FakeSaiRoute.h"

#include "fboss/agent/hw/sai/api/AddressUtil.h"

#include <folly/logging/xlog.h>

using facebook::fboss::FakeSai;
using facebook::fboss::FakeRoute;

sai_status_t create_route_entry_fn(
    const sai_route_entry_t* route_entry,
    uint32_t attr_count,
    const sai_attribute_t* attr_list) {
  auto fs = FakeSai::getInstance();
  auto re = std::make_tuple(
      route_entry->switch_id,
      route_entry->vr_id,
      facebook::fboss::fromSaiIpPrefix(route_entry->destination));
  fs->rm.create(re);
  for (int i = 0; i < attr_count; ++i) {
    set_route_entry_attribute_fn(route_entry, &attr_list[i]);
  }
  return SAI_STATUS_SUCCESS;
}

sai_status_t remove_route_entry_fn(
    const sai_route_entry_t* /* route_entry */) {
  auto fs = FakeSai::getInstance();
  return SAI_STATUS_FAILURE;
}

sai_status_t set_route_entry_attribute_fn(
    const sai_route_entry_t* route_entry,
    const sai_attribute_t* attr) {
  auto fs = FakeSai::getInstance();
  auto re = std::make_tuple(
      route_entry->switch_id,
      route_entry->vr_id,
      facebook::fboss::fromSaiIpPrefix(route_entry->destination));
  auto& fr = fs->rm.get(re);
  switch (attr->id) {
    case SAI_ROUTE_ENTRY_ATTR_NEXT_HOP_ID:
      fr.nextHopId = attr->value.oid;
      break;
    default:
      return SAI_STATUS_INVALID_PARAMETER;
  }
  return SAI_STATUS_FAILURE;
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
  const auto& fr = fs->rm.get(re);
  for (int i = 0; i < attr_count; ++i) {
    switch(attr_list[i].id) {
      case SAI_ROUTE_ENTRY_ATTR_NEXT_HOP_ID:
        attr_list[i].value.oid = fr.nextHopId;
        break;
      default:
        return SAI_STATUS_INVALID_PARAMETER;
    }
  }
  return SAI_STATUS_SUCCESS;
}

namespace facebook {
namespace fboss {

static sai_route_api_t _route_api;

void populate_route_api(sai_route_api_t** route_api) {
  _route_api.create_route_entry = &create_route_entry_fn;
  _route_api.remove_route_entry = &remove_route_entry_fn;
  _route_api.set_route_entry_attribute = &set_route_entry_attribute_fn;
  _route_api.get_route_entry_attribute = &get_route_entry_attribute_fn;
  *route_api = &_route_api;
}

} // namespace fboss
} // namespace facebook
