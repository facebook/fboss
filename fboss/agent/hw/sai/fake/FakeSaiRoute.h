/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#pragma once

#include "fboss/agent/hw/sai/fake/FakeManager.h"

#include <folly/IPAddress.h>
#include <folly/MacAddress.h>

#include <tuple>

extern "C" {
  #include <sai.h>
}

sai_status_t create_route_entry_fn(
    const sai_route_entry_t* route_entry,
    uint32_t attr_count,
    const sai_attribute_t* attr_list);

sai_status_t remove_route_entry_fn(
    const sai_route_entry_t* route_entry);

sai_status_t set_route_entry_attribute_fn(
    const sai_route_entry_t* route_entry,
    const sai_attribute_t* attr);

sai_status_t get_route_entry_attribute_fn(
    const sai_route_entry_t* route_entry,
    uint32_t attr_count,
    sai_attribute_t* attr_list);

namespace facebook {
namespace fboss {

struct FakeRoute {
  FakeRoute() {}
  sai_object_id_t nextHopId;
};

using FakeRouteEntry =
    std::tuple<sai_object_id_t, sai_object_id_t, folly::CIDRNetwork>;
using FakeRouteManager = FakeManager<FakeRouteEntry, FakeRoute>;

void populate_route_api(sai_route_api_t** route_api);
} // namespace fboss
} // namespace facebook
