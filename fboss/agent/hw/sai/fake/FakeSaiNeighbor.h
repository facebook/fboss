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

sai_status_t create_neighbor_entry_fn(
    const sai_neighbor_entry_t* neighbor_entry,
    uint32_t attr_count,
    const sai_attribute_t* attr_list);

sai_status_t remove_neighbor_entry_fn(
    const sai_neighbor_entry_t* neighbor_entry);

sai_status_t set_neighbor_entry_attribute_fn(
    const sai_neighbor_entry_t* neighbor_entry,
    const sai_attribute_t* attr);

sai_status_t get_neighbor_entry_attribute_fn(
    const sai_neighbor_entry_t* neighbor_entry,
    uint32_t attr_count,
    sai_attribute_t* attr_list);

namespace facebook {
namespace fboss {

struct FakeNeighbor {
  explicit FakeNeighbor(const folly::MacAddress& dstMac) : dstMac(dstMac) {}
  folly::MacAddress dstMac;
};

using FakeNeighborEntry =
    std::tuple<sai_object_id_t, sai_object_id_t, folly::IPAddress>;
using FakeNeighborManager = FakeManager<FakeNeighborEntry, FakeNeighbor>;

void populate_neighbor_api(sai_neighbor_api_t** neighbor_api);
} // namespace fboss
} // namespace facebook
