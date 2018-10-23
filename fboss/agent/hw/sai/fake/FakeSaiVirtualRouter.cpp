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
#include "FakeSaiVirtualRouter.h"

#include <folly/logging/xlog.h>

sai_status_t create_virtual_router_fn(
    sai_object_id_t* /* virtual_router_id */,
    sai_object_id_t /* switch_id */,
    uint32_t /* attr_count */,
    const sai_attribute_t* /* attr_list */) {
  return SAI_STATUS_SUCCESS;
}

sai_status_t remove_virtual_router_fn(sai_object_id_t /* virtual_router_id */) {
  return SAI_STATUS_FAILURE;
}

sai_status_t set_virtual_router_attribute_fn(
    sai_object_id_t /* virtual_router_id */,
    const sai_attribute_t* /* attr */) {
  return SAI_STATUS_FAILURE;
}

sai_status_t get_virtual_router_attribute_fn(
    sai_object_id_t /* virtual_router_id */,
    uint32_t /* attr_count */,
    sai_attribute_t* /* attr */) {
  return SAI_STATUS_FAILURE;
}

namespace facebook {
namespace fboss {

static sai_virtual_router_api_t _virtual_router_api;

void populate_virtual_router_api(
    sai_virtual_router_api_t** virtual_router_api) {
  _virtual_router_api.create_virtual_router = &create_virtual_router_fn;
  _virtual_router_api.remove_virtual_router = &remove_virtual_router_fn;
  _virtual_router_api.set_virtual_router_attribute =
      &set_virtual_router_attribute_fn;
  _virtual_router_api.get_virtual_router_attribute =
      &get_virtual_router_attribute_fn;
  *virtual_router_api = &_virtual_router_api;
}

} // namespace fboss
} // namespace facebook
