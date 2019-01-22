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

#include <unordered_map>
#include <vector>

extern "C" {
  #include <sai.h>
}

sai_status_t create_port_fn(
    sai_object_id_t* port_id,
    sai_object_id_t switch_id,
    uint32_t attr_count,
    const sai_attribute_t* attr_list);

sai_status_t remove_port_fn(sai_object_id_t port_id);

sai_status_t set_port_attribute_fn(
    sai_object_id_t port_id,
    const sai_attribute_t* attr);

sai_status_t get_port_attribute_fn(
    sai_object_id_t port_id,
    uint32_t attr_count,
    sai_attribute_t* attr);

namespace facebook {
namespace fboss {

class FakePort {
 public:
  FakePort(std::vector<uint32_t> lanes, sai_uint32_t speed)
      : lanes(lanes), speed(speed) {}
  sai_object_id_t id;
  bool adminState{false};
  std::vector<uint32_t> lanes;
  uint32_t speed{0};
};

using FakePortManager = FakeManager<sai_object_id_t, FakePort>;

void populate_port_api(sai_port_api_t** port_api);
} // namespace fboss
} // namespace facebook
