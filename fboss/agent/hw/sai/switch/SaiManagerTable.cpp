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

#include "fboss/agent/hw/sai/store/SaiStore.h"
#include "fboss/agent/hw/sai/switch/ConcurrentIndices.h"
#include "fboss/agent/hw/sai/switch/SaiAclTableGroupManager.h"
#include "fboss/agent/hw/sai/switch/SaiAclTableManager.h"
#include "fboss/agent/hw/sai/switch/SaiBridgeManager.h"
#include "fboss/agent/hw/sai/switch/SaiBufferManager.h"
#include "fboss/agent/hw/sai/switch/SaiDebugCounterManager.h"
#include "fboss/agent/hw/sai/switch/SaiFdbManager.h"
#include "fboss/agent/hw/sai/switch/SaiHashManager.h"
#include "fboss/agent/hw/sai/switch/SaiHostifManager.h"
#include "fboss/agent/hw/sai/switch/SaiInSegEntryManager.h"
#include "fboss/agent/hw/sai/switch/SaiNeighborManager.h"
#include "fboss/agent/hw/sai/switch/SaiNextHopGroupManager.h"
#include "fboss/agent/hw/sai/switch/SaiNextHopManager.h"
#include "fboss/agent/hw/sai/switch/SaiPortManager.h"
#include "fboss/agent/hw/sai/switch/SaiQosMapManager.h"
#include "fboss/agent/hw/sai/switch/SaiQueueManager.h"
#include "fboss/agent/hw/sai/switch/SaiRouteManager.h"
#include "fboss/agent/hw/sai/switch/SaiRouterInterfaceManager.h"
#include "fboss/agent/hw/sai/switch/SaiSchedulerManager.h"
#include "fboss/agent/hw/sai/switch/SaiSwitchManager.h"
#include "fboss/agent/hw/sai/switch/SaiUnsupportedFeatureManager.h"
#include "fboss/agent/hw/sai/switch/SaiVirtualRouterManager.h"
#include "fboss/agent/hw/sai/switch/SaiVlanManager.h"
#include "fboss/agent/hw/sai/switch/SaiWredManager.h"

#include <optional>

namespace facebook::fboss {

SaiManagerTable::SaiManagerTable(SaiPlatform* platform, BootType bootType) {
  switchManager_ = std::make_unique<SaiSwitchManager>(this, platform, bootType);
}

void SaiManagerTable::createSaiTableManagers(
    SaiPlatform* platform,
    ConcurrentIndices* concurrentIndices) {
  aclTableGroupManager_ =
      std::make_unique<SaiAclTableGroupManager>(this, platform);
  aclTableManager_ = std::make_unique<SaiAclTableManager>(this, platform);
  bridgeManager_ = std::make_unique<SaiBridgeManager>(this, platform);
  bufferManager_ = std::make_unique<SaiBufferManager>(this, platform);
  debugCounterManager_ = std::make_unique<SaiDebugCounterManager>(this);
  fdbManager_ =
      std::make_unique<SaiFdbManager>(this, platform, concurrentIndices);
  hashManager_ = std::make_unique<SaiHashManager>(this);
  queueManager_ = std::make_unique<SaiQueueManager>(this, platform);
  hostifManager_ = std::make_unique<SaiHostifManager>(this, platform);
  portManager_ =
      std::make_unique<SaiPortManager>(this, platform, concurrentIndices);
  qosMapManager_ = std::make_unique<SaiQosMapManager>(this, platform);
  virtualRouterManager_ =
      std::make_unique<SaiVirtualRouterManager>(this, platform);
  vlanManager_ =
      std::make_unique<SaiVlanManager>(this, platform, concurrentIndices);
  routeManager_ = std::make_unique<SaiRouteManager>(this, platform);
  routerInterfaceManager_ =
      std::make_unique<SaiRouterInterfaceManager>(this, platform);
  schedulerManager_ = std::make_unique<SaiSchedulerManager>(this, platform);
  nextHopManager_ = std::make_unique<SaiNextHopManager>(this, platform);
  nextHopGroupManager_ =
      std::make_unique<SaiNextHopGroupManager>(this, platform);
  neighborManager_ = std::make_unique<SaiNeighborManager>(this, platform);
  inSegEntryManager_ = std::make_unique<SaiInSegEntryManager>(this, platform);
  lagManager_ = std::make_unique<SaiUnsupportedFeatureManager>("LAG");
  wredManager_ = std::make_unique<SaiWredManager>(this, platform);
}

SaiManagerTable::~SaiManagerTable() {
  // unsupported features reset first. Order does not matter for these
  lagManager_.reset();
  // Need to destroy routes and label fib entries before destroying other
  // managers, as the route and label fib entry destructors will trigger calls
  // in those managers
  routeManager().clear();
  inSegEntryManager_.reset();
  // Reset neighbor mgr before reseting rif mgr, since the
  // neighbor entries refer to rifs. While at it, also reset fdb
  // and next hop mgrs. Fdb is reset after neighbor mgr since
  // neighbor mgr also creates FDB entries. Strictly speaking this
  // is not necessary, since NeighborHandle holds SaiFdbEntry shared_ptr,
  // which gets pruned directly via Api layer and does not go through
  // FdbManager. But that's a implementation detail, we could imagine
  // NeighborHandle pruning calling Fdb entry prune via FdbManager.
  // Reasoning along similar lines NextHopGroup and NextHop managers
  // resets are placed after NeighborManager reset
  neighborManager_.reset();
  fdbManager_.reset();
  nextHopGroupManager_.reset();
  nextHopManager_.reset();
  routerInterfaceManager_.reset();
  virtualRouterManager_.reset();
  bridgeManager_.reset();
  vlanManager_.reset();
  debugCounterManager_.reset();
  portManager_.reset();
  // Hash manager is going away, reset hashes
  switchManager_->resetHashes();
  hashManager_.reset();
  // Qos map manager is going away, reset global qos maps
  switchManager_->resetQosMaps();
  qosMapManager_.reset();
  hostifManager_.reset();

  // Reset ACL Table group before Acl Table, since ACL Table group members
  // refer to ACL Table and those references to ACL Table must be released
  // before attempting to reset (remove) ACL Table.
  aclTableGroupManager_.reset();
  aclTableManager_.reset();

  bufferManager_.reset();
  wredManager_.reset();
  switchManager_.reset();
}

SaiAclTableGroupManager& SaiManagerTable::aclTableGroupManager() {
  return *aclTableGroupManager_;
}
const SaiAclTableGroupManager& SaiManagerTable::aclTableGroupManager() const {
  return *aclTableGroupManager_;
}

SaiAclTableManager& SaiManagerTable::aclTableManager() {
  return *aclTableManager_;
}
const SaiAclTableManager& SaiManagerTable::aclTableManager() const {
  return *aclTableManager_;
}

SaiBridgeManager& SaiManagerTable::bridgeManager() {
  return *bridgeManager_;
}
const SaiBridgeManager& SaiManagerTable::bridgeManager() const {
  return *bridgeManager_;
}

SaiBufferManager& SaiManagerTable::bufferManager() {
  return *bufferManager_;
}
const SaiBufferManager& SaiManagerTable::bufferManager() const {
  return *bufferManager_;
}

SaiDebugCounterManager& SaiManagerTable::debugCounterManager() {
  return *debugCounterManager_;
}
const SaiDebugCounterManager& SaiManagerTable::debugCounterManager() const {
  return *debugCounterManager_;
}

SaiFdbManager& SaiManagerTable::fdbManager() {
  return *fdbManager_;
}
const SaiFdbManager& SaiManagerTable::fdbManager() const {
  return *fdbManager_;
}

SaiHashManager& SaiManagerTable::hashManager() {
  return *hashManager_;
}
const SaiHashManager& SaiManagerTable::hashManager() const {
  return *hashManager_;
}

SaiHostifManager& SaiManagerTable::hostifManager() {
  return *hostifManager_;
}

const SaiHostifManager& SaiManagerTable::hostifManager() const {
  return *hostifManager_;
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

SaiQueueManager& SaiManagerTable::queueManager() {
  return *queueManager_;
}
const SaiQueueManager& SaiManagerTable::queueManager() const {
  return *queueManager_;
}

SaiQosMapManager& SaiManagerTable::qosMapManager() {
  return *qosMapManager_;
}
const SaiQosMapManager& SaiManagerTable::qosMapManager() const {
  return *qosMapManager_;
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

SaiSchedulerManager& SaiManagerTable::schedulerManager() {
  return *schedulerManager_;
}
const SaiSchedulerManager& SaiManagerTable::schedulerManager() const {
  return *schedulerManager_;
}

SaiSwitchManager& SaiManagerTable::switchManager() {
  return *switchManager_;
}
const SaiSwitchManager& SaiManagerTable::switchManager() const {
  return *switchManager_;
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

SaiInSegEntryManager& SaiManagerTable::inSegEntryManager() {
  return *inSegEntryManager_;
}

const SaiInSegEntryManager& SaiManagerTable::inSegEntryManager() const {
  return *inSegEntryManager_;
}

SaiUnsupportedFeatureManager& SaiManagerTable::lagManager() {
  return *lagManager_;
}
const SaiUnsupportedFeatureManager& SaiManagerTable::lagManager() const {
  return *lagManager_;
}

SaiWredManager& SaiManagerTable::wredManager() {
  return *wredManager_;
}
const SaiWredManager& SaiManagerTable::wredManager() const {
  return *wredManager_;
}
} // namespace facebook::fboss
