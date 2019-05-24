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

#include <folly/MacAddress.h>

#include <tuple>

extern "C" {
#include <sai.h>
}

sai_status_t create_fdb_entry_fn(
    const sai_fdb_entry_t* fdb_entry,
    uint32_t attr_count,
    const sai_attribute_t* attr_list);

sai_status_t remove_fdb_entry_fn(const sai_fdb_entry_t* fdb_entry);

sai_status_t set_fdb_entry_attribute_fn(
    const sai_fdb_entry_t* fdb_entry,
    const sai_attribute_t* attr);

sai_status_t get_fdb_entry_attribute_fn(
    const sai_fdb_entry_t* fdb_entry,
    uint32_t attr_count,
    sai_attribute_t* attr_list);

namespace facebook {
namespace fboss {

struct FakeFdb {
  explicit FakeFdb(const sai_object_id_t bridgePortId)
      : bridgePortId(bridgePortId) {}
  sai_object_id_t bridgePortId;
};

using FakeFdbEntry =
    std::tuple<sai_object_id_t, sai_object_id_t, folly::MacAddress>;
using FakeFdbManager = FakeManager<FakeFdbEntry, FakeFdb>;

void populate_fdb_api(sai_fdb_api_t** fdb_api);
} // namespace fboss
} // namespace facebook
