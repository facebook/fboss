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

struct FakeNeighbor {
  FakeNeighbor(
      const folly::MacAddress& dstMac,
      sai_uint32_t metadata,
      sai_uint32_t encapIndex,
      bool isLocal,
      bool noHostRoute)
      : dstMac(dstMac),
        metadata(metadata),
        encapIndex(encapIndex),
        isLocal(isLocal),
        noHostRoute(noHostRoute) {}
  folly::MacAddress dstMac;
  sai_uint32_t metadata{0};
  sai_uint32_t encapIndex{0};
  bool isLocal{true};
  bool noHostRoute{false};
};

using FakeNeighborEntry =
    std::tuple<sai_object_id_t, sai_object_id_t, folly::IPAddress>;
using FakeNeighborManager = FakeManager<FakeNeighborEntry, FakeNeighbor>;

void populate_neighbor_api(sai_neighbor_api_t** neighbor_api);

} // namespace facebook::fboss
