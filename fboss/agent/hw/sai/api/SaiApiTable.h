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

#include "fboss/agent/hw/sai/api/BridgeApi.h"
#include "fboss/agent/hw/sai/api/LagApi.h"
#include "fboss/agent/hw/sai/api/NextHopApi.h"
#include "fboss/agent/hw/sai/api/NeighborApi.h"
#include "fboss/agent/hw/sai/api/PortApi.h"
#include "fboss/agent/hw/sai/api/RouterInterfaceApi.h"
#include "fboss/agent/hw/sai/api/SwitchApi.h"
#include "fboss/agent/hw/sai/api/VirtualRouterApi.h"
#include "fboss/agent/hw/sai/api/VlanApi.h"


#include <memory>

namespace facebook {
namespace fboss {

class SaiApiTable {
 public:
  SaiApiTable();
  SaiApiTable(const SaiApiTable& other) = delete;
  SaiApiTable& operator=(const SaiApiTable& other) = delete;

  BridgeApi& bridgeApi() {
    return *bridgeApi_;
  }
  const BridgeApi& bridgeApi() const {
    return *bridgeApi_;
  }

  LagApi& lagApi() {
    return *lagApi_;
  }
  const LagApi& lagApi() const {
    return *lagApi_;
  }

  NextHopApi& nextHopApi() {
    return *nextHopApi_;
  }
  const NextHopApi& nextHopApi() const {
    return *nextHopApi_;
  }

  NeighborApi& neighborApi() {
    return *neighborApi_;
  }
  const NeighborApi& neighborApi() const {
    return *neighborApi_;
  }

  PortApi& portApi() {
    return *portApi_;
  }
  const PortApi& portApi() const {
    return *portApi_;
  }

  RouterInterfaceApi& routerInterfaceApi() {
    return *routerInterfaceApi_;
  }
  const RouterInterfaceApi& routerInterfaceApi() const {
    return *routerInterfaceApi_;
  }

  SwitchApi& switchApi() {
    return *switchApi_;
  }
  const SwitchApi& switchApi() const {
    return *switchApi_;
  }

  VirtualRouterApi& virtualRouterApi() {
    return *virtualRouterApi_;
  }
  const VirtualRouterApi& virtualRouterApi() const {
    return *virtualRouterApi_;
  }

  VlanApi& vlanApi() {
    return *vlanApi_;
  }
  const VlanApi& vlanApi() const {
    return *vlanApi_;
  }

 private:
  std::unique_ptr<BridgeApi> bridgeApi_;
  std::unique_ptr<LagApi> lagApi_;
  std::unique_ptr<NextHopApi> nextHopApi_;
  std::unique_ptr<NeighborApi> neighborApi_;
  std::unique_ptr<PortApi> portApi_;
  std::unique_ptr<RouterInterfaceApi> routerInterfaceApi_;
  std::unique_ptr<SwitchApi> switchApi_;
  std::unique_ptr<VirtualRouterApi> virtualRouterApi_;
  std::unique_ptr<VlanApi> vlanApi_;

};

} // namespace fboss
} // namespace facebook
