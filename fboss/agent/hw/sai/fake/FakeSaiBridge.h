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
  sai_object_id_t bridgeId;
  sai_object_id_t id;
  sai_object_id_t portId;
  int32_t type;
};

class FakeBridge {
 public:
  sai_object_id_t id;
  std::unordered_map<sai_object_id_t, FakeBridgePort>& portMap() {
    return portMap_;
  }
  const std::unordered_map<sai_object_id_t, FakeBridgePort>& portMap() const {
    return portMap_;
  }

 private:
  std::unordered_map<sai_object_id_t, FakeBridgePort> portMap_;
};

class FakeBridgeManager {
 public:
  sai_object_id_t addBridge(const FakeBridge& b) {
    auto ins = bridgeMap_.insert({++bridgeId, b});
    ins.first->second.id = bridgeId;
    return bridgeId;
  }
  void deleteBridge(sai_object_id_t bridgeId) {
    size_t erased = bridgeMap_.erase(bridgeId);
    if (!erased) {
      //TODO
    }
  }
  FakeBridge& getBridge(const sai_object_id_t& bridgeId) {
    return bridgeMap_.at(bridgeId);
  }
  const FakeBridge& getBridge(const sai_object_id_t& bridgeId) const {
    return bridgeMap_.at(bridgeId);
  }
  sai_object_id_t addBridgePort(
      sai_object_id_t bridgeId,
      const FakeBridgePort& bp) {
    auto& bridgePortMap = getBridge(bridgeId).portMap();
    auto ins = bridgePortMap.insert({++bridgePortId, bp});
    ins.first->second.id = bridgePortId;
    bridgePortToBridgeMap_.insert({bridgePortId, bridgeId});
    return bridgePortId;
  }
  void deleteBridgePort(sai_object_id_t bridgePortId) {
    auto bridgeId = bridgePortToBridgeMap_.at(bridgePortId);
    auto& bridgePortMap = getBridge(bridgeId).portMap();
    size_t erased = bridgePortMap.erase(bridgePortId);
    if (!erased) {
      //TODO
    }
  }

  FakeBridgePort& getBridgePort(const sai_object_id_t& bridgePortId) {
    auto bridgeId = bridgePortToBridgeMap_.at(bridgePortId);
    auto& bridgePortMap = getBridge(bridgeId).portMap();
    return bridgePortMap.at(bridgePortId);
  }

  const FakeBridgePort& getBridgePort(
      const sai_object_id_t& bridgePortId) const {
    auto bridgeId = bridgePortToBridgeMap_.at(bridgePortId);
    auto bridgePortMap = getBridge(bridgeId).portMap();
    return bridgePortMap.at(bridgePortId);
  }

 private:
  size_t bridgeId = 0;
  size_t bridgePortId = 0;
  std::unordered_map<sai_object_id_t, sai_object_id_t> bridgePortToBridgeMap_;
  std::unordered_map<sai_object_id_t, FakeBridge> bridgeMap_;
};

void populate_bridge_api(sai_bridge_api_t** bridge_api);
} // namespace fboss
} // namespace facebook
