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
#include "fboss/agent/FbossError.h"
#include "fboss/agent/LockPolicy.h"
#include "fboss/agent/Utils.h"
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/hw/HwPortFb303Stats.h"
#include "fboss/agent/hw/HwResourceStatsPublisher.h"
#include "fboss/agent/hw/HwSysPortFb303Stats.h"
#include "fboss/agent/hw/gen-cpp2/hardware_stats_types.h"
#include "fboss/agent/hw/sai/api/AclApi.h"
#include "fboss/agent/hw/sai/api/AdapterKeySerializers.h"
#include "fboss/agent/hw/sai/api/BridgeApi.h"
#include "fboss/agent/hw/sai/api/FdbApi.h"
#include "fboss/agent/hw/sai/api/HostifApi.h"
#include "fboss/agent/hw/sai/api/HwWriteBehavior.h"
#include "fboss/agent/hw/sai/api/LoggingUtil.h"
#include "fboss/agent/hw/sai/api/SaiApiTable.h"
#include "fboss/agent/hw/sai/api/SaiObjectApi.h"
#include "fboss/agent/hw/sai/api/Types.h"
#include "fboss/agent/hw/sai/store/SaiStore.h"
#include "fboss/agent/hw/sai/switch/ConcurrentIndices.h"
#include "fboss/agent/hw/sai/switch/SaiAclTableGroupManager.h"
#include "fboss/agent/hw/sai/switch/SaiAclTableManager.h"
#include "fboss/agent/hw/sai/switch/SaiBufferManager.h"
#include "fboss/agent/hw/sai/switch/SaiCounterManager.h"
#include "fboss/agent/hw/sai/switch/SaiDebugCounterManager.h"
#include "fboss/agent/hw/sai/switch/SaiHashManager.h"
#include "fboss/agent/hw/sai/switch/SaiHostifManager.h"
#include "fboss/agent/hw/sai/switch/SaiInSegEntryManager.h"
#include "fboss/agent/hw/sai/switch/SaiLagManager.h"
#include "fboss/agent/hw/sai/switch/SaiManagerTable.h"
#include "fboss/agent/hw/sai/switch/SaiMirrorManager.h"
#include "fboss/agent/hw/sai/switch/SaiNeighborManager.h"
#include "fboss/agent/hw/sai/switch/SaiNextHopGroupManager.h"
#include "fboss/agent/hw/sai/switch/SaiPortManager.h"
#include "fboss/agent/hw/sai/switch/SaiRouteManager.h"
#include "fboss/agent/hw/sai/switch/SaiRouterInterfaceManager.h"
#include "fboss/agent/hw/sai/switch/SaiRxPacket.h"
#include "fboss/agent/hw/sai/switch/SaiSwitchManager.h"
#include "fboss/agent/hw/sai/switch/SaiSystemPortManager.h"
#include "fboss/agent/hw/sai/switch/SaiTamManager.h"
#include "fboss/agent/hw/sai/switch/SaiTunnelManager.h"
#include "fboss/agent/hw/sai/switch/SaiTxPacket.h"
#include "fboss/agent/hw/sai/switch/SaiUdfManager.h"
#include "fboss/agent/hw/sai/switch/SaiVlanManager.h"
#include "fboss/agent/packet/EthHdr.h"
#include "fboss/agent/packet/PktUtil.h"
#include "fboss/agent/platforms/sai/SaiPlatform.h"

#include "fboss/agent/rib/RoutingInformationBase.h"
#include "fboss/agent/state/Port.h"
#include "fboss/agent/state/StateDelta.h"
#include "fboss/agent/state/SwitchState.h"

#include "fboss/agent/FabricReachabilityManager.h"
#include "fboss/agent/hw/HwSwitchWarmBootHelper.h"
#include "fboss/agent/hw/UnsupportedFeatureManager.h"
#include "fboss/agent/hw/switch_asics/HwAsic.h"
#include "folly/MacAddress.h"

#include "fboss/lib/phy/PhyUtils.h"
#include "fboss/lib/phy/gen-cpp2/phy_types.h"

#include <folly/logging/xlog.h>

#include <chrono>
#include <optional>

extern "C" {
#include <sai.h>
}

using std::chrono::duration_cast;
using std::chrono::seconds;
using std::chrono::system_clock;

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

DEFINE_bool(
    check_wb_handles,
    false,
    "Fail if any warm boot handles are left unclaimed.");

DECLARE_bool(enable_acl_table_group);

DEFINE_bool(
    force_recreate_acl_tables,
    false,
    "force recreate acl tables during warmboot.");

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

std::string fdbEventToString(sai_fdb_event_t event) {
  switch (event) {
    case SAI_FDB_EVENT_LEARNED:
      return "learned";
    case SAI_FDB_EVENT_AGED:
      return "aged";
    case SAI_FDB_EVENT_MOVE:
      return "moved";
    case SAI_FDB_EVENT_FLUSHED:
      return "flushed";
  }
  return "unknown";
}

static std::set<facebook::fboss::cfg::PacketRxReason> kAllowedRxReasons = {
    facebook::fboss::cfg::PacketRxReason::TTL_1};
} // namespace

namespace facebook::fboss {

// We need this global SaiSwitch* to support registering SAI callbacks
// which can then use SaiSwitch to do their work. The current callback
// facility in SAI does not support passing user data to come back
// with the callback.
// N.B., if we want to have multiple SaiSwitches in a device with multiple
// cards being managed by one instance of FBOSS, this will need to be
// extended, presumably into an array keyed by switch id.
static folly::ConcurrentHashMap<sai_object_id_t, SaiSwitch*> __gSaiIdToSwitch;

// Free functions to register as callbacks
void __gPacketRxCallback(
    sai_object_id_t switch_id,
    sai_size_t buffer_size,
    const void* buffer,
    uint32_t attr_count,
    const sai_attribute_t* attr_list) {
  __gSaiIdToSwitch[switch_id]->packetRxCallback(
      SwitchSaiId{switch_id}, buffer_size, buffer, attr_count, attr_list);
}

void __gFdbEventCallback(
    uint32_t count,
    const sai_fdb_event_notification_data_t* data) {
  if (!count) {
    return;
  }
  auto switchId = data[0].fdb_entry.switch_id;
  __gSaiIdToSwitch[switchId]->fdbEventCallback(count, data);
}
/*
 * Link, parity and TAM event notifications don't have switch id information
 * in the callback. So in that sense these are not compatible with a 1 sai
 * adapter handling multiple ASICs/SaiSwitches scenario. Luckily for our use
 * case multi chip scenario happens only in phys, where we do not setup
 * call backs. If we need to expand this to a multi switch ASIC scenario,
 * the following callbacks will need a SAI spec enhancement.
 */
void __glinkStateChangedNotification(
    uint32_t count,
    const sai_port_oper_status_notification_t* data) {
  __gSaiIdToSwitch.begin()->second->linkStateChangedCallbackTopHalf(
      count, data);
}

void __gParityErrorSwitchEventCallback(
    sai_size_t buffer_size,
    const void* buffer,
    uint32_t event_type) {
  __gSaiIdToSwitch.begin()->second->parityErrorSwitchEventCallback(
      buffer_size, buffer, event_type);
}

void __gTamEventCallback(
    sai_object_id_t tam_event_id,
    sai_size_t buffer_size,
    const void* buffer,
    uint32_t attr_count,
    const sai_attribute_t* attr_list) {
  __gSaiIdToSwitch.begin()->second->tamEventCallback(
      tam_event_id, buffer_size, buffer, attr_count, attr_list);
}

PortSaiId SaiSwitch::getCPUPortSaiId() const {
  return managerTable_->switchManager().getCpuPort();
}

SaiSwitch::SaiSwitch(SaiPlatform* platform, uint32_t featuresDesired)
    : HwSwitch(featuresDesired),
      platform_(platform),
      saiStore_(std::make_unique<SaiStore>()),
      fabricReachabilityManager_(
          std::make_unique<FabricReachabilityManager>()) {
  utilCreateDir(platform_->getVolatileStateDir());
  utilCreateDir(platform_->getPersistentStateDir());
}

SaiSwitch::~SaiSwitch() {}

HwInitResult SaiSwitch::initImpl(
    Callback* callback,
    bool failHwCallsOnWarmboot,
    cfg::SwitchType switchType,
    std::optional<int64_t> switchId) noexcept {
  asicType_ = platform_->getAsic()->getAsicType();
  bootType_ = platform_->getWarmBootHelper()->canWarmBoot()
      ? BootType::WARM_BOOT
      : BootType::COLD_BOOT;
  auto behavior{HwWriteBehavior::WRITE};
  if (bootType_ == BootType::WARM_BOOT && failHwCallsOnWarmboot &&
      platform_->getAsic()->isSupported(
          HwAsic::Feature::ZERO_SDK_WRITE_WARMBOOT)) {
    behavior = HwWriteBehavior::FAIL;
  }
  HwInitResult ret;
  {
    std::lock_guard<std::mutex> lock(saiSwitchMutex_);
    ret = initLocked(lock, behavior, callback, switchType, switchId);
  }

  {
    HwWriteBehaviorRAII writeBehavior{behavior};
    stateChangedImpl(
        StateDelta(std::make_shared<SwitchState>(), ret.switchState));
    managerTable_->fdbManager().removeUnclaimedDynanicEntries();
    managerTable_->hashManager().removeUnclaimedDefaultHash();
#if defined(SAI_VERSION_8_2_0_0_ODP) ||                                        \
    defined(SAI_VERSION_8_2_0_0_SIM_ODP) ||                                    \
    defined(SAI_VERSION_8_2_0_0_DNX_ODP) ||                                    \
    defined(SAI_VERSION_9_2_0_0_ODP) || defined(SAI_VERSION_9_0_EA_SIM_ODP) || \
    defined(SAI_VERSION_10_0_EA_DNX_SIM_ODP) ||                                \
    defined(SAI_VERSION_10_0_EA_DNX_ODP) ||                                    \
    defined(SAI_VERSION_10_0_EA_ODP) || defined(SAI_VERSION_10_0_EA_SIM_ODP)
    // TODO(zecheng): Remove after devices warmbooted to 8.2.
    managerTable_->wredManager().removeUnclaimedWredProfile();
#endif
#if SAI_API_VERSION >= SAI_VERSION(1, 10, 2)
    // Sai spec 1.10.2 introduces the new attribute of Label for Acl counter.
    // Therefore, counters created before sai spec 1.10.2 will be treated as
    // unclaimed since they have no label (and SDK gives default values to the
    // label field).
    managerTable_->aclTableManager().removeUnclaimedAclCounter();
#endif
    if (bootType_ == BootType::WARM_BOOT) {
      saiStore_->printWarmbootHandles();
      if (FLAGS_check_wb_handles == true) {
        saiStore_->checkUnexpectedUnclaimedWarmbootHandles();
      } else {
        saiStore_->removeUnexpectedUnclaimedWarmbootHandles();
      }

      /* handle the case where warm boot happened before sw switch could be
       * notified and process link down event. this retains objects such as
       * static fdb entry and neighbor. reapplying warm boot state would expand
       * ecmp group to a port that is down. rectify that as quick as possible to
       * prevent drops. */
      std::vector<sai_port_oper_status_notification_t> portStatus{};
      for (auto entry : saiStore_->get<SaiPortTraits>()) {
        auto port = entry.second.lock();
        auto portOperStatus =
            SaiApiTable::getInstance()->portApi().getAttribute(
                port->adapterKey(), SaiPortTraits::Attributes::OperStatus{});
        sai_port_oper_status_notification_t notification{};
        notification.port_id = port->adapterKey();
        notification.port_state =
            static_cast<sai_port_oper_status_t>(portOperStatus);
        portStatus.push_back(notification);
      }
      linkStateChangedCallbackBottomHalf(std::move(portStatus));
    }
  }
  return ret;
}

void SaiSwitch::printDiagCmd(const std::string& /*cmd*/) const {
  // TODO: Needs to be implemented.
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

  if (runState_ >= SwitchRunState::INITIALIZED) {
    fdbEventBottomHalfEventBase_.terminateLoopSoon();
    fdbEventBottomHalfThread_->join();
  }
}

template <typename LockPolicyT>
void SaiSwitch::processDefaultDataPlanePolicyDelta(
    const StateDelta& delta,
    const LockPolicyT& lockPolicy) {
  auto qosDelta = delta.getDefaultDataPlaneQosPolicyDelta();
  auto& qosMapManager = managerTable_->qosMapManager();
  auto& portManager = managerTable_->portManager();
  auto& switchManager = managerTable_->switchManager();
  auto& systemPortManager = managerTable_->systemPortManager();
  if ((qosDelta.getOld() != qosDelta.getNew())) {
    [[maybe_unused]] const auto& lock = lockPolicy.lock();
    if (qosDelta.getOld() && qosDelta.getNew()) {
      if (*qosDelta.getOld() != *qosDelta.getNew()) {
        portManager.clearQosPolicy();
        systemPortManager.clearQosPolicy();
        switchManager.clearQosPolicy();
        qosMapManager.removeQosMap();
        qosMapManager.addQosMap(qosDelta.getNew());
        portManager.setQosPolicy();
        systemPortManager.setQosPolicy();
        switchManager.setQosPolicy();
      }
    } else if (qosDelta.getNew()) {
      qosMapManager.addQosMap(qosDelta.getNew());
      portManager.setQosPolicy();
      systemPortManager.setQosPolicy();
      switchManager.setQosPolicy();
    } else if (qosDelta.getOld()) {
      portManager.clearQosPolicy();
      systemPortManager.clearQosPolicy();
      switchManager.clearQosPolicy();
      qosMapManager.removeQosMap();
    }
  }
}

template <typename LockPolicyT>
void SaiSwitch::processLinkStateChangeDelta(
    const StateDelta& delta,
    const LockPolicyT& lockPolicy) {
  DeltaFunctions::forEachChanged(
      delta.getPortsDelta(),
      [&](const std::shared_ptr<Port>& oldPort,
          const std::shared_ptr<Port>& newPort) {
        [[maybe_unused]] const auto& lock = lockPolicy.lock();
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

        if (oldPort->getLedPortExternalState() !=
            newPort->getLedPortExternalState()) {
          if (newPort->getLedPortExternalState().has_value()) {
            auto platformPort = platform_->getPort(id);
            platformPort->externalState(
                newPort->getLedPortExternalState().value());
          }
        }
      });
}

bool SaiSwitch::transactionsSupported() const {
  return true;
}

void SaiSwitch::rollback(
    const std::shared_ptr<SwitchState>& knownGoodState) noexcept {
  auto curBootType = getBootType();
  // Attempt rollback
  // Detailed design is in the sai_switch_transactions wiki, but at a high
  // level the steps of the rollback are 1) Clear out our internal data
  // structures (stores, managers) in SW, while throttling writes to HW 2)
  // Reinit managers and SaiStores. SaiStore* will now have all the HW state
  // 3) Replay StateDelta(emptySwitchState, delta.oldState()) to get us to
  // the pre transaction state 4) Clear out any remaining handles in
  // SaiStore to flush state left in HW due to the failed transaction Steps
  // 2-4 are exactly the same as what we do for warmboot and piggy back
  // heavily on it for both code reuse and correctness
  try {
    CoarseGrainedLockPolicy lockPolicy(saiSwitchMutex_);
    auto hwSwitchJson = toFollyDynamicLocked(lockPolicy.lock());
    {
      HwWriteBehaviorRAII writeBehavior{HwWriteBehavior::SKIP};
      managerTable_->reset(true /*skip switch manager reset*/);
    }
    // The work flow below is essentially a in memory warm boot,
    // so set the bootType to warm boot for duration of roll back. We
    // will restore it once we are done with roll back.
    bootType_ = BootType::WARM_BOOT;
    initStoreAndManagersLocked(
        lockPolicy.lock(),
        // We are being strict here in the sense of not allowing any HW
        // writes during this reinit of managers. However this does rely
        // on SaiStore being able to fetch exact state via get apis. If
        // gets are missing for some APIs we may choose to relax this to
        // WRITE as long as it does not effect forwarding. If it does
        // affect forwarding, we must fix for both transactions as well as
        // warmboot use case.
        HwWriteBehavior::FAIL,
        &hwSwitchJson[kAdapterKeys],
        &hwSwitchJson[kAdapterKey2AdapterHostKey]);
    stateChangedImplLocked(
        StateDelta(std::make_shared<SwitchState>(), knownGoodState),
        lockPolicy);
    saiStore_->printWarmbootHandles();
    saiStore_->removeUnexpectedUnclaimedWarmbootHandles();
    bootType_ = curBootType;
  } catch (const std::exception& ex) {
    // Rollback failed. Fail hard.
    XLOG(FATAL) << " Roll back failed with : " << ex.what();
  }
}

bool SaiSwitch::l2LearningModeChangeProhibited() const {
  // We don't allow for l2 learning mode change after initial setting. This
  // means
  // - For cold boot prohibit changes after first config application
  // - For warmboot prohibit changes after we have applied the warm boot
  // state (in init). We still need to apply the initial changes in warmboot
  // to setup our internal data structures correctly
  auto l2LearningChangeProhibitedAfter = bootType_ == BootType::WARM_BOOT
      ? SwitchRunState::INITIALIZED
      : SwitchRunState::CONFIGURED;
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
  return getSwitchRunState() >= l2LearningChangeProhibitedAfter;
}

std::shared_ptr<SwitchState> SaiSwitch::stateChangedImpl(
    const StateDelta& delta) {
  FineGrainedLockPolicy lockPolicy(saiSwitchMutex_);
  return stateChangedImplLocked(delta, lockPolicy);
}

template <typename LockPolicyT>
std::shared_ptr<SwitchState> SaiSwitch::stateChangedImplLocked(
    const StateDelta& delta,
    const LockPolicyT& lockPolicy) {
  // Unsupported features
  checkUnsupportedDelta(
      delta.getTeFlowEntriesDelta(), managerTable_->teFlowEntryManager());
  // update switch settings first
  processSwitchSettingsChanged(delta, lockPolicy);

  // Remove system ports (which may depend on local ports
  // before removing ports)
  processRemovedDelta(
      delta.getSystemPortsDelta(),
      managerTable_->systemPortManager(),
      lockPolicy,
      &SaiSystemPortManager::removeSystemPort);
  processRemovedDelta(
      delta.getRemoteSystemPortsDelta(),
      managerTable_->systemPortManager(),
      lockPolicy,
      &SaiSystemPortManager::removeSystemPort);
  processRemovedDelta(
      delta.getPortsDelta(),
      managerTable_->portManager(),
      lockPolicy,
      &SaiPortManager::removePort);
  processChangedDelta(
      delta.getPortsDelta(),
      managerTable_->portManager(),
      lockPolicy,
      &SaiPortManager::changePort);
  processChangedDelta(
      delta.getSystemPortsDelta(),
      managerTable_->systemPortManager(),
      lockPolicy,
      &SaiSystemPortManager::changeSystemPort);
  processChangedDelta(
      delta.getRemoteSystemPortsDelta(),
      managerTable_->systemPortManager(),
      lockPolicy,
      &SaiSystemPortManager::changeSystemPort);
  processChangedDelta(
      delta.getPortsDelta(),
      managerTable_->portManager(),
      lockPolicy,
      &SaiPortManager::loadPortQueuesForChangedPort);
  processAddedDelta(
      delta.getPortsDelta(),
      managerTable_->portManager(),
      lockPolicy,
      &SaiPortManager::addPort);
  processAddedDelta(
      delta.getSystemPortsDelta(),
      managerTable_->systemPortManager(),
      lockPolicy,
      &SaiSystemPortManager::addSystemPort);
  processAddedDelta(
      delta.getRemoteSystemPortsDelta(),
      managerTable_->systemPortManager(),
      lockPolicy,
      &SaiSystemPortManager::addSystemPort);
  processAddedDelta(
      delta.getPortsDelta(),
      managerTable_->portManager(),
      lockPolicy,
      &SaiPortManager::loadPortQueuesForAddedPort);

  // VOQ/Fabric switches require that the packets are not tagged with any VLAN.
  // Thus, no VLAN delta processing is needed for these switches
  if (!(switchType_ == cfg::SwitchType::FABRIC ||
        switchType_ == cfg::SwitchType::VOQ)) {
    processDelta(
        delta.getVlansDelta(),
        managerTable_->vlanManager(),
        lockPolicy,
        &SaiVlanManager::changeVlan,
        &SaiVlanManager::addVlan,
        &SaiVlanManager::removeVlan);
  }

  {
    // this is specific for fabric/voq switches for now
    [[maybe_unused]] const auto& lock = lockPolicy.lock();
    fabricReachabilityManager_->stateUpdated(delta);
  }

  // LAGs
  processDelta(
      delta.getAggregatePortsDelta(),
      managerTable_->lagManager(),
      lockPolicy,
      &SaiLagManager::changeLag,
      &SaiLagManager::addLag,
      &SaiLagManager::removeLag);

  if (platform_->getAsic()->isSupported(HwAsic::Feature::BRIDGE_PORT_8021Q)) {
    // Add/Change bridge ports
    DeltaFunctions::forEachChanged(
        delta.getPortsDelta(),
        [&](const std::shared_ptr<Port>& oldPort,
            const std::shared_ptr<Port>& newPort) {
          auto portID = oldPort->getID();
          [[maybe_unused]] const auto& lock = lockPolicy.lock();
          if (managerTable_->lagManager().isLagMember(portID)) {
            // if port is member of lag, ignore it
            return;
          }
          managerTable_->portManager().changeBridgePort(oldPort, newPort);
        });

    DeltaFunctions::forEachAdded(
        delta.getPortsDelta(), [&](const std::shared_ptr<Port>& newPort) {
          auto portID = newPort->getID();
          [[maybe_unused]] const auto& lock = lockPolicy.lock();
          if (managerTable_->lagManager().isLagMember(portID)) {
            // if port is member of lag, ignore it
            return;
          }
          managerTable_->portManager().addBridgePort(newPort);
        });

    DeltaFunctions::forEachChanged(
        delta.getAggregatePortsDelta(),
        [&](const std::shared_ptr<AggregatePort>& oldAggPort,
            const std::shared_ptr<AggregatePort>& newAggPort) {
          [[maybe_unused]] const auto& lock = lockPolicy.lock();
          managerTable_->lagManager().changeBridgePort(oldAggPort, newAggPort);
        });

    DeltaFunctions::forEachAdded(
        delta.getAggregatePortsDelta(),
        [&](const std::shared_ptr<AggregatePort>& newAggPort) {
          [[maybe_unused]] const auto& lock = lockPolicy.lock();
          managerTable_->lagManager().addBridgePort(newAggPort);
        });
  }
  processDefaultDataPlanePolicyDelta(delta, lockPolicy);
  processDelta(
      delta.getIntfsDelta(),
      managerTable_->routerInterfaceManager(),
      lockPolicy,
      &SaiRouterInterfaceManager::changeLocalRouterInterface,
      &SaiRouterInterfaceManager::addLocalRouterInterface,
      &SaiRouterInterfaceManager::removeLocalRouterInterface);

  processDelta(
      delta.getRemoteIntfsDelta(),
      managerTable_->routerInterfaceManager(),
      lockPolicy,
      &SaiRouterInterfaceManager::changeRemoteRouterInterface,
      &SaiRouterInterfaceManager::addRemoteRouterInterface,
      &SaiRouterInterfaceManager::removeRemoteRouterInterface);

  // For VOQ switches, neighbor tables live on port based
  // RIFs
  auto processNeighborDeltaForIntfs = [this,
                                       &lockPolicy](const auto& intfsDelta) {
    for (const auto& intfDelta : intfsDelta) {
      processDelta(
          intfDelta.getArpEntriesDelta(),
          managerTable_->neighborManager(),
          lockPolicy,
          &SaiNeighborManager::changeNeighbor<ArpEntry>,
          &SaiNeighborManager::addNeighbor<ArpEntry>,
          &SaiNeighborManager::removeNeighbor<ArpEntry>);

      processDelta(
          intfDelta.getNdpEntriesDelta(),
          managerTable_->neighborManager(),
          lockPolicy,
          &SaiNeighborManager::changeNeighbor<NdpEntry>,
          &SaiNeighborManager::addNeighbor<NdpEntry>,
          &SaiNeighborManager::removeNeighbor<NdpEntry>);
    }
  };
  processNeighborDeltaForIntfs(delta.getIntfsDelta());
  processNeighborDeltaForIntfs(delta.getRemoteIntfsDelta());
  for (const auto& vlanDelta : delta.getVlansDelta()) {
    processDelta(
        vlanDelta.getArpDelta(),
        managerTable_->neighborManager(),
        lockPolicy,
        &SaiNeighborManager::changeNeighbor<ArpEntry>,
        &SaiNeighborManager::addNeighbor<ArpEntry>,
        &SaiNeighborManager::removeNeighbor<ArpEntry>);

    processDelta(
        vlanDelta.getNdpDelta(),
        managerTable_->neighborManager(),
        lockPolicy,
        &SaiNeighborManager::changeNeighbor<NdpEntry>,
        &SaiNeighborManager::addNeighbor<NdpEntry>,
        &SaiNeighborManager::removeNeighbor<NdpEntry>);

    processDelta(
        vlanDelta.getMacDelta(),
        managerTable_->fdbManager(),
        lockPolicy,
        &SaiFdbManager::changeMac,
        &SaiFdbManager::addMac,
        &SaiFdbManager::removeMac);
  }

  auto processV4RoutesDelta = [this, &lockPolicy](
                                  RouterID rid, const auto& routesDelta) {
    processDelta(
        routesDelta,
        managerTable_->routeManager(),
        lockPolicy,
        &SaiRouteManager::changeRoute<folly::IPAddressV4>,
        &SaiRouteManager::addRoute<folly::IPAddressV4>,
        &SaiRouteManager::removeRoute<folly::IPAddressV4>,
        rid);
  };

  auto processV6RoutesDelta = [this, &lockPolicy](
                                  RouterID rid, const auto& routesDelta) {
    processDelta(
        routesDelta,
        managerTable_->routeManager(),
        lockPolicy,
        &SaiRouteManager::changeRoute<folly::IPAddressV6>,
        &SaiRouteManager::addRoute<folly::IPAddressV6>,
        &SaiRouteManager::removeRoute<folly::IPAddressV6>,
        rid);
  };

  for (const auto& routeDelta : delta.getFibsDelta()) {
    auto routerID = routeDelta.getOld() ? routeDelta.getOld()->getID()
                                        : routeDelta.getNew()->getID();
    processV4RoutesDelta(
        routerID, routeDelta.getFibDelta<folly::IPAddressV4>());
    processV6RoutesDelta(
        routerID, routeDelta.getFibDelta<folly::IPAddressV6>());
  }
  {
    auto multiSwitchControlPlaneDelta = delta.getControlPlaneDelta();
    [[maybe_unused]] const auto& lock = lockPolicy.lock();
    managerTable_->hostifManager().processHostifDelta(
        multiSwitchControlPlaneDelta);
  }

  if (platform_->getAsic()->isSupported(HwAsic::Feature::SAI_MPLS_INSEGMENT)) {
    processDelta(
        delta.getLabelForwardingInformationBaseDelta(),
        managerTable_->inSegEntryManager(),
        lockPolicy,
        &SaiInSegEntryManager::processChangedInSegEntry,
        &SaiInSegEntryManager::processAddedInSegEntry,
        &SaiInSegEntryManager::processRemovedInSegEntry);
  }

#if SAI_API_VERSION >= SAI_VERSION(1, 12, 0)
  if (platform_->getAsic()->isSupported(HwAsic::Feature::SAI_UDF_HASH)) {
    // There're several constraints for load balancer and Udf objects.
    // 1. Udf Match needs to be processed before Udf Group (create, remove and
    // changed).
    // 2. In the case of removing Udf group: load balancer needs to remove
    // association with Udf Group.
    // 3. In the case of creating load balancer: Udf group needs to be created
    // first.
    // 4. In the case of changing load balancer: a. new Udf group needs to be
    // created, b. Load balancer starts to use new Udf group, c. Remove old Udf
    // group.
    processAddedDelta(
        delta.getUdfPacketMatcherDelta(),
        managerTable_->udfManager(),
        lockPolicy,
        &SaiUdfManager::addUdfMatch);
    processAddedDelta(
        delta.getUdfGroupDelta(),
        managerTable_->udfManager(),
        lockPolicy,
        &SaiUdfManager::addUdfGroup);
    processAddedDelta(
        delta.getLoadBalancersDelta(),
        managerTable_->switchManager(),
        lockPolicy,
        &SaiSwitchManager::addOrUpdateLoadBalancer);
    // TODO(zecheng): Process Udf Match changed
    // TODO(zecheng): Process Udf Group changed
    processChangedDelta(
        delta.getLoadBalancersDelta(),
        managerTable_->switchManager(),
        lockPolicy,
        &SaiSwitchManager::changeLoadBalancer);
    processRemovedDelta(
        delta.getLoadBalancersDelta(),
        managerTable_->switchManager(),
        lockPolicy,
        &SaiSwitchManager::removeLoadBalancer);
    processRemovedDelta(
        delta.getUdfPacketMatcherDelta(),
        managerTable_->udfManager(),
        lockPolicy,
        &SaiUdfManager::removeUdfMatch);
    processRemovedDelta(
        delta.getUdfGroupDelta(),
        managerTable_->udfManager(),
        lockPolicy,
        &SaiUdfManager::removeUdfGroup);
  } else {
    processDelta(
        delta.getLoadBalancersDelta(),
        managerTable_->switchManager(),
        lockPolicy,
        &SaiSwitchManager::changeLoadBalancer,
        &SaiSwitchManager::addOrUpdateLoadBalancer,
        &SaiSwitchManager::removeLoadBalancer);
  }
#else
  processDelta(
      delta.getLoadBalancersDelta(),
      managerTable_->switchManager(),
      lockPolicy,
      &SaiSwitchManager::changeLoadBalancer,
      &SaiSwitchManager::addOrUpdateLoadBalancer,
      &SaiSwitchManager::removeLoadBalancer);
#endif

  /*
   * Add/update mirrors before processing ACL, as ACLs with action
   * INGRESS/EGRESS Mirror rely on the Mirror being created.
   */
  processDelta(
      delta.getMirrorsDelta(),
      managerTable_->mirrorManager(),
      lockPolicy,
      &SaiMirrorManager::changeMirror,
      &SaiMirrorManager::addNode,
      &SaiMirrorManager::removeMirror);

  processDelta(
      delta.getIpTunnelsDelta(),
      managerTable_->tunnelManager(),
      lockPolicy,
      &SaiTunnelManager::changeTunnel,
      &SaiTunnelManager::addTunnel,
      &SaiTunnelManager::removeTunnel);

  bool multipleAclTableSupport =
      platform_->getAsic()->isSupported(HwAsic::Feature::MULTIPLE_ACL_TABLES);
#if defined(TAJO_SDK_VERSION_1_42_1) || defined(TAJO_SDK_VERSION_1_42_8)
  multipleAclTableSupport = false;
#endif
  if (FLAGS_enable_acl_table_group && multipleAclTableSupport) {
    processDelta(
        delta.getAclTableGroupsDelta(),
        managerTable_->aclTableGroupManager(),
        lockPolicy,
        &SaiAclTableGroupManager::changedAclTableGroup,
        &SaiAclTableGroupManager::addAclTableGroup,
        &SaiAclTableGroupManager::removeAclTableGroup);

    if (delta.getAclTableGroupsDelta().getNew()) {
      // Process delta for the entries of each table in the new state
      for (const auto& [_, tableGroupMap] :
           *delta.getAclTableGroupsDelta().getNew()) {
        processAclTableGroupDelta(delta, *tableGroupMap, lockPolicy);
      }
    }
  } else {
    std::set<cfg::AclTableQualifier> oldRequiredQualifiers{};
    std::set<cfg::AclTableQualifier> newRequiredQualifiers{};
    if (delta.getAclsDelta().getOld()) {
      oldRequiredQualifiers =
          delta.getAclsDelta().getOld()->requiredQualifiers();
    }
    if (delta.getAclsDelta().getNew()) {
      newRequiredQualifiers =
          delta.getAclsDelta().getNew()->requiredQualifiers();
    }
    bool aclTableUpdateSupport = platform_->getAsic()->isSupported(
        HwAsic::Feature::SAI_ACL_TABLE_UPDATE);
#if defined(TAJO_SDK_VERSION_1_42_1) || defined(TAJO_SDK_VERSION_1_42_8)
    aclTableUpdateSupport = false;
#endif
    if (!oldRequiredQualifiers.empty() &&
        oldRequiredQualifiers != newRequiredQualifiers &&
        aclTableUpdateSupport &&
        !managerTable_->aclTableManager()
             .areQualifiersSupportedInDefaultAclTable(newRequiredQualifiers)) {
      // qualifiers changed and default acl table doesn't support all of them,
      // remove default acl table and add a new one. table removal should
      // clear acl entries too
      managerTable_->switchManager().resetIngressAcl();
      managerTable_->aclTableManager().removeDefaultAclTable();
      managerTable_->aclTableManager().addDefaultAclTable();
      managerTable_->switchManager().setIngressAcl();
    }

    processDelta(
        delta.getAclsDelta(),
        managerTable_->aclTableManager(),
        lockPolicy,
        &SaiAclTableManager::changedAclEntry,
        &SaiAclTableManager::addAclEntry,
        &SaiAclTableManager::removeAclEntry,
        kAclTable1);
  }

  if (platform_->getAsic()->isSupported(
          HwAsic::Feature::RESOURCE_USAGE_STATS)) {
    updateResourceUsage(lockPolicy);
  }

  // Process link state change delta and update the LED status
  processLinkStateChangeDelta(delta, lockPolicy);

  return delta.newState();
}

template <typename LockPolicyT>
void SaiSwitch::updateResourceUsage(const LockPolicyT& lockPolicy) {
  [[maybe_unused]] const auto& lock = lockPolicy.lock();

  try {
    // TODO - compute used resource stats from internal data structures and
    // populate them here

    // TODO(skhare) Add resource usage support for multiple ACL tables
    if (!FLAGS_enable_acl_table_group) {
      auto aclTableHandle =
          managerTable_->aclTableManager().getAclTableHandle(kAclTable1);
      auto aclTableId = aclTableHandle->aclTable->adapterKey();
      auto& aclApi = SaiApiTable::getInstance()->aclApi();

      hwResourceStats_.acl_entries_free() = aclApi.getAttribute(
          aclTableId, SaiAclTableTraits::Attributes::AvailableEntry{});
      hwResourceStats_.acl_counters_free() = aclApi.getAttribute(
          aclTableId, SaiAclTableTraits::Attributes::AvailableCounter{});
    }

    auto& switchApi = SaiApiTable::getInstance()->switchApi();
    hwResourceStats_.lpm_ipv4_free() = switchApi.getAttribute(
        switchId_, SaiSwitchTraits::Attributes::AvailableIpv4RouteEntry{});
    hwResourceStats_.lpm_ipv6_free() = switchApi.getAttribute(
        switchId_, SaiSwitchTraits::Attributes::AvailableIpv6RouteEntry{});
    hwResourceStats_.l3_ipv4_nexthops_free() = switchApi.getAttribute(
        switchId_, SaiSwitchTraits::Attributes::AvailableIpv4NextHopEntry{});
    hwResourceStats_.l3_ipv6_nexthops_free() = switchApi.getAttribute(
        switchId_, SaiSwitchTraits::Attributes::AvailableIpv6NextHopEntry{});
    hwResourceStats_.l3_ecmp_groups_free() = switchApi.getAttribute(
        switchId_, SaiSwitchTraits::Attributes::AvailableNextHopGroupEntry{});
    hwResourceStats_.l3_ecmp_group_members_free() = switchApi.getAttribute(
        switchId_,
        SaiSwitchTraits::Attributes::AvailableNextHopGroupMemberEntry{});
    hwResourceStats_.l3_ipv4_host_free() = switchApi.getAttribute(
        switchId_, SaiSwitchTraits::Attributes::AvailableIpv4NeighborEntry{});
    hwResourceStats_.l3_ipv6_host_free() = switchApi.getAttribute(
        switchId_, SaiSwitchTraits::Attributes::AvailableIpv6NeighborEntry{});
    hwResourceStats_.hw_table_stats_stale() = false;
  } catch (const SaiApiError& e) {
    XLOG(ERR) << " Failed to get resource usage hwResourceStats_: "
              << *e.message();
    hwResourceStats_.hw_table_stats_stale() = true;
  }
}

void SaiSwitch::processSwitchSettingsChangedLocked(
    const std::lock_guard<std::mutex>& lock,
    const StateDelta& delta) {
  const auto switchSettingDelta = delta.getSwitchSettingsDelta();
  DeltaFunctions::forEachAdded(
      switchSettingDelta, [&](const auto& newSwitchSettings) {
        processSwitchSettingsChangedEntryLocked(
            lock, std::make_shared<SwitchSettings>(), newSwitchSettings);
      });
  DeltaFunctions::forEachChanged(
      switchSettingDelta,
      [&](const auto& oldSwitchSettings, const auto& newSwitchSettings) {
        processSwitchSettingsChangedEntryLocked(
            lock, oldSwitchSettings, newSwitchSettings);
      });
  DeltaFunctions::forEachRemoved(
      switchSettingDelta, [&](const auto& oldSwitchSettings) {
        processSwitchSettingsChangedEntryLocked(
            lock, oldSwitchSettings, std::make_shared<SwitchSettings>());
      });
}

void SaiSwitch::processSwitchSettingsChangedEntryLocked(
    const std::lock_guard<std::mutex>& /*lock*/,
    const std::shared_ptr<SwitchSettings>& oldSwitchSettings,
    const std::shared_ptr<SwitchSettings>& newSwitchSettings) {
  if (oldSwitchSettings->getL2LearningMode() !=
      newSwitchSettings->getL2LearningMode()) {
    XLOG(DBG3) << "Configuring L2LearningMode old: "
               << static_cast<int>(oldSwitchSettings->getL2LearningMode())
               << " new: "
               << static_cast<int>(newSwitchSettings->getL2LearningMode());
    managerTable_->bridgeManager().setL2LearningMode(
        newSwitchSettings->getL2LearningMode());
  }

  {
    const auto oldVal = oldSwitchSettings->getL2AgeTimerSeconds();
    const auto newVal = newSwitchSettings->getL2AgeTimerSeconds();
    if (oldVal != newVal) {
      XLOG(DBG3) << "Configuring l2AgeTimerSeconds old: "
                 << static_cast<int>(oldVal)
                 << " new: " << static_cast<int>(newVal);
      managerTable_->switchManager().setMacAgingSeconds(newVal);
    }
  }

  {
    const auto oldVal = oldSwitchSettings->isPtpTcEnable();
    const auto newVal = newSwitchSettings->isPtpTcEnable();
    if (oldVal != newVal) {
      XLOG(DBG3) << "Configuring ptpTcEnable old: " << oldVal
                 << " new: " << newVal;
      // update already added ports
      managerTable_->portManager().setPtpTcEnable(newVal);
      // cache the new status, used if the ports are not added yet
      managerTable_->switchManager().setPtpTcEnabled(newVal);
    }
  }

  if (oldSwitchSettings->getMaxRouteCounterIDs() !=
      newSwitchSettings->getMaxRouteCounterIDs()) {
    managerTable_->counterManager().setMaxRouteCounterIDs(
        newSwitchSettings->getMaxRouteCounterIDs());
  }

  const auto oldVal = oldSwitchSettings->isSwitchDrained();
  const auto newVal = newSwitchSettings->isSwitchDrained();
  if (oldVal != newVal) {
    managerTable_->switchManager().setSwitchIsolate(newVal);
  }
}

template <typename LockPolicyT>
void SaiSwitch::processSwitchSettingsChanged(
    const StateDelta& delta,
    const LockPolicyT& lockPolicy) {
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
    processSwitchSettingsChangedLocked(lockPolicy.lock(), delta);
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

CpuPortStats SaiSwitch::getCpuPortStats() const {
  std::lock_guard<std::mutex> lock(saiSwitchMutex_);
  auto& cpuStat = managerTable_->hostifManager().getCpuFb303Stats();
  return cpuStat.getCpuPortStats();
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

std::map<std::string, HwSysPortStats> SaiSwitch::getSysPortStats() const {
  std::lock_guard<std::mutex> lock(saiSwitchMutex_);
  return getSysPortStatsLocked(lock);
}

FabricReachabilityStats SaiSwitch::getFabricReachabilityStats() const {
  std::lock_guard<std::mutex> lock(saiSwitchMutex_);
  return getSwitchStats()->getFabricReachabilityStats();
}

std::map<std::string, HwSysPortStats> SaiSwitch::getSysPortStatsLocked(
    const std::lock_guard<std::mutex>& /* lock */) const {
  std::map<std::string, HwSysPortStats> portStatsMap;
  auto& portIdStatsMap = managerTable_->systemPortManager().getLastPortStats();
  for (auto& entry : portIdStatsMap) {
    portStatsMap.emplace(entry.second->portName(), entry.second->portStats());
  }
  return portStatsMap;
}

std::map<PortID, phy::PhyInfo> SaiSwitch::updateAllPhyInfo() {
  std::lock_guard<std::mutex> lock(saiSwitchMutex_);
  return updateAllPhyInfoLocked();
}

std::map<PortID, phy::PhyInfo> SaiSwitch::updateAllPhyInfoLocked() {
  std::map<PortID, phy::PhyInfo> returnPhyParams;
  auto& portManager = managerTable_->portManager();

  for (const auto& portIdAndHandle : managerTable_->portManager()) {
    PortID swPort = portIdAndHandle.first;
    if (portManager.getPortType(swPort) == cfg::PortType::RECYCLE_PORT) {
      continue;
    }

    auto portHandle = portIdAndHandle.second.get();
    if (portHandle == nullptr) {
      XLOG(DBG3) << "PortHandle not found for port "
                 << static_cast<int>(swPort);
      continue;
    }

    auto fb303PortStat = portManager.getLastPortStat(swPort);
    if (fb303PortStat == nullptr) {
      XLOG(DBG3) << "fb303PortStat not found for port "
                 << static_cast<int>(swPort);
      continue;
    }

    phy::PhyInfo lastPhyInfo;
    lastPhyInfo.phyChip().ensure();
    lastPhyInfo.line().ensure();
    if (auto itr = lastPhyInfos_.find(swPort); itr != lastPhyInfos_.end()) {
      lastPhyInfo = itr->second;
    }

    phy::PhyInfo phyParams;
    phyParams.phyChip().ensure();
    phyParams.line().ensure();

    phyParams.state() = phy::PhyState();
    phyParams.stats() = phy::PhyStats();
    // LINE Side always exists
    phyParams.state()->line()->side() = phy::Side::LINE;
    phyParams.stats()->line()->side() = phy::Side::LINE;

    phyParams.name() = fb303PortStat->portName();
    phyParams.state()->name() = *phyParams.name();
    phyParams.switchID() = getSaiSwitchId();
    phyParams.state()->switchID() = *phyParams.switchID();
    // Global phy parameters
    phy::DataPlanePhyChip phyChip;
    auto chipType = getPlatform()->getAsic()->getDataPlanePhyChipType();
    phyChip.type() = chipType;
    bool isXphy = *phyChip.type() == phy::DataPlanePhyChipType::XPHY;
    phyParams.phyChip() = phyChip;
    phyParams.state()->phyChip() = *phyParams.phyChip();
    phyParams.linkState() = portManager.isUp(swPort);
    phyParams.state()->linkState() = *phyParams.linkState();
    phyParams.speed() = portManager.getSpeed(swPort);
    phyParams.state()->speed() = *phyParams.speed();
    phyParams.line()->side() = phy::Side::LINE;

    if (isXphy) {
      phyParams.system() = phy::PhySideInfo();
      phyParams.system()->side() = phy::Side::SYSTEM;

      phyParams.state()->system() = phy::PhySideState();
      phyParams.state()->system()->side() = phy::Side::SYSTEM;
      phyParams.stats()->system() = phy::PhySideStats();
      phyParams.stats()->system()->side() = phy::Side::SYSTEM;
    }

    phyParams.line()->interfaceType() = getInterfaceType(swPort, chipType);
    phyParams.state()->line()->interfaceType() =
        *phyParams.line()->interfaceType();
    phyParams.line()->medium() = portManager.getMedium(swPort);
    phyParams.state()->line()->medium() = *phyParams.line()->medium();
    // Update PMD Info
    phy::PmdInfo lastLinePmdInfo = *lastPhyInfo.line()->pmd();
    phy::PmdState lastLinePmdState;
    if (auto lastState = lastPhyInfo.state()) {
      lastLinePmdState = *lastState->line()->pmd();
    }
    phy::PmdStats lastLinePmdStats;
    if (auto lastStats = lastPhyInfo.stats()) {
      lastLinePmdStats = *lastStats->line()->pmd();
    }
    updatePmdInfo(
        *phyParams.line(),
        *phyParams.state()->line(),
        *phyParams.stats()->line(),
        portHandle->port,
        lastLinePmdInfo,
        lastLinePmdState,
        lastLinePmdStats);
    if (isXphy) {
      CHECK(phyParams.system().has_value());
      CHECK(phyParams.state()->system().has_value());
      CHECK(phyParams.stats()->system().has_value());
      phy::PmdInfo lastSysPmdInfo;
      phy::PmdState lastSysPmdState;
      phy::PmdStats lastSysPmdStats;
      if (auto lastSys = lastPhyInfo.system()) {
        lastSysPmdInfo = *lastSys->pmd();
      }
      if (lastPhyInfo.state().has_value() &&
          lastPhyInfo.state()->system().has_value()) {
        lastSysPmdState = *lastPhyInfo.state()->system()->pmd();
      }
      if (lastPhyInfo.stats().has_value() &&
          lastPhyInfo.stats()->system().has_value()) {
        lastSysPmdStats = *lastPhyInfo.stats()->system()->pmd();
      }
      updatePmdInfo(
          *phyParams.system(),
          *phyParams.state()->system(),
          *phyParams.stats()->system(),
          portHandle->sysPort,
          lastSysPmdInfo,
          lastSysPmdState,
          lastSysPmdStats);
    }

    // Update PCS Info
    updatePcsInfo(
        *phyParams.line(),
        *(*phyParams.state()).line(),
        *(*phyParams.stats()).line(),
        swPort,
        phy::Side::LINE,
        lastPhyInfo,
        fb303PortStat,
        *phyParams.speed(),
        portHandle->port);

    // Update Reconciliation Sublayer (RS) Info
    updateRsInfo(
        *phyParams.line(), *phyParams.state()->line(), portHandle->port);

    // PhyInfo update timestamp
    auto now = duration_cast<seconds>(system_clock::now().time_since_epoch());
    phyParams.timeCollected() = now.count();
    phyParams.state()->timeCollected() = now.count();
    phyParams.stats()->timeCollected() = now.count();
    returnPhyParams[swPort] = phyParams;
  }
  lastPhyInfos_ = returnPhyParams;
  return returnPhyParams;
}

void SaiSwitch::updatePmdInfo(
    phy::PhySideInfo& sideInfo,
    phy::PhySideState& sideState,
    phy::PhySideStats& sideStats,
    std::shared_ptr<SaiPort> port,
    [[maybe_unused]] phy::PmdInfo& lastPmdInfo,
    [[maybe_unused]] phy::PmdState& lastPmdState,
    [[maybe_unused]] phy::PmdStats& lastPmdStats) {
  uint32_t numPmdLanes;
  if (platform_->getAsic()->isSupported(
          HwAsic::Feature::SAI_PORT_GET_PMD_LANES)) {
    // HwLaneList might mean physical port list instead of pmd lane list on TH4
    // So, use getNumPmdLanes() to get the number of pmd lanes
    numPmdLanes =
        managerTable_->portManager().getNumPmdLanes(port->adapterKey());
  } else {
    numPmdLanes = GET_ATTR(Port, HwLaneList, port->attributes()).size();
  }
  if (!numPmdLanes) {
    return;
  }

  std::map<int, phy::LaneInfo> laneInfos;
  std::map<int, phy::LaneStats> laneStats;
  std::map<int, phy::LaneState> laneStates;
  auto eyeStatus =
      managerTable_->portManager().getPortEyeValues(port->adapterKey());
  // Collect eyeInfos for all lanes
  std::map<int, std::vector<phy::EyeInfo>> eyeInfos;
  for (int i = 0; i < eyeStatus.size(); i++) {
    std::vector<phy::EyeInfo> eyeInfo;
    phy::EyeInfo oneLaneEyeInfo;
    int width{0}, height{0};
    if (getPlatform()->getAsic()->getAsicType() ==
        cfg::AsicType::ASIC_TYPE_ELBERT_8DD) {
      width = eyeStatus[i].right - eyeStatus[i].left;
      height = eyeStatus[i].up - eyeStatus[i].down;
    } else {
      width = eyeStatus[i].right + eyeStatus[i].left;
      height = eyeStatus[i].up + eyeStatus[i].down;
    }
    // Skip if both width and height are 0, it's likely a bad reading or the
    // eye reporting is not supported (On BCM PHYs eye is not supported for
    // PAM4 speeds)
    if (width == 0 && height == 0) {
      continue;
    }
    oneLaneEyeInfo.height() = height;
    oneLaneEyeInfo.width() = width;
    eyeInfos[eyeStatus[i].lane].push_back(oneLaneEyeInfo);
  }

  for (auto eyeInfo : eyeInfos) {
    auto laneId = eyeInfo.first;
    phy::LaneInfo laneInfo;
    phy::LaneStats laneStat;
    if (laneInfos.find(laneId) != laneInfos.end()) {
      laneInfo = laneInfos[laneId];
    }
    if (laneStats.find(laneId) != laneStats.end()) {
      laneStat = laneStats[laneId];
    }
    laneInfo.lane() = laneId;
    laneInfo.eyes() = eyeInfo.second;
    laneInfos[laneId] = laneInfo;

    laneStat.lane() = laneId;
    laneStat.eyes() = eyeInfo.second;
    laneStats[laneId] = laneStat;
  }

#if SAI_API_VERSION >= SAI_VERSION(1, 10, 3) || defined(TAJO_SDK_VERSION_1_42_8)
  auto pmdSignalDetect = managerTable_->portManager().getRxSignalDetect(
      port->adapterKey(), numPmdLanes);
  for (auto pmd : pmdSignalDetect) {
    auto laneId = pmd.lane;
    phy::LaneInfo laneInfo;
    phy::LaneStats laneStat;
    phy::LaneState laneState;
    if (laneInfos.find(laneId) != laneInfos.end()) {
      laneInfo = laneInfos[laneId];
    }
    if (laneStats.find(laneId) != laneStats.end()) {
      laneStat = laneStats[laneId];
    }
    if (laneStates.find(laneId) != laneStates.end()) {
      laneState = laneStates[laneId];
    }
    laneState.lane() = laneId;
    laneStat.lane() = laneId;
    laneInfo.signalDetectLive() = pmd.value.current_status;
    laneInfo.signalDetectChanged() = pmd.value.changed;
    laneState.signalDetectLive() = pmd.value.current_status;
    laneState.signalDetectChanged() = pmd.value.changed;
    utility::updateSignalDetectChangedCount(
        pmd.value.changed, laneId, laneInfo, lastPmdInfo);
    utility::updateSignalDetectChangedCount(
        pmd.value.changed, laneId, laneStat, lastPmdStats);
    laneInfos[laneId] = laneInfo;
    laneStats[laneId] = laneStat;
    laneStates[laneId] = laneState;
  }

  auto pmdLockStatus = managerTable_->portManager().getRxLockStatus(
      port->adapterKey(), numPmdLanes);
  for (auto pmd : pmdLockStatus) {
    auto laneId = pmd.lane;
    phy::LaneInfo laneInfo;
    phy::LaneStats laneStat;
    phy::LaneState laneState;
    if (laneInfos.find(laneId) != laneInfos.end()) {
      laneInfo = laneInfos[laneId];
    }
    if (laneStats.find(laneId) != laneStats.end()) {
      laneStat = laneStats[laneId];
    }
    if (laneStates.find(laneId) != laneStates.end()) {
      laneState = laneStates[laneId];
    }
    laneState.lane() = laneId;
    laneStat.lane() = laneId;
    laneInfo.cdrLockLive() = pmd.value.current_status;
    laneState.cdrLockLive() = pmd.value.current_status;
    laneInfo.cdrLockChanged() = pmd.value.changed;
    laneState.cdrLockChanged() = pmd.value.changed;
    utility::updateCdrLockChangedCount(
        pmd.value.changed, laneId, laneInfo, lastPmdInfo);
    utility::updateCdrLockChangedCount(
        pmd.value.changed, laneId, laneStat, lastPmdStats);
    laneInfos[laneId] = laneInfo;
    laneStats[laneId] = laneStat;
    laneStates[laneId] = laneState;
  }
#endif

  for (auto laneInfo : laneInfos) {
    sideInfo.pmd()->lanes()[laneInfo.first] = laneInfo.second;
  }
  for (auto laneStat : laneStats) {
    sideStats.pmd()->lanes()[laneStat.first] = laneStat.second;
  }
  for (auto laneState : laneStates) {
    sideState.pmd()->lanes()[laneState.first] = laneState.second;
  }
}

void SaiSwitch::updatePcsInfo(
    phy::PhySideInfo& sideInfo,
    phy::PhySideState& sideState,
    phy::PhySideStats& sideStats,
    PortID swPort,
    phy::Side side,
    phy::PhyInfo& lastPhyInfo,
    const HwPortFb303Stats* fb303PortStat,
    cfg::PortSpeed speed,
    std::shared_ptr<SaiPort> port) {
  auto fecMode = getPortFECMode(swPort);

  phy::PcsState pcsState;
#if SAI_API_VERSION >= SAI_VERSION(1, 10, 3) || defined(TAJO_SDK_VERSION_1_42_8)
  if (auto pcsLinkStatus =
          managerTable_->portManager().getPcsRxLinkStatus(port->adapterKey())) {
    pcsState.pcsRxStatusLive() = pcsLinkStatus->current_status;
    pcsState.pcsRxStatusLatched() = pcsLinkStatus->changed;
  }
#endif

  if (utility::isReedSolomonFec(fecMode)) {
    phy::PcsStats pcsStats;
    phy::PcsInfo pcsInfo;
    phy::RsFecInfo rsFec;

#if SAI_API_VERSION >= SAI_VERSION(1, 10, 3) || defined(TAJO_SDK_VERSION_1_42_8)
    auto fecLanes = utility::reedSolomonFecLanes(speed);
    auto fecAmLock = managerTable_->portManager().getFecAlignmentLockStatus(
        port->adapterKey(), fecLanes);
    phy::RsFecState fecState;
    for (auto fecAm : fecAmLock) {
      // SDKs sometimes return data for more FEC lanes than the FEC block on the
      // port actually uses
      if (fecAm.lane < fecLanes) {
        phy::RsFecLaneState fecLaneState;
        fecLaneState.lane() = fecAm.lane;
        fecLaneState.fecAlignmentLockLive() = fecAm.value.current_status;
        fecLaneState.fecAlignmentLockChanged() = fecAm.value.changed;

        fecState.lanes()[fecAm.lane] = fecLaneState;
      }
    }
    pcsState.rsFecState() = fecState;
#endif

    rsFec.correctedCodewords() =
        *(fb303PortStat->portStats().fecCorrectableErrors());
    rsFec.uncorrectedCodewords() =
        *(fb303PortStat->portStats().fecUncorrectableErrors());

    phy::RsFecInfo lastRsFec;
    std::optional<phy::PcsInfo> lastPcs;
    if (side == phy::Side::LINE) {
      if (auto pcs = lastPhyInfo.line()->pcs()) {
        lastPcs = *pcs;
      }
    } else if (auto sysSide = lastPhyInfo.system()) {
      if (sysSide->pcs().has_value()) {
        lastPcs = *sysSide->pcs();
      }
    }
    if (lastPcs.has_value() && lastPcs->rsFec().has_value()) {
      lastRsFec = *lastPcs->rsFec();
    }

    auto now = duration_cast<seconds>(system_clock::now().time_since_epoch());
    utility::updateCorrectedBitsAndPreFECBer(
        rsFec, /* current RsFecInfo to update */
        lastRsFec, /* previous RsFecInfo */
        std::nullopt, /* counter not available from hardware */
        now.count() - *lastPhyInfo.timeCollected(), /* timeDeltaInSeconds */
        fecMode, /* operational FecMode */
        speed /* operational Speed */);
    pcsInfo.rsFec() = rsFec;
    pcsStats.rsFec() = rsFec;
    sideInfo.pcs() = pcsInfo;
    sideStats.pcs() = pcsStats;
  }

  sideState.pcs() = pcsState;
}

void SaiSwitch::updateRsInfo(
    phy::PhySideInfo& sideInfo,
    phy::PhySideState& sideState,
    std::shared_ptr<SaiPort> port) {
  auto errStatus =
      managerTable_->portManager().getPortErrStatus(port->adapterKey());
  phy::LinkFaultStatus faultStatus;
  for (auto& err : errStatus) {
    if (err == SAI_PORT_ERR_STATUS_SIGNAL_LOCAL_ERROR) {
      faultStatus.localFault() = true;
    } else if (err == SAI_PORT_ERR_STATUS_REMOTE_FAULT_STATUS) {
      faultStatus.remoteFault() = true;
    }
  }
  if (*faultStatus.localFault() || *faultStatus.remoteFault()) {
    phy::RsInfo rsInfo;
    rsInfo.faultStatus() = faultStatus;
    sideInfo.rs() = rsInfo;
    sideState.rs() = rsInfo;
  }
}

std::map<PortID, FabricEndpoint> SaiSwitch::getFabricReachability() const {
  std::lock_guard<std::mutex> lock(saiSwitchMutex_);
  return getFabricReachabilityLocked();
}

std::map<PortID, FabricEndpoint> SaiSwitch::getFabricReachabilityLocked()
    const {
  return fabricReachabilityManager_->getReachabilityInfo();
}

std::vector<PortID> SaiSwitch::getSwitchReachability(SwitchID switchId) const {
  std::lock_guard<std::mutex> lock(saiSwitchMutex_);
  return getSwitchReachabilityLocked(switchId);
}

std::vector<PortID> SaiSwitch::getSwitchReachabilityLocked(
    SwitchID switchId) const {
  return managerTable_->portManager().getFabricReachabilityForSwitch(switchId);
}

void SaiSwitch::fetchL2Table(std::vector<L2EntryThrift>* l2Table) const {
  std::lock_guard<std::mutex> lock(saiSwitchMutex_);
  fetchL2TableLocked(lock, l2Table);
}

void SaiSwitch::gracefulExitImpl(
    const state::WarmbootState& thriftSwitchState) {
  std::lock_guard<std::mutex> lock(saiSwitchMutex_);
  gracefulExitLocked(lock, thriftSwitchState);
}

void SaiSwitch::gracefulExitLocked(
    const std::lock_guard<std::mutex>& lock,
    const state::WarmbootState& thriftSwitchState) {
  std::chrono::steady_clock::time_point begin =
      std::chrono::steady_clock::now();
  XLOG(DBG2) << "[Exit] Starting SAI Switch graceful exit";

  SaiSwitchTraits::Attributes::SwitchRestartWarm restartWarm{true};
  SaiApiTable::getInstance()->switchApi().setAttribute(switchId_, restartWarm);
  if (platform_->getAsic()->isSupported(HwAsic::Feature::P4_WARMBOOT)) {
#if defined(TAJO_SDK_VERSION_1_62_0) || defined(TAJO_SDK_VERSION_1_65_0)
    SaiSwitchTraits::Attributes::RestartIssu restartIssu{true};
    SaiApiTable::getInstance()->switchApi().setAttribute(
        switchId_, restartIssu);
#endif
  }
#if defined(TAJO_SDK_VERSION_1_42_8)
  checkAndSetSdkDowngradeVersion();
#endif
  folly::dynamic follySwitchState = folly::dynamic::object;
  follySwitchState[kHwSwitch] = toFollyDynamicLocked(lock);
  platform_->getWarmBootHelper()->storeWarmBootState(
      follySwitchState, thriftSwitchState);
  platform_->getWarmBootHelper()->setCanWarmBoot();
  std::chrono::steady_clock::time_point wbSaiSwitchWrite =
      std::chrono::steady_clock::now();
  XLOG(DBG2) << "[Exit] SaiSwitch warm boot state write time: "
             << std::chrono::duration_cast<std::chrono::duration<float>>(
                    wbSaiSwitchWrite - begin)
                    .count();
  managerTable_->switchManager().gracefulExit();
  std::chrono::steady_clock::time_point wbSdk =
      std::chrono::steady_clock::now();
  XLOG(DBG2) << "[Exit] Warm boot sdk time: "
             << std::chrono::duration_cast<std::chrono::duration<float>>(
                    wbSdk - wbSaiSwitchWrite)
                    .count();
  XLOG(DBG2) << " [Exit] SaiSwitch Graceful exit locked time: "
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

    std::optional<AggregatePortID> swAggPort{};
    const auto aggrItr = concurrentIndices_->memberPort2AggregatePortIds.find(
        PortSaiId(operStatus[i].port_id));
    if (aggrItr != concurrentIndices_->memberPort2AggregatePortIds.end()) {
      swAggPort = aggrItr->second;
    }

    XLOGF(
        DBG2,
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
       *    - SwSwitch in turn schedules a non coalescing port down state
       * update
       *    - Schedules a neighbor remove state update
       * - Meanwhile, if we get a port up event, we will just signal this upto
       * the SwSwitch and not handle this is in the fast path. Reason being,
       * post a link up the link is not immediately ready for packet handling,
       * so if we expand ECMP groups in the fast path, we will see some ms of
       * traffic loss. So we let the link up processing happen via switch
       * updates, which means that it will be queued behind the link down and
       * neighbor purge. So a ECMP group reexpansion would need both a link up
       * and neighbor add state update for expansion. At this point we are
       * guaranteed to have the link be ready for packet transmission, since
       * we already resolved neighbors over that link.
       */
      std::lock_guard<std::mutex> lock{saiSwitchMutex_};
      if (swAggPort) {
        // member of lag is gone down. unbundle it from LAG
        // once link comes back up LACP engine in SwSwitch will bundle it
        // again
        managerTable_->lagManager().disableMember(swAggPort.value(), swPortId);
        if (!managerTable_->lagManager().isMinimumLinkMet(swAggPort.value())) {
          // remove fdb entries on LAG, this would remove neighbors, next hops
          // will point to drop and next hop group will shrink.
          managerTable_->fdbManager().handleLinkDown(
              SaiPortDescriptor(swAggPort.value()));
        }
      }
      managerTable_->fdbManager().handleLinkDown(SaiPortDescriptor(swPortId));
      if (getPlatform()->getAsic()->getSwitchType() == cfg::SwitchType::VOQ) {
        // On VOQ switches, there are not FDB entries. Rather we only have
        // L3 ports which in turn are associated with RIFs or type (system)
        // port. Neighbors then tied to these RIFs. So we need to directly
        // signal Port RIF neighbors of a corresponding link going down.
        managerTable_->neighborManager().handleLinkDown(
            SaiPortDescriptor(swPortId));
      }
      /*
       * Enable AFE adaptive mode (S249471) on TAJO platforms when a port
       * flaps
       */
      if (asicType_ == cfg::AsicType::ASIC_TYPE_EBRO) {
        managerTable_->portManager().enableAfeAdaptiveMode(swPortId);
      }
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
  return bootType_;
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
  auto state = getProgrammedState()->clone();

  auto switchId = platform_->getAsic()->getSwitchId()
      ? *platform_->getAsic()->getSwitchId()
      : 0;
  auto matcher =
      HwSwitchMatcher(std::unordered_set<SwitchID>({SwitchID(switchId)}));
  auto scopeResolver = platform_->scopeResolver();
  if (platform_->getAsic()->isSupported(HwAsic::Feature::CPU_PORT)) {
    // get cpu queue settings
    auto cpu = std::make_shared<ControlPlane>();
    auto cpuQueues = managerTable_->hostifManager().getQueueSettings();
    cpu->resetQueues(cpuQueues);
    auto multiSwitchControlPlane = std::make_shared<MultiControlPlane>();
    multiSwitchControlPlane->addNode(
        scopeResolver->scope(cpu).matcherString(), cpu);
    state->resetControlPlane(multiSwitchControlPlane);
  }
  if (platform_->getAsic()->isSupported(HwAsic::Feature::FABRIC_PORTS)) {
    if (switchType_ == cfg::SwitchType::FABRIC ||
        switchType_ == cfg::SwitchType::VOQ) {
      auto& switchApi = SaiApiTable::getInstance()->switchApi();
      auto fabricPorts = switchApi.getAttribute(
          switchId_, SaiSwitchTraits::Attributes::FabricPortList{});
      auto& portStore = saiStore_->get<SaiPortTraits>();
      for (auto& fid : fabricPorts) {
        // Add to warm boot handles so object has a reference and
        // is preserved in Port store
        portStore.loadObjectOwnedByAdapter(
            PortSaiId(fid), true /* add to warm boot handles*/);
      }
    }
  }
  // TODO(joseph5wu) We need to design how to restore xphy ports for the state
  // Temporarily skip resetPorts for ASIC_TYPE_ELBERT_8DD
  if (platform_->getAsic()->getAsicType() !=
      cfg::AsicType::ASIC_TYPE_ELBERT_8DD) {
    auto switchId = platform_->getAsic()->getSwitchId()
        ? *platform_->getAsic()->getSwitchId()
        : 0;
    HwSwitchMatcher matcher(std::unordered_set<SwitchID>({SwitchID(switchId)}));
    // reconstruct ports
    auto portMaps =
        managerTable_->portManager().reconstructPortsFromStore(switchType_);
    auto ports = std::make_shared<MultiSwitchPortMap>();
    if (FLAGS_hide_fabric_ports) {
      for (const auto& portMap : std::as_const(*portMaps)) {
        for (const auto& [id, port] : std::as_const(*portMap.second)) {
          if (port->getPortType() != cfg::PortType::FABRIC_PORT) {
            ports->addNode(port, scopeResolver->scope(port));
          }
        }
      }
    } else {
      ports = std::move(portMaps);
    }
    state->resetPorts(ports);
  }

  // For VOQ switch, create system ports for existing egress ports
  if (switchType_ == cfg::SwitchType::VOQ) {
    CHECK(getSwitchId().has_value());
    auto multiSysPorts = std::make_shared<MultiSwitchSystemPortMap>();
    auto sysPorts = managerTable_->systemPortManager().constructSystemPorts(
        state->getPorts(),
        getSwitchId().value(),
        platform_->getAsic()->getSystemPortRange());

    for (auto iter : std::as_const(*sysPorts)) {
      multiSysPorts->addNode(iter.second, scopeResolver->scope(iter.second));
    }
    state->resetSystemPorts(multiSysPorts);
  }

  auto multiSwitchSwitchSettings = std::make_shared<MultiSwitchSettings>();
  auto switchSettings = std::make_shared<SwitchSettings>();
  switchSettings->setSwitchIdToSwitchInfo(
      scopeResolver->switchIdToSwitchInfo());
  multiSwitchSwitchSettings->addNode(
      scopeResolver->scope(switchSettings).matcherString(), switchSettings);
  state->resetSwitchSettings(multiSwitchSwitchSettings);
  return state;
}

HwInitResult SaiSwitch::initLocked(
    const std::lock_guard<std::mutex>& lock,
    HwWriteBehavior behavior,
    Callback* callback,
    cfg::SwitchType switchType,
    std::optional<int64_t> switchId) noexcept {
  HwInitResult ret;
  switchType_ = switchType;
  ret.rib = std::make_unique<RoutingInformationBase>();
  ret.bootType = bootType_;
  std::unique_ptr<folly::dynamic> adapterKeysJson;
  std::unique_ptr<folly::dynamic> adapterKeys2AdapterHostKeysJson;

  concurrentIndices_ = std::make_unique<ConcurrentIndices>();
  managerTable_ = std::make_unique<SaiManagerTable>(
      platform_, bootType_, switchType, switchId);
  switchId_ = managerTable_->switchManager().getSwitchSaiId();
  callback_ = callback;
  __gSaiIdToSwitch.insert_or_assign(switchId_, this);
  SaiApiTable::getInstance()->enableLogging(FLAGS_enable_sai_log);
  if (bootType_ == BootType::WARM_BOOT) {
    auto [switchStateJson, switchStateThrift] =
        platform_->getWarmBootHelper()->getWarmBootState();
    if (switchStateThrift) {
      try {
        ret.switchState =
            SwitchState::fromThrift(*switchStateThrift->swSwitchState());
      } catch (const FbossError& error) {
        if (!dumpBinaryThriftToFile(
                platform_->getCrashThriftSwitchStateFile(),
                *switchStateThrift->swSwitchState())) {
          XLOG(ERR) << "failed to dump switch state to file: "
                    << getPlatform()->getCrashThriftSwitchStateFile();
        } else {
          XLOG(DBG2) << "dumped switch state to file: "
                     << getPlatform()->getCrashThriftSwitchStateFile();
        }
        XLOG(FATAL) << "Failed to recover switch state from thrift. "
                    << error.what();
      }
    } else {
      XLOG(FATAL) << "Thrift switch state not found";
    }
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
    const auto& routeTables = *(switchStateThrift->routeTables());
    if (!routeTables.empty()) {
      ret.rib = RoutingInformationBase::fromThrift(
          routeTables,
          ret.switchState->getFibs(),
          ret.switchState->getLabelForwardingInformationBase());
    }
  }
  initStoreAndManagersLocked(
      lock,
      behavior,
      adapterKeysJson.get(),
      adapterKeys2AdapterHostKeysJson.get());
  if (bootType_ != BootType::WARM_BOOT) {
    setProgrammedState(std::make_shared<SwitchState>());
    ret.switchState = getColdBootSwitchState();
    CHECK(ret.switchState->getSwitchSettings()->size());
    if (getPlatform()->getAsic()->isSupported(HwAsic::Feature::MAC_AGING)) {
      managerTable_->switchManager().setMacAgingSeconds(
          ret.switchState->getSwitchSettings()
              ->cbegin()
              ->second->getL2AgeTimerSeconds());
    }
  }
  if (getPlatform()->getAsic()->isSupported(HwAsic::Feature::L2_LEARNING)) {
    // for both cold and warm boot, recover l2 learning mode
    CHECK(ret.switchState->getSwitchSettings()->size());
    managerTable_->bridgeManager().setL2LearningMode(
        ret.switchState->getSwitchSettings()
            ->cbegin()
            ->second->getL2LearningMode());
  }

  ret.switchState->publish();
  return ret;
}

void SaiSwitch::initStoreAndManagersLocked(
    const std::lock_guard<std::mutex>& /*lock*/,
    HwWriteBehavior behavior,
    const folly::dynamic* adapterKeys,
    const folly::dynamic* adapterKeys2AdapterHostKeys) {
  saiStore_->setSwitchId(switchId_);
  saiStore_->reload(adapterKeys, adapterKeys2AdapterHostKeys);
  managerTable_->createSaiTableManagers(
      saiStore_.get(), platform_, concurrentIndices_.get());
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
  {
    HwWriteBehaviorRAII writeBehavior{behavior};
    if (getPlatform()->getAsic()->isSupported(
            HwAsic::Feature::ACL_TABLE_GROUP)) {
      if (!FLAGS_enable_acl_table_group) {
        auto aclTableGroup =
            std::make_shared<AclTableGroup>(cfg::AclStage::INGRESS);
        managerTable_->aclTableGroupManager().addAclTableGroup(aclTableGroup);

        managerTable_->aclTableManager().addDefaultAclTable();
      }
    }

    if (getPlatform()->getAsic()->isSupported(HwAsic::Feature::DEBUG_COUNTER)) {
      managerTable_->debugCounterManager().setupDebugCounters();
    }
    if (platform_->getAsic()->isSupported(HwAsic::Feature::BUFFER_POOL)) {
      managerTable_->bufferManager().setupBufferPool();
    }
    if (platform_->getAsic()->isSupported(
            HwAsic::Feature::COUNTER_REFRESH_INTERVAL)) {
      managerTable_->switchManager().setupCounterRefreshInterval();
    }
  }
} // namespace facebook::fboss

void SaiSwitch::initLinkScanLocked(
    const std::lock_guard<std::mutex>& /* lock */) {
  linkStateBottomHalfThread_ = std::make_unique<std::thread>([this]() {
    initThread("fbossSaiLnkScnBH");
    linkStateBottomHalfEventBase_.loopForever();
  });
  linkStateBottomHalfEventBase_.runInEventBaseThread([=]() {
    auto& switchApi = SaiApiTable::getInstance()->switchApi();
    switchApi.registerPortStateChangeCallback(
        switchId_, __glinkStateChangedNotification);

    /* report initial link status after registering link scan call back.  link
     * state changes after reporting initial link state and before registering
     * link scan callback, could get lost. to mitigate this, register link scan
     * callback before reporting initial link state. doing this in context of
     * same thread/event base ensures initial link state always preceedes any
     * link state changes that may happen after registering link scan call back
     * are reported in order. */
    for (const auto& portIdAndHandle : managerTable_->portManager()) {
      const auto& port = portIdAndHandle.second->port;
      auto operStatus = SaiApiTable::getInstance()->portApi().getAttribute(
          port->adapterKey(), SaiPortTraits::Attributes::OperStatus{});
      callback_->linkStateChanged(
          portIdAndHandle.first, operStatus == SAI_PORT_OPER_STATUS_UP);
    }
  });
}

bool SaiSwitch::isMissingSrcPortAllowed(HostifTrapSaiId hostifTrapSaiId) {
  static std::set<facebook::fboss::cfg::PacketRxReason> kAllowedRxReasons = {
      facebook::fboss::cfg::PacketRxReason::TTL_1};
  const auto hostifTrapItr =
      concurrentIndices_->hostifTrapIds.find(hostifTrapSaiId);
  if (hostifTrapItr == concurrentIndices_->hostifTrapIds.cend()) {
    return false;
  }
  return (
      kAllowedRxReasons.find(hostifTrapItr->second) != kAllowedRxReasons.end());
}

// If the SAI impl supports reading intf type, get it from portManager
// otherwise just return the value from our platform config.
phy::InterfaceType SaiSwitch::getInterfaceType(
    PortID portID,
    phy::DataPlanePhyChipType chipType) const {
  if (platform_->getAsic()->isSupported(HwAsic::Feature::PORT_INTERFACE_TYPE)) {
    return managerTable_->portManager().getInterfaceType(portID);
  }

  auto platPort = platform_->getPort(portID);
  auto profileConfig =
      platPort->getPortProfileConfig(platPort->getCurrentProfile());
  if (chipType == phy::DataPlanePhyChipType::IPHY) {
    if (!profileConfig.iphy()->interfaceType()) {
      XLOG(WARNING) << "No interfaceType set for iphy on port " << portID;
      return phy::InterfaceType::NONE;
    }
    return *profileConfig.iphy()->interfaceType();
  }
  if (chipType == phy::DataPlanePhyChipType::XPHY) {
    // TODO: we should eventually support system side stat collection for xphy
    // as well.
    if (!profileConfig.xphyLine() ||
        !profileConfig.xphyLine()->interfaceType()) {
      XLOG(WARNING) << "No interfaceType set for xphy on port " << portID;
      return phy::InterfaceType::NONE;
    }
    return *profileConfig.xphyLine()->interfaceType();
  }
  XLOG(WARNING) << "Invalid chip type " << static_cast<int>(chipType)
                << " found for port " << portID;

  return phy::InterfaceType::NONE;
}

void SaiSwitch::packetRxCallback(
    SwitchSaiId /* switch_id */,
    sai_size_t buffer_size,
    const void* buffer,
    uint32_t attr_count,
    const sai_attribute_t* attr_list) {
  std::optional<PortSaiId> portSaiIdOpt;
  std::optional<LagSaiId> lagSaiIdOpt;
  std::optional<HostifTrapSaiId> hostifTrapSaiIdOpt;
  for (uint32_t index = 0; index < attr_count; index++) {
    switch (attr_list[index].id) {
      case SAI_HOSTIF_PACKET_ATTR_INGRESS_PORT:
        portSaiIdOpt = attr_list[index].value.oid;
        break;
      case SAI_HOSTIF_PACKET_ATTR_INGRESS_LAG:
        lagSaiIdOpt = attr_list[index].value.oid;
        break;
      case SAI_HOSTIF_PACKET_ATTR_HOSTIF_TRAP_ID:
        hostifTrapSaiIdOpt = attr_list[index].value.oid;
        break;
      default:
        XLOG(DBG2) << "invalid attribute received";
    }
  }

  /*
   * for Tajo, the TTL 1 processing is done at egress and hence the
   * packet received to CPU does not carry source port attribute. Hence
   * set the source port as 0 and derive the vlan tag from the packet
   * and send it to sw switch for processing.
   */
  bool allowMissingSrcPort = hostifTrapSaiIdOpt.has_value() &&
      platform_->getAsic()->getAsicType() == cfg::AsicType::ASIC_TYPE_EBRO &&
      isMissingSrcPortAllowed(hostifTrapSaiIdOpt.value());

  if (!portSaiIdOpt && !allowMissingSrcPort) {
    XLOG(DBG5)
        << "discarded a packet with missing SAI_HOSTIF_PACKET_ATTR_INGRESS_PORT.";
    return;
  }

  if (portSaiIdOpt.has_value()) {
    auto iter = concurrentIndices_->memberPort2AggregatePortIds.find(
        portSaiIdOpt.value());

    if (iter != concurrentIndices_->memberPort2AggregatePortIds.end()) {
      // hack if SAI_HOSTIF_PACKET_ATTR_INGRESS_LAG is not set on packet on lag!
      // if port belongs to some aggregate port, process packet as if coming
      // from lag.
      auto [portSaiId, aggregatePortId] = *iter;
      std::ignore = portSaiId;
      for (auto entry : concurrentIndices_->aggregatePortIds) {
        auto [lagSaiId, swLagId] = entry;
        if (swLagId == aggregatePortId) {
          lagSaiIdOpt = lagSaiId;
          break;
        }
      }
    }
  }

  auto portSaiId = portSaiIdOpt.has_value() ? portSaiIdOpt.value()
                                            : PortSaiId(SAI_NULL_OBJECT_ID);
  auto packetRxReason = cfg::PacketRxReason::UNMATCHED;
  if (hostifTrapSaiIdOpt.has_value()) {
    const auto hostifTrapItr =
        concurrentIndices_->hostifTrapIds.find(hostifTrapSaiIdOpt.value());
    if (hostifTrapItr != concurrentIndices_->hostifTrapIds.cend()) {
      packetRxReason = hostifTrapItr->second;
    }
  }

  if (!lagSaiIdOpt) {
    packetRxCallbackPort(
        buffer_size, buffer, portSaiId, allowMissingSrcPort, packetRxReason);
  } else {
    packetRxCallbackLag(
        buffer_size,
        buffer,
        lagSaiIdOpt.value(),
        portSaiId,
        allowMissingSrcPort,
        packetRxReason);
  }
}

void SaiSwitch::packetRxCallbackPort(
    sai_size_t buffer_size,
    const void* buffer,
    PortSaiId portSaiId,
    bool allowMissingSrcPort,
    cfg::PacketRxReason rxReason) {
  PortID swPortId(0);
  std::optional<VlanID> swVlanId = (switchType_ == cfg::SwitchType::VOQ ||
                                    switchType_ == cfg::SwitchType::FABRIC)
      ? std::nullopt
      : std::make_optional(VlanID(0));
  auto swVlanIdStr = [swVlanId]() {
    return swVlanId.has_value()
        ? folly::to<std::string>(static_cast<int>(swVlanId.value()))
        : "None";
  };

  auto rxPacket = std::make_unique<SaiRxPacket>(
      buffer_size, buffer, PortID(0), VlanID(0), rxReason);
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
  if (!(switchType_ == cfg::SwitchType::VOQ ||
        switchType_ == cfg::SwitchType::FABRIC)) {
    if (portSaiId == getCPUPortSaiId() ||
        (allowMissingSrcPort &&
         portItr == concurrentIndices_->portIds.cend())) {
      folly::io::Cursor cursor(rxPacket->buf());
      EthHdr ethHdr{cursor};
      auto vlanTags = ethHdr.getVlanTags();
      if (vlanTags.size() == 1) {
        swVlanId = VlanID(vlanTags[0].vid());
        XLOG(DBG6) << "Rx packet on cpu port. "
                   << "Found vlan from packet: " << swVlanIdStr();
      } else {
        XLOG(ERR) << "RX packet on cpu port has no vlan tag "
                  << "or multiple vlan tags: 0x" << std::hex << portSaiId;
        return;
      }
    } else if (portItr == concurrentIndices_->portIds.cend()) {
      // TODO: add counter to keep track of spurious rx packet
      XLOG(DBG) << "RX packet had port with unknown sai id: 0x" << std::hex
                << portSaiId;
      return;
    } else {
      swPortId = portItr->second;
      const auto vlanItr =
          concurrentIndices_->vlanIds.find(PortDescriptorSaiId(portSaiId));
      if (vlanItr == concurrentIndices_->vlanIds.cend()) {
        XLOG(ERR) << "RX packet had port in no known vlan: 0x" << std::hex
                  << portSaiId;
        return;
      }
      swVlanId = vlanItr->second;
    }
  } else { // VOQ / FABRIC
    if (portSaiId != getCPUPortSaiId()) {
      if (portItr == concurrentIndices_->portIds.cend()) {
        // TODO: add counter to keep track of spurious rx packet
        XLOG(ERR) << "RX packet had port with unknown sai id: 0x" << std::hex
                  << portSaiId;
        return;
      } else {
        swPortId = portItr->second;
        XLOG(DBG6) << "VOQ RX packet with sai id: 0x" << std::hex << portSaiId
                   << " portID: " << swPortId;
      }
    }
  }

  /*
   * Set the correct vlan and port ID for the rx packet
   */
  rxPacket->setSrcPort(swPortId);
  rxPacket->setSrcVlan(swVlanId);

  XLOG(DBG6) << "Rx packet on port: " << swPortId << " vlan: " << swVlanIdStr()
             << " trap: " << packetRxReasonToString(rxReason);

  folly::io::Cursor c0(rxPacket->buf());
  XLOG(DBG6) << PktUtil::hexDump(c0);
  callback_->packetReceived(std::move(rxPacket));
}

void SaiSwitch::packetRxCallbackLag(
    sai_size_t buffer_size,
    const void* buffer,
    LagSaiId lagSaiId,
    PortSaiId portSaiId,
    bool allowMissingSrcPort,
    cfg::PacketRxReason rxReason) {
  AggregatePortID swAggPortId(0);
  PortID swPortId(0);
  VlanID swVlanId(0);
  auto rxPacket = std::make_unique<SaiRxPacket>(
      buffer_size, buffer, PortID(0), VlanID(0), rxReason);

  const auto aggPortItr = concurrentIndices_->aggregatePortIds.find(lagSaiId);

  if (aggPortItr == concurrentIndices_->aggregatePortIds.end()) {
    // TODO: add counter to keep track of spurious rx packet
    XLOG(ERR) << "RX packet had lag with unknown sai id: 0x" << std::hex
              << lagSaiId;
    return;
  }
  swAggPortId = aggPortItr->second;
  const auto vlanItr =
      concurrentIndices_->vlanIds.find(PortDescriptorSaiId(lagSaiId));
  if (vlanItr == concurrentIndices_->vlanIds.cend()) {
    XLOG(ERR) << "RX packet had lag in no known vlan: 0x" << std::hex
              << lagSaiId;
    return;
  }
  swVlanId = vlanItr->second;
  rxPacket->setSrcAggregatePort(swAggPortId);
  rxPacket->setSrcVlan(swVlanId);

  auto swPortItr = concurrentIndices_->portIds.find(portSaiId);
  if (swPortItr == concurrentIndices_->portIds.cend() && !allowMissingSrcPort) {
    XLOG(ERR) << "RX packet for lag has invalid port : 0x" << std::hex
              << portSaiId;
    return;
  }
  swPortId = swPortItr->second;
  rxPacket->setSrcPort(swPortId);
  XLOG(DBG6) << "Rx packet on lag: " << swAggPortId << ", port: " << swPortId
             << " vlan: " << swVlanId
             << " trap: " << packetRxReasonToString(rxReason);
  folly::io::Cursor c0(rxPacket->buf());
  XLOG(DBG6) << PktUtil::hexDump(c0);
  callback_->packetReceived(std::move(rxPacket));
}

bool SaiSwitch::isFeatureSetupLocked(
    FeaturesDesired feature,
    const std::lock_guard<std::mutex>& /*lock*/) const {
  if (!(feature & getFeaturesDesired())) {
    return false;
  }
  bool isConfigured = runState_ >= SwitchRunState::CONFIGURED;
  bool isInitialized = runState_ >= SwitchRunState::INITIALIZED;
  if (feature == FeaturesDesired::LINKSCAN_DESIRED) {
    return isConfigured || (bootType_ == BootType::WARM_BOOT && isInitialized);
  }
  if (feature == FeaturesDesired::PACKET_RX_DESIRED) {
    return isConfigured || (bootType_ == BootType::WARM_BOOT && isInitialized);
  }
  if (feature == FeaturesDesired::TAM_EVENT_NOTIFY_DESIRED) {
    return isConfigured;
  }
  CHECK(false) << " Unhandled feature " << feature;
  return false;
}

void SaiSwitch::unregisterCallbacksLocked(
    const std::lock_guard<std::mutex>& lock) noexcept {
  auto& switchApi = SaiApiTable::getInstance()->switchApi();
  if (isFeatureSetupLocked(FeaturesDesired::LINKSCAN_DESIRED, lock)) {
    switchApi.unregisterPortStateChangeCallback(switchId_);
  }
  if (isFeatureSetupLocked(FeaturesDesired::PACKET_RX_DESIRED, lock)) {
    switchApi.unregisterRxCallback(switchId_);
  }
  if (isFeatureSetupLocked(FeaturesDesired::TAM_EVENT_NOTIFY_DESIRED, lock)) {
#if defined(SAI_VERSION_7_2_0_0_ODP) || defined(SAI_VERSION_8_2_0_0_ODP) ||    \
    defined(SAI_VERSION_8_2_0_0_SIM_ODP) ||                                    \
    defined(SAI_VERSION_8_2_0_0_DNX_ODP) ||                                    \
    defined(SAI_VERSION_9_2_0_0_ODP) || defined(SAI_VERSION_9_0_EA_SIM_ODP) || \
    defined(SAI_VERSION_10_0_EA_DNX_SIM_ODP) ||                                \
    defined(SAI_VERSION_10_0_EA_DNX_ODP) ||                                    \
    defined(SAI_VERSION_10_0_EA_ODP) || defined(SAI_VERSION_10_0_EA_SIM_ODP)
    switchApi.unregisterParityErrorSwitchEventCallback(switchId_);
#else
    switchApi.unregisterTamEventCallback(switchId_);
#endif
  }
  switchApi.unregisterFdbEventCallback(switchId_);
}

bool SaiSwitch::isValidStateUpdateLocked(
    const std::lock_guard<std::mutex>& /* lock */,
    const StateDelta& delta) const {
  auto globalQosDelta = delta.getDefaultDataPlaneQosPolicyDelta();
  auto isValid = true;
  if (globalQosDelta.getNew()) {
    auto& newPolicy = globalQosDelta.getNew();
    if (newPolicy->getDscpMap()->get<switch_state_tags::from>()->size() == 0 ||
        newPolicy->getTrafficClassToQueueId()->size() == 0) {
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
  if (qosDelta.getNew()->numNodes() > 0) {
    XLOG(ERR) << "Only default data plane qos policy is supported";
    return false;
  }

  if (delta.oldState()->getSwitchSettings()->size() &&
      delta.newState()->getSwitchSettings()->empty()) {
    throw FbossError("Switch settings cannot be removed from SwitchState");
  }

  DeltaFunctions::forEachChanged(
      delta.getSwitchSettingsDelta(),
      [&](const std::shared_ptr<SwitchSettings>& oldSwitchSettings,
          const std::shared_ptr<SwitchSettings>& newSwitchSettings) {
        if (*oldSwitchSettings != *newSwitchSettings) {
          if (oldSwitchSettings->getL2LearningMode() !=
              newSwitchSettings->getL2LearningMode()) {
            if (l2LearningModeChangeProhibited()) {
              throw FbossError(
                  "Chaging L2 learning mode after initial config "
                  "application is not permitted");
            }
          }
        }
        if (newSwitchSettings->isQcmEnable()) {
          throw FbossError("QCM is not supported on SAI");
        }
      });

  if (delta.newState()->getMirrors()->numNodes() >
      getPlatform()->getAsic()->getMaxMirrors()) {
    XLOG(ERR) << "Number of mirrors configured is high on this platform";
    return false;
  }

  DeltaFunctions::forEachChanged(
      delta.getMirrorsDelta(),
      [&](const std::shared_ptr<Mirror>& /* oldMirror */,
          const std::shared_ptr<Mirror>& newMirror) {
        if (newMirror->getTruncate() &&
            !getPlatform()->getAsic()->isSupported(
                HwAsic::Feature::MIRROR_PACKET_TRUNCATION)) {
          XLOG(ERR)
              << "Mirror packet truncation is not supported on this platform";
          isValid = false;
        }
      });

  auto qualifiersSupported =
      managerTable_->aclTableManager().getSupportedQualifierSet();
  DeltaFunctions::forEachChanged(
      delta.getAclsDelta(),
      [qualifiersSupported, &isValid](
          const std::shared_ptr<AclEntry>& /*oldEntry*/,
          const std::shared_ptr<AclEntry>& newEntry) {
        auto qualifiersRequired = newEntry->getRequiredAclTableQualifiers();
        std::vector<cfg::AclTableQualifier> difference{
            qualifiersRequired.size()};
        auto iter = std::set_difference(
            qualifiersRequired.begin(),
            qualifiersRequired.end(),
            qualifiersSupported.begin(),
            qualifiersSupported.end(),
            difference.begin());
        difference.resize(iter - difference.begin());
        if (!difference.empty()) {
          XLOG(ERR) << "Invalid ACL qualifier used";
          isValid = false;
          return LoopAction::BREAK;
        }
        return LoopAction::CONTINUE;
      }

  );

  // Only single watchdog recovery action is supported.
  // TODO - Add support for per port watchdog recovery action
  std::shared_ptr<Port> firstPort;
  std::optional<cfg::PfcWatchdogRecoveryAction> recoveryAction{};
  for (const auto& portMap : std::as_const(*delta.newState()->getPorts())) {
    for (const auto& port : std::as_const(*portMap.second)) {
      if (port.second->getPfc().has_value() &&
          port.second->getPfc()->watchdog().has_value()) {
        auto pfcWd = port.second->getPfc()->watchdog().value();
        if (!recoveryAction.has_value()) {
          recoveryAction = *pfcWd.recoveryAction();
          firstPort = port.second;
        } else if (*recoveryAction != *pfcWd.recoveryAction()) {
          // Error: All ports should have the same recovery action configured
          XLOG(ERR) << "PFC watchdog deadlock recovery action on "
                    << port.second->getName() << " conflicting with "
                    << firstPort->getName();
          isValid = false;
        }
      }
    }
  }

  return isValid;
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
  /* Strip vlan tag with pipeline bypass, for all asic types. */
  getSwitchStats()->txSent();
  folly::io::Cursor cursor(pkt->buf());
  EthHdr ethHdr{cursor};
  if (!ethHdr.getVlanTags().empty()) {
    CHECK_EQ(ethHdr.getVlanTags().size(), 1)
        << "found more than one vlan tags while sending packet";
    /* Strip vlans as pipeline bypass doesn't handle this */
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
  auto adapterKeys = saiStore_->adapterKeysFollyDynamic();
  // Need to provide full namespace scope for toFollyDynamic to disambiguate
  // from member SaiSwitch::toFollyDynamic
  auto switchKeys = folly::dynamic::array(
      facebook::fboss::toFollyDynamic<SaiSwitchTraits>(switchId_));
  adapterKeys[saiObjectTypeToString(SaiSwitchTraits::ObjectType)] = switchKeys;

  folly::dynamic hwSwitch = folly::dynamic::object;
  hwSwitch[kAdapterKeys] = adapterKeys;
  hwSwitch[kAdapterKey2AdapterHostKey] =
      saiStore_->adapterKeys2AdapterHostKeysFollyDynamic();
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
        initLinkScanLocked(lock);
      }
      if (getFeaturesDesired() & FeaturesDesired::PACKET_RX_DESIRED) {
        auto& switchApi = SaiApiTable::getInstance()->switchApi();
        switchApi.registerRxCallback(switchId_, __gPacketRxCallback);
      }
      if (getFeaturesDesired() & FeaturesDesired::TAM_EVENT_NOTIFY_DESIRED) {
        auto& switchApi = SaiApiTable::getInstance()->switchApi();
#if defined(SAI_VERSION_7_2_0_0_ODP) || defined(SAI_VERSION_8_2_0_0_ODP) ||    \
    defined(SAI_VERSION_8_2_0_0_SIM_ODP) ||                                    \
    defined(SAI_VERSION_8_2_0_0_DNX_ODP) ||                                    \
    defined(SAI_VERSION_9_2_0_0_ODP) || defined(SAI_VERSION_9_0_EA_SIM_ODP) || \
    defined(SAI_VERSION_10_0_EA_DNX_SIM_ODP) ||                                \
    defined(SAI_VERSION_10_0_EA_DNX_ODP) ||                                    \
    defined(SAI_VERSION_10_0_EA_ODP) || defined(SAI_VERSION_10_0_EA_SIM_ODP)
        switchApi.registerParityErrorSwitchEventCallback(
            switchId_, (void*)__gParityErrorSwitchEventCallback);
#else
        switchApi.registerTamEventCallback(switchId_, __gTamEventCallback);
#endif
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
  XLOG(DBG2) << "Received " << count << " learn notifications";
  if (runState_ < SwitchRunState::CONFIGURED) {
    // receive learn events after switch is configured to prevent
    // fdb entries from being created against ports  which would
    // become members of lags later on.
    XLOG(WARNING)
        << "ignoring learn notifications as switch is not configured.";
    return;
  }
  /*
   * FDB event notifications can be parsed in several ways
   * 1) Deep copy of sai_fdb_event_notification_data_t. This includes
   * allocating memory for sai_attribute_t in the fdb event data and freeing
   * them in the fdb bottom half thread.
   * 2) Create a temporary struct FdbEventNotificationData which is a subset
   * of sai_fdb_event_notification_data_t. This will be used to store only the
   * necessary attributes and pass it on to fdb bottom half thread. 3) Avoid
   * creating temporary structs and create L2Entry which can be sent to sw
   * switch. The problem with this approach is we need to hold the sai switch
   * mutex in the callback thread. Also, this can cause deadlock when we used
   * the bridge port id to fetch the port id from the SDK. Going with option 2
   * for fdb event notifications.
   */
  std::vector<FdbEventNotificationData> fdbEventNotificationDataTmp;
  for (auto i = 0; i < count; i++) {
    BridgePortSaiId bridgePortSaiId{0};
    sai_uint32_t fdbMetaData = 0;
    for (auto j = 0; j < data[i].attr_count; j++) {
      switch (data[i].attr[j].id) {
        case SAI_FDB_ENTRY_ATTR_BRIDGE_PORT_ID:
          bridgePortSaiId = data[i].attr[j].value.oid;
          break;
        case SAI_FDB_ENTRY_ATTR_META_DATA:
          fdbMetaData = data[i].attr[j].value.u32;
          break;
        default:
          break;
      }
    }
    fdbEventNotificationDataTmp.push_back(FdbEventNotificationData(
        data[i].event_type, data[i].fdb_entry, bridgePortSaiId, fdbMetaData));
  }
  fdbEventBottomHalfEventBase_.runInEventBaseThread(
      [this,
       fdbNotifications = std::move(fdbEventNotificationDataTmp)]() mutable {
        auto lock = std::lock_guard<std::mutex>(saiSwitchMutex_);
        fdbEventCallbackLockedBottomHalf(lock, std::move(fdbNotifications));
      });
}

void SaiSwitch::fdbEventCallbackLockedBottomHalf(
    const std::lock_guard<std::mutex>& /*lock*/,
    std::vector<FdbEventNotificationData> fdbNotifications) {
  if (managerTable_->bridgeManager().getL2LearningMode() !=
      cfg::L2LearningMode::SOFTWARE) {
    // Some platforms call fdb callback even when mode is set to HW. In
    // keeping with our native SDK approach, don't send these events up.
    return;
  }
  for (const auto& fdbNotification : fdbNotifications) {
    auto ditr =
        kL2AddrUpdateOperationsOfInterest.find(fdbNotification.eventType);
    if (ditr != kL2AddrUpdateOperationsOfInterest.end()) {
      auto updateTypeStr = fdbEventToString(ditr->first);
      auto l2Entry = getL2Entry(fdbNotification);
      if (l2Entry) {
        XLOG(DBG2) << "Received FDB " << updateTypeStr
                   << " notification for: " << l2Entry->str();
        callback_->l2LearningUpdateReceived(l2Entry.value(), ditr->second);
      }
    }
  }
}

std::optional<L2Entry> SaiSwitch::getL2Entry(
    const FdbEventNotificationData& fdbEvent) const {
  /*
   * Idea behind returning a optional here on spurious FDB event
   * (say with bogus port or vlan ID) vs throwing a exception
   * is to protect the application from crashes in face of
   * spurious events. This is similar to the philosophy we follow
   * for spurious linkstate change or packet RX callbacks.
   */
  if (!fdbEvent.bridgePortSaiId) {
    XLOG(ERR) << "Missing bridge port attribute in FDB event";
    return std::nullopt;
  }
  auto portOrLagSaiId = SaiApiTable::getInstance()->bridgeApi().getAttribute(
      fdbEvent.bridgePortSaiId, SaiBridgePortTraits::Attributes::PortId{});

  L2Entry::L2EntryType entryType{L2Entry::L2EntryType::L2_ENTRY_TYPE_PENDING};
  auto mac = fromSaiMacAddress(fdbEvent.fdbEntry.mac_address);
  switch (fdbEvent.eventType) {
    // For learning events consider entry type as pending (else the
    // fdb event should not have been punted to us)
    // For Aging events, consider the entry as already validated (programmed)
    case SAI_FDB_EVENT_LEARNED:
      if (platform_->getAsic()->isSupported(
              HwAsic::Feature::PENDING_L2_ENTRY)) {
        entryType = L2Entry::L2EntryType::L2_ENTRY_TYPE_PENDING;
      } else {
        entryType = L2Entry::L2EntryType::L2_ENTRY_TYPE_VALIDATED;
      }
      break;
    case SAI_FDB_EVENT_AGED:
      entryType = L2Entry::L2EntryType::L2_ENTRY_TYPE_VALIDATED;
      break;
    default:
      XLOG(ERR) << "Unexpected fdb event type: " << fdbEvent.eventType
                << " for " << mac;
      return std::nullopt;
  }

  // fdb learn event can happen over bridge port of lag or port
  auto portType = sai_object_type_query(portOrLagSaiId);
  if (portType != SAI_OBJECT_TYPE_PORT && portType != SAI_OBJECT_TYPE_LAG) {
    XLOG(ERR) << "FDB event for unexpected object type "
              << saiObjectTypeToString(portType);
    return std::nullopt;
  }
  PortDescriptorSaiId portDescSaiId = (portType == SAI_OBJECT_TYPE_PORT)
      ? PortDescriptorSaiId(PortSaiId(portOrLagSaiId))
      : PortDescriptorSaiId(LagSaiId(portOrLagSaiId));
  std::optional<PortDescriptor> portDesc{};
  switch (portDescSaiId.type()) {
    case PortDescriptorSaiId::PortType::PHYSICAL: {
      auto portItr =
          concurrentIndices_->portIds.find(portDescSaiId.phyPortID());
      if (portItr != concurrentIndices_->portIds.end()) {
        portDesc = PortDescriptor(portItr->second);
      }
    } break;

    case PortDescriptorSaiId::PortType::AGGREGATE: {
      auto aggregatePortItr =
          concurrentIndices_->aggregatePortIds.find(portDescSaiId.aggPortID());
      if (aggregatePortItr != concurrentIndices_->aggregatePortIds.end()) {
        portDesc = PortDescriptor(aggregatePortItr->second);
      }
    }; break;
    case PortDescriptorSaiId::PortType::SYSTEM_PORT:
      XLOG(FATAL) << " Unexpected fdb event for sys port";
      break;
  }

  if (!portDesc) {
    XLOG(ERR) << " FDB event for " << mac
              << ", got non existent port id : " << portDescSaiId.str();
    return std::nullopt;
  }

  auto vlanItr = concurrentIndices_->vlanIds.find(portDescSaiId);
  if (vlanItr == concurrentIndices_->vlanIds.end()) {
    XLOG(ERR) << " FDB event for " << mac
              << " could not look up VLAN for : " << portDescSaiId.str();
    return std::nullopt;
  }

  auto classID = fdbEvent.fdbMetaData != 0
      ? std::optional<cfg::AclLookupClass>(
            cfg::AclLookupClass(fdbEvent.fdbMetaData))
      : std::nullopt;

  return L2Entry{
      fromSaiMacAddress(fdbEvent.fdbEntry.mac_address),
      vlanItr->second,
      portDesc.value(),
      entryType,
      classID};
}

template <
    typename Delta,
    typename Manager,
    typename LockPolicyT,
    typename... Args,
    typename ChangeFunc,
    typename AddedFunc,
    typename RemovedFunc>
void SaiSwitch::processDelta(
    Delta delta,
    Manager& manager,
    const LockPolicyT& lockPolicy,
    ChangeFunc changedFunc,
    AddedFunc addedFunc,
    RemovedFunc removedFunc,
    Args... args) {
  DeltaFunctions::forEachChanged(
      delta,
      [&](auto removed, auto added) {
        [[maybe_unused]] const auto& lock = lockPolicy.lock();
        (manager.*changedFunc)(removed, added, args...);
      },
      [&](auto added) {
        [[maybe_unused]] const auto& lock = lockPolicy.lock();
        (manager.*addedFunc)(added, args...);
      },
      [&](auto removed) {
        [[maybe_unused]] const auto& lock = lockPolicy.lock();
        (manager.*removedFunc)(removed, args...);
      });
}

template <
    typename Delta,
    typename Manager,
    typename LockPolicyT,
    typename... Args,
    typename ChangeFunc>
void SaiSwitch::processChangedDelta(
    Delta delta,
    Manager& manager,
    const LockPolicyT& lockPolicy,
    ChangeFunc changedFunc,
    Args... args) {
  DeltaFunctions::forEachChanged(delta, [&](auto added, auto removed) {
    [[maybe_unused]] const auto& lock = lockPolicy.lock();
    (manager.*changedFunc)(added, removed, args...);
  });
}

template <
    typename Delta,
    typename Manager,
    typename LockPolicyT,
    typename... Args,
    typename AddedFunc>
void SaiSwitch::processAddedDelta(
    Delta delta,
    Manager& manager,
    const LockPolicyT& lockPolicy,
    AddedFunc addedFunc,
    Args... args) {
  DeltaFunctions::forEachAdded(delta, [&](auto added) {
    [[maybe_unused]] const auto& lock = lockPolicy.lock();
    (manager.*addedFunc)(added, args...);
  });
}

template <
    typename Delta,
    typename Manager,
    typename LockPolicyT,
    typename... Args,
    typename RemovedFunc>
void SaiSwitch::processRemovedDelta(
    Delta delta,
    Manager& manager,
    const LockPolicyT& lockPolicy,
    RemovedFunc removedFunc,
    Args... args) {
  DeltaFunctions::forEachRemoved(delta, [&](auto removed) {
    [[maybe_unused]] const auto& lock = lockPolicy.lock();
    (manager.*removedFunc)(removed, args...);
  });
}

void SaiSwitch::dumpDebugState(const std::string& path) const {
  saiCheckError(sai_dbg_generate_dump(path.c_str()));
}

std::string SaiSwitch::listObjectsLocked(
    const std::vector<sai_object_type_t>& objects,
    bool cached,
    const std::lock_guard<std::mutex>& lock) const {
  const SaiStore* store = saiStore_.get();
  SaiStore directToHwStore;
  if (!cached) {
    directToHwStore.setSwitchId(getSaiSwitchId());
    auto json = toFollyDynamicLocked(lock);
    std::unique_ptr<folly::dynamic> adapterKeysJson;
    std::unique_ptr<folly::dynamic> adapterKeys2AdapterHostKeysJson;
    if (platform_->getAsic()->isSupported(HwAsic::Feature::OBJECT_KEY_CACHE)) {
      adapterKeysJson = std::make_unique<folly::dynamic>(json[kAdapterKeys]);
    }
    adapterKeys2AdapterHostKeysJson =
        std::make_unique<folly::dynamic>(json[kAdapterKey2AdapterHostKey]);
    directToHwStore.reload(
        adapterKeysJson.get(), adapterKeys2AdapterHostKeysJson.get());
    store = &directToHwStore;
  }
  std::string output;
  std::for_each(objects.begin(), objects.end(), [&output, store](auto objType) {
    output += store->storeStr(objType);
  });
  return output;
}

std::string SaiSwitch::listObjects(
    const std::vector<HwObjectType>& types,
    bool cached) const {
  bool listManagedObjects{false};
  std::vector<sai_object_type_t> objTypes;
  for (auto type : types) {
    switch (type) {
      case HwObjectType::PORT:
        objTypes.push_back(SAI_OBJECT_TYPE_PORT);
        objTypes.push_back(SAI_OBJECT_TYPE_PORT_SERDES);
        objTypes.push_back(SAI_OBJECT_TYPE_PORT_CONNECTOR);
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
      case HwObjectType::ACL:
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
      case HwObjectType::LABEL_ENTRY:
        objTypes.push_back(SAI_OBJECT_TYPE_INSEG_ENTRY);
        break;
      case HwObjectType::MACSEC:
        objTypes.push_back(SAI_OBJECT_TYPE_MACSEC);
        objTypes.push_back(SAI_OBJECT_TYPE_MACSEC_PORT);
        objTypes.push_back(SAI_OBJECT_TYPE_MACSEC_FLOW);
        objTypes.push_back(SAI_OBJECT_TYPE_MACSEC_SC);
        objTypes.push_back(SAI_OBJECT_TYPE_MACSEC_SA);
        break;
      case HwObjectType::SAI_MANAGED_OBJECTS:
        listManagedObjects = true;
        break;
      case HwObjectType::IPTUNNEL:
        objTypes.push_back(SAI_OBJECT_TYPE_TUNNEL);
        objTypes.push_back(SAI_OBJECT_TYPE_TUNNEL_TERM_TABLE_ENTRY);
        break;
    }
  }
  std::lock_guard<std::mutex> lk(saiSwitchMutex_);
  auto output = listObjectsLocked(objTypes, cached, lk);
  if (listManagedObjects) {
    listManagedObjectsLocked(output, lk);
  }
  return output;
}

void SaiSwitch::listManagedObjectsLocked(
    std::string& output,
    const std::lock_guard<std::mutex>& /*lock*/) const {
  output += "\nmanaged sai objects\n";
  output += managerTable_->fdbManager().listManagedObjects();
  output += managerTable_->neighborManager().listManagedObjects();
  output += managerTable_->nextHopManager().listManagedObjects();
  output += managerTable_->nextHopGroupManager().listManagedObjects();
}

uint32_t SaiSwitch::generateDeterministicSeed(
    LoadBalancerID loadBalancerID,
    folly::MacAddress platformMac) const {
  auto mac64 = platformMac.u64HBO();
  uint32_t mac32 = static_cast<uint32_t>(mac64 & 0xFFFFFFFF);
  uint32_t seed = 0;
  switch (loadBalancerID) {
    case LoadBalancerID::ECMP:
      seed = folly::hash::twang_32from64(mac64);
      break;
    case LoadBalancerID::AGGREGATE_PORT:
      seed = folly::hash::jenkins_rev_mix32(mac32);
      break;
  }
  return seed;
}

phy::FecMode SaiSwitch::getPortFECMode(PortID portId) const {
  return managerTable_->portManager().getFECMode(portId);
}

void SaiSwitch::rollbackInTest(
    const std::shared_ptr<SwitchState>& knownGoodState) {
  rollback(knownGoodState);
  setProgrammedState(knownGoodState);
}

template <typename LockPolicyT>
void SaiSwitch::processAclTableGroupDelta(
    const StateDelta& delta,
    const AclTableGroupMap& aclTableGroupMap,
    const LockPolicyT& lockPolicy) {
  for (const auto& [_, tableGroup] : aclTableGroupMap) {
    auto aclStage = tableGroup->getID();

    processDelta(
        delta.getAclTablesDelta(aclStage),
        managerTable_->aclTableManager(),
        lockPolicy,
        &SaiAclTableManager::changedAclTable,
        &SaiAclTableManager::addAclTable,
        &SaiAclTableManager::removeAclTable,
        aclStage);

    if (delta.getAclTablesDelta(aclStage).getNew()) {
      // Process delta for the entries of each table in the new state
      for (const auto& iter :
           std::as_const(*delta.getAclTablesDelta(aclStage).getNew())) {
        auto table = iter.second;
        auto tableName = table->getID();
        processDelta(
            delta.getAclsDelta(aclStage, tableName),
            managerTable_->aclTableManager(),
            lockPolicy,
            &SaiAclTableManager::changedAclEntry,
            &SaiAclTableManager::addAclEntry,
            &SaiAclTableManager::removeAclEntry,
            tableName);
      }
    }
  }
}
} // namespace facebook::fboss
