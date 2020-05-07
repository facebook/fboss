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
namespace facebook::fboss {

struct FakeRoute {
  FakeRoute() {}
  sai_object_id_t nextHopId{0};
  int32_t packetAction{0};
  uint32_t metadata{0};
};

using FakeRouteEntry =
    std::tuple<sai_object_id_t, sai_object_id_t, folly::CIDRNetwork>;
using FakeRouteManager = FakeManager<FakeRouteEntry, FakeRoute>;

void populate_route_api(sai_route_api_t** route_api);

} // namespace facebook::fboss
