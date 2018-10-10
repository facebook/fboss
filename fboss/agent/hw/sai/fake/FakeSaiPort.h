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

#include <unordered_map>

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
  FakePort(bool adminState = true, sai_uint32_t speed = 100000)
      : adminState(adminState), speed(speed) {}
  sai_object_id_t id;
  bool adminState;
  sai_uint32_t speed;
};

class FakePortManager {
 public:
  sai_object_id_t addPort(const FakePort& p) {
    auto ins = port_map_.insert({++port_id_, p});
    ins.first->second.id = port_id_;
    return port_id_;
  }
  void deletePort(sai_object_id_t id) {
    size_t erased = port_map_.erase(id);
    if (!erased) {
    }
  }
  FakePort& getPort(sai_object_id_t id) {
    return port_map_.at(id);
  }
  const FakePort& getPort(sai_object_id_t id) const {
    return port_map_.at(id);
  }
  size_t numPorts() const {
    return port_map_.size();
  }
  const std::unordered_map<sai_object_id_t, FakePort>& portMap() const {
    return port_map_;
  }

 private:
  size_t port_id_ = 0;
  std::unordered_map<sai_object_id_t, FakePort> port_map_;
};

void populate_port_api(sai_port_api_t** port_api);
}
} // namespace facebook
