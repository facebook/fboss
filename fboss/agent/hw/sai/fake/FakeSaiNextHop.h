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

#include "FakeManager.h"

#include <folly/IPAddress.h>

extern "C" {
  #include <sai.h>
}

sai_status_t create_next_hop_fn(
    sai_object_id_t* next_hop_id,
    sai_object_id_t switch_id,
    uint32_t attr_count,
    const sai_attribute_t* attr_list);

sai_status_t remove_next_hop_fn(sai_object_id_t next_hop_id);

sai_status_t set_next_hop_attribute_fn(
    sai_object_id_t next_hop_id,
    const sai_attribute_t* attr);

sai_status_t get_next_hop_attribute_fn(
    sai_object_id_t next_hop_id,
    uint32_t attr_count,
    sai_attribute_t* attr);

namespace facebook {
namespace fboss {

struct FakeNextHop {
 public:
  FakeNextHop(
      sai_next_hop_type_t type,
      const folly::IPAddress& ip,
      sai_object_id_t routerInterfaceId)
      : type(type), ip(ip), routerInterfaceId(routerInterfaceId) {}
  sai_next_hop_type_t type;
  folly::IPAddress ip;
  sai_object_id_t routerInterfaceId;
  sai_object_id_t id;
};

using FakeNextHopManager = FakeManager<sai_object_id_t, FakeNextHop>;

void populate_next_hop_api(sai_next_hop_api_t** next_hop_api);
} // namespace fboss
} // namespace facebook
