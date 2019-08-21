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
#include "fboss/agent/hw/sai/api/FdbApi.h"
#include "fboss/agent/hw/sai/api/HostifApi.h"
#include "fboss/agent/hw/sai/api/LagApi.h"
#include "fboss/agent/hw/sai/api/NeighborApi.h"
#include "fboss/agent/hw/sai/api/NextHopApi.h"
#include "fboss/agent/hw/sai/api/NextHopGroupApi.h"
#include "fboss/agent/hw/sai/api/PortApi.h"
#include "fboss/agent/hw/sai/api/QueueApi.h"
#include "fboss/agent/hw/sai/api/RouteApi.h"
#include "fboss/agent/hw/sai/api/RouterInterfaceApi.h"
#include "fboss/agent/hw/sai/api/SchedulerApi.h"
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

  FdbApi& fdbApi() {
    return *fdbApi_;
  }
  const FdbApi& fdbApi() const {
    return *fdbApi_;
  }

  HostifApi& hostifApi() {
    return *hostifApi_;
  }
  const HostifApi& hostifApi() const {
    return *hostifApi_;
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

  NextHopGroupApi& nextHopGroupApi() {
    return *nextHopGroupApi_;
  }
  const NextHopGroupApi& nextHopGroupApi() const {
    return *nextHopGroupApi_;
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

  QueueApi& queueApi() {
    return *queueApi_;
  }

  const QueueApi& queueApi() const {
    return *queueApi_;
  }

  RouteApi& routeApi() {
    return *routeApi_;
  }
  const RouteApi& routeApi() const {
    return *routeApi_;
  }
  RouterInterfaceApi& routerInterfaceApi() {
    return *routerInterfaceApi_;
  }
  const RouterInterfaceApi& routerInterfaceApi() const {
    return *routerInterfaceApi_;
  }
  SchedulerApi& schedulerApi() {
    return *schedulerApi_;
  }
  const SchedulerApi& schedulerApi() const {
    return *schedulerApi_;
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
  std::unique_ptr<FdbApi> fdbApi_;
  std::unique_ptr<HostifApi> hostifApi_;
  std::unique_ptr<LagApi> lagApi_;
  std::unique_ptr<NextHopApi> nextHopApi_;
  std::unique_ptr<NextHopGroupApi> nextHopGroupApi_;
  std::unique_ptr<NeighborApi> neighborApi_;
  std::unique_ptr<PortApi> portApi_;
  std::unique_ptr<QueueApi> queueApi_;
  std::unique_ptr<RouteApi> routeApi_;
  std::unique_ptr<RouterInterfaceApi> routerInterfaceApi_;
  std::unique_ptr<SchedulerApi> schedulerApi_;
  std::unique_ptr<SwitchApi> switchApi_;
  std::unique_ptr<VirtualRouterApi> virtualRouterApi_;
  std::unique_ptr<VlanApi> vlanApi_;
};

} // namespace fboss
} // namespace facebook
