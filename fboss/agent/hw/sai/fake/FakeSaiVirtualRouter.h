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

#include <folly/MacAddress.h>

extern "C" {
  #include <sai.h>
}

sai_status_t create_virtual_router_fn(
    sai_object_id_t* virtual_router_id,
    sai_object_id_t switch_id,
    uint32_t attr_count,
    const sai_attribute_t* attr_list);

sai_status_t remove_virtual_router_fn(sai_object_id_t virtual_router_id);

sai_status_t set_virtual_router_attribute_fn(
    sai_object_id_t virtual_router_id,
    const sai_attribute_t* attr);

sai_status_t get_virtual_router_attribute_fn(
    sai_object_id_t virtual_router_id,
    uint32_t attr_count,
    sai_attribute_t* attr);

namespace facebook {
namespace fboss {

class FakeVirtualRouter {
 public:
  sai_object_id_t id;
};

using FakeVirtualRouterManager =
    FakeManager<sai_object_id_t, FakeVirtualRouter>;

void populate_virtual_router_api(sai_virtual_router_api_t** virtual_router_api);

} // namespace fboss
} // namespace facebook
