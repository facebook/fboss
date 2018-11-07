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

extern "C" {
  #include <sai.h>
}

sai_status_t create_bridge_fn(
    sai_object_id_t* bridge_id,
    sai_object_id_t switch_id,
    uint32_t attr_count,
    const sai_attribute_t* attr_list);

sai_status_t remove_bridge_fn(sai_object_id_t bridge_id);

sai_status_t get_bridge_attribute_fn(
    sai_object_id_t bridge_id,
    uint32_t attr_count,
    sai_attribute_t* attr);

sai_status_t set_bridge_attribute_fn(
    sai_object_id_t bridge_id,
    const sai_attribute_t* attr);

sai_status_t create_bridge_port_fn(
    sai_object_id_t* bridge_port_id,
    sai_object_id_t switch_id,
    uint32_t attr_count,
    const sai_attribute_t* attr_list);

sai_status_t remove_bridge_port_fn(sai_object_id_t bridge_port_id);

sai_status_t get_bridge_port_attribute_fn(
    sai_object_id_t bridge_id,
    uint32_t attr_count,
    sai_attribute_t* attr);

sai_status_t set_bridge_port_attribute_fn(
    sai_object_id_t bridge_port_id,
    const sai_attribute_t* attr);

namespace facebook {
namespace fboss {

class FakeBridgePort {
 public:
  explicit FakeBridgePort(sai_object_id_t bridgeId) : bridgeId(bridgeId) {}
  sai_object_id_t bridgeId;
  sai_object_id_t id;
  sai_object_id_t portId;
  int32_t type;
};

class FakeBridge {
 public:
  sai_object_id_t id;
  FakeManager<sai_object_id_t, FakeBridgePort>& fm() {
    return fm_;
  }
  const FakeManager<sai_object_id_t, FakeBridgePort>& fm() const {
    return fm_;
  }

 private:
  FakeManager<sai_object_id_t, FakeBridgePort> fm_;
};

using FakeBridgeManager = FakeManagerWithMembers<FakeBridge, FakeBridgePort>;

void populate_bridge_api(sai_bridge_api_t** bridge_api);
} // namespace fboss
} // namespace facebook
