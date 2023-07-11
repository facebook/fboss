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

#include "fboss/agent/hw/UnsupportedFeatureManager.h"
#include "fboss/agent/hw/sai/store/SaiStore.h"
#include "fboss/agent/hw/sai/switch/ConcurrentIndices.h"
#include "fboss/agent/hw/sai/switch/SaiAclTableGroupManager.h"
#include "fboss/agent/hw/sai/switch/SaiAclTableManager.h"
#include "fboss/agent/hw/sai/switch/SaiBridgeManager.h"
#include "fboss/agent/hw/sai/switch/SaiBufferManager.h"
#include "fboss/agent/hw/sai/switch/SaiCounterManager.h"
#include "fboss/agent/hw/sai/switch/SaiDebugCounterManager.h"
#include "fboss/agent/hw/sai/switch/SaiFdbManager.h"
#include "fboss/agent/hw/sai/switch/SaiHashManager.h"
#include "fboss/agent/hw/sai/switch/SaiHostifManager.h"
#include "fboss/agent/hw/sai/switch/SaiInSegEntryManager.h"
#include "fboss/agent/hw/sai/switch/SaiLagManager.h"
#include "fboss/agent/hw/sai/switch/SaiMacsecManager.h"
#include "fboss/agent/hw/sai/switch/SaiMirrorManager.h"
#include "fboss/agent/hw/sai/switch/SaiNeighborManager.h"
#include "fboss/agent/hw/sai/switch/SaiNextHopGroupManager.h"
#include "fboss/agent/hw/sai/switch/SaiNextHopManager.h"
#include "fboss/agent/hw/sai/switch/SaiPortManager.h"
#include "fboss/agent/hw/sai/switch/SaiQosMapManager.h"
#include "fboss/agent/hw/sai/switch/SaiQueueManager.h"
#include "fboss/agent/hw/sai/switch/SaiRouteManager.h"
#include "fboss/agent/hw/sai/switch/SaiRouterInterfaceManager.h"
#include "fboss/agent/hw/sai/switch/SaiSamplePacketManager.h"
#include "fboss/agent/hw/sai/switch/SaiSchedulerManager.h"
#include "fboss/agent/hw/sai/switch/SaiSwitchManager.h"
#include "fboss/agent/hw/sai/switch/SaiSystemPortManager.h"
#include "fboss/agent/hw/sai/switch/SaiTamManager.h"
#include "fboss/agent/hw/sai/switch/SaiTunnelManager.h"
#include "fboss/agent/hw/sai/switch/SaiUdfManager.h"
#include "fboss/agent/hw/sai/switch/SaiVirtualRouterManager.h"
#include "fboss/agent/hw/sai/switch/SaiVlanManager.h"
#include "fboss/agent/hw/sai/switch/SaiWredManager.h"

#include <optional>

namespace facebook::fboss {

SaiManagerTable::SaiManagerTable(
    SaiPlatform* platform,
    BootType bootType,
    cfg::SwitchType switchType,
    std::optional<int64_t> switchId) {
  switchManager_ = std::make_unique<SaiSwitchManager>(
      this, platform, bootType, switchType, switchId);
}

void SaiManagerTable::createSaiTableManagers(
    SaiStore* saiStore,
    SaiPlatform* platform,
    ConcurrentIndices* concurrentIndices) {
  aclTableGroupManager_ =
      std::make_unique<SaiAclTableGroupManager>(saiStore, this, platform);
  aclTableManager_ =
      std::make_unique<SaiAclTableManager>(saiStore, this, platform);
  bridgeManager_ = std::make_unique<SaiBridgeManager>(saiStore, this, platform);
  bufferManager_ = std::make_unique<SaiBufferManager>(saiStore, this, platform);
  counterManager_ =
      std::make_unique<SaiCounterManager>(saiStore, this, platform);
  debugCounterManager_ =
      std::make_unique<SaiDebugCounterManager>(saiStore, this, platform);
  fdbManager_ = std::make_unique<SaiFdbManager>(
      saiStore, this, platform, concurrentIndices);
  hashManager_ = std::make_unique<SaiHashManager>(saiStore, this, platform);
  queueManager_ = std::make_unique<SaiQueueManager>(saiStore, this, platform);
  hostifManager_ = std::make_unique<SaiHostifManager>(
      saiStore, this, platform, concurrentIndices);
  mirrorManager_ = std::make_unique<SaiMirrorManager>(saiStore, this, platform);
  portManager_ = std::make_unique<SaiPortManager>(
      saiStore, this, platform, concurrentIndices);
  systemPortManager_ = std::make_unique<SaiSystemPortManager>(
      saiStore, this, platform, concurrentIndices);
  qosMapManager_ = std::make_unique<SaiQosMapManager>(saiStore, this, platform);
  macsecManager_ = std::make_unique<SaiMacsecManager>(saiStore, this);
  virtualRouterManager_ =
      std::make_unique<SaiVirtualRouterManager>(saiStore, this, platform);
  vlanManager_ = std::make_unique<SaiVlanManager>(saiStore, this, platform);
  routeManager_ = std::make_unique<SaiRouteManager>(saiStore, this, platform);
  routerInterfaceManager_ =
      std::make_unique<SaiRouterInterfaceManager>(saiStore, this, platform);
  samplePacketManager_ =
      std::make_unique<SaiSamplePacketManager>(saiStore, this, platform);
  schedulerManager_ =
      std::make_unique<SaiSchedulerManager>(saiStore, this, platform);
  nextHopManager_ =
      std::make_unique<SaiNextHopManager>(saiStore, this, platform);
  nextHopGroupManager_ =
      std::make_unique<SaiNextHopGroupManager>(saiStore, this, platform);
  neighborManager_ =
      std::make_unique<SaiNeighborManager>(saiStore, this, platform);
  inSegEntryManager_ =
      std::make_unique<SaiInSegEntryManager>(saiStore, this, platform);
  lagManager_ = std::make_unique<SaiLagManager>(
      saiStore, this, platform, concurrentIndices);
  wredManager_ = std::make_unique<SaiWredManager>(saiStore, this, platform);
  // CSP CS00011823810
#if !defined(SAI_VERSION_7_2_0_0_ODP) && !defined(SAI_VERSION_8_2_0_0_ODP) && \
    !defined(SAI_VERSION_8_2_0_0_DNX_ODP) &&                                  \
    !defined(SAI_VERSION_8_2_0_0_SIM_ODP) &&                                  \
    !defined(SAI_VERSION_9_0_EA_SIM_ODP) &&                                   \
    !defined(SAI_VERSION_10_0_EA_DNX_SIM_ODP) &&                              \
    !defined(SAI_VERSION_9_2_0_0_ODP) &&                                      \
    !defined(SAI_VERSION_10_0_EA_DNX_ODP) &&                                  \
    !defined(SAI_VERSION_10_0_EA_ODP) && !defined(SAI_VERSION_10_0_EA_SIM_ODP)
  tamManager_ = std::make_unique<SaiTamManager>(saiStore, this, platform);
#endif
  tunnelManager_ = std::make_unique<SaiTunnelManager>(saiStore, this, platform);
  teFlowEntryManager_ =
      std::make_unique<UnsupportedFeatureManager>("EM entries");
#if SAI_API_VERSION >= SAI_VERSION(1, 12, 0)
  udfManager_ = std::make_unique<SaiUdfManager>(saiStore, this, platform);
#endif
}

SaiManagerTable::~SaiManagerTable() {
  reset(false);
}

void SaiManagerTable::reset(bool skipSwitchManager) {
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
  lagManager_.reset();
  bridgeManager_.reset();
  vlanManager_.reset();
  counterManager_.reset();
  debugCounterManager_.reset();
  // Mirroring and port has a circular dependency. Monitor port
  // is an attribute of mirror and mirror oid is an attribute of
  // Port. The right sequence here is to reset mirror first
  // which will set all port mirror oids to NULL OID followed by
  // removing all mirror sessions. Ports no longer has any
  // dependency with mirror and can be removed.
  mirrorManager_.reset();
  macsecManager_.reset();
  systemPortManager_->resetQueues();
  // Reset the qos maps on system port before ports are
  // removed from the system.
  systemPortManager_->resetQosMaps();
  // On Silicon one family chips, queues are tied to system
  // ports. So before we delete system ports, its required
  // to reset the queue associations.
  portManager_->resetQueues();
  systemPortManager_.reset();
  portManager_.reset();
  // Hash manager is going away, reset hashes
  switchManager_->resetHashes();
  hashManager_.reset();
  // Qos map manager is going away, reset global qos maps
  switchManager_->resetQosMaps();
  qosMapManager_.reset();
  hostifManager_.reset();
  samplePacketManager_.reset();

  // ACL Table Group is going away, reset ingressACL pointing to it
  if (!skipSwitchManager) {
    switchManager_->resetIngressAcl();
  }

  // Reset ACL Table group before Acl Table, since ACL Table group members
  // refer to ACL Table and those references to ACL Table must be released
  // before attempting to reset (remove) ACL Table.
  aclTableGroupManager_.reset();
  aclTableManager_.reset();

  bufferManager_.reset();
  wredManager_.reset();

  // CSP CS00011823810
#if !defined(SAI_VERSION_7_2_0_0_ODP) && !defined(SAI_VERSION_8_2_0_0_ODP) && \
    !defined(SAI_VERSION_8_2_0_0_DNX_ODP) &&                                  \
    !defined(SAI_VERSION_8_2_0_0_SIM_ODP) &&                                  \
    !defined(SAI_VERSION_9_0_EA_SIM_ODP) &&                                   \
    !defined(SAI_VERSION_10_0_EA_DNX_SIM_ODP) &&                              \
    !defined(SAI_VERSION_9_2_0_0_ODP) &&                                      \
    !defined(SAI_VERSION_10_0_EA_DNX_ODP) &&                                  \
    !defined(SAI_VERSION_10_0_EA_ODP) && !defined(SAI_VERSION_10_0_EA_SIM_ODP)
  tamManager_.reset();
#endif
  tunnelManager_.reset();
  queueManager_.reset();
  routeManager_.reset();
  schedulerManager_.reset();
  teFlowEntryManager_.reset();

#if SAI_API_VERSION >= SAI_VERSION(1, 12, 0)
  udfManager_.reset();
#endif

  if (!skipSwitchManager) {
    switchManager_.reset();
  }
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

SaiCounterManager& SaiManagerTable::counterManager() {
  return *counterManager_;
}
const SaiCounterManager& SaiManagerTable::counterManager() const {
  return *counterManager_;
}

SaiDebugCounterManager& SaiManagerTable::debugCounterManager() {
  return *debugCounterManager_;
}
const SaiDebugCounterManager& SaiManagerTable::debugCounterManager() const {
  return *debugCounterManager_;
}

UnsupportedFeatureManager& SaiManagerTable::teFlowEntryManager() {
  return *teFlowEntryManager_;
}
const UnsupportedFeatureManager& SaiManagerTable::teFlowEntryManager() const {
  return *teFlowEntryManager_;
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

SaiMacsecManager& SaiManagerTable::macsecManager() {
  return *macsecManager_;
}

const SaiMacsecManager& SaiManagerTable::macsecManager() const {
  return *macsecManager_;
}

SaiMirrorManager& SaiManagerTable::mirrorManager() {
  return *mirrorManager_;
}

const SaiMirrorManager& SaiManagerTable::mirrorManager() const {
  return *mirrorManager_;
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

SaiSystemPortManager& SaiManagerTable::systemPortManager() {
  return *systemPortManager_;
}
const SaiSystemPortManager& SaiManagerTable::systemPortManager() const {
  return *systemPortManager_;
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

SaiSamplePacketManager& SaiManagerTable::samplePacketManager() {
  return *samplePacketManager_;
}
const SaiSamplePacketManager& SaiManagerTable::samplePacketManager() const {
  return *samplePacketManager_;
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

SaiLagManager& SaiManagerTable::lagManager() {
  return *lagManager_;
}
const SaiLagManager& SaiManagerTable::lagManager() const {
  return *lagManager_;
}

SaiWredManager& SaiManagerTable::wredManager() {
  return *wredManager_;
}
const SaiWredManager& SaiManagerTable::wredManager() const {
  return *wredManager_;
}

SaiTamManager& SaiManagerTable::tamManager() {
  return *tamManager_;
}

const SaiTamManager& SaiManagerTable::tamManager() const {
  return *tamManager_;
}

SaiTunnelManager& SaiManagerTable::tunnelManager() {
  return *tunnelManager_;
}

const SaiTunnelManager& SaiManagerTable::tunnelManager() const {
  return *tunnelManager_;
}

SaiUdfManager& SaiManagerTable::udfManager() {
  return *udfManager_;
}

const SaiUdfManager& SaiManagerTable::udfManager() const {
  return *udfManager_;
}
} // namespace facebook::fboss
