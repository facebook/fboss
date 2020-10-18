/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/sai/switch/SaiSwitch.h"

#include "fboss/agent/Constants.h"
#include "fboss/agent/Utils.h"
#include "fboss/agent/hw/HwPortFb303Stats.h"
#include "fboss/agent/hw/HwResourceStatsPublisher.h"
#include "fboss/agent/hw/gen-cpp2/hardware_stats_types.h"
#include "fboss/agent/hw/sai/api/AdapterKeySerializers.h"
#include "fboss/agent/hw/sai/api/FdbApi.h"
#include "fboss/agent/hw/sai/api/HostifApi.h"
#include "fboss/agent/hw/sai/api/LoggingUtil.h"
#include "fboss/agent/hw/sai/api/SaiApiTable.h"
#include "fboss/agent/hw/sai/api/SaiObjectApi.h"
#include "fboss/agent/hw/sai/api/Types.h"
#include "fboss/agent/hw/sai/store/SaiStore.h"
#include "fboss/agent/hw/sai/switch/ConcurrentIndices.h"
#include "fboss/agent/hw/sai/switch/SaiAclTableGroupManager.h"
#include "fboss/agent/hw/sai/switch/SaiAclTableManager.h"
#include "fboss/agent/hw/sai/switch/SaiBufferManager.h"
#include "fboss/agent/hw/sai/switch/SaiDebugCounterManager.h"
#include "fboss/agent/hw/sai/switch/SaiHashManager.h"
#include "fboss/agent/hw/sai/switch/SaiHostifManager.h"
#include "fboss/agent/hw/sai/switch/SaiInSegEntryManager.h"
#include "fboss/agent/hw/sai/switch/SaiManagerTable.h"
#include "fboss/agent/hw/sai/switch/SaiNeighborManager.h"
#include "fboss/agent/hw/sai/switch/SaiPortManager.h"
#include "fboss/agent/hw/sai/switch/SaiRouteManager.h"
#include "fboss/agent/hw/sai/switch/SaiRouterInterfaceManager.h"
#include "fboss/agent/hw/sai/switch/SaiRxPacket.h"
#include "fboss/agent/hw/sai/switch/SaiSwitchManager.h"
#include "fboss/agent/hw/sai/switch/SaiTxPacket.h"
#include "fboss/agent/hw/sai/switch/SaiUnsupportedFeatureManager.h"
#include "fboss/agent/hw/sai/switch/SaiVlanManager.h"
#include "fboss/agent/packet/EthHdr.h"
#include "fboss/agent/packet/PktUtil.h"
#include "fboss/agent/platforms/sai/SaiPlatform.h"
#include "fboss/agent/state/Port.h"
#include "fboss/agent/state/StateDelta.h"
#include "fboss/agent/state/SwitchState.h"

#include "fboss/agent/hw/HwSwitchWarmBootHelper.h"
#include "fboss/agent/hw/switch_asics/HwAsic.h"

#include <folly/logging/xlog.h>

#include <chrono>
#include <optional>

extern "C" {
#include <sai.h>
}

DEFINE_bool(flexports, true, "Load the agent with flexport support enabled");
/*
 * Setting the default sai sdk logging level to CRITICAL for several reasons:
 * 1) These are synchronous writes to the syslog so that agent
 * is not blocked by this.
 * 2) Route, Port and Neighbor change frequently and so to limit the writes to
 * the syslog .
 * 3) Agent might miss some of the control packets (eg: LACP heartbeats)
 * which can cause port-channel flaps
 */
DEFINE_string(
    enable_sai_log,
    "CRITICAL",
    "Turn on SAI SDK logging. Options are DEBUG|INFO|NOTICE|WARN|ERROR|CRITICAL");

namespace {
/*
 * For the devices/SDK we use, the only events we should get (and process)
 * are LEARN and AGED.
 * Learning generates LEARN, aging generates AGED, while Mac move results in
 * DELETE followed by ADD.
 * We ASSERT this explicitly via HW tests.
 */
const std::map<sai_fdb_event_t, facebook::fboss::L2EntryUpdateType>
    kL2AddrUpdateOperationsOfInterest = {
        {SAI_FDB_EVENT_LEARNED,
         facebook::fboss::L2EntryUpdateType::L2_ENTRY_UPDATE_TYPE_ADD},
        {SAI_FDB_EVENT_AGED,
         facebook::fboss::L2EntryUpdateType::L2_ENTRY_UPDATE_TYPE_DELETE},
};
} // namespace

namespace facebook::fboss {

// We need this global SaiSwitch* to support registering SAI callbacks
// which can then use SaiSwitch to do their work. The current callback
// facility in SAI does not support passing user data to come back
// with the callback.
// N.B., if we want to have multiple SaiSwitches in a device with multiple
// cards being managed by one instance of FBOSS, this will need to be
// extended, presumably into an array keyed by switch id.
static SaiSwitch* __gSaiSwitch;

// Free functions to register as callbacks
void __gPacketRxCallback(
    sai_object_id_t switch_id,
    sai_size_t buffer_size,
    const void* buffer,
    uint32_t attr_count,
    const sai_attribute_t* attr_list) {
  __gSaiSwitch->packetRxCallback(
      SwitchSaiId{switch_id}, buffer_size, buffer, attr_count, attr_list);
}

void __glinkStateChangedNotification(
    uint32_t count,
    const sai_port_oper_status_notification_t* data) {
  __gSaiSwitch->linkStateChangedCallbackTopHalf(count, data);
}

void __gFdbEventCallback(
    uint32_t count,
    const sai_fdb_event_notification_data_t* data) {
  __gSaiSwitch->fdbEventCallback(count, data);
}

PortSaiId SaiSwitch::getCPUPortSaiId(SwitchSaiId switchId) {
  static std::optional<PortSaiId> kCpuPortId;
  if (!kCpuPortId) {
    kCpuPortId = SaiApiTable::getInstance()->switchApi().getAttribute(
        switchId, SaiSwitchTraits::Attributes::CpuPort{});
  }
  return kCpuPortId.value();
}

SaiSwitch::SaiSwitch(SaiPlatform* platform, uint32_t featuresDesired)
    : HwSwitch(featuresDesired), platform_(platform) {
  utilCreateDir(platform_->getVolatileStateDir());
  utilCreateDir(platform_->getPersistentStateDir());
}

SaiSwitch::~SaiSwitch() {}

HwInitResult SaiSwitch::init(Callback* callback) noexcept {
  HwInitResult ret;
  {
    std::lock_guard<std::mutex> lock(saiSwitchMutex_);
    ret = initLocked(lock, callback);
    /*
     * SwitchState does not have notion of AclTableGroup or AclTable today.
     * Thus, stateChanged() can not process aclTableGroupChanges or
     * aclTableChanges. Thus, we create single AclTableGroup for Ingress and a
     * single AclTable at its member during init(). Every AclEntry from
     * SwitchState is added to this AclTable by stateChanged() while processing
     * AclEntryChanges.
     *
     * During cold boot, addAclTableGroup()/addAclTable() populate
     * AclTableGroupManager/AclTableManager local data structures + program the
     * ASIC by calling SAI AclApi.
     *
     * During warm boot, SaiStore reload() reloads SAI objects for
     * AclTableGroup/AclTable in SaiStore. Thus,
     * addAclTableGroup()/addAclTable() only populate
     * AclTableGroupManager/AclTableManager local data structure.
     *
     * In future, SwitchState would be extended to carry AclTable, at that time,
     * the implementation would be on the following lines:
     *     - SwitchState AclTable configuration would include Stage
     *       e.g. ingress or egress.
     *     - For every stage supported for AclTable, SaiSwitch::init would
     *       pre-create an AclTableGroup.
     *     - statechanged() would contain AclTable delta processing which would
     *       add/remove/change AclTable and using 'stage', also update the
     *       corresponding Acl Table group member.
     *     - statechanged() would continue to carry AclEntry delta processing.
     */
    managerTable_->aclTableGroupManager().addAclTableGroup(
        SAI_ACL_STAGE_INGRESS);
    managerTable_->aclTableManager().addAclTable(kAclTable1);

    if (getPlatform()->getAsic()->isSupported(HwAsic::Feature::DEBUG_COUNTER)) {
      managerTable_->debugCounterManager().setupDebugCounters();
    }
    if (platform_->getAsic()->isSupported(HwAsic::Feature::BUFFER_PROFILE)) {
      managerTable_->bufferManager().setupEgressBufferPool();
    }
  }

  stateChanged(StateDelta(std::make_shared<SwitchState>(), ret.switchState));
  return ret;
}

void SaiSwitch::unregisterCallbacks() noexcept {
  // after unregistering there could still be a single packet in our
  // pipeline. To fully shut down rx, we need to stop the thread and
  // let the possible last packet get processed. Since processing a
  // packet takes the saiSwitchMutex_, before calling join() on the thread
  // we need to release the lock.
  {
    std::lock_guard<std::mutex> lock(saiSwitchMutex_);
    unregisterCallbacksLocked(lock);
  }

  // linkscan is turned off and the evb loop is set to break
  // just need to block until the last event is processed
  if (runState_ >= SwitchRunState::CONFIGURED &&
      getFeaturesDesired() & FeaturesDesired::LINKSCAN_DESIRED) {
    linkStateBottomHalfEventBase_.terminateLoopSoon();
    linkStateBottomHalfThread_->join();
    // link scan is completely shut-off
  }

  fdbEventBottomHalfEventBase_.terminateLoopSoon();
  fdbEventBottomHalfThread_->join();
}

template <typename ManagerT>
void SaiSwitch::processDefaultDataPlanePolicyDelta(
    const StateDelta& delta,
    ManagerT& mgr) {
  auto qosDelta = delta.getDefaultDataPlaneQosPolicyDelta();
  auto& qosMapManager = managerTable_->qosMapManager();
  if ((qosDelta.getOld() != qosDelta.getNew())) {
    auto lock = std::lock_guard<std::mutex>(saiSwitchMutex_);
    if (qosDelta.getOld() && qosDelta.getNew()) {
      if (*qosDelta.getOld() != *qosDelta.getNew()) {
        mgr.clearQosPolicy();
        qosMapManager.removeQosMap();
        qosMapManager.addQosMap(qosDelta.getNew());
        mgr.setQosPolicy();
      }
    } else if (qosDelta.getNew()) {
      qosMapManager.addQosMap(qosDelta.getNew());
      mgr.setQosPolicy();
    } else if (qosDelta.getOld()) {
      mgr.clearQosPolicy();
      qosMapManager.removeQosMap();
    }
  }
}

void SaiSwitch::processLinkStateChangeDelta(const StateDelta& delta) {
  DeltaFunctions::forEachChanged(
      delta.getPortsDelta(),
      [&](const std::shared_ptr<Port>& oldPort,
          const std::shared_ptr<Port>& newPort) {
        if (!oldPort->isEnabled() && !newPort->isEnabled()) {
          return;
        }
        auto id = newPort->getID();
        auto adminStateChanged =
            oldPort->getAdminState() != newPort->getAdminState();
        if (adminStateChanged) {
          auto adminStr = (newPort->isEnabled()) ? "ENABLED" : "DISABLED";
          XLOG(DBG1) << "Admin state changed on port " << id << ": "
                     << adminStr;
        }
        auto operStateChanged =
            oldPort->getOperState() != newPort->getOperState();
        if (operStateChanged) {
          auto operStr = (newPort->isUp()) ? "UP" : "DOWN";
          XLOG(DBG1) << "Oper state changed on port " << id << ": " << operStr;
        }

        if (adminStateChanged || operStateChanged) {
          auto platformPort = platform_->getPort(id);
          platformPort->linkStatusChanged(
              newPort->isUp(), newPort->isEnabled());
        }
      });
}

std::shared_ptr<SwitchState> SaiSwitch::stateChanged(const StateDelta& delta) {
  processRemovedDelta(
      delta.getPortsDelta(),
      managerTable_->portManager(),
      &SaiPortManager::removePort);
  processChangedDelta(
      delta.getPortsDelta(),
      managerTable_->portManager(),
      &SaiPortManager::changePort);
  processAddedDelta(
      delta.getPortsDelta(),
      managerTable_->portManager(),
      &SaiPortManager::addPort);
  processDelta(
      delta.getVlansDelta(),
      managerTable_->vlanManager(),
      &SaiVlanManager::changeVlan,
      &SaiVlanManager::addVlan,
      &SaiVlanManager::removeVlan);

  // LAGs
  processDelta(
      delta.getAggregatePortsDelta(),
      managerTable_->lagManager(),
      &SaiUnsupportedFeatureManager::processChanged,
      &SaiUnsupportedFeatureManager::processAdded,
      &SaiUnsupportedFeatureManager::processRemoved);

  if (platform_->getAsic()->isSupported(HwAsic::Feature::QOS_MAP_GLOBAL)) {
    processDefaultDataPlanePolicyDelta(delta, managerTable_->switchManager());
  } else {
    processDefaultDataPlanePolicyDelta(delta, managerTable_->portManager());
  }

  processDelta(
      delta.getIntfsDelta(),
      managerTable_->routerInterfaceManager(),
      &SaiRouterInterfaceManager::changeRouterInterface,
      &SaiRouterInterfaceManager::addRouterInterface,
      &SaiRouterInterfaceManager::removeRouterInterface);

  for (const auto& vlanDelta : delta.getVlansDelta()) {
    processDelta(
        vlanDelta.getArpDelta(),
        managerTable_->neighborManager(),
        &SaiNeighborManager::changeNeighbor<ArpEntry>,
        &SaiNeighborManager::addNeighbor<ArpEntry>,
        &SaiNeighborManager::removeNeighbor<ArpEntry>);

    processDelta(
        vlanDelta.getNdpDelta(),
        managerTable_->neighborManager(),
        &SaiNeighborManager::changeNeighbor<NdpEntry>,
        &SaiNeighborManager::addNeighbor<NdpEntry>,
        &SaiNeighborManager::removeNeighbor<NdpEntry>);

    processDelta(
        vlanDelta.getMacDelta(),
        managerTable_->fdbManager(),
        &SaiFdbManager::changeMac,
        &SaiFdbManager::addMac,
        &SaiFdbManager::removeMac);
  }

  for (const auto& routeDelta : delta.getRouteTablesDelta()) {
    auto routerID = routeDelta.getOld() ? routeDelta.getOld()->getID()
                                        : routeDelta.getNew()->getID();
    processDelta(
        routeDelta.getRoutesV4Delta(),
        managerTable_->routeManager(),
        &SaiRouteManager::changeRoute<folly::IPAddressV4>,
        &SaiRouteManager::addRoute<folly::IPAddressV4>,
        &SaiRouteManager::removeRoute<folly::IPAddressV4>,
        routerID);

    processDelta(
        routeDelta.getRoutesV6Delta(),
        managerTable_->routeManager(),
        &SaiRouteManager::changeRoute<folly::IPAddressV6>,
        &SaiRouteManager::addRoute<folly::IPAddressV6>,
        &SaiRouteManager::removeRoute<folly::IPAddressV6>,
        routerID);
  }

  {
    auto controlPlaneDelta = delta.getControlPlaneDelta();
    if (controlPlaneDelta.getOld() != controlPlaneDelta.getNew()) {
      auto lock = std::lock_guard<std::mutex>(saiSwitchMutex_);
      managerTable_->hostifManager().processHostifDelta(controlPlaneDelta);
    }
  }

  processDelta(
      delta.getLabelForwardingInformationBaseDelta(),
      managerTable_->inSegEntryManager(),
      &SaiInSegEntryManager::processChangedInSegEntry,
      &SaiInSegEntryManager::processAddedInSegEntry,
      &SaiInSegEntryManager::processRemovedInSegEntry);
  processDelta(
      delta.getLoadBalancersDelta(),
      managerTable_->switchManager(),
      &SaiSwitchManager::changeLoadBalancer,
      &SaiSwitchManager::addOrUpdateLoadBalancer,
      &SaiSwitchManager::removeLoadBalancer);

  processDelta(
      delta.getAclsDelta(),
      managerTable_->aclTableManager(),
      &SaiAclTableManager::changedAclEntry,
      &SaiAclTableManager::addAclEntry,
      &SaiAclTableManager::removeAclEntry,
      kAclTable1);

  processSwitchSettingsChanged(delta);
  if (platform_->getAsic()->isSupported(
          HwAsic::Feature::RESOURCE_USAGE_STATS)) {
    auto lock = std::lock_guard<std::mutex>(saiSwitchMutex_);
    updateResourceUsageLocked(lock);
  }

  // Process link state change delta and update the LED status
  processLinkStateChangeDelta(delta);

  return delta.newState();
}

void SaiSwitch::updateResourceUsageLocked(
    const std::lock_guard<std::mutex>& /*lock*/) {
  auto aclTableHandle =
      managerTable_->aclTableManager().getAclTableHandle(kAclTable1);
  auto aclTableId = aclTableHandle->aclTable->adapterKey();
  auto& aclApi = SaiApiTable::getInstance()->aclApi();
  try {
    // TODO - compute used resource stats from internal data structures and
    // populate them here
    hwResourceStats_.acl_entries_free_ref() = aclApi.getAttribute(
        aclTableId, SaiAclTableTraits::Attributes::AvailableEntry{});
    hwResourceStats_.acl_counters_free_ref() = aclApi.getAttribute(
        aclTableId, SaiAclTableTraits::Attributes::AvailableCounter{});

    auto& switchApi = SaiApiTable::getInstance()->switchApi();
    hwResourceStats_.lpm_ipv4_free_ref() = switchApi.getAttribute(
        switchId_, SaiSwitchTraits::Attributes::AvailableIpv4RouteEntry{});
    hwResourceStats_.lpm_ipv6_free_ref() = switchApi.getAttribute(
        switchId_, SaiSwitchTraits::Attributes::AvailableIpv6RouteEntry{});
    hwResourceStats_.l3_ipv4_nexthops_free_ref() = switchApi.getAttribute(
        switchId_, SaiSwitchTraits::Attributes::AvailableIpv4NextHopEntry{});
    hwResourceStats_.l3_ipv6_nexthops_free_ref() = switchApi.getAttribute(
        switchId_, SaiSwitchTraits::Attributes::AvailableIpv6NextHopEntry{});
    hwResourceStats_.l3_ecmp_groups_free_ref() = switchApi.getAttribute(
        switchId_, SaiSwitchTraits::Attributes::AvailableNextHopGroupEntry{});
    hwResourceStats_.l3_ecmp_group_members_free_ref() = switchApi.getAttribute(
        switchId_,
        SaiSwitchTraits::Attributes::AvailableNextHopGroupMemberEntry{});
    hwResourceStats_.l3_ipv4_host_free_ref() = switchApi.getAttribute(
        switchId_, SaiSwitchTraits::Attributes::AvailableIpv4NeighborEntry{});
    hwResourceStats_.l3_ipv6_host_free_ref() = switchApi.getAttribute(
        switchId_, SaiSwitchTraits::Attributes::AvailableIpv6NeighborEntry{});
    hwResourceStats_.hw_table_stats_stale_ref() = false;
  } catch (const SaiApiError& e) {
    XLOG(ERR) << " Failed to get resource usage hwResourceStats_: "
              << *e.message_ref();
    hwResourceStats_.hw_table_stats_stale_ref() = true;
  }
}

void SaiSwitch::processSwitchSettingsChanged(const StateDelta& delta) {
  const auto switchSettingsDelta = delta.getSwitchSettingsDelta();
  const auto& oldSwitchSettings = switchSettingsDelta.getOld();
  const auto& newSwitchSettings = switchSettingsDelta.getNew();

  /*
   * SwitchSettings are mandatory and can thus only be modified.
   * Every field in SwitchSettings must always be set in new SwitchState.
   */
  CHECK(oldSwitchSettings);
  CHECK(newSwitchSettings);

  if (oldSwitchSettings != newSwitchSettings) {
    if (oldSwitchSettings->getL2LearningMode() !=
        newSwitchSettings->getL2LearningMode()) {
      // We don't allow for l2 learning mode change after initial setting. This
      // means
      // - For cold boot prohibit changes after first config application
      // - For warmboot prohibit changes after we have applied the warm boot
      // state (in init). We still need to apply the initial changes in warmboot
      // to setup our internal data structures correctly
      auto l2LearningChangeProhibitedAfter = bootType_ == BootType::WARM_BOOT
          ? SwitchRunState::INITIALIZED
          : SwitchRunState::CONFIGURED;
      if (getSwitchRunState() >= l2LearningChangeProhibitedAfter) {
        /*
         * Changing L2 learning mode on SAI is quite complex and I am
         *     inclined to not support it at all here (see prod plan below).
         * Consider,
         *
         * - On transition from SW to HW learning, we need to clear out
         * SwitchState MAC table and our FDB table of dynamic entries (to get
         * back to a state where we would be had we had HW learning mode).
         * Clearing out MAC table in SwitchState is easy enough. However we just
         * can't prune out the FDB entries in FDB manager, since that would
         * clear hw causing MACs to be unprogrammed. There are bunch of tricks
         * we can adopt to work around
         *                                  - Keep around the FDB entries but
         * mark them dead. This now complicates what we do when new FDB entries
         * get programmed with the same MAC (which we won;t know about since we
         * are now in HW learning mode) or when links go down, do we reset the
         * dead entries or not?
         *                                                      - Blow away the
         * FDB entries but blackhole writes to HW. For this we will have to
         * create a SAI blackholer since to one we have for native and block HW
         * writes. In addition we would have to block observers to FDB entry
         * reset, since we don't want ARP,NDP entries to observe this and start
         * shrinking ECMP groups. This is getting to be quite complex
         *                                                                                   -
         * On transition from HW to SW learning We need to traverse the hw FDB
         * entries and populate our MAC table. This is doable but needs a SAI
         * extension to either observe or iterate SAI FDB table updates in HW
         * learning mode. On the observer side things look clearer since we
         * would just observe what's already in HW.
         *
         * Considering all this, just prohibit transitions. For prod,
         * we will just deploy withSW L2 learning  everywhere. SW l2 learning
         * is a superset of HW l2 learning in terms of functionality, so for SAI
         * we will just standardize on that.
         */
        throw FbossError(
            "Chaging L2 learning mode after initial config "
            "application is not permitted");
      }
      auto lock = std::lock_guard<std::mutex>(saiSwitchMutex_);
      XLOG(DBG3) << "Configuring L2LearningMode old: "
                 << static_cast<int>(oldSwitchSettings->getL2LearningMode())
                 << " new: "
                 << static_cast<int>(newSwitchSettings->getL2LearningMode());
      managerTable_->portManager().setL2LearningMode(
          newSwitchSettings->getL2LearningMode());
    }
    if (newSwitchSettings->isQcmEnable()) {
      throw FbossError("QCM is not supported on SAI");
    }
  }
}

bool SaiSwitch::isValidStateUpdate(const StateDelta& delta) const {
  std::lock_guard<std::mutex> lock(saiSwitchMutex_);
  return isValidStateUpdateLocked(lock, delta);
}

std::unique_ptr<TxPacket> SaiSwitch::allocatePacket(uint32_t size) const {
  getSwitchStats()->txPktAlloc();
  return std::make_unique<SaiTxPacket>(size);
}

bool SaiSwitch::sendPacketSwitchedAsync(
    std::unique_ptr<TxPacket> pkt) noexcept {
  return sendPacketSwitchedSync(std::move(pkt));
}

bool SaiSwitch::sendPacketOutOfPortAsync(
    std::unique_ptr<TxPacket> pkt,
    PortID portID,
    std::optional<uint8_t> queueId) noexcept {
  return sendPacketOutOfPortSync(std::move(pkt), portID, queueId);
}

void SaiSwitch::updateStatsImpl(SwitchStats* /* switchStats */) {
  auto& portManager = managerTable_->portManager();
  auto iter = concurrentIndices_->portIds.begin();
  while (iter != concurrentIndices_->portIds.end()) {
    {
      std::lock_guard<std::mutex> locked(saiSwitchMutex_);
      portManager.updateStats(iter->second);
    }
    ++iter;
  }
  {
    std::lock_guard<std::mutex> locked(saiSwitchMutex_);
    managerTable_->hostifManager().updateStats();
  }
  {
    std::lock_guard<std::mutex> locked(saiSwitchMutex_);
    managerTable_->bufferManager().updateStats();
  }
  {
    std::lock_guard<std::mutex> locked(saiSwitchMutex_);
    HwResourceStatsPublisher().publish(hwResourceStats_);
  }
}

uint64_t SaiSwitch::getDeviceWatermarkBytes() const {
  std::lock_guard<std::mutex> locked(saiSwitchMutex_);
  return getDeviceWatermarkBytesLocked(locked);
}

uint64_t SaiSwitch::getDeviceWatermarkBytesLocked(
    const std::lock_guard<std::mutex>& /*lock*/) const {
  return managerTable_->bufferManager().getDeviceWatermarkBytes();
}

folly::F14FastMap<std::string, HwPortStats> SaiSwitch::getPortStats() const {
  std::lock_guard<std::mutex> lock(saiSwitchMutex_);
  return getPortStatsLocked(lock);
}

folly::F14FastMap<std::string, HwPortStats> SaiSwitch::getPortStatsLocked(
    const std::lock_guard<std::mutex>& /* lock */) const {
  folly::F14FastMap<std::string, HwPortStats> portStatsMap;
  auto& portIdStatsMap = managerTable_->portManager().getLastPortStats();
  for (auto& entry : portIdStatsMap) {
    portStatsMap.emplace(entry.second->portName(), entry.second->portStats());
  }
  return portStatsMap;
}

void SaiSwitch::fetchL2Table(std::vector<L2EntryThrift>* l2Table) const {
  std::lock_guard<std::mutex> lock(saiSwitchMutex_);
  fetchL2TableLocked(lock, l2Table);
}

void SaiSwitch::gracefulExit(folly::dynamic& switchState) {
  if (!platform_->getAsic()->isSupported(HwAsic::Feature::WARM_BOOT)) {
    XLOG(ERR) << " Asic does not support warm boot, skipping graceful exit";
    return;
  }
  std::lock_guard<std::mutex> lock(saiSwitchMutex_);
  gracefulExitLocked(switchState, lock);
}

void SaiSwitch::gracefulExitLocked(
    folly::dynamic& switchState,
    const std::lock_guard<std::mutex>& lock) {
  std::chrono::steady_clock::time_point begin =
      std::chrono::steady_clock::now();
  XLOG(INFO) << "[Exit] Starting SAI Switch graceful exit";
  SaiSwitchTraits::Attributes::SwitchRestartWarm restartWarm{true};
  SaiApiTable::getInstance()->switchApi().setAttribute(switchId_, restartWarm);
  switchState[kHwSwitch] = toFollyDynamicLocked(lock);
  platform_->getWarmBootHelper()->storeWarmBootState(switchState);
  platform_->getWarmBootHelper()->setCanWarmBoot();
  std::chrono::steady_clock::time_point wbSaiSwitchWrite =
      std::chrono::steady_clock::now();
  XLOG(INFO) << "[Exit] SaiSwitch warm boot state write time: "
             << std::chrono::duration_cast<std::chrono::duration<float>>(
                    wbSaiSwitchWrite - begin)
                    .count();
  managerTable_->switchManager().gracefulExit();
  std::chrono::steady_clock::time_point wbSdk =
      std::chrono::steady_clock::now();
  XLOG(INFO) << "[Exit] Warm boot sdk time: "
             << std::chrono::duration_cast<std::chrono::duration<float>>(
                    wbSdk - wbSaiSwitchWrite)
                    .count();
  XLOG(INFO) << " [Exit] SaiSwitch Graceful exit locked time: "
             << std::chrono::duration_cast<std::chrono::duration<float>>(
                    wbSdk - begin)
                    .count();
}

folly::dynamic SaiSwitch::toFollyDynamic() const {
  std::lock_guard<std::mutex> lock(saiSwitchMutex_);
  return toFollyDynamicLocked(lock);
}

void SaiSwitch::switchRunStateChangedImpl(SwitchRunState newState) {
  std::lock_guard<std::mutex> lock(saiSwitchMutex_);
  switchRunStateChangedImplLocked(lock, newState);
}

void SaiSwitch::exitFatal() const {
  std::lock_guard<std::mutex> lock(saiSwitchMutex_);
  exitFatalLocked(lock);
}

bool SaiSwitch::isPortUp(PortID port) const {
  std::lock_guard<std::mutex> lock(saiSwitchMutex_);
  return isPortUpLocked(lock, port);
}

bool SaiSwitch::getAndClearNeighborHit(
    RouterID /*vrf*/,
    folly::IPAddress& /*ip*/) {
  return true;
}

void SaiSwitch::clearPortStats(
    const std::unique_ptr<std::vector<int32_t>>& ports) {
  auto& portManager = managerTable_->portManager();
  for (auto port : *ports) {
    std::lock_guard<std::mutex> lock(saiSwitchMutex_);
    portManager.clearStats(static_cast<PortID>(port));
  }
}

cfg::PortSpeed SaiSwitch::getPortMaxSpeed(PortID port) const {
  std::lock_guard<std::mutex> lock(saiSwitchMutex_);
  return getPortMaxSpeedLocked(lock, port);
}

void SaiSwitch::linkStateChangedCallbackTopHalf(
    uint32_t count,
    const sai_port_oper_status_notification_t* operStatus) {
  std::vector<sai_port_oper_status_notification_t> operStatusTmp;
  operStatusTmp.resize(count);
  std::copy(operStatus, operStatus + count, operStatusTmp.data());
  linkStateBottomHalfEventBase_.runInEventBaseThread(
      [this, operStatus = std::move(operStatusTmp)]() mutable {
        linkStateChangedCallbackBottomHalf(std::move(operStatus));
      });
}

void SaiSwitch::linkStateChangedCallbackBottomHalf(
    std::vector<sai_port_oper_status_notification_t> operStatus) {
  std::map<PortID, bool> swPortId2Status;
  for (auto i = 0; i < operStatus.size(); i++) {
    bool up = operStatus[i].port_state == SAI_PORT_OPER_STATUS_UP;

    // Look up SwitchState PortID by port sai id in ConcurrentIndices
    const auto portItr =
        concurrentIndices_->portIds.find(PortSaiId(operStatus[i].port_id));
    if (portItr == concurrentIndices_->portIds.cend()) {
      XLOG(WARNING)
          << "received port notification for port with unknown sai id: "
          << operStatus[i].port_id;
      continue;
    }
    PortID swPortId = portItr->second;

    XLOGF(
        INFO,
        "Link state changed {} ({}): {}",
        swPortId,
        PortSaiId{operStatus[i].port_id},
        up ? "up" : "down");

    if (!up) {
      /*
       * Only link down are handled in the fast path. We let the
       * link up processing happen via the regular state change
       * mechanism. Reason for that is, post a link down
       * - We signal FDB entry, neighbor entry, next hop and next hop group
       *   that a link went down.
       * - Next hop group then shrinks the group based on which next hops are
       * affected.
       * - We now signal the callback (SwSwitch for wedge_agent, HwTest for hw
       * tests) for this link down state
       *    - SwSwitch in turn schedules a non coalescing port down state update
       *    - Schedules a neighbor remove state update
       * - Meanwhile, if we get a port up event, we will just signal this upto
       * the SwSwitch and not handle this is in the fast path. Reason being,
       * post a link up the link is not immediately ready for packet handling,
       * so if we expand ECMP groups in the fast path, we will see some ms of
       * traffic loss. So we let the link up processing happen via switch
       * updates, which means that it will be queued behind the link down and
       * neighbor purge. So a ECMP group reexpansion would need both a link up
       * and neighbor add state update for expansion. At this point we are
       * guaranteed to have the link be ready for packet transmission, since we
       * already resolved neighbors over that link.
       */
      std::lock_guard<std::mutex> lock{saiSwitchMutex_};
      managerTable_->fdbManager().handleLinkDown(swPortId);
    }
    swPortId2Status[swPortId] = up;
  }
  // Issue callbacks in a separate loop so fast link status change
  // processing is not at the mercy of what the callback (SwSwitch, HwTest)
  // does with the callback notification.
  for (auto swPortIdAndStatus : swPortId2Status) {
    callback_->linkStateChanged(
        swPortIdAndStatus.first, swPortIdAndStatus.second);
  }
}

BootType SaiSwitch::getBootType() const {
  std::lock_guard<std::mutex> lock(saiSwitchMutex_);
  return getBootTypeLocked(lock);
}

const SaiManagerTable* SaiSwitch::managerTable() const {
  std::lock_guard<std::mutex> lock(saiSwitchMutex_);
  return managerTableLocked(lock);
}

SaiManagerTable* SaiSwitch::managerTable() {
  std::lock_guard<std::mutex> lock(saiSwitchMutex_);
  return managerTableLocked(lock);
}

// Begin Locked functions with actual SaiSwitch functionality

std::shared_ptr<SwitchState> SaiSwitch::getColdBootSwitchState() {
  auto state = std::make_shared<SwitchState>();

  if (platform_->getAsic()->isSupported(HwAsic::Feature::QUEUE)) {
    // get cpu queue settings
    auto cpu = std::make_shared<ControlPlane>();
    auto cpuQueues = managerTable_->hostifManager().getQueueSettings();
    cpu->resetQueues(cpuQueues);
    state->resetControlPlane(cpu);
  }
  // reconstruct ports
  state->resetPorts(managerTable_->portManager().reconstructPortsFromStore());
  state->publish();
  return state;
}

HwInitResult SaiSwitch::initLocked(
    const std::lock_guard<std::mutex>& lock,
    Callback* callback) noexcept {
  HwInitResult ret;

  auto wbHelper = platform_->getWarmBootHelper();
  bootType_ =
      wbHelper->canWarmBoot() ? BootType::WARM_BOOT : BootType::COLD_BOOT;
  ret.bootType = bootType_;
  std::unique_ptr<folly::dynamic> adapterKeysJson;
  std::unique_ptr<folly::dynamic> adapterKeys2AdapterHostKeysJson;

  sai_api_initialize(0, platform_->getServiceMethodTable());
  if (bootType_ == BootType::WARM_BOOT) {
    auto switchStateJson = wbHelper->getWarmBootState();
    ret.switchState = SwitchState::fromFollyDynamic(switchStateJson[kSwSwitch]);
    ret.switchState->publish();
    if (platform_->getAsic()->isSupported(HwAsic::Feature::OBJECT_KEY_CACHE)) {
      adapterKeysJson = std::make_unique<folly::dynamic>(
          switchStateJson[kHwSwitch][kAdapterKeys]);
      const auto& switchKeysJson = (*adapterKeysJson)[saiObjectTypeToString(
          SaiSwitchTraits::ObjectType)];
      CHECK_EQ(1, switchKeysJson.size());
    }
    // adapter host keys may not be recoverable for all types of object, such
    // as next hop group.
    if (switchStateJson[kHwSwitch].find(kAdapterKey2AdapterHostKey) !=
        switchStateJson[kHwSwitch].items().end()) {
      adapterKeys2AdapterHostKeysJson = std::make_unique<folly::dynamic>(
          switchStateJson[kHwSwitch][kAdapterKey2AdapterHostKey]);
    }
  }
  SaiApiTable::getInstance()->queryApis();
  concurrentIndices_ = std::make_unique<ConcurrentIndices>();
  managerTable_ = std::make_unique<SaiManagerTable>(platform_, bootType_);
  switchId_ = managerTable_->switchManager().getSwitchSaiId();
  // TODO(borisb): find a cleaner solution to this problem.
  // perhaps reload fixes it?
  auto saiStore = SaiStore::getInstance();
  saiStore->setSwitchId(switchId_);
  saiStore->reload(
      adapterKeysJson.get(), adapterKeys2AdapterHostKeysJson.get());
  managerTable_->createSaiTableManagers(platform_, concurrentIndices_.get());
  callback_ = callback;
  __gSaiSwitch = this;
  SaiApiTable::getInstance()->enableLogging(FLAGS_enable_sai_log);
  if (bootType_ != BootType::WARM_BOOT) {
    ret.switchState = getColdBootSwitchState();
  }
  return ret;
}

void SaiSwitch::initLinkScanLocked(
    const std::lock_guard<std::mutex>& /* lock */) {
  linkStateBottomHalfThread_ = std::make_unique<std::thread>([this]() {
    initThread("fbossSaiLnkScnBH");
    linkStateBottomHalfEventBase_.loopForever();
  });
  auto& switchApi = SaiApiTable::getInstance()->switchApi();
  switchApi.registerPortStateChangeCallback(
      switchId_, __glinkStateChangedNotification);
}

void SaiSwitch::packetRxCallback(
    SwitchSaiId /* switch_id */,
    sai_size_t buffer_size,
    const void* buffer,
    uint32_t attr_count,
    const sai_attribute_t* attr_list) {
  std::optional<PortSaiId> portSaiIdOpt;
  for (uint32_t index = 0; index < attr_count; index++) {
    switch (attr_list[index].id) {
      case SAI_HOSTIF_PACKET_ATTR_INGRESS_PORT:
        portSaiIdOpt = attr_list[index].value.oid;
        break;
      case SAI_HOSTIF_PACKET_ATTR_INGRESS_LAG:
      case SAI_HOSTIF_PACKET_ATTR_HOSTIF_TRAP_ID:
        break;
      default:
        XLOG(INFO) << "invalid attribute received";
    }
  }
  CHECK(portSaiIdOpt);
  PortSaiId portSaiId{portSaiIdOpt.value()};
  PortID swPortId(0);
  VlanID swVlanId(0);
  auto rxPacket =
      std::make_unique<SaiRxPacket>(buffer_size, buffer, PortID(0), VlanID(0));
  const auto portItr = concurrentIndices_->portIds.find(portSaiId);
  /*
   * When a packet is received with source port as cpu port, do the following:
   * 1) Check if a packet has a vlan tag and only one tag. If the packet is
   * not tagged or contains multiple vlan tags, log error and return.
   * 2) If a valid vlan is found, send the packet to Sw switch for further
   * processing.
   * NOTE: The port id is set to 0 since the sw switch uses the vlan id for
   * packet processing.
   *
   * For eg:, For LL V4, IP packets are sent to the asic first. Since the ARP
   * is not resolved, the packet will be punted back to CPU with the ingress
   * port set to CPU port. These packets are handled here by finding the
   * vlan id in the packet and sending it to SwSwitch.
   *
   * We use the cached cpu port id to avoid holding manager table locks in
   * the Rx path.
   */
  if (portSaiId == getCPUPortSaiId(switchId_)) {
    folly::io::Cursor cursor(rxPacket->buf());
    EthHdr ethHdr{cursor};
    auto vlanTags = ethHdr.getVlanTags();
    if (vlanTags.size() == 1) {
      swVlanId = VlanID(vlanTags[0].vid());
      XLOG(DBG6) << "Rx packet on cpu port. "
                 << "Found vlan from packet: " << swVlanId;
    } else {
      XLOG(ERR) << "RX packet on cpu port has no vlan tag "
                << "or multiple vlan tags: 0x" << std::hex << portSaiId;
      return;
    }
  } else if (portItr == concurrentIndices_->portIds.cend()) {
    // TODO: add counter to keep track of spurious rx packet
    XLOG(ERR) << "RX packet had port with unknown sai id: 0x" << std::hex
              << portSaiId;
    return;
  } else {
    swPortId = portItr->second;
    const auto vlanItr = concurrentIndices_->vlanIds.find(portSaiId);
    if (vlanItr == concurrentIndices_->vlanIds.cend()) {
      XLOG(ERR) << "RX packet had port in no known vlan: 0x" << std::hex
                << portSaiId;
      return;
    }
    swVlanId = vlanItr->second;
  }

  /*
   * Set the correct vlan and port ID for the rx packet
   */
  rxPacket->setSrcPort(swPortId);
  rxPacket->setSrcVlan(swVlanId);

  XLOG(DBG6) << "Rx packet on port: " << swPortId << " and vlan: " << swVlanId;
  folly::io::Cursor c0(rxPacket->buf());
  XLOG(DBG6) << PktUtil::hexDump(c0);
  callback_->packetReceived(std::move(rxPacket));
}

void SaiSwitch::unregisterCallbacksLocked(
    const std::lock_guard<std::mutex>& /* lock */) noexcept {
  auto& switchApi = SaiApiTable::getInstance()->switchApi();
  /*
   * Ungregister callbacks based on the run state. fdb is registered
   * in initialized state and linkscan and packet rx callbacks are
   * registered in configured state.
   */
  if (runState_ >= SwitchRunState::CONFIGURED) {
    if (getFeaturesDesired() & FeaturesDesired::LINKSCAN_DESIRED) {
      switchApi.unregisterPortStateChangeCallback(switchId_);
    }
    if (getFeaturesDesired() & FeaturesDesired::PACKET_RX_DESIRED) {
      switchApi.unregisterRxCallback(switchId_);
    }
  }
  switchApi.unregisterFdbEventCallback(switchId_);
}

bool SaiSwitch::isValidStateUpdateLocked(
    const std::lock_guard<std::mutex>& /* lock */,
    const StateDelta& delta) const {
  auto globalQosDelta = delta.getDefaultDataPlaneQosPolicyDelta();
  if (globalQosDelta.getNew()) {
    auto& newPolicy = globalQosDelta.getNew();
    if (newPolicy->getDscpMap().empty() ||
        newPolicy->getTrafficClassToQueueId().empty()) {
      XLOG(ERR)
          << " Both DSCP to TC and TC to Queue maps must be provided in valid qos policies";
      return false;
    }
    /*
     * Not adding a check for expMap even though we don't support
     * MPLS QoS yet. Unfortunately, SwSwitch implicitly sets a exp map
     * even if the config doesn't have one. So no point in warning/failing
     * on what could be just system generated behavior.
     * TODO: see if we can stop doing this at SwSwitch layre
     */
  }

  auto qosDelta = delta.getQosPoliciesDelta();
  if (qosDelta.getNew()->size() > 0) {
    XLOG(ERR) << "Only default data plane qos policy is supported";
    return false;
  }

  return true;
}

bool SaiSwitch::sendPacketSwitchedSync(std::unique_ptr<TxPacket> pkt) noexcept {
  folly::io::Cursor cursor(pkt->buf());
  if (platform_->getAsic()->isSupported(
          HwAsic::Feature::SMAC_EQUALS_DMAC_CHECK_ENABLED)) {
    EthHdr ethHdr{cursor};
    if (ethHdr.getSrcMac() == ethHdr.getDstMac()) {
      auto* pktData = pkt->buf()->writableData();
      /* pktData[6]...pktData[11] is src mac */
      folly::MacAddress hackedMac{"fa:ce:b0:00:00:0c"};
      for (auto i = 0; i < folly::MacAddress::SIZE; i++) {
        pktData[folly::MacAddress::SIZE + i] = hackedMac.bytes()[i];
      }
      XLOG(DBG5) << "hacked packet as source and destination mac are same";
      cursor.reset(pkt->buf());
    }
  }
  getSwitchStats()->txSent();

  XLOG(DBG6) << "sending packet with pipeline look up";
  XLOG(DBG6) << PktUtil::hexDump(cursor);
  SaiTxPacketTraits::Attributes::TxType txType(
      SAI_HOSTIF_TX_TYPE_PIPELINE_LOOKUP);
  SaiTxPacketTraits::TxAttributes attributes{txType, 0, std::nullopt};
  SaiHostifApiPacket txPacket{
      reinterpret_cast<void*>(pkt->buf()->writableData()),
      pkt->buf()->length()};
  auto& hostifApi = SaiApiTable::getInstance()->hostifApi();
  auto rv = hostifApi.send(attributes, switchId_, txPacket);
  if (rv != SAI_STATUS_SUCCESS) {
    saiLogError(
        rv, SAI_API_HOSTIF, "failed to send packet with pipeline lookup");
  }
  return rv == SAI_STATUS_SUCCESS;
}

bool SaiSwitch::sendPacketOutOfPortSync(
    std::unique_ptr<TxPacket> pkt,
    PortID portID,
    std::optional<uint8_t> queueId) noexcept {
  auto portItr = concurrentIndices_->portSaiIds.find(portID);
  if (portItr == concurrentIndices_->portSaiIds.end()) {
    XLOG(ERR) << "Failed to send packet on invalid port: " << portID;
    return false;
  }
  /* TODO: this hack is required, sending packet out of port with with
  pipeline bypass, doesn't cause vlan tag stripping. fix this once a pipeline
  bypass with vlan stripping is available. */
  getSwitchStats()->txSent();
  if (platform_->getAsic()->isSupported(
          HwAsic::Feature::TX_VLAN_STRIPPING_ON_PORT)) {
    folly::io::Cursor cursor(pkt->buf());
    EthHdr ethHdr{cursor};
    if (!ethHdr.getVlanTags().empty()) {
      CHECK_EQ(ethHdr.getVlanTags().size(), 1)
          << "found more than one vlan tags while sending packet";
      /* hack to strip vlans as pipeline bypass doesn't handle this */
      cursor.reset(pkt->buf());
      XLOG(DBG5) << "strip vlan for packet";
      XLOG(DBG5) << PktUtil::hexDump(cursor);

      constexpr auto kVlanTagSize = 4;
      auto ethPayload = pkt->buf()->clone();
      // trim DA(6), SA(6) & vlan (4)
      ethPayload->trimStart(
          folly::MacAddress::SIZE + folly::MacAddress::SIZE + kVlanTagSize);

      // trim rest of packet except DA(6), SA(6)
      auto totalLength = pkt->buf()->length();
      pkt->buf()->trimEnd(
          totalLength - folly::MacAddress::SIZE - folly::MacAddress::SIZE);

      // append to trimmed ethernet header remaining payload
      pkt->buf()->appendChain(std::move(ethPayload));
      pkt->buf()->coalesce();
      cursor.reset(pkt->buf());
      XLOG(DBG5) << "stripped vlan, new packet";
      XLOG(DBG5) << PktUtil::hexDump(cursor);
    }
  }

  SaiHostifApiPacket txPacket{
      reinterpret_cast<void*>(pkt->buf()->writableData()),
      pkt->buf()->length()};

  SaiTxPacketTraits::Attributes::TxType txType(
      SAI_HOSTIF_TX_TYPE_PIPELINE_BYPASS);
  SaiTxPacketTraits::Attributes::EgressPortOrLag egressPort(portItr->second);
  SaiTxPacketTraits::Attributes::EgressQueueIndex egressQueueIndex(
      queueId.value_or(0));
  SaiTxPacketTraits::TxAttributes attributes{
      txType, egressPort, egressQueueIndex};
  auto& hostifApi = SaiApiTable::getInstance()->hostifApi();
  auto rv = hostifApi.send(attributes, switchId_, txPacket);
  if (rv != SAI_STATUS_SUCCESS) {
    saiLogError(rv, SAI_API_HOSTIF, "failed to send packet pipeline bypass");
  }
  return rv == SAI_STATUS_SUCCESS;
}

void SaiSwitch::fetchL2TableLocked(
    const std::lock_guard<std::mutex>& /* lock */,
    std::vector<L2EntryThrift>* l2Table) const {
  *l2Table = managerTable_->fdbManager().getL2Entries();
}

folly::dynamic SaiSwitch::toFollyDynamicLocked(
    const std::lock_guard<std::mutex>& /* lock */) const {
  auto adapterKeys = SaiStore::getInstance()->adapterKeysFollyDynamic();
  // Need to provide full namespace scope for toFollyDynamic to disambiguate
  // from member SaiSwitch::toFollyDynamic
  auto switchKeys = folly::dynamic::array(
      facebook::fboss::toFollyDynamic<SaiSwitchTraits>(switchId_));
  adapterKeys[saiObjectTypeToString(SaiSwitchTraits::ObjectType)] = switchKeys;

  folly::dynamic hwSwitch = folly::dynamic::object;
  hwSwitch[kAdapterKeys] = adapterKeys;
  hwSwitch[kAdapterKey2AdapterHostKey] =
      SaiStore::getInstance()->adapterKeys2AdapterHostKeysFollyDynamic();
  return hwSwitch;
}

bool SaiSwitch::isFullyInitialized() const {
  auto state = getSwitchRunState();
  return state >= SwitchRunState::INITIALIZED &&
      state != SwitchRunState::EXITING;
}
SwitchRunState SaiSwitch::getSwitchRunState() const {
  return runState_;
}

void SaiSwitch::switchRunStateChangedImplLocked(
    const std::lock_guard<std::mutex>& lock,
    SwitchRunState newState) {
  switch (newState) {
    case SwitchRunState::INITIALIZED: {
      fdbEventBottomHalfThread_ = std::make_unique<std::thread>([this]() {
        initThread("fbossSaiFdbBH");
        fdbEventBottomHalfEventBase_.loopForever();
      });
      auto& switchApi = SaiApiTable::getInstance()->switchApi();
      switchApi.registerFdbEventCallback(switchId_, __gFdbEventCallback);
    } break;
    case SwitchRunState::CONFIGURED: {
      if (getFeaturesDesired() & FeaturesDesired::LINKSCAN_DESIRED) {
        /*
         * Post warmboot synchronize hw link state with switch state
         * maintained at callback (SwSwitch or HwTest). Since the
         * callback_->linkStateChanged is called asynchronously, its possible
         * that prior to going down for warm boot there was a link event which
         * did not get communicated to up via callback_ before we received the
         * signal to do graceful exit. In such a case the switch state would
         * have a incorrect view of link state than the HW. Synchronize them.
         * Note that we dd this prior to registering our own linkscan callback
         * handler. This isn't required, but seems easier to reason about than
         * having interspersed callbacks from HW
         */
        /*
         * For cold boot, if port link state is changed before link scan is
         * registered, switch state will not reflect correct port status. as a
         * result invoke link scan call back for cold boot.
         */
        for (const auto& portIdAndHandle : managerTable_->portManager()) {
          const auto& port = portIdAndHandle.second->port;
          auto operStatus = SaiApiTable::getInstance()->portApi().getAttribute(
              port->adapterKey(), SaiPortTraits::Attributes::OperStatus{});
          callback_->linkStateChanged(
              portIdAndHandle.first, operStatus == SAI_PORT_OPER_STATUS_UP);
        }
        initLinkScanLocked(lock);
      }
      if (getFeaturesDesired() & FeaturesDesired::PACKET_RX_DESIRED) {
        auto& switchApi = SaiApiTable::getInstance()->switchApi();
        switchApi.registerRxCallback(switchId_, __gPacketRxCallback);
      }
    } break;
    default:
      break;
  }
  runState_ = newState;
}

void SaiSwitch::exitFatalLocked(
    const std::lock_guard<std::mutex>& /* lock */) const {}

bool SaiSwitch::isPortUpLocked(
    const std::lock_guard<std::mutex>& /* lock */,
    PortID /* port */) const {
  return true;
}

cfg::PortSpeed SaiSwitch::getPortMaxSpeedLocked(
    const std::lock_guard<std::mutex>& lock,
    PortID port) const {
  auto* managerTable = managerTableLocked(lock);
  return managerTable->portManager().getMaxSpeed(port);
}

BootType SaiSwitch::getBootTypeLocked(
    const std::lock_guard<std::mutex>& /* lock */) const {
  return bootType_;
}

const SaiManagerTable* SaiSwitch::managerTableLocked(
    const std::lock_guard<std::mutex>& /* lock */) const {
  return managerTable_.get();
}

SaiManagerTable* SaiSwitch::managerTableLocked(
    const std::lock_guard<std::mutex>& /* lock */) {
  return managerTable_.get();
}

void SaiSwitch::fdbEventCallback(
    uint32_t count,
    const sai_fdb_event_notification_data_t* data) {
  std::vector<sai_fdb_event_notification_data_t> fdbNotificationsTmp;
  fdbNotificationsTmp.resize(count);
  std::copy(data, data + count, fdbNotificationsTmp.data());
  fdbEventBottomHalfEventBase_.runInEventBaseThread(
      [this, fdbNotifications = std::move(fdbNotificationsTmp)]() mutable {
        auto lock = std::lock_guard<std::mutex>(saiSwitchMutex_);
        fdbEventCallbackLockedBottomHalf(lock, std::move(fdbNotifications));
      });
}

void SaiSwitch::fdbEventCallbackLockedBottomHalf(
    const std::lock_guard<std::mutex>& /*lock*/,
    std::vector<sai_fdb_event_notification_data_t> fdbNotifications) {
  if (managerTable_->portManager().getL2LearningMode() !=
      cfg::L2LearningMode::SOFTWARE) {
    // Some platforms call fdb callback even when mode is set to HW. In
    // keeping with our native SDK approach, don't send these events up.
    return;
  }
  for (const auto& fdbNotification : fdbNotifications) {
    auto ditr =
        kL2AddrUpdateOperationsOfInterest.find(fdbNotification.event_type);
    if (ditr != kL2AddrUpdateOperationsOfInterest.end()) {
      auto l2Entry = getL2Entry(fdbNotification);
      if (l2Entry) {
        callback_->l2LearningUpdateReceived(l2Entry.value(), ditr->second);
      }
    }
  }
}

std::optional<L2Entry> SaiSwitch::getL2Entry(
    const sai_fdb_event_notification_data_t& fdbEvent) const {
  /*
   * Idea behind returning a optional here on spurious FDB event
   * (say with bogus port or vlan ID) vs throwing a exception
   * is to protect the application from crashes in face of
   * spurious events. This is similar to the philosophy we follow
   * for spurious linkstate change or packet RX callbacks.
   */
  std::optional<PortSaiId> portSaiId;
  for (int i = 0; i < fdbEvent.attr_count && !portSaiId; ++i) {
    const auto attr = fdbEvent.attr;
    switch (fdbEvent.attr[i].id) {
      case SAI_FDB_ENTRY_ATTR_BRIDGE_PORT_ID:
        portSaiId = SaiApiTable::getInstance()->bridgeApi().getAttribute(
            BridgePortSaiId{attr[i].value.oid},
            SaiBridgePortTraits::Attributes::PortId{});
        break;
      default:
        break;
    }
  }
  if (!portSaiId) {
    XLOG(ERR) << "Missing port attribute in FDB event";
    return std::nullopt;
  }
  L2Entry::L2EntryType entryType{L2Entry::L2EntryType::L2_ENTRY_TYPE_PENDING};
  auto mac = fromSaiMacAddress(fdbEvent.fdb_entry.mac_address);
  switch (fdbEvent.event_type) {
    // For learning events consider entry type as pending (else the
    // fdb event should not have been punted to us)
    // For Aging events, consider the entry as already validated (programmed)
    case SAI_FDB_EVENT_LEARNED:
      entryType = L2Entry::L2EntryType::L2_ENTRY_TYPE_PENDING;
      break;
    case SAI_FDB_EVENT_AGED:
      entryType = L2Entry::L2EntryType::L2_ENTRY_TYPE_VALIDATED;
      break;
    default:
      XLOG(ERR) << "Unexpected fdb event type: " << fdbEvent.event_type
                << " for " << mac;
      return std::nullopt;
  }
  auto portItr = concurrentIndices_->portIds.find(portSaiId.value());
  if (portItr == concurrentIndices_->portIds.end()) {
    XLOG(ERR) << " FDB event for " << mac
              << ", got non existent port id : " << portSaiId.value();
    return std::nullopt;
  }
  auto vlanItr = concurrentIndices_->vlanIds.find(portSaiId.value());
  if (vlanItr == concurrentIndices_->vlanIds.end()) {
    XLOG(ERR) << " FDB event for " << mac
              << " could not look up VLAN for : " << portItr->second;
    return std::nullopt;
  }

  return L2Entry{fromSaiMacAddress(fdbEvent.fdb_entry.mac_address),
                 vlanItr->second,
                 PortDescriptor(portItr->second),
                 entryType};
}

template <
    typename Delta,
    typename Manager,
    typename... Args,
    typename ChangeFunc,
    typename AddedFunc,
    typename RemovedFunc>
void SaiSwitch::processDelta(
    Delta delta,
    Manager& manager,
    ChangeFunc changedFunc,
    AddedFunc addedFunc,
    RemovedFunc removedFunc,
    Args... args) {
  DeltaFunctions::forEachChanged(
      delta,
      [&](const std::shared_ptr<typename Delta::Node>& removed,
          const std::shared_ptr<typename Delta::Node>& added) {
        auto lock = std::lock_guard<std::mutex>(saiSwitchMutex_);
        (manager.*changedFunc)(removed, added, args...);
      },
      [&](const std::shared_ptr<typename Delta::Node>& added) {
        auto lock = std::lock_guard<std::mutex>(saiSwitchMutex_);
        (manager.*addedFunc)(added, args...);
      },
      [&](const std::shared_ptr<typename Delta::Node>& removed) {
        auto lock = std::lock_guard<std::mutex>(saiSwitchMutex_);
        (manager.*removedFunc)(removed, args...);
      });
}

template <
    typename Delta,
    typename Manager,
    typename... Args,
    typename ChangeFunc>
void SaiSwitch::processChangedDelta(
    Delta delta,
    Manager& manager,
    ChangeFunc changedFunc,
    Args... args) {
  DeltaFunctions::forEachChanged(
      delta,
      [&](const std::shared_ptr<typename Delta::Node>& added,
          const std::shared_ptr<typename Delta::Node>& removed) {
        auto lock = std::lock_guard<std::mutex>(saiSwitchMutex_);
        (manager.*changedFunc)(added, removed, args...);
      });
}

template <
    typename Delta,
    typename Manager,
    typename... Args,
    typename AddedFunc>
void SaiSwitch::processAddedDelta(
    Delta delta,
    Manager& manager,
    AddedFunc addedFunc,
    Args... args) {
  DeltaFunctions::forEachAdded(
      delta, [&](const std::shared_ptr<typename Delta::Node>& added) {
        auto lock = std::lock_guard<std::mutex>(saiSwitchMutex_);
        (manager.*addedFunc)(added, args...);
      });
}

template <
    typename Delta,
    typename Manager,
    typename... Args,
    typename RemovedFunc>
void SaiSwitch::processRemovedDelta(
    Delta delta,
    Manager& manager,
    RemovedFunc removedFunc,
    Args... args) {
  DeltaFunctions::forEachRemoved(
      delta, [&](const std::shared_ptr<typename Delta::Node>& removed) {
        auto lock = std::lock_guard<std::mutex>(saiSwitchMutex_);
        (manager.*removedFunc)(removed, args...);
      });
}

void SaiSwitch::dumpDebugState(const std::string& path) const {
  saiCheckError(sai_dbg_generate_dump(path.c_str()));
}

std::string SaiSwitch::listObjects(
    const std::vector<sai_object_type_t>& objects) const {
  std::string output;
  std::for_each(objects.begin(), objects.end(), [&output](auto objType) {
    output += SaiStore::getInstance()->storeStr(objType);
  });
  return output;
}

std::string SaiSwitch::listObjects(
    const std::vector<HwObjectType>& types) const {
  std::vector<sai_object_type_t> objTypes;
  for (auto type : types) {
    switch (type) {
      case HwObjectType::PORT:
        objTypes.push_back(SAI_OBJECT_TYPE_PORT);
        objTypes.push_back(SAI_OBJECT_TYPE_PORT_SERDES);
        break;
      case HwObjectType::LAG:
        objTypes.push_back(SAI_OBJECT_TYPE_LAG);
        objTypes.push_back(SAI_OBJECT_TYPE_LAG_MEMBER);
        break;
      case HwObjectType::VIRTUAL_ROUTER:
        objTypes.push_back(SAI_OBJECT_TYPE_VIRTUAL_ROUTER);
        break;
      case HwObjectType::NEXT_HOP:
        objTypes.push_back(SAI_OBJECT_TYPE_NEXT_HOP);
        break;
      case HwObjectType::NEXT_HOP_GROUP:
        objTypes.push_back(SAI_OBJECT_TYPE_NEXT_HOP_GROUP);
        objTypes.push_back(SAI_OBJECT_TYPE_NEXT_HOP_GROUP_MEMBER);
        break;
      case HwObjectType::ROUTER_INTERFACE:
        objTypes.push_back(SAI_OBJECT_TYPE_ROUTER_INTERFACE);
        break;
      case HwObjectType::CPU_TRAP:
        objTypes.push_back(SAI_OBJECT_TYPE_HOSTIF_TRAP_GROUP);
        objTypes.push_back(SAI_OBJECT_TYPE_HOSTIF_TRAP);
        break;
      case HwObjectType::HASH:
        objTypes.push_back(SAI_OBJECT_TYPE_HASH);
        break;
      case HwObjectType::MIRROR:
        objTypes.push_back(SAI_OBJECT_TYPE_MIRROR_SESSION);
        break;
      case HwObjectType::QOS_MAP:
        objTypes.push_back(SAI_OBJECT_TYPE_QOS_MAP);
        break;
      case HwObjectType::QUEUE:
        objTypes.push_back(SAI_OBJECT_TYPE_QUEUE);
        objTypes.push_back(SAI_OBJECT_TYPE_WRED);
        break;
      case HwObjectType::SCHEDULER:
        objTypes.push_back(SAI_OBJECT_TYPE_SCHEDULER);
        break;
      case HwObjectType::L2_ENTRY:
        objTypes.push_back(SAI_OBJECT_TYPE_FDB_ENTRY);
        break;
      case HwObjectType::NEIGHBOR_ENTRY:
        objTypes.push_back(SAI_OBJECT_TYPE_NEIGHBOR_ENTRY);
        break;
      case HwObjectType::ROUTE_ENTRY:
        objTypes.push_back(SAI_OBJECT_TYPE_ROUTE_ENTRY);
        break;
      case HwObjectType::VLAN:
        objTypes.push_back(SAI_OBJECT_TYPE_VLAN);
        objTypes.push_back(SAI_OBJECT_TYPE_VLAN_MEMBER);
        break;
      case HwObjectType::BRIDGE:
        objTypes.push_back(SAI_OBJECT_TYPE_BRIDGE);
        objTypes.push_back(SAI_OBJECT_TYPE_BRIDGE_PORT);
        break;
      case HwObjectType::BUFFER:
        objTypes.push_back(SAI_OBJECT_TYPE_BUFFER_POOL);
        objTypes.push_back(SAI_OBJECT_TYPE_BUFFER_PROFILE);
        break;
      case HwObjectType::ACL_:
        objTypes.push_back(SAI_OBJECT_TYPE_ACL_TABLE_GROUP);
        objTypes.push_back(SAI_OBJECT_TYPE_ACL_TABLE_GROUP_MEMBER);
        objTypes.push_back(SAI_OBJECT_TYPE_ACL_TABLE);
        objTypes.push_back(SAI_OBJECT_TYPE_ACL_ENTRY);
        objTypes.push_back(SAI_OBJECT_TYPE_ACL_COUNTER);
        break;
      case HwObjectType::DEBUG_COUNTER:
        objTypes.push_back(SAI_OBJECT_TYPE_DEBUG_COUNTER);
        break;
      case HwObjectType::TELEMETRY:
        objTypes.push_back(SAI_OBJECT_TYPE_TAM_REPORT);
        objTypes.push_back(SAI_OBJECT_TYPE_TAM_EVENT_ACTION);
        objTypes.push_back(SAI_OBJECT_TYPE_TAM_EVENT);
        objTypes.push_back(SAI_OBJECT_TYPE_TAM);
        break;
    }
  }
  return listObjects(objTypes);
}
} // namespace facebook::fboss
