/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/sai/switch/SaiManagerTable.h"

#include "fboss/agent/hw/sai/api/SaiApiTable.h"
#include "fboss/agent/hw/sai/switch/SaiBridgeManager.h"
#include "fboss/agent/hw/sai/switch/SaiNeighborManager.h"
#include "fboss/agent/hw/sai/switch/SaiNextHopGroupManager.h"
#include "fboss/agent/hw/sai/switch/SaiNextHopManager.h"
#include "fboss/agent/hw/sai/switch/SaiPortManager.h"
#include "fboss/agent/hw/sai/switch/SaiRouteManager.h"
#include "fboss/agent/hw/sai/switch/SaiRouterInterfaceManager.h"
#include "fboss/agent/hw/sai/switch/SaiVirtualRouterManager.h"
#include "fboss/agent/hw/sai/switch/SaiVlanManager.h"

namespace facebook {
namespace fboss {

SaiManagerTable::SaiManagerTable(SaiApiTable* apiTable) : apiTable_(apiTable) {
  bridgeManager_ = std::make_unique<SaiBridgeManager>(apiTable_, this);
  portManager_ = std::make_unique<SaiPortManager>(apiTable_, this);
  virtualRouterManager_ =
      std::make_unique<SaiVirtualRouterManager>(apiTable_, this);
  vlanManager_ = std::make_unique<SaiVlanManager>(apiTable_, this);
  routeManager_ = std::make_unique<SaiRouteManager>(apiTable_, this);
  routerInterfaceManager_ =
      std::make_unique<SaiRouterInterfaceManager>(apiTable_, this);
  nextHopManager_ = std::make_unique<SaiNextHopManager>(apiTable_, this);
  nextHopGroupManager_ =
      std::make_unique<SaiNextHopGroupManager>(apiTable_, this);
  neighborManager_ = std::make_unique<SaiNeighborManager>(apiTable_, this);
}

SaiManagerTable::~SaiManagerTable() {
  // Need to destroy routes before destroying other managers, as the
  // route destructor will trigger calls in those managers
  routeManager().clear();
}

SaiBridgeManager& SaiManagerTable::bridgeManager() {
  return *bridgeManager_;
}
const SaiBridgeManager& SaiManagerTable::bridgeManager() const {
  return *bridgeManager_;
}

SaiNeighborManager& SaiManagerTable::neighborManager() {
  return *neighborManager_;
}
const SaiNeighborManager& SaiManagerTable::neighborManager() const {
  return *neighborManager_;
}

SaiNextHopManager& SaiManagerTable::nextHopManager() {
  return *nextHopManager_;
}
const SaiNextHopManager& SaiManagerTable::nextHopManager() const {
  return *nextHopManager_;
}

SaiNextHopGroupManager& SaiManagerTable::nextHopGroupManager() {
  return *nextHopGroupManager_;
}
const SaiNextHopGroupManager& SaiManagerTable::nextHopGroupManager() const {
  return *nextHopGroupManager_;
}

SaiPortManager& SaiManagerTable::portManager() {
  return *portManager_;
}
const SaiPortManager& SaiManagerTable::portManager() const {
  return *portManager_;
}

SaiRouteManager& SaiManagerTable::routeManager() {
  return *routeManager_;
}
const SaiRouteManager& SaiManagerTable::routeManager() const {
  return *routeManager_;
}

SaiRouterInterfaceManager& SaiManagerTable::routerInterfaceManager() {
  return *routerInterfaceManager_;
}
const SaiRouterInterfaceManager& SaiManagerTable::routerInterfaceManager()
    const {
  return *routerInterfaceManager_;
}

SaiVirtualRouterManager& SaiManagerTable::virtualRouterManager() {
  return *virtualRouterManager_;
}
const SaiVirtualRouterManager& SaiManagerTable::virtualRouterManager() const {
  return *virtualRouterManager_;
}

SaiVlanManager& SaiManagerTable::vlanManager() {
  return *vlanManager_;
}
const SaiVlanManager& SaiManagerTable::vlanManager() const {
  return *vlanManager_;
}

} // namespace fboss
} // namespace facebook
