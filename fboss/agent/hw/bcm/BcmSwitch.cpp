/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/bcm/BcmSwitch.h"

#include <boost/cast.hpp>
#include <boost/filesystem/operations.hpp>
#include <fstream>
#include <map>
#include <optional>
#include <utility>

#include <folly/Conv.h>
#include <folly/FileUtil.h>
#include <folly/Memory.h>
#include <folly/hash/Hash.h>
#include <folly/logging/xlog.h>

#include "fboss/agent/Constants.h"
#include "fboss/agent/FbossError.h"
#include "fboss/agent/LacpTypes.h"
#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/SwitchStats.h"
#include "fboss/agent/Utils.h"
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/hw/BufferStatsLogger.h"
#include "fboss/agent/hw/bcm/BcmAPI.h"
#include "fboss/agent/hw/bcm/BcmAclEntry.h"
#include "fboss/agent/hw/bcm/BcmAclTable.h"
#include "fboss/agent/hw/bcm/BcmAddressFBConvertors.h"
#include "fboss/agent/hw/bcm/BcmBstStatsMgr.h"
#include "fboss/agent/hw/bcm/BcmControlPlane.h"
#include "fboss/agent/hw/bcm/BcmCosManager.h"
#include "fboss/agent/hw/bcm/BcmEgressManager.h"
#include "fboss/agent/hw/bcm/BcmError.h"
#include "fboss/agent/hw/bcm/BcmFacebookAPI.h"
#include "fboss/agent/hw/bcm/BcmFieldProcessorUtils.h"
#include "fboss/agent/hw/bcm/BcmHost.h"
#include "fboss/agent/hw/bcm/BcmHostKey.h"
#include "fboss/agent/hw/bcm/BcmIntf.h"
#include "fboss/agent/hw/bcm/BcmLabelMap.h"
#include "fboss/agent/hw/bcm/BcmLabelSwitchingUtils.h"
#include "fboss/agent/hw/bcm/BcmLogBuffer.h"
#include "fboss/agent/hw/bcm/BcmMacTable.h"
#include "fboss/agent/hw/bcm/BcmMirrorTable.h"
#include "fboss/agent/hw/bcm/BcmNextHop.h"
#include "fboss/agent/hw/bcm/BcmPlatform.h"
#include "fboss/agent/hw/bcm/BcmPort.h"
#include "fboss/agent/hw/bcm/BcmPortGroup.h"
#include "fboss/agent/hw/bcm/BcmPortTable.h"
#include "fboss/agent/hw/bcm/BcmQcmManager.h"
#include "fboss/agent/hw/bcm/BcmQosPolicyTable.h"
#include "fboss/agent/hw/bcm/BcmRoute.h"
#include "fboss/agent/hw/bcm/BcmRtag7LoadBalancer.h"
#include "fboss/agent/hw/bcm/BcmRxPacket.h"
#include "fboss/agent/hw/bcm/BcmSflowExporter.h"
#include "fboss/agent/hw/bcm/BcmStatUpdater.h"
#include "fboss/agent/hw/bcm/BcmSwitchEventCallback.h"
#include "fboss/agent/hw/bcm/BcmSwitchEventUtils.h"
#include "fboss/agent/hw/bcm/BcmSwitchSettings.h"
#include "fboss/agent/hw/bcm/BcmTableStats.h"
#include "fboss/agent/hw/bcm/BcmTrunk.h"
#include "fboss/agent/hw/bcm/BcmTrunkTable.h"
#include "fboss/agent/hw/bcm/BcmTxPacket.h"
#include "fboss/agent/hw/bcm/BcmUnit.h"
#include "fboss/agent/hw/bcm/BcmWarmBootCache.h"
#include "fboss/agent/hw/bcm/BcmWarmBootHelper.h"
#include "fboss/agent/hw/bcm/BcmWarmBootState.h"
#include "fboss/agent/hw/bcm/PacketTraceUtils.h"
#include "fboss/agent/hw/bcm/RxUtils.h"
#include "fboss/agent/hw/bcm/gen-cpp2/bcmswitch_constants.h"
#include "fboss/agent/hw/bcm/gen-cpp2/packettrace_types.h"
#include "fboss/agent/hw/mock/MockRxPacket.h"
#include "fboss/agent/hw/switch_asics/HwAsic.h"
#include "fboss/agent/state/AclEntry.h"
#include "fboss/agent/state/AggregatePort.h"
#include "fboss/agent/state/ArpEntry.h"
#include "fboss/agent/state/DeltaFunctions.h"
#include "fboss/agent/state/ForwardingInformationBase.h"
#include "fboss/agent/state/ForwardingInformationBaseContainer.h"
#include "fboss/agent/state/ForwardingInformationBaseDelta.h"
#include "fboss/agent/state/ForwardingInformationBaseMap.h"
#include "fboss/agent/state/Interface.h"
#include "fboss/agent/state/InterfaceMap.h"
#include "fboss/agent/state/LoadBalancer.h"
#include "fboss/agent/state/LoadBalancerMap.h"
#include "fboss/agent/state/NodeMapDelta-defs.h"
#include "fboss/agent/state/NodeMapDelta.h"
#include "fboss/agent/state/Port.h"
#include "fboss/agent/state/PortMap.h"
#include "fboss/agent/state/Route.h"
#include "fboss/agent/state/RouteDelta.h"
#include "fboss/agent/state/RouteTable.h"
#include "fboss/agent/state/RouteTableMap.h"
#include "fboss/agent/state/RouteTableRib.h"
#include "fboss/agent/state/SflowCollector.h"
#include "fboss/agent/state/SflowCollectorMap.h"
#include "fboss/agent/state/StateDelta.h"
#include "fboss/agent/state/SwitchState-defs.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/state/Vlan.h"
#include "fboss/agent/state/VlanMap.h"
#include "fboss/agent/state/VlanMapDelta.h"
#include "fboss/agent/types.h"

extern "C" {
#include <bcm/link.h>
#include <bcm/port.h>
#include <bcm/stg.h>
#include <bcm/switch.h>
#include <bcm/vlan.h>

#if (!defined(BCM_VER_MAJOR))
#define BCM_WARM_BOOT_SUPPORT
#include <soc/opensoc.h>
#else
#include <soc/drv.h>
#endif
}

using std::make_pair;
using std::make_shared;
using std::make_unique;
using std::move;
using std::shared_ptr;
using std::string;
using std::unique_ptr;
using std::chrono::duration_cast;
using std::chrono::seconds;

using facebook::fboss::DeltaFunctions::forEachAdded;
using facebook::fboss::DeltaFunctions::forEachChanged;
using facebook::fboss::DeltaFunctions::forEachRemoved;

using folly::IPAddress;
using folly::IPAddressV4;
using folly::IPAddressV6;

using namespace std::chrono;

using namespace facebook::fboss::utility;

/**
 * Set L2 Aging to 5 mins by default, same as Arista -
 * https://www.arista.com/en/um-eos/eos-section-19-3-mac-address-table
 */
DEFINE_int32(
    l2AgeTimerSeconds,
    300,
    "Time to transition L2 from hit -> miss -> removed");
DEFINE_int32(linkscan_interval_us, 250000, "The Broadcom linkscan interval");
DEFINE_bool(flexports, false, "Load the agent with flexport support enabled");
DEFINE_int32(
    update_bststats_interval_s,
    60,
    "Update BST stats for ODS interval in seconds");
DEFINE_bool(force_init_fp, true, "Force full field processor initialization");
DEFINE_string(
    script_pre_asic_init,
    "script_pre_asic_init",
    "Broadcom script file to be run before ASIC init");

DEFINE_int32(cosq_hipri, 9, "The high priority cos queue number");

DEFINE_int32(cosq_midpri, 2, "The medium priority cos queue number");

DEFINE_int32(cosq_default, 1, "The default cos queue number");

DEFINE_int32(cosq_lopri, 0, "The low priority cos queue number");

DEFINE_int32(acl_gid, 128, "Content aware processor group ID for ACLs");
DEFINE_int32(
    copp_acl_entry_priority_start,
    1,
    "Starting ACL entry priority for CoPP");

DEFINE_int32(acl_g_pri, 0, "Group priority for ACL field group");
DEFINE_int32(
    qcm_ifp_gid,
    5,
    "Content aware processor group ID for ACLs specific to qcm");
// Put lowest priority for this group among all i.e. lower than acl_g_pri.
DEFINE_int32(qcm_ifp_pri, -1, "Group priority for ACL field group");

enum : uint8_t {
  kRxCallbackPriority = 1,
};

namespace {
constexpr auto kHostTable = "hostTable";
constexpr int kLogBcmErrorFreqMs = 3000;
/*
 * Key to determine whether alpm is enabled
 */
constexpr folly::StringPiece kAlpmSetting = "l3_alpm_enable";

void rethrowIfHwNotFull(const facebook::fboss::BcmError& error) {
  if (error.getBcmError() != BCM_E_FULL) {
    // If this is not because of TCAM being full, rethrow the exception.
    throw error;
  }

  XLOG(WARNING) << error.what();
}

bool routeTableModified(const facebook::fboss::StateDelta& delta) {
  return delta.getRouteTablesDelta().begin() !=
      delta.getRouteTablesDelta().end();
}

bool fibModified(const facebook::fboss::StateDelta& delta) {
  return delta.getFibsDelta().begin() != delta.getFibsDelta().end();
}

bool bothStandAloneRibOrRouteTableRibUsed(
    const facebook::fboss::StateDelta& delta) {
  return routeTableModified(delta) && fibModified(delta);
}

/*
 * For the devices/SDK we use, the only events we should get (and process)
 * are ADD and DELETE.
 * Learning generates ADD, aging generates DELETE, while Mac move results in
 * DELETE followed by ADD.
 * We ASSERT this explicitly via HW tests.
 */
const std::map<int, facebook::fboss::L2EntryUpdateType>
    kL2AddrUpdateOperationsOfInterest = {
        {BCM_L2_CALLBACK_DELETE,
         facebook::fboss::L2EntryUpdateType::L2_ENTRY_UPDATE_TYPE_DELETE},
        {BCM_L2_CALLBACK_ADD,
         facebook::fboss::L2EntryUpdateType::L2_ENTRY_UPDATE_TYPE_ADD},
};

using facebook::fboss::AggregatePortID;
using facebook::fboss::L2Entry;
using facebook::fboss::L2EntryUpdateType;
using facebook::fboss::macFromBcm;
using facebook::fboss::PortDescriptor;
using facebook::fboss::PortID;
using facebook::fboss::VlanID;
using facebook::fboss::cfg::AclLookupClass;

L2Entry createL2Entry(const bcm_l2_addr_t* l2Addr, bool isPending) {
  CHECK(l2Addr);

  auto l2EntryType = isPending ? L2Entry::L2EntryType::L2_ENTRY_TYPE_PENDING
                               : L2Entry::L2EntryType::L2_ENTRY_TYPE_VALIDATED;

  if (!(l2Addr->flags & BCM_L2_TRUNK_MEMBER)) {
    return L2Entry(
        macFromBcm(l2Addr->mac),
        VlanID(l2Addr->vid),
        PortDescriptor(PortID(l2Addr->port)),
        l2EntryType,
        l2Addr->group != 0
            ? std::optional<AclLookupClass>(AclLookupClass(l2Addr->group))
            : std::nullopt);
  } else {
    return L2Entry(
        macFromBcm(l2Addr->mac),
        VlanID(l2Addr->vid),
        PortDescriptor(AggregatePortID(l2Addr->tgid)),
        l2EntryType,
        l2Addr->group != 0
            ? std::optional<AclLookupClass>(AclLookupClass(l2Addr->group))
            : std::nullopt);
  }
}

bool isL2OperationOfInterest(int operation) {
  return kL2AddrUpdateOperationsOfInterest.find(operation) !=
      kL2AddrUpdateOperationsOfInterest.end();
}

L2EntryUpdateType getL2EntryUpdateType(int operation) {
  CHECK(isL2OperationOfInterest(operation));
  return kL2AddrUpdateOperationsOfInterest.find(operation)->second;
}

/*
 * How many bytes we copy for sFlow sampling
 */
const unsigned int kMaxSflowSnapLen = 128;

bool isValidLabeledNextHopSet(
    facebook::fboss::BcmPlatform* platform,
    const facebook::fboss::LabelNextHopSet& nexthops) {
  if (!facebook::fboss::LabelForwardingInformationBase::isValidNextHopSet(
          nexthops)) {
    return false;
  }
  std::optional<facebook::fboss::LabelForwardingAction::LabelForwardingType>
      forwardingType;
  for (const auto& nexthop : nexthops) {
    const auto& labelForwardingAction = nexthop.labelForwardingAction();
    if (labelForwardingAction->type() ==
            facebook::fboss::LabelForwardingAction::LabelForwardingType::PUSH &&
        labelForwardingAction->pushStack()->size() >
            platform->getAsic()->getMaxLabelStackDepth()) {
      // label stack to push exceeds what platform can support
      XLOG(ERR) << "next hoop " << nexthop.str() << " label stack exceeds "
                << platform->getAsic()->getMaxLabelStackDepth();
      return false;
    }
    if (!forwardingType.has_value()) {
      forwardingType = labelForwardingAction->type();
    } else if (forwardingType != labelForwardingAction->type()) {
      //  each next hop must have same LabelForwardingAction type.
      XLOG(ERR) << "more than one forwarding action in next hop set";
      return false;
    }
  }
  return true;
}

} // namespace

namespace facebook::fboss {

/*
 * Get current port speed from BCM SDK and convert to
 * cfg::PortSpeed
 */

bool BcmSwitch::getPortFECEnabled(PortID port) const {
  // relies on getBcmPort() to throw an if not found
  return getPortTable()->getBcmPort(port)->isFECEnabled();
}

cfg::PortSpeed BcmSwitch::getPortMaxSpeed(PortID port) const {
  return getPortTable()->getBcmPort(port)->getMaxSpeed();
}

BcmSwitch::BcmSwitch(BcmPlatform* platform, uint32_t featuresDesired)
    : BcmSwitchIf(featuresDesired),
      platform_(platform),
      mmuBufferBytes_(platform->getMMUBufferBytes()),
      mmuCellBytes_(platform->getMMUCellBytes()),
      warmBootCache_(new BcmWarmBootCache(this)),
      portTable_(new BcmPortTable(this)),
      intfTable_(new BcmIntfTable(this)),
      hostTable_(new BcmHostTable(this)),
      egressManager_(new BcmEgressManager(this)),
      neighborTable_(new BcmNeighborTable(this)),
      l3NextHopTable_(new BcmL3NextHopTable(this)),
      mplsNextHopTable_(new BcmMplsNextHopTable(this)),
      multiPathNextHopTable_(new BcmMultiPathNextHopTable(this)),
      labelMap_(new BcmLabelMap(this)),
      routeTable_(new BcmRouteTable(this)),
      qosPolicyTable_(new BcmQosPolicyTable(this)),
      aclTable_(new BcmAclTable(this)),
      trunkTable_(new BcmTrunkTable(this)),
      sFlowExporterTable_(new BcmSflowExporterTable()),
      rtag7LoadBalancer_(new BcmRtag7LoadBalancer(this)),
      mirrorTable_(new BcmMirrorTable(this)),
      bstStatsMgr_(new BcmBstStatsMgr(this)),
      switchSettings_(new BcmSwitchSettings(this)),
      macTable_(new BcmMacTable(this)),
      qcmManager_(new BcmQcmManager(this)) {
  exportSdkVersion();
}

BcmSwitch::~BcmSwitch() {
  XLOG(ERR) << "Destroying BcmSwitch";
  resetTables();
  if (unitObject_) {
    // In agent this would be done in the signal handler
    // gracefulExit().  In bcm_tests there is no signal.
    // So if unitObject_ is still valid, destroy it.
    unitObject_.reset();
  }
}

void BcmSwitch::resetTables() {
  std::unique_lock<std::mutex> lk(lock_);
  unregisterCallbacks();
  routeTable_.reset();
  labelMap_.reset();
  l3NextHopTable_.reset();
  mplsNextHopTable_.reset();
  // Release host entries before reseting switch's host table
  // entries so that if host try to refer to look up host table
  // via the BCM switch during their destruction the pointer
  // access is still valid.
  hostTable_->releaseHosts();
  // reset neighbors before resetting host table
  neighborTable_.reset();
  // reset interfaces before host table, as interfaces have
  // host references now.
  intfTable_.reset();
  egressManager_.reset();
  multiPathNextHopTable_.reset();
  hostTable_.reset();
  toCPUEgress_.reset();
  portTable_.reset();
  qosPolicyTable_.reset();
  aclTable_->releaseAcls();
  aclTable_.reset();
  mirrorTable_.reset();
  trunkTable_.reset();
  controlPlane_.reset();
  rtag7LoadBalancer_.reset();
  bcmStatUpdater_.reset();
  bstStatsMgr_.reset();
  switchSettings_.reset();
  macTable_.reset();
  qcmManager_.reset();
  // Reset warmboot cache last in case Bcm object destructors
  // access it during object deletion.
  warmBootCache_.reset();
}

void BcmSwitch::unregisterCallbacks() {
  if (flags_ & RX_REGISTERED) {
    bcm_rx_stop(unit_, nullptr);
    auto rv = bcm_rx_unregister(unit_, packetRxCallback, kRxCallbackPriority);
    CHECK(BCM_SUCCESS(rv)) << "failed to unregister BcmSwitch rx callback: "
                           << bcm_errmsg(rv);
    flags_ &= ~RX_REGISTERED;
  }
  // Note that we don't explicitly call bcm_linkscan_detach() here--
  // this call is not thread safe and should only be called from the main
  // thread.  However, bcm_detach() / _bcm_shutdown() will clean up the
  // linkscan module properly.
  if (flags_ & LINKSCAN_REGISTERED) {
    auto rv = bcm_linkscan_unregister(unit_, linkscanCallback);
    CHECK(BCM_SUCCESS(rv)) << "failed to unregister BcmSwitch linkscan "
                              "callback: "
                           << bcm_errmsg(rv);

    stopLinkscanThread();
    flags_ &= ~LINKSCAN_REGISTERED;
  }

  /*
   * SOFTWARE L2 learning mode enables callback when there are updates to L2
   * table. Disable it: we don't want these callbacks (that call BcmSwitch
   * method) to fire after BcmSwitch object is destroyed.
   */
  if (switchSettings_->getL2LearningMode().has_value() &&
      switchSettings_->getL2LearningMode().value() ==
          cfg::L2LearningMode::SOFTWARE) {
    switchSettings_->disableL2LearningCallback();
  }
}

void BcmSwitch::gracefulExit(folly::dynamic& switchState) {
  steady_clock::time_point begin = steady_clock::now();
  XLOG(INFO) << "[Exit] Starting BCM Switch graceful exit";
  // Ideally, preparePortsForGracefulExit() would run in update EVB of the
  // SwSwitch, but it does not really matter at the graceful exit time. If
  // this is a concern, this can be moved to the updateEventBase_ of SwSwitch.
  portTable_->preparePortsForGracefulExit();
  bstStatsMgr_->stopBufferStatCollection();
  qcmManager_->stop();

  std::lock_guard<std::mutex> g(lock_);

  // This will run some common shell commands to give more info about
  // the underlying bcm sdk state
  dumpState(platform_->getWarmBootHelper()->shutdownSdkDumpFile());

  switchState[kHwSwitch] = toFollyDynamic();
  unitObject_->writeWarmBootState(switchState);
  unitObject_.reset();
  XLOG(INFO)
      << "[Exit] BRCM Graceful Exit time "
      << duration_cast<duration<float>>(steady_clock::now() - begin).count();
}

folly::dynamic BcmSwitch::toFollyDynamic() const {
  return warmBootCache_->getWarmBootStateFollyDynamic();
}

int BcmSwitch::addL2TableCb(
    int /*unit*/,
    bcm_l2_addr_t* l2Addr,
    void* userData) {
  auto* bcmSw = static_cast<BcmSwitch*>(userData);
  bcmSw->callback_->l2LearningUpdateReceived(
      createL2Entry(l2Addr, bcmSw->isL2EntryPending(l2Addr)),
      getL2EntryUpdateType(BCM_L2_CALLBACK_ADD));

  return 0;
}

int BcmSwitch::addL2TableCbForPendingOnly(
    int /*unit*/,
    bcm_l2_addr_t* l2Addr,
    void* userData) {
  auto* bcmSw = static_cast<BcmSwitch*>(userData);
  if (bcmSw->isL2EntryPending(l2Addr)) {
    bcmSw->callback_->l2LearningUpdateReceived(
        createL2Entry(l2Addr, bcmSw->isL2EntryPending(l2Addr)),
        getL2EntryUpdateType(BCM_L2_CALLBACK_ADD));
  }

  return 0;
}

int BcmSwitch::deleteL2TableCb(
    int /*unit*/,
    bcm_l2_addr_t* l2Addr,
    void* userData) {
  auto* bcmSw = static_cast<BcmSwitch*>(userData);
  bcmSw->callback_->l2LearningUpdateReceived(
      createL2Entry(l2Addr, bcmSw->isL2EntryPending(l2Addr)),
      getL2EntryUpdateType(BCM_L2_CALLBACK_DELETE));

  return 0;
}

void BcmSwitch::switchRunStateChangedImpl(SwitchRunState newState) {
  std::lock_guard<std::mutex> g(lock_);
  switch (newState) {
    case SwitchRunState::INITIALIZED:
      setupLinkscan();
      setupPacketRx();
      break;

    case SwitchRunState::CONFIGURED: {
      /*
       * When L2LearningMode::SOFTWARE is set, during warmboot time window,
       * there is no software running that can receive L2 table update
       * callbacks. However, the hardware continues to learn unknown source MAC
       * + VLAN as PENDING entries during that time window. Once agent comes
       * up, it must update its MAC table and VALIDATE the PENDING entries.
       */
      if (bootType_ == BootType::WARM_BOOT &&
          switchSettings_->getL2LearningMode().has_value()) {
        if (switchSettings_->getL2LearningMode().value() ==
            cfg::L2LearningMode::SOFTWARE) {
          auto rv = bcm_l2_traverse(
              getUnit(), BcmSwitch::addL2TableCbForPendingOnly, this);
          bcmCheckError(rv, "bcm_l2_traverse failed");
        }
      }
      break;
    }

    default:
      break;
  }
}

bool BcmSwitch::isPortEnabled(PortID port) const {
  return portTable_->getBcmPort(port)->isEnabled();
}

bool BcmSwitch::isPortUp(PortID port) const {
  return portTable_->getBcmPort(port)->isUp();
}

std::shared_ptr<SwitchState> BcmSwitch::getColdBootSwitchState() const {
  auto bootState = make_shared<SwitchState>();

  uint32_t flags = 0;
  for (const auto& kv : *portTable_) {
    uint32_t port_flags;
    PortID portID = kv.first;

    auto rv = bcm_port_learn_get(getUnit(), portID, &port_flags);
    bcmCheckError(rv, "Unable to get L2 Learning flags for port: ", portID);

    if (flags == 0) {
      flags = port_flags;
    } else if (flags != port_flags) {
      throw FbossError(
          "Every port should have same L2 Learning setting by default");
    }
  }

  // This is cold boot, so there cannot be any L2 update callback registered.
  // Thus, BCM_PORT_LEARN_ARL | BCM_PORT_LEARN_FWD should be enough to
  // ascertain HARDWARE as L2 learning mode.
  cfg::L2LearningMode l2LearningMode;
  if (flags == (BCM_PORT_LEARN_ARL | BCM_PORT_LEARN_FWD)) {
    l2LearningMode = cfg::L2LearningMode::HARDWARE;
  } else if (flags == (BCM_PORT_LEARN_ARL | BCM_PORT_LEARN_PENDING)) {
    l2LearningMode = cfg::L2LearningMode::SOFTWARE;
  } else {
    throw FbossError(
        "L2 Learning mode is neither SOFTWARE, nor HARDWARE, flags: ", flags);
  }

  auto switchSettings = make_shared<SwitchSettings>();
  switchSettings->setL2LearningMode(l2LearningMode);
  bootState->resetSwitchSettings(switchSettings);

  bcm_vlan_t defaultVlan;
  auto rv = bcm_vlan_default_get(getUnit(), &defaultVlan);
  bcmCheckError(rv, "Unable to get default VLAN");
  bootState->setDefaultVlan(VlanID(defaultVlan));
  // get cpu queue settings
  auto cpu = make_shared<ControlPlane>();
  auto cpuQueues = controlPlane_->getMulticastQueueSettings();
  auto rxReasonToQueue = controlPlane_->getRxReasonToQueue();
  cpu->resetQueues(cpuQueues);
  cpu->resetRxReasonToQueue(rxReasonToQueue);
  bootState->resetControlPlane(cpu);

  // On cold boot all ports are in Vlan 1
  auto vlan = make_shared<Vlan>(VlanID(1), "InitVlan");
  Vlan::MemberPorts memberPorts;
  for (const auto& kv : *portTable_) {
    PortID portID = kv.first;
    BcmPort* bcmPort = kv.second;
    string name = folly::to<string>("port", portID);

    auto swPort = make_shared<Port>(portID, name);
    auto platformPort = getPlatform()->getPlatformPort(portID);
    swPort->setProfileId(
        platformPort->getProfileIDBySpeed(bcmPort->getSpeed()));
    swPort->setSpeed(bcmPort->getSpeed());
    if (platform_->getAsic()->isSupported(HwAsic::Feature::L3_QOS)) {
      auto queues = bcmPort->getCurrentQueueSettings();
      swPort->resetPortQueues(queues);
    }
    bootState->addPort(swPort);

    memberPorts.insert(make_pair(portID, false));
  }
  vlan->setPorts(memberPorts);
  bootState->addVlan(vlan);
  bootState->publish();
  return bootState;
}

std::shared_ptr<SwitchState> BcmSwitch::applyAndGetWarmBootSwitchState() {
  auto warmBootState = warmBootCache_->getDumpedSwSwitchState().clone();

  // Ensure we have the correct oper state set post warm boot. Since
  // the SwSwitch Callback manages the oper state of a port, we want
  // to ensure the port status is consistent with reality. This also
  // enables warm boot testing to mirror reality w/out being attached
  // to SwSwitch.
  for (auto port : *warmBootState->getPorts()) {
    auto bcmPort = portTable_->getBcmPort(port->getID());
    auto newPort = port->modify(&warmBootState);
    auto isUp = bcmPort->isUp();
    auto wasUp = port->isUp();
    XLOG(INFO) << "Verifying oper state on port "
               << static_cast<int>(port->getID()) << "(" << port->getName()
               << "): wasUp=" << wasUp << ", isUp=" << isUp;
    if (wasUp != isUp) {
      // Use same log format as SwSwitch::linkStateChanged for grep purposes
      XLOG(INFO) << "Link state changed: " << static_cast<int>(port->getID())
                 << "->" << (isUp ? "UP" : "DOWN") << " (during warm boot)";
    }
    newPort->setOperState(isUp);
  }

  getPlatform()->preWarmbootStateApplied();

  warmBootState->publish();
  return stateChangedImpl(
      StateDelta(make_shared<SwitchState>(), warmBootState));
}

void BcmSwitch::runBcmScript(const std::string& filename) const {
  std::ifstream scriptFile(filename);

  if (!scriptFile.good()) {
    return;
  }

  XLOG(INFO) << "Run script " << filename;
  printDiagCmd(folly::to<string>("rcload ", filename));
}

void BcmSwitch::setupLinkscan() {
  if (!(getFeaturesDesired() & FeaturesDesired::LINKSCAN_DESIRED)) {
    XLOG(DBG1) << " Skipping linkscan registeration as the feature is disabled";
    return;
  }
  linkScanBottomHalfThread_ = std::make_unique<std::thread>([=]() {
    initThread("fbossLinkScanBH");
    linkScanBottomHalfEventBase_.loopForever();
  });
  auto rv = bcm_linkscan_register(unit_, linkscanCallback);
  bcmCheckError(rv, "failed to register for linkscan events");
  flags_ |= LINKSCAN_REGISTERED;
  rv = bcm_linkscan_enable_set(unit_, FLAGS_linkscan_interval_us);
  bcmCheckError(rv, "failed to enable linkscan");
  if (getBootType() == BootType::WARM_BOOT) {
    bcm_port_config_t pcfg;
    bcm_port_config_t_init(&pcfg);
    rv = bcm_port_config_get(unit_, &pcfg);
    bcmCheckError(rv, "failed to get port configuration");
    forceLinkscanOn(pcfg.port);
    // Sometimes after warmboot sw does not have the correct state of every port
    // so we need to update sw state here. This needs to be done after linkscan
    // is enabled otherwise the sdk may return inconsistent results
    for (auto& port : *portTable_) {
      callback_->linkStateChanged(port.first, port.second->isUp());
    }
  }
}

void BcmSwitch::setMacAging(std::chrono::seconds agingInterval) {
  /**
   * Enable MAC aging on the ASIC.
   * Every `seconds` seconds, all addresses
   * that have not been updated/used since the last age timer interval
   * are removed from the table.
   *
   * This is implemented by tracking the 'hit' bit in the L2 table.
   * On the aging timer, all MACs that have the hit bit set have it cleared,
   * and all MACs without the hit bit set are removed from the table.
   *
   * The ASIC will automatically set the hit bit anytime that MAC address is
   * looked up and used in a packet, either as a SRC lookup or DST lookup.
   *
   * Only set if timer is different than what's currently set.
   */
  int currentSeconds;
  int targetSeconds = agingInterval.count(); // force to 32bit
  bcmCheckError(
      bcm_l2_age_timer_get(unit_, &currentSeconds),
      "failed to get l2_age_timer");
  if (currentSeconds != targetSeconds) {
    XLOG(DBG1) << "Changing MAC Aging timer from " << currentSeconds << " to "
               << targetSeconds;
    bcmCheckError(
        bcm_l2_age_timer_set(unit_, targetSeconds),
        "failed to set l2_age_timer");
  }
}

HwInitResult BcmSwitch::init(Callback* callback) {
  HwInitResult ret;

  std::lock_guard<std::mutex> g(lock_);

  steady_clock::time_point begin = steady_clock::now();
  CHECK(!unitObject_);
  unitObject_ = BcmAPI::createOnlyUnit(platform_);
  unit_ = unitObject_->getNumber();
  unitObject_->setCookie(this);

  BcmAPI::initUnit(unit_, platform_);

  bootType_ = platform_->getWarmBootHelper()->canWarmBoot()
      ? BootType::WARM_BOOT
      : BootType::COLD_BOOT;
  auto warmBoot = bootType_ == BootType::WARM_BOOT;
  callback_ = callback;

  ret.initializedTime =
      duration_cast<duration<float>>(steady_clock::now() - begin).count();

  XLOG(INFO) << "Initializing BcmSwitch for unit " << unit_;

  // Add callbacks for unit and parity errors as early as possible to handle
  // critical events
  BcmSwitchEventUtils::initUnit(unit_, this);
  auto fatalCob = make_shared<BcmSwitchEventUnitFatalErrorCallback>();
  auto nonFatalCob = make_shared<BcmSwitchEventUnitNonFatalErrorCallback>();
  BcmSwitchEventUtils::registerSwitchEventCallback(
      unit_, BCM_SWITCH_EVENT_STABLE_FULL, fatalCob);
  BcmSwitchEventUtils::registerSwitchEventCallback(
      unit_, BCM_SWITCH_EVENT_STABLE_ERROR, fatalCob);
  BcmSwitchEventUtils::registerSwitchEventCallback(
      unit_, BCM_SWITCH_EVENT_UNCONTROLLED_SHUTDOWN, fatalCob);
  BcmSwitchEventUtils::registerSwitchEventCallback(
      unit_, BCM_SWITCH_EVENT_WARM_BOOT_DOWNGRADE, fatalCob);
  BcmSwitchEventUtils::registerSwitchEventCallback(
      unit_, BCM_SWITCH_EVENT_PARITY_ERROR, nonFatalCob);

  // Create bcmStatUpdater to cache the stat ids
  bcmStatUpdater_ = std::make_unique<BcmStatUpdater>(this, isAlpmEnabled());

  XLOG(INFO) << " Is ALPM enabled: " << isAlpmEnabled();
  // Additional switch configuration
  auto state = make_shared<SwitchState>();
  bcm_port_config_t pcfg;
  bcm_port_config_t_init(&pcfg);
  auto rv = bcm_port_config_get(unit_, &pcfg);
  bcmCheckError(rv, "failed to get port configuration");

  if (!warmBoot) {
    LOG(INFO) << " Performing cold boot ";
    /* initialize mirroring module */
    initMirrorModule();
    /* initialize MPLS */
    initMplsModule();
  } else {
    LOG(INFO) << "Performing warm boot ";
    // This dumps debug info about initial sdk state. Useful after warm boot.
    dumpState(platform_->getWarmBootHelper()->startupSdkDumpFile());
  }

  rv = bcm_switch_control_set(unit_, bcmSwitchL3EgressMode, 1);
  bcmCheckError(rv, "failed to set L3 egress mode");
  // Trap IPv4 Address Resolution Protocol (ARP) packets.
  // TODO: We may want to trap ARP on a per-port or per-VLAN basis.
  rv = bcm_switch_control_set(unit_, bcmSwitchArpRequestToCpu, 1);
  bcmCheckError(rv, "failed to set ARP request trapping");
  rv = bcm_switch_control_set(unit_, bcmSwitchArpReplyToCpu, 1);
  bcmCheckError(rv, "failed to set ARP reply trapping");
  // Trap IP header TTL or hoplimit 1 to CPU
  rv = bcm_switch_control_set(unit_, bcmSwitchL3UcastTtl1ToCpu, 1);
  bcmCheckError(rv, "failed to set L3 header error trapping");
  // Trap DHCP packets to CPU
  rv = bcm_switch_control_set(unit_, bcmSwitchDhcpPktToCpu, 1);
  bcmCheckError(rv, "failed to set DHCP packet trapping");
  // Trap Dest miss
  rv = bcm_switch_control_set(unit_, bcmSwitchUnknownL3DestToCpu, 1);
  bcmCheckError(rv, "failed to set destination miss trapping");
  rv = bcm_switch_control_set(unit_, bcmSwitchV6L3DstMissToCpu, 1);
  bcmCheckError(rv, "failed to set IPv6 destination miss trapping");
  // Trap IPv6 Neighbor Discovery Protocol (NDP) packets.
  // TODO: We may want to trap NDP on a per-port or per-VLAN basis.
  rv = bcm_switch_control_set(unit_, bcmSwitchNdPktToCpu, 1);
  bcmCheckError(rv, "failed to set NDP trapping");
  disableHotSwap();

  if (FLAGS_force_init_fp || !warmBoot || haveMissingOrQSetChangedFPGroups()) {
    initFieldProcessor();
    setupFPGroups();
  }

  dropDhcpPackets();
  setL3MtuFailPackets();
  mmuState_ = queryMmuState();

  // enable IPv4 and IPv6 on CPU port
  bcm_port_t idx;
  BCM_PBMP_ITER(pcfg.cpu, idx) {
    rv = bcm_port_control_set(unit_, idx, bcmPortControlIP4, 1);
    bcmCheckError(rv, "failed to enable IPv4 on cpu port ", idx);
    rv = bcm_port_control_set(unit_, idx, bcmPortControlIP6, 1);
    bcmCheckError(rv, "failed to enable IPv6 on cpu port ", idx);
    XLOG(DBG2) << "Enabled IPv4/IPv6 on CPU port " << idx;
  }

  // verify the drop egress ID is really dropping
  BcmEgress::verifyDropEgress(unit_);

  setupCos();

  if (warmBoot) {
    // This needs to be done after we have set
    // bcmSwitchL3EgressMode else the egress ids
    // in the host table don't show up correctly.
    warmBootCache_->populate();
  }
  setupToCpuEgress();
  portTable_->initPorts(&pcfg, warmBoot);

  bstStatsMgr_->startBufferStatCollection();

  // Set the spanning tree state of all ports to forwarding.
  // TODO: Eventually the spanning tree state should be part of the Port
  // state, and this should be handled in applyConfig().
  //
  // Spanning tree group settings
  // TODO: This should eventually be done as part of applyConfig()
  bcm_stg_t stg = 1;
  BCM_PBMP_ITER(pcfg.port, idx) {
    rv = bcm_stg_stp_set(unit_, stg, idx, BCM_STG_STP_FORWARD);
    bcmCheckError(rv, "failed to set spanning tree state on port ", idx);
  }

  ret.bootType = bootType_;

  if (warmBoot) {
    auto warmBootState = applyAndGetWarmBootSwitchState();
    hostTable_->warmBootHostEntriesSynced();
    ret.switchState = warmBootState;
    // Done with warm boot, clear warm boot cache
    warmBootCache_->clear();
    if (getPlatform()->getAsic()->getAsicType() ==
        HwAsic::AsicType::ASIC_TYPE_TRIDENT2) {
      for (auto ip : {folly::IPAddress("0.0.0.0"), folly::IPAddress("::")}) {
        bcm_l3_route_t rt;
        bcm_l3_route_t_init(&rt);
        rt.l3a_flags |= ip.isV6() ? BCM_L3_IP6 : 0;
        bcm_l3_route_get(getUnit(), &rt);
        rt.l3a_flags |= BCM_L3_REPLACE;
        bcm_l3_route_add(getUnit(), &rt);
      }
    }
  } else {
    ret.switchState = getColdBootSwitchState();
  }

  setMacAging(std::chrono::seconds(FLAGS_l2AgeTimerSeconds));

  macTable_ = std::make_unique<BcmMacTable>(this);

  ret.bootTime =
      duration_cast<duration<float>>(steady_clock::now() - begin).count();
  return ret;
}

void BcmSwitch::setupToCpuEgress() {
  // create an egress object for ToCPU
  toCPUEgress_ = make_unique<BcmEgress>(this);
  toCPUEgress_->programToCPU();
}

void BcmSwitch::setupPacketRx() {
  static bcm_rx_cfg_t rxCfg = {
      (16 * 1032), // packet alloc size (12K packets plus spare)
      16, // Packets per chain
      0, // Default pkt rate, global (all COS, one unit)
      0, // Burst
      {// 1 RX channel
       {0, 0, 0, 0}, // Channel 0 is usually TX
       {
           // Channel 1, default RX
           4, // DV count (number of chains)
           0, // Default pkt rate, DEPRECATED
           0, // No flags
           0xff // COS bitmap channel to receive
       }},
      nullptr, // Use default alloc function
      nullptr, // Use default free function
      0, // flags
      0, // Num CPU addrs
      nullptr // cpu_addresses
  };

  if (!(getFeaturesDesired() & FeaturesDesired::PACKET_RX_DESIRED)) {
    XLOG(DBG1) << " Skip setting up packet RX since its explicitly disabled";
    return;
  }
  // Register our packet handler callback function.
  uint32_t rxFlags = BCM_RCO_F_ALL_COS;
  auto rv = bcm_rx_register(
      unit_, // int unit
      "fboss_rx", // const char *name
      packetRxCallback, // bcm_rx_cb_f callback
      kRxCallbackPriority, // uint8 priority
      this, // void *cookie
      rxFlags); // uint32 flags
  bcmCheckError(rv, "failed to register packet rx callback");
  flags_ |= RX_REGISTERED;

  if (!isRxThreadRunning()) {
    rv = bcm_rx_start(unit_, &rxCfg);
  }
  bcmCheckError(rv, "failed to start broadcom packet rx API");
}

void BcmSwitch::processSwitchSettingsChanged(const StateDelta& delta) {
  const auto switchSettingsDelta = delta.getSwitchSettingsDelta();
  const auto& oldSwitchSettings = switchSettingsDelta.getOld();
  const auto& newSwitchSettings = switchSettingsDelta.getNew();

  /*
   * SwitchSettings are mandatory and can thus only be modified.
   * Every field in SwitchSettings must always be set in new SwitchState.
   */
  CHECK(oldSwitchSettings);
  CHECK(newSwitchSettings);

  if (oldSwitchSettings->getL2LearningMode() !=
      newSwitchSettings->getL2LearningMode()) {
    XLOG(DBG3) << "Configuring L2LearningMode old: "
               << static_cast<int>(oldSwitchSettings->getL2LearningMode())
               << " new: "
               << static_cast<int>(newSwitchSettings->getL2LearningMode());
    switchSettings_->setL2LearningMode(newSwitchSettings->getL2LearningMode());
  }

  if (oldSwitchSettings->isQcmEnable() != newSwitchSettings->isQcmEnable()) {
    XLOG(DBG3) << "Set QCM enable: " << std::boolalpha
               << newSwitchSettings->isQcmEnable();
    switchSettings_->setQcmEnable(
        newSwitchSettings->isQcmEnable(), delta.newState());
  }

  // Its possible during switch init, qcm is enabled
  // but all the ports are not enabled yet, so we trigger processing
  // ports on every cfg reload
  // QCM logic determines eligibility if there are any new ports
  // and update internally
  if (newSwitchSettings->isQcmEnable()) {
    if (oldSwitchSettings->isQcmEnable()) {
      // check whether config params changed on the fly
      qcmManager_->processQcmConfigChanged(delta.newState(), delta.oldState());
    }
    XLOG(DBG3) << "Qcm enabled. Evaluate the monitored ports";
    qcmManager_->processPortsForQcm(delta.newState());
  }
}

void BcmSwitch::processMacTableChanges(const StateDelta& stateDelta) {
  for (const auto& vlanDelta : stateDelta.getVlansDelta()) {
    auto vlanId = vlanDelta.getOld() ? vlanDelta.getOld()->getID()
                                     : vlanDelta.getNew()->getID();
    for (const auto& delta : vlanDelta.getMacDelta()) {
      const auto* oldEntry = delta.getOld().get();
      const auto* newEntry = delta.getNew().get();

      if (!oldEntry) {
        macTable_->processMacAdded(newEntry, vlanId);
      } else if (!newEntry) {
        macTable_->processMacRemoved(oldEntry, vlanId);
      } else {
        macTable_->processMacChanged(oldEntry, newEntry, vlanId);
      }
    }
  }
}

std::shared_ptr<SwitchState> BcmSwitch::stateChanged(const StateDelta& delta) {
  // Take the lock before modifying any objects
  std::lock_guard<std::mutex> g(lock_);
  auto appliedState = stateChangedImpl(delta);
  appliedState->publish();
  return appliedState;
}

std::shared_ptr<SwitchState> BcmSwitch::stateChangedImpl(
    const StateDelta& delta) {
  // Reconfigure port groups in case we are changing between using a port as
  // 1, 2 or 4 ports. Only do this if flexports are enabled
  // Calling reconfigure port group first to make sure the ports of SW state
  // already exists in HW.
  if (FLAGS_flexports) {
    reconfigurePortGroups(delta);
  }

  forEachAdded(delta.getPortsDelta(), [this](const auto& newPort) {
    if (!portTable_->getBcmPortIf(newPort->getID())) {
      throw FbossError(
          "Cannot add a port:", newPort->getID(), " unknown to hardware");
    }
  });

  forEachRemoved(delta.getPortsDelta(), [this](const auto& oldPort) {
    if (portTable_->getBcmPortIf(oldPort->getID())) {
      throw FbossError(
          "Cannot remove a port:", oldPort->getID(), " still in port table");
    }
  });
  auto appliedState = delta.newState();
  // TODO: This function contains high-level logic for how to apply the
  // StateDelta, and isn't particularly hardware-specific.  I plan to refactor
  // it, and move it out into a common helper class that can be shared by
  // many different HwSwitch implementations.

  // As the first step, disable ports that are now disabled.
  // This ensures that we immediately stop forwarding traffic on these ports.
  processDisabledPorts(delta);

  processSwitchSettingsChanged(delta);

  processMacTableChanges(delta);

  processLoadBalancerChanges(delta);

  CHECK(!bothStandAloneRibOrRouteTableRibUsed(delta));

  // remove all routes to be deleted
  processRemovedRoutes(delta);
  processRemovedFibRoutes(delta);

  // Any neighbor removals, and modify appliedState if some changes fail to
  // apply
  processNeighborDelta(delta, &appliedState, REMOVED);

  // delete all interface not existing anymore. that should stop
  // all traffic on that interface now
  forEachRemoved(delta.getIntfsDelta(), &BcmSwitch::processRemovedIntf, this);

  // Add all new VLANs, and modify VLAN port memberships.
  // We don't actually delete removed VLANs at this point, we simply remove
  // all members from the VLAN.  This way any ports that ingress packets to this
  // VLAN will still use this VLAN until we get the new VLAN fully configured.
  forEachChanged(
      delta.getVlansDelta(),
      &BcmSwitch::processChangedVlan,
      &BcmSwitch::processAddedVlan,
      &BcmSwitch::preprocessRemovedVlan,
      this);

  // Broadcom requires a default VLAN to always exist.
  // This VLAN is used as the default ingress VLAN for ports that don't have a
  // default ingress set.
  //
  // We always specify the ingress VLAN for all enabled ports, so this VLAN is
  // never really used for us.  We instead always point the default VLAN.
  if (delta.oldState()->getDefaultVlan() !=
      delta.newState()->getDefaultVlan()) {
    changeDefaultVlan(delta.newState()->getDefaultVlan());
  }

  // Update changed interfaces
  forEachChanged(delta.getIntfsDelta(), &BcmSwitch::processChangedIntf, this);

  // Remove deleted VLANs
  forEachRemoved(delta.getVlansDelta(), &BcmSwitch::processRemovedVlan, this);

  // Add all new interfaces
  forEachAdded(delta.getIntfsDelta(), &BcmSwitch::processAddedIntf, this);

  // Any changes to the Qos maps
  processQosChanges(delta);

  processControlPlaneChanges(delta);

  // Any neighbor additions/changes, and modify appliedState if some changes
  // fail to apply
  processNeighborDelta(delta, &appliedState, ADDED);
  processNeighborDelta(delta, &appliedState, CHANGED);

  // process label forwarding changes after neighbor entries are updated
  processChangedLabelForwardingInformationBase(delta);

  // Add/update mirrors before processing Acl and port changes
  // This is to ensure that port and acls can access latest mirrors
  forEachAdded(
      delta.getMirrorsDelta(),
      &BcmMirrorTable::processAddedMirror,
      writableBcmMirrorTable());
  forEachChanged(
      delta.getMirrorsDelta(),
      &BcmMirrorTable::processChangedMirror,
      writableBcmMirrorTable());

  // Any ACL changes
  processAclChanges(delta);

  // Any changes to the set of sFlow collectors
  processSflowCollectorChanges(delta);

  // Any changes to the sampling rate of sflow
  processSflowSamplingRateChanges(delta);

  // Process any new routes or route changes
  processAddedChangedRoutes(delta, &appliedState);
  processAddedChangedFibRoutes(delta, &appliedState);

  processAggregatePortChanges(delta);

  processAddedPorts(delta);
  processChangedPorts(delta);

  // delete any removed mirrors after processing port and acl changes
  forEachRemoved(
      delta.getMirrorsDelta(),
      &BcmMirrorTable::processRemovedMirror,
      writableBcmMirrorTable());

  pickupLinkStatusChanges(delta);

  // As the last step, enable newly enabled ports.  Doing this as the
  // last step ensures that we only start forwarding traffic once the
  // ports are correctly configured. Note that this will also set the
  // ingressVlan and speed correctly before enabling.
  processEnabledPorts(delta);

  bcmStatUpdater_->refreshPostBcmStateChange(delta);

  return appliedState;
}

unique_ptr<TxPacket> BcmSwitch::allocatePacket(uint32_t size) const {
  // For future reference: Allocating the packet data requires the unit number
  // of the unit that the packet will be used with.  Our allocatePacket() API
  // doesn't require the caller to specify which ports they plan to use the
  // packet with.
  //
  // At the moment we only support a single unit, so this isn't really an
  // issue.  However, it may be more challenging for a HwSwitch implementation
  // that supports multiple units.  Fortunately, the linux userspace
  // implemetation uses the same DMA pool for all local units, so it wouldn't
  // really matter which unit we specified when allocating the buffer.
  return make_unique<BcmTxPacket>(unit_, size, this);
}

void BcmSwitch::processDisabledPorts(const StateDelta& delta) {
  forEachChanged(
      delta.getPortsDelta(),
      [&](const shared_ptr<Port>& oldPort, const shared_ptr<Port>& newPort) {
        if (oldPort->isEnabled() && !newPort->isEnabled()) {
          auto bcmPort = portTable_->getBcmPort(newPort->getID());
          XLOG(INFO) << "Disabling port: " << newPort->getID();
          bcmPort->disable(newPort);
        }
      });
}

void BcmSwitch::processEnabledPortQueues(const shared_ptr<Port>& port) {
  auto id = port->getID();
  auto bcmPort = portTable_->getBcmPort(id);

  for (const auto& queue : port->getPortQueues()) {
    XLOG(DBG1) << "Enable cos queue settings on port " << port->getID()
               << " queue: " << static_cast<int>(queue->getID());
    bcmPort->setupQueue(*queue);
  }
}

void BcmSwitch::processEnabledPorts(const StateDelta& delta) {
  forEachChanged(
      delta.getPortsDelta(),
      [&](const shared_ptr<Port>& oldPort, const shared_ptr<Port>& newPort) {
        if (!newPort->isEnabled()) {
          // skip if the port is not even enabled
          return;
        }
        // There're only two cases that we should enable the port:
        // 1) If the old port is disabled
        // 2) If the speed changes
        if (!oldPort->isEnabled() ||
            (oldPort->getSpeed() != newPort->getSpeed())) {
          auto bcmPort = portTable_->getBcmPort(newPort->getID());
          bcmPort->enable(newPort);
          processEnabledPortQueues(newPort);
        }
      });

  forEachAdded(delta.getPortsDelta(), [&](const shared_ptr<Port>& port) {
    // For any newly created port, as long as it needs enable, always enable
    if (port->isEnabled()) {
      auto bcmPort = portTable_->getBcmPort(port->getID());
      bcmPort->enable(port);
      processEnabledPortQueues(port);
    }
  });
}

void BcmSwitch::processChangedPortQueues(
    const shared_ptr<Port>& oldPort,
    const shared_ptr<Port>& newPort) {
  auto id = newPort->getID();
  auto bcmPort = portTable_->getBcmPort(id);

  // We expect the number of port queues to remain constant because this is
  // defined by the hardware
  for (const auto& newQueue : newPort->getPortQueues()) {
    if (oldPort->getPortQueues().size() > 0 &&
        *(oldPort->getPortQueues().at(newQueue->getID())) == *newQueue) {
      continue;
    }

    XLOG(DBG1) << "New cos queue settings on port " << id << " queue "
               << static_cast<int>(newQueue->getID());
    bcmPort->setupStatsIfNeeded(newPort);
    bcmPort->setupQueue(*newQueue);
  }
}

void BcmSwitch::processAddedPorts(const StateDelta& delta) {
  // For now, this just enables stats on newly added ports
  forEachAdded(delta.getPortsDelta(), [&](const shared_ptr<Port>& port) {
    auto id = port->getID();
    auto bcmPort = portTable_->getBcmPort(id);

    if (port->isEnabled()) {
      // This ensures all the settings are up to date. We need to
      // be very careful that program() is both idempotent and
      // does not cause port flaps if called with the same
      // settings twice.
      bcmPort->program(port);
      bcmPort->enableStatCollection(port);
    }
  });
}

void BcmSwitch::processChangedPorts(const StateDelta& delta) {
  forEachChanged(
      delta.getPortsDelta(),
      [&](const shared_ptr<Port>& oldPort, const shared_ptr<Port>& newPort) {
        auto id = newPort->getID();
        auto bcmPort = portTable_->getBcmPort(id);

        if (!oldPort->isEnabled() && !newPort->isEnabled()) {
          // No need to process changes on disabled ports. We will pick up
          // changes should the port ever become enabled.
          return;
        }

        auto speedChanged = oldPort->getSpeed() != newPort->getSpeed();
        XLOG_IF(DBG1, speedChanged) << "New speed on port " << id;

        auto profileIDChanged =
            oldPort->getProfileID() != newPort->getProfileID();
        XLOG_IF(DBG1, profileIDChanged) << "New profileID on port " << id;

        auto vlanChanged =
            oldPort->getIngressVlan() != newPort->getIngressVlan();
        XLOG_IF(DBG1, vlanChanged) << "New ingress vlan on port " << id;

        auto pauseChanged = oldPort->getPause() != newPort->getPause();
        XLOG_IF(DBG1, pauseChanged) << "New pause settings on port " << id;

        auto sFlowChanged = (oldPort->getSflowIngressRate() !=
                             newPort->getSflowIngressRate()) ||
            (oldPort->getSflowEgressRate() != newPort->getSflowEgressRate());
        XLOG_IF(DBG1, sFlowChanged) << "New sFlow settings on port " << id;

        auto fecChanged = oldPort->getFEC() != newPort->getFEC();
        XLOG_IF(DBG1, fecChanged) << "New FEC settings on port " << id;

        auto loopbackChanged =
            oldPort->getLoopbackMode() != newPort->getLoopbackMode();
        XLOG_IF(DBG1, loopbackChanged)
            << "New loopback mode settings on port " << id;

        auto mirrorChanged =
            (oldPort->getIngressMirror() != newPort->getIngressMirror()) ||
            (oldPort->getEgressMirror() != newPort->getEgressMirror());
        XLOG_IF(DBG1, mirrorChanged) << "New mirror settings on port " << id;

        auto qosPolicyChanged =
            oldPort->getQosPolicy() != newPort->getQosPolicy();
        XLOG_IF(DBG1, qosPolicyChanged) << "New Qos Policy on port " << id;

        auto nameChanged = oldPort->getName() != newPort->getName();
        XLOG_IF(DBG1, nameChanged) << "New name on port " << id;

        auto asicPrbsChanged = oldPort->getAsicPrbs() != newPort->getAsicPrbs();
        XLOG_IF(DBG1, asicPrbsChanged) << "New asicPrbs on port " << id;

        if (speedChanged || profileIDChanged || vlanChanged || pauseChanged ||
            sFlowChanged || fecChanged || loopbackChanged || mirrorChanged ||
            qosPolicyChanged || nameChanged || asicPrbsChanged) {
          bcmPort->program(newPort);
        }

        if (newPort->getPortQueues().size() != 0 &&
            !platform_->getAsic()->isSupported(HwAsic::Feature::L3_QOS)) {
          throw FbossError(
              "Changing settings for cos queues not supported on ",
              "this platform");
        }

        processChangedPortQueues(oldPort, newPort);
      });
}

void BcmSwitch::pickupLinkStatusChanges(const StateDelta& delta) {
  forEachChanged(
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
          auto bcmPort = portTable_->getBcmPort(id);
          bcmPort->linkStatusChanged(newPort);
        }
      });
}

void BcmSwitch::reconfigurePortGroups(const StateDelta& delta) {
  // This logic is a bit messy. We could encode some notion of port
  // groups into the switch state somehow so it is easy to generate
  // deltas for these. For now, we need pass around the SwitchState
  // object and get the relevant ports manually.

  // Note that reconfigurePortGroups will program the speed and enable
  // newly enabled ports in its group. This means it can overlap a bit
  // with the work done in processEnabledPorts and processChangedPorts.
  // Both BcmPort::program and BcmPort::enable should be no-ops if already
  // programmed or already enabled. However, this MUST BE called before
  // those methods as enabling or changing the speed of a port may require
  // changing the configuration of a port group.

  auto oldState = delta.oldState();
  auto newState = delta.newState();
  forEachChanged(
      delta.getPortsDelta(),
      [&](const shared_ptr<Port>& oldPort, const shared_ptr<Port>& newPort) {
        auto enabled = !oldPort->isEnabled() && newPort->isEnabled();
        auto speedProfileChanged =
            oldPort->getProfileID() != newPort->getProfileID();
        auto sFlowChanged = (oldPort->getSflowIngressRate() !=
                             newPort->getSflowIngressRate()) ||
            (oldPort->getSflowEgressRate() != newPort->getSflowEgressRate());

        if (enabled || speedProfileChanged || sFlowChanged) {
          if (!isValidPortUpdate(oldPort, newPort, newState)) {
            // Fail hard
            throw FbossError("Invalid port configuration passed in ");
          }
          auto bcmPort = portTable_->getBcmPort(newPort->getID());
          auto portGroup = bcmPort->getPortGroup();
          if (portGroup) {
            portGroup->reconfigureIfNeeded(oldState, newState);
          }
        }
      });
}

bool BcmSwitch::isValidPortUpdate(
    const shared_ptr<Port>& oldPort,
    const shared_ptr<Port>& newPort,
    const shared_ptr<SwitchState>& newState) const {
  if (auto mirrorName = newPort->getIngressMirror()) {
    auto mirror = newState->getMirrors()->getMirrorIf(mirrorName.value());
    if (!mirror) {
      XLOG(ERR) << "Ingress mirror " << mirrorName.value()
                << " for port : " << newPort->getID() << " not found";
      return false;
    }
  }

  if (auto mirrorName = newPort->getEgressMirror()) {
    auto mirror = newState->getMirrors()->getMirrorIf(mirrorName.value());
    if (!mirror) {
      XLOG(ERR) << "Egress mirror " << mirrorName.value()
                << " for port : " << newPort->getID() << " not found";
      return false;
    }
  }

  if (newPort->getSampleDestination() &&
      newPort->getSampleDestination().value() ==
          cfg::SampleDestination::MIRROR) {
    auto ingressMirrorName = newPort->getIngressMirror();
    if (!ingressMirrorName) {
      XLOG(ERR)
          << "Ingress mirror  not set for sampled port with sample destination of mirror not set : "
          << newPort->getID();
      return false;
    }
    auto ingressMirror =
        newState->getMirrors()->getMirrorIf(ingressMirrorName.value());
    if (ingressMirror->type() != Mirror::Type::SFLOW) {
      XLOG(ERR) << "Ingress mirror " << ingressMirrorName.value()
                << " for sampled port not sflow : " << newPort->getID();
      return false;
    }

    auto egressMirrorName = newPort->getEgressMirror();
    if (egressMirrorName) {
      XLOG(ERR) << "Egress mirror " << ingressMirrorName.value()
                << " for sampled port set on : " << newPort->getID()
                << " , egress sampling is not supported!";
      return false;
    }
  }

  auto enabled = !oldPort->isEnabled() && newPort->isEnabled();
  auto speedChanged = oldPort->getSpeed() != newPort->getSpeed();

  if (speedChanged || enabled) {
    auto bcmPort = portTable_->getBcmPort(newPort->getID());
    auto portGroup = bcmPort->getPortGroup();
    XLOG(DBG1) << "Verifying port group config for : " << newPort->getID();
    return !portGroup || portGroup->validConfiguration(newState);
  }
  if (!isValidPortQueueUpdate(
          oldPort->getPortQueues(), newPort->getPortQueues())) {
    return false;
  }
  if (!isValidPortQosPolicyUpdate(oldPort, newPort, newState)) {
    return false;
  }

  return true;
}

template <typename AddrT>
bool BcmSwitch::isRouteUpdateValid(const StateDelta& delta) const {
  using RouteT = Route<AddrT>;

  auto maxLabelStackDepth = getPlatform()->getAsic()->getMaxLabelStackDepth();
  auto validateLabeledRoute = [maxLabelStackDepth](const auto& route) {
    for (const auto& nhop : route->getForwardInfo().getNextHopSet()) {
      if (!nhop.labelForwardingAction()) {
        continue;
      }
      if (nhop.labelForwardingAction()->type() !=
          LabelForwardingAction::LabelForwardingType::PUSH) {
        return false;
      } else {
        if (nhop.labelForwardingAction()->pushStack()->size() >
            maxLabelStackDepth) {
          return false;
        }
      }
    }
    return true;
  };

  bool isValid = true;
  for (const auto& rDelta : delta.getRouteTablesDelta()) {
    forEachChanged(
        rDelta.template getRoutesDelta<AddrT>(),
        [&isValid, validateLabeledRoute](
            const std::shared_ptr<RouteT>& /*oldRoute*/,
            const std::shared_ptr<RouteT>& newRoute) {
          if (!validateLabeledRoute(newRoute)) {
            isValid = false;
            return LoopAction::BREAK;
          }
          return LoopAction::CONTINUE;
        },
        [&isValid,
         validateLabeledRoute](const std::shared_ptr<RouteT>& addedRoute) {
          if (!validateLabeledRoute(addedRoute)) {
            isValid = false;
            return LoopAction::BREAK;
          }
          return LoopAction::CONTINUE;
        },
        [](const std::shared_ptr<RouteT>& /*removedRoute*/) {});
  }
  return isValid;
}

bool BcmSwitch::isValidStateUpdate(const StateDelta& delta) const {
  auto newState = delta.newState();
  auto isValid = true;

  forEachChanged(
      delta.getPortsDelta(),
      [&](const shared_ptr<Port>& oldPort, const shared_ptr<Port>& newPort) {
        if (isValid && !isValidPortUpdate(oldPort, newPort, newState)) {
          isValid = false;
        }
      });
  isValid = isValid &&
      (newState->getMirrors()->size() <= platform_->getAsic()->getMaxMirrors());

  forEachChanged(
      delta.getMirrorsDelta(),
      [&](const shared_ptr<Mirror>& /* oldMirror */,
          const shared_ptr<Mirror>& newMirror) {
        if (newMirror->getTruncate() &&
            !getPlatform()->mirrorPktTruncationSupported()) {
          XLOG(ERR)
              << "Mirror packet truncation is not supported on this platform";
          isValid = false;
        }
      });

  int sflowMirrorCount = 0;
  for (const auto& mirror : *(newState->getMirrors())) {
    if (mirror->type() == Mirror::Type::SFLOW) {
      sflowMirrorCount++;
    }
  }

  if (sflowMirrorCount > 1) {
    XLOG(ERR) << "More than one sflow mirrors configured";
    isValid = false;
  }
  forEachAdded(
      delta.getQosPoliciesDelta(),
      [&](const std::shared_ptr<QosPolicy>& qosPolicy) {
        isValid = isValid && BcmQosPolicyTable::isValid(qosPolicy);
      });

  forEachChanged(
      delta.getQosPoliciesDelta(),
      [&](const std::shared_ptr<QosPolicy>& oldQosPolicy,
          const std::shared_ptr<QosPolicy>& newQosPolicy) {
        isValid = isValid && BcmQosPolicyTable::isValid(newQosPolicy);
      });

  forEachChanged(
      delta.getLabelForwardingInformationBaseDelta(),
      [&](const std::shared_ptr<LabelForwardingEntry>& /*oldEntry*/,
          const std::shared_ptr<LabelForwardingEntry>& newEntry) {
        // changed Fn
        isValid = isValid && isValidLabelForwardingEntry(newEntry.get());
      },
      [&](const std::shared_ptr<LabelForwardingEntry>& newEntry) {
        // added Fn
        isValid = isValid && isValidLabelForwardingEntry(newEntry.get());
      },
      [](const std::shared_ptr<LabelForwardingEntry>& /*oldEntry*/) {
        // removed Fn
      });

  isValid = isValid && isRouteUpdateValid<folly::IPAddressV4>(delta);
  isValid = isValid && isRouteUpdateValid<folly::IPAddressV6>(delta);

  return isValid;
}

void BcmSwitch::changeDefaultVlan(VlanID id) {
  auto rv = bcm_vlan_default_set(unit_, id);
  bcmCheckError(rv, "failed to set default VLAN to ", id);
}

void BcmSwitch::processChangedVlan(
    const shared_ptr<Vlan>& oldVlan,
    const shared_ptr<Vlan>& newVlan) {
  // Update port membership
  bcm_pbmp_t addedPorts;
  BCM_PBMP_CLEAR(addedPorts);
  bcm_pbmp_t addedUntaggedPorts;
  BCM_PBMP_CLEAR(addedUntaggedPorts);
  bcm_pbmp_t removedPorts;
  BCM_PBMP_CLEAR(removedPorts);
  const auto& oldPorts = oldVlan->getPorts();
  const auto& newPorts = newVlan->getPorts();

  auto oldIter = oldPorts.begin();
  auto newIter = newPorts.begin();
  uint32_t numAdded{0};
  uint32_t numRemoved{0};
  while (oldIter != oldPorts.end() || newIter != newPorts.end()) {
    if (oldIter == oldPorts.end() ||
        (newIter != newPorts.end() && newIter->first < oldIter->first)) {
      // This port was added
      ++numAdded;
      bcm_port_t bcmPort = portTable_->getBcmPortId(newIter->first);
      BCM_PBMP_PORT_ADD(addedPorts, bcmPort);
      if (!newIter->second.tagged) {
        BCM_PBMP_PORT_ADD(addedUntaggedPorts, bcmPort);
      }
      ++newIter;
    } else if (
        newIter == newPorts.end() ||
        (oldIter != oldPorts.end() && oldIter->first < newIter->first)) {
      // This port was removed
      ++numRemoved;
      bcm_port_t bcmPort = portTable_->getBcmPortId(oldIter->first);
      BCM_PBMP_PORT_ADD(removedPorts, bcmPort);
      ++oldIter;
    } else {
      ++oldIter;
      ++newIter;
    }
  }

  XLOG(DBG2) << "updating VLAN " << newVlan->getID() << ": " << numAdded
             << " ports added, " << numRemoved << " ports removed";
  if (numRemoved) {
    auto rv = bcm_vlan_port_remove(unit_, newVlan->getID(), removedPorts);
    bcmCheckError(rv, "failed to remove ports from VLAN ", newVlan->getID());
  }
  if (numAdded) {
    auto rv = bcm_vlan_port_add(
        unit_, newVlan->getID(), addedPorts, addedUntaggedPorts);
    bcmCheckError(rv, "failed to add ports to VLAN ", newVlan->getID());
  }
}

void BcmSwitch::processAddedVlan(const shared_ptr<Vlan>& vlan) {
  XLOG(DBG2) << "creating VLAN " << vlan->getID() << " with "
             << vlan->getPorts().size() << " ports";

  bcm_pbmp_t pbmp;
  bcm_pbmp_t ubmp;
  BCM_PBMP_CLEAR(pbmp);
  BCM_PBMP_CLEAR(ubmp);

  for (const auto& entry : vlan->getPorts()) {
    bcm_port_t bcmPort = portTable_->getBcmPortId(entry.first);
    BCM_PBMP_PORT_ADD(pbmp, bcmPort);
    if (!entry.second.tagged) {
      BCM_PBMP_PORT_ADD(ubmp, bcmPort);
    }
  }
  typedef BcmWarmBootCache::VlanInfo VlanInfo;
  // Since during warm boot all VLAN in the config will show
  // up as added VLANs we only need to consult the warm boot
  // cache here.
  auto vlanItr = warmBootCache_->findVlanInfo(vlan->getID());
  if (vlanItr != warmBootCache_->vlan2VlanInfo_end()) {
    // Compare with existing vlan to determine if we need to reprogram
    auto equivalent = [](VlanInfo newVlan, VlanInfo existingVlan) {
      return BCM_PBMP_EQ(newVlan.allPorts, existingVlan.allPorts) &&
          BCM_PBMP_EQ(newVlan.untagged, existingVlan.untagged);
    };
    const auto& existingVlan = vlanItr->second;
    if (!equivalent(VlanInfo(vlan->getID(), ubmp, pbmp), existingVlan)) {
      XLOG(DBG1) << "updating VLAN " << vlan->getID() << " with "
                 << vlan->getPorts().size() << " ports";
      auto oldVlan = vlan->clone();
      warmBootCache_->fillVlanPortInfo(oldVlan.get());
      processChangedVlan(oldVlan, vlan);
    } else {
      XLOG(DBG1) << " Vlan : " << vlan->getID() << " already exists ";
    }
    warmBootCache_->programmed(vlanItr);
  } else {
    XLOG(DBG1) << "creating VLAN " << vlan->getID() << " with "
               << vlan->getPorts().size() << " ports";
    auto rv = bcm_vlan_create(unit_, vlan->getID());
    bcmCheckError(rv, "failed to add VLAN ", vlan->getID());
    rv = bcm_vlan_port_add(unit_, vlan->getID(), pbmp, ubmp);
    bcmCheckError(rv, "failed to add members to new VLAN ", vlan->getID());
  }
}

void BcmSwitch::preprocessRemovedVlan(const shared_ptr<Vlan>& vlan) {
  // Remove all ports from this VLAN at this phase.
  XLOG(DBG2) << "preparing to remove VLAN " << vlan->getID();
  auto rv = bcm_vlan_gport_delete_all(unit_, vlan->getID());
  bcmCheckError(rv, "failed to remove members from VLAN ", vlan->getID());
}

void BcmSwitch::processRemovedVlan(const shared_ptr<Vlan>& vlan) {
  XLOG(DBG2) << "removing VLAN " << vlan->getID();

  auto rv = bcm_vlan_destroy(unit_, vlan->getID());
  bcmCheckError(rv, "failed to remove VLAN ", vlan->getID());
}

void BcmSwitch::processChangedIntf(
    const shared_ptr<Interface>& oldIntf,
    const shared_ptr<Interface>& newIntf) {
  CHECK_EQ(oldIntf->getID(), newIntf->getID());
  XLOG(DBG2) << "changing interface " << oldIntf->getID();
  intfTable_->programIntf(newIntf);
}

void BcmSwitch::processAddedIntf(const shared_ptr<Interface>& intf) {
  XLOG(DBG2) << "adding interface " << intf->getID();
  intfTable_->addIntf(intf);
}

void BcmSwitch::processRemovedIntf(const shared_ptr<Interface>& intf) {
  XLOG(DBG2) << "deleting interface " << intf->getID();
  intfTable_->deleteIntf(intf);
}

void BcmSwitch::processQosChanges(const StateDelta& delta) {
  auto oldDefaultQosPolicy = delta.oldState()
      ? delta.oldState()->getDefaultDataPlaneQosPolicy()
      : nullptr;
  auto newDefaultQosPolicy = delta.newState()
      ? delta.newState()->getDefaultDataPlaneQosPolicy()
      : nullptr;

  if (oldDefaultQosPolicy && newDefaultQosPolicy) {
    qosPolicyTable_->processChangedDefaultDataPlaneQosPolicy(
        oldDefaultQosPolicy, newDefaultQosPolicy);
  } else if (oldDefaultQosPolicy) {
    qosPolicyTable_->processRemovedDefaultDataPlaneQosPolicy(
        oldDefaultQosPolicy);
  } else if (newDefaultQosPolicy) {
    qosPolicyTable_->processAddedDefaultDataPlaneQosPolicy(newDefaultQosPolicy);
  }

  forEachChanged(
      delta.getQosPoliciesDelta(),
      &BcmQosPolicyTable::processChangedQosPolicy,
      &BcmQosPolicyTable::processAddedQosPolicy,
      &BcmQosPolicyTable::processRemovedQosPolicy,
      qosPolicyTable_.get());
}

void BcmSwitch::processAclChanges(const StateDelta& delta) {
  forEachChanged(
      delta.getAclsDelta(),
      &BcmSwitch::processChangedAcl,
      &BcmSwitch::processAddedAcl,
      &BcmSwitch::processRemovedAcl,
      this);
}

void BcmSwitch::processAggregatePortChanges(const StateDelta& delta) {
  forEachChanged(
      delta.getAggregatePortsDelta(),
      &BcmSwitch::processChangedAggregatePort,
      &BcmSwitch::processAddedAggregatePort,
      &BcmSwitch::processRemovedAggregatePort,
      this);
}

void BcmSwitch::processChangedSflowCollector(
    const std::shared_ptr<SflowCollector>& /* oldCollector */,
    const std::shared_ptr<SflowCollector>& /* newCollector */) {
  XLOG(ERR) << "sFlow collector should should only change on restarts";
}

void BcmSwitch::processRemovedSflowCollector(
    const std::shared_ptr<SflowCollector>& collector) {
  if (!sFlowExporterTable_->contains(collector)) {
    throw FbossError("Tried to remove non-existent sFlow exporter");
  }

  sFlowExporterTable_->removeExporter(collector->getID());
}

void BcmSwitch::processAddedSflowCollector(
    const shared_ptr<SflowCollector>& collector) {
  if (sFlowExporterTable_->contains(collector)) {
    // Something is wrong.
    throw FbossError("Tried to add an existing sFlow exporter");
  }

  sFlowExporterTable_->addExporter(collector);
}

void BcmSwitch::processSflowSamplingRateChanges(const StateDelta& delta) {
  forEachChanged(
      delta.getPortsDelta(),
      [&](const shared_ptr<Port>& oldPort, const shared_ptr<Port>& newPort) {
        auto oldIngressRate = oldPort->getSflowIngressRate();
        auto oldEgressRate = oldPort->getSflowEgressRate();
        auto newIngressRate = newPort->getSflowIngressRate();
        auto newEgressRate = newPort->getSflowEgressRate();
        auto sFlowChanged = (oldIngressRate != newIngressRate) ||
            (oldEgressRate != newEgressRate);
        if (sFlowChanged) {
          auto id = newPort->getID();
          sFlowExporterTable_->updateSamplingRates(
              id, newIngressRate, newEgressRate);
        }
      });
}

void BcmSwitch::processSflowCollectorChanges(const StateDelta& delta) {
  forEachChanged(
      delta.getSflowCollectorsDelta(),
      &BcmSwitch::processChangedSflowCollector,
      &BcmSwitch::processAddedSflowCollector,
      &BcmSwitch::processRemovedSflowCollector,
      this);
}

template <typename NeighborEntryT>
void BcmSwitch::processAddedAndChangedNeighbor(
    const BcmHostKey& neighborKey,
    const BcmIntf* intf,
    const NeighborEntryT* entry) {
  auto* neighbor = neighborTable_->getNeighbor(neighborKey);
  CHECK(neighbor);
  if (entry->isPending()) {
    hostTable_->programHostsToCPU(neighborKey, intf->getBcmIfId());
    return;
  }
  auto neighborMac = entry->getMac();
  auto isTrunk = entry->getPort().isAggregatePort();
  if (isTrunk) {
    auto trunk = entry->getPort().aggPortID();
    hostTable_->programHostsToTrunk(
        neighborKey,
        intf->getBcmIfId(),
        neighborMac,
        getTrunkTable()->getBcmTrunkId(trunk));
  } else {
    auto port = entry->getPort().phyPortID();
    hostTable_->programHostsToPort(
        neighborKey,
        intf->getBcmIfId(),
        neighborMac,
        getPortTable()->getBcmPortId(port),
        entry->getClassID());
  }
  std::for_each(
      writableMplsNextHopTable()->getNextHops().begin(),
      writableMplsNextHopTable()->getNextHops().end(),
      [neighborKey](const auto& entry) {
        auto nhop = entry.second.lock();
        if (nhop->getBcmHostKey() == neighborKey) {
          nhop->program(neighborKey);
        }
      });
}

template <typename NeighborEntryT>
void BcmSwitch::processAddedNeighborEntry(const NeighborEntryT* addedEntry) {
  CHECK(addedEntry);
  if (addedEntry->isPending()) {
    XLOG(DBG3) << "adding pending neighbor entry to "
               << addedEntry->getIP().str()
               << (addedEntry->getClassID().has_value()
                       ? static_cast<int>(addedEntry->getClassID().value())
                       : 0);
  } else {
    XLOG(DBG3) << "adding neighbor entry " << addedEntry->getIP().str()
               << " to " << addedEntry->getMac().toString()
               << (addedEntry->getClassID().has_value()
                       ? static_cast<int>(addedEntry->getClassID().value())
                       : 0);
  }

  const auto* intf = getIntfTable()->getBcmIntf(addedEntry->getIntfID());
  auto vrf = getBcmVrfId(intf->getInterface()->getRouterID());

  auto neighborKey =
      BcmHostKey(vrf, IPAddress(addedEntry->getIP()), addedEntry->getIntfID());
  neighborTable_->registerNeighbor(neighborKey);
  processAddedAndChangedNeighbor(neighborKey, intf, addedEntry);
}

template <typename NeighborEntryT>
void BcmSwitch::processChangedNeighborEntry(
    const NeighborEntryT* oldEntry,
    const NeighborEntryT* newEntry) {
  CHECK(oldEntry);
  CHECK(newEntry);
  CHECK_EQ(oldEntry->getIP(), newEntry->getIP());
  if (newEntry->isPending()) {
    XLOG(DBG3) << "changing neighbor entry " << newEntry->getIP().str()
               << " classID: "
               << (newEntry->getClassID().has_value()
                       ? static_cast<int>(newEntry->getClassID().value())
                       : 0)
               << " to pending";
  } else {
    XLOG(DBG3) << "changing neighbor entry " << newEntry->getIP().str()
               << " classID: "
               << (newEntry->getClassID().has_value()
                       ? static_cast<int>(newEntry->getClassID().value())
                       : 0)
               << " to " << newEntry->getMac().toString();
  }

  const auto* intf = getIntfTable()->getBcmIntf(newEntry->getIntfID());
  auto vrf = getBcmVrfId(intf->getInterface()->getRouterID());

  auto neighborKey =
      BcmHostKey(vrf, IPAddress(newEntry->getIP()), newEntry->getIntfID());

  processAddedAndChangedNeighbor(neighborKey, intf, newEntry);
}

template <typename NeighborEntryT>
void BcmSwitch::processRemovedNeighborEntry(
    const NeighborEntryT* removedEntry) {
  CHECK(removedEntry);
  XLOG(DBG3) << "deleting neighbor entry " << removedEntry->getIP().str();

  const auto* intf = getIntfTable()->getBcmIntf(removedEntry->getIntfID());
  auto vrf = getBcmVrfId(intf->getInterface()->getRouterID());

  auto neighborKey = BcmHostKey(
      vrf, IPAddress(removedEntry->getIP()), removedEntry->getIntfID());
  neighborTable_->unregisterNeighbor(neighborKey);
  hostTable_->programHostsToCPU(neighborKey, intf->getBcmIfId());
  std::for_each(
      writableMplsNextHopTable()->getNextHops().begin(),
      writableMplsNextHopTable()->getNextHops().end(),
      [neighborKey](const auto& entry) {
        auto nhop = entry.second.lock();
        if (nhop->getBcmHostKey() == neighborKey) {
          nhop->program(neighborKey);
        }
      });
}

void BcmSwitch::processNeighborDelta(
    const StateDelta& delta,
    std::shared_ptr<SwitchState>* appliedState,
    DeltaType optype) {
  processNeighborTableDelta<folly::IPAddressV4>(delta, appliedState, optype);
  processNeighborTableDelta<folly::IPAddressV6>(delta, appliedState, optype);
}

template <typename AddrT>
void BcmSwitch::processNeighborTableDelta(
    const StateDelta& stateDelta,
    std::shared_ptr<SwitchState>* appliedState,
    DeltaType optype) {
  using NeighborTableT = std::conditional_t<
      std::is_same<AddrT, folly::IPAddressV4>::value,
      ArpTable,
      NdpTable>;
  using NeighborEntryT = typename NeighborTableT::Entry;
  using NeighborEntryDeltaT = DeltaValue<NeighborEntryT>;
  std::vector<NeighborEntryDeltaT> discardedNeighborEntryDelta;

  for (const auto& vlanDelta : stateDelta.getVlansDelta()) {
    for (const auto& delta :
         vlanDelta.template getNeighborDelta<NeighborTableT>()) {
      try {
        const auto* oldEntry = delta.getOld().get();
        const auto* newEntry = delta.getNew().get();

        if (optype == ADDED && !oldEntry) {
          processAddedNeighborEntry(newEntry);
        } else if (optype == REMOVED && !newEntry) {
          processRemovedNeighborEntry(oldEntry);
        } else if (optype == CHANGED && oldEntry && newEntry) {
          processChangedNeighborEntry(oldEntry, newEntry);
        }
      } catch (const BcmError& error) {
        rethrowIfHwNotFull(error);
        discardedNeighborEntryDelta.push_back(delta);
      }
    }
  }

  for (const auto& delta : discardedNeighborEntryDelta) {
    SwitchState::revertNewNeighborEntry<NeighborEntryT, NeighborTableT>(
        delta.getNew(), delta.getOld(), appliedState);
  }
}

template <typename RouteT>
void BcmSwitch::processChangedRoute(
    const RouterID& id,
    const shared_ptr<RouteT>& oldRoute,
    const shared_ptr<RouteT>& newRoute) {
  std::string routeMessage;
  folly::toAppend(
      "changing route entry @ vrf ",
      id,
      " from old: ",
      oldRoute->str(),
      "to new: ",
      newRoute->str(),
      &routeMessage);
  XLOG(DBG3) << routeMessage;
  // if the new route is not resolved, delete it instead of changing it
  if (!newRoute->isResolved()) {
    XLOG(DBG1) << "Non-resolved route HW programming is skipped";
    processRemovedRoute(id, oldRoute);
  } else {
    routeTable_->addRoute(getBcmVrfId(id), newRoute.get());
  }
}

template <typename RouteT>
void BcmSwitch::processAddedRoute(
    const RouterID& id,
    const shared_ptr<RouteT>& route) {
  std::string routeMessage;
  folly::toAppend(
      "adding route entry @ vrf ", id, " ", route->str(), &routeMessage);
  XLOG(DBG3) << routeMessage;
  // if the new route is not resolved, ignore it
  if (!route->isResolved()) {
    XLOG(DBG1) << "Non-resolved route HW programming is skipped";
    return;
  }
  routeTable_->addRoute(getBcmVrfId(id), route.get());
}

template <typename RouteT>
void BcmSwitch::processRemovedRoute(
    const RouterID id,
    const shared_ptr<RouteT>& route) {
  XLOG(DBG3) << "removing route entry @ vrf " << id << " " << route->str();
  if (!route->isResolved()) {
    XLOG(DBG1) << "Non-resolved route HW programming is skipped";
    return;
  }
  routeTable_->deleteRoute(getBcmVrfId(id), route.get());
}

void BcmSwitch::processRemovedRoutes(const StateDelta& delta) {
  for (auto const& rtDelta : delta.getRouteTablesDelta()) {
    if (!rtDelta.getOld()) {
      // no old route table, must not removed route, skip
      continue;
    }
    RouterID id = rtDelta.getOld()->getID();
    forEachRemoved(
        rtDelta.getRoutesV4Delta(),
        &BcmSwitch::processRemovedRoute<RouteV4>,
        this,
        id);
    forEachRemoved(
        rtDelta.getRoutesV6Delta(),
        &BcmSwitch::processRemovedRoute<RouteV6>,
        this,
        id);
  }
}

void BcmSwitch::processRemovedFibRoutes(const StateDelta& delta) {
  for (const auto& fibDelta : delta.getFibsDelta()) {
    auto oldFib = fibDelta.getOld();

    if (!oldFib) {
      // This FIB did not exist in the previous state. Thus, all routes in it
      // must necessarily have been _added_.
      continue;
    }

    CHECK(oldFib);
    RouterID vrf = oldFib->getID();

    forEachRemoved(
        fibDelta.getV4FibDelta(),
        &BcmSwitch::processRemovedRoute<RouteV4>,
        this,
        vrf);
    forEachRemoved(
        fibDelta.getV6FibDelta(),
        &BcmSwitch::processRemovedRoute<RouteV6>,
        this,
        vrf);
  }
}

void BcmSwitch::processAddedChangedRoutes(
    const StateDelta& delta,
    std::shared_ptr<SwitchState>* appliedState) {
  processRouteTableDelta<folly::IPAddressV4>(delta, appliedState);
  processRouteTableDelta<folly::IPAddressV6>(delta, appliedState);
}

template <typename AddrT>
void BcmSwitch::processRouteTableDelta(
    const StateDelta& delta,
    std::shared_ptr<SwitchState>* appliedState) {
  using RouteT = Route<AddrT>;
  using PrefixT = typename Route<AddrT>::Prefix;
  std::map<RouterID, std::vector<PrefixT>> discardedPrefixes;
  if (!isRouteUpdateValid<AddrT>(delta)) {
    // typically indicate label stack depth exceeded.
    throw FbossError("invalid route update");
  }
  for (auto const& rtDelta : delta.getRouteTablesDelta()) {
    if (!rtDelta.getNew()) {
      // no new route table, must not have added or changed route, skip
      continue;
    }
    RouterID id = rtDelta.getNew()->getID();
    forEachChanged(
        rtDelta.template getRoutesDelta<AddrT>(),
        [&](const shared_ptr<RouteT>& oldRoute,
            const shared_ptr<RouteT>& newRoute) {
          try {
            processChangedRoute(id, oldRoute, newRoute);
          } catch (const BcmError& e) {
            rethrowIfHwNotFull(e);
            discardedPrefixes[id].push_back(oldRoute->prefix());
          }
        },
        [&](const shared_ptr<RouteT>& addedRoute) {
          try {
            processAddedRoute(id, addedRoute);
          } catch (const BcmError& e) {
            rethrowIfHwNotFull(e);
            discardedPrefixes[id].push_back(addedRoute->prefix());
          }
        },
        [](const shared_ptr<RouteT>& /*deletedRoute*/) {
          // do nothing
        });
  }

  // discard  routes
  for (const auto& entry : discardedPrefixes) {
    const auto id = entry.first;
    for (const auto& prefix : entry.second) {
      const auto newRoute = delta.newState()
                                ->getRouteTables()
                                ->getRouteTable(id)
                                ->template getRib<AddrT>()
                                ->routes()
                                ->getRouteIf(prefix);
      const auto oldRoute = delta.oldState()
                                ->getRouteTables()
                                ->getRouteTable(id)
                                ->template getRib<AddrT>()
                                ->routes()
                                ->getRouteIf(prefix);
      SwitchState::revertNewRouteEntry(id, newRoute, oldRoute, appliedState);
    }
  }
}

void BcmSwitch::processAddedChangedFibRoutes(
    const StateDelta& delta,
    std::shared_ptr<SwitchState>* /* appliedState */) {
  for (auto const& fibDelta : delta.getFibsDelta()) {
    auto newFib = fibDelta.getNew();

    if (!newFib) {
      // This FIB does not exist in the new tate. Thus, all routes in it
      // must necessarily have been deleted.
      continue;
    }

    CHECK(newFib);
    RouterID vrf = newFib->getID();

    forEachChanged(
        fibDelta.getV4FibDelta(),
        &BcmSwitch::processChangedRoute<RouteV4>,
        &BcmSwitch::processAddedRoute<RouteV4>,
        [&](BcmSwitch*, const RouterID&, const shared_ptr<RouteV4>&) {},
        this,
        vrf);

    forEachChanged(
        fibDelta.getV6FibDelta(),
        &BcmSwitch::processChangedRoute<RouteV6>,
        &BcmSwitch::processAddedRoute<RouteV6>,
        [&](BcmSwitch*, const RouterID&, const shared_ptr<RouteV6>&) {},
        this,
        vrf);
  }
}

void BcmSwitch::linkscanCallback(
    int unit,
    bcm_port_t bcmPort,
    bcm_port_info_t* info) {
  try {
    BcmUnit* unitObj = BcmAPI::getUnit(unit);
    BcmSwitch* hw = static_cast<BcmSwitch*>(unitObj->getCookie());
    bool up = info->linkstatus == BCM_PORT_LINK_STATUS_UP;

    hw->linkScanBottomHalfEventBase_.runInEventBaseThread(
        [hw, bcmPort, up]() { hw->linkStateChangedHwNotLocked(bcmPort, up); });
  } catch (const std::exception& ex) {
    XLOG(ERR) << "unhandled exception while processing linkscan callback "
              << "for unit " << unit << " port " << bcmPort << ": "
              << folly::exceptionStr(ex);
  }
}

void BcmSwitch::linkStateChangedHwNotLocked(bcm_port_t bcmPortId, bool up) {
  CHECK(linkScanBottomHalfEventBase_.inRunningEventBaseThread());

  if (!up) {
    auto trunk = trunkTable_->linkDownHwNotLocked(bcmPortId);
    if (trunk != BcmTrunk::INVALID) {
      XLOG(INFO) << "Shrinking ECMP entries egressing over trunk " << trunk;
      writableEgressManager()->trunkDownHwNotLocked(trunk);
    }
    writableEgressManager()->linkDownHwNotLocked(bcmPortId);
  } else {
    // For port up events we wait till ARP/NDP entries
    // are re resolved after port up before adding them
    // back. Adding them earlier leads to packet loss.
  }
  callback_->linkStateChanged(portTable_->getPortId(bcmPortId), up);
}

bcm_rx_t BcmSwitch::packetRxCallback(int unit, bcm_pkt_t* pkt, void* cookie) {
  auto* bcmSw = static_cast<BcmSwitch*>(cookie);
  DCHECK_EQ(bcmSw->getUnit(), unit);
  DCHECK_EQ(bcmSw->getUnit(), pkt->unit);
  return bcmSw->packetReceived(pkt);
}

bcm_rx_t BcmSwitch::packetReceived(bcm_pkt_t* pkt) noexcept {
  // Log it if it's an sFlow sample
  if (handleSflowPacket(pkt)) {
    // It was just here because it was an sFlow packet
    bcm_rx_free(pkt->unit, pkt);
    return BCM_RX_HANDLED_OWNED;
  }

  // Otherwise, send it to the SwSwitch
  unique_ptr<BcmRxPacket> bcmPkt;
  try {
    bcmPkt = createRxPacket(pkt);
  } catch (const std::exception& ex) {
    XLOG(ERR) << "failed to allocate BcmRxPacket for receive handling: "
              << folly::exceptionStr(ex);
    return BCM_RX_NOT_HANDLED;
  }

  callback_->packetReceived(std::move(bcmPkt));
  return BCM_RX_HANDLED_OWNED;
}

bool BcmSwitch::sendPacketSwitchedAsync(unique_ptr<TxPacket> pkt) noexcept {
  unique_ptr<BcmTxPacket> bcmPkt(
      boost::polymorphic_downcast<BcmTxPacket*>(pkt.release()));
  return BCM_SUCCESS(BcmTxPacket::sendAsync(std::move(bcmPkt), this));
}

bool BcmSwitch::sendPacketOutOfPortAsync(
    unique_ptr<TxPacket> pkt,
    PortID portID,
    std::optional<uint8_t> queue) noexcept {
  unique_ptr<BcmTxPacket> bcmPkt(
      boost::polymorphic_downcast<BcmTxPacket*>(pkt.release()));
  bcmPkt->setDestModPort(getPortTable()->getBcmPortId(portID));
  if (queue) {
    bcmPkt->setCos(*queue);
  }
  return BCM_SUCCESS(BcmTxPacket::sendAsync(std::move(bcmPkt), this));
}

bool BcmSwitch::sendPacketSwitchedSync(unique_ptr<TxPacket> pkt) noexcept {
  unique_ptr<BcmTxPacket> bcmPkt(
      boost::polymorphic_downcast<BcmTxPacket*>(pkt.release()));
  return BCM_SUCCESS(BcmTxPacket::sendSync(std::move(bcmPkt), this));
}

bool BcmSwitch::sendPacketOutOfPortSync(
    unique_ptr<TxPacket> pkt,
    PortID portID,
    std::optional<uint8_t> cos) noexcept {
  unique_ptr<BcmTxPacket> bcmPkt(
      boost::polymorphic_downcast<BcmTxPacket*>(pkt.release()));
  if (cos.has_value()) {
    bcmPkt->setCos(cos.value());
  }
  bcmPkt->setDestModPort(getPortTable()->getBcmPortId(portID));
  return BCM_SUCCESS(BcmTxPacket::sendSync(std::move(bcmPkt), this));
}

void BcmSwitch::updateStatsImpl(SwitchStats* /* switchStats */) {
  // Update global statistics.
  updateGlobalStats();
  // Update cpu or host bound packet stats
  controlPlane_->updateQueueCounters();
}

folly::F14FastMap<std::string, HwPortStats> BcmSwitch::getPortStats() const {
  // TODO
  return {};
}

shared_ptr<BcmSwitchEventCallback> BcmSwitch::registerSwitchEventCallback(
    bcm_switch_event_t eventID,
    shared_ptr<BcmSwitchEventCallback> callback) {
  return BcmSwitchEventUtils::registerSwitchEventCallback(
      unit_, eventID, callback);
}
void BcmSwitch::unregisterSwitchEventCallback(bcm_switch_event_t eventID) {
  return BcmSwitchEventUtils::unregisterSwitchEventCallback(unit_, eventID);
}

void BcmSwitch::updateGlobalStats() {
  portTable_->updatePortStats();
  trunkTable_->updateStats();
  bcmStatUpdater_->updateStats();

  auto now =
      std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
  if ((now - bstStatsUpdateTime_ >= FLAGS_update_bststats_interval_s) ||
      bstStatsMgr_->isFineGrainedBufferStatLoggingEnabled()) {
    bstStatsUpdateTime_ = now;
    bstStatsMgr_->updateStats();
  }
}

uint64_t BcmSwitch::getDeviceWatermarkBytes() const {
  return bstStatsMgr_->getDeviceWatermarkBytes();
}

bcm_if_t BcmSwitch::getDropEgressId() const {
  return BcmEgress::getDropEgressId();
}

bcm_if_t BcmSwitch::getToCPUEgressId() const {
  if (toCPUEgress_) {
    return toCPUEgress_->getID();
  } else {
    return BcmEgressBase::INVALID;
  }
}

bool BcmSwitch::getAndClearNeighborHit(
    RouterID /*vrf*/,
    folly::IPAddress& /*ip*/) {
  // TODO(aeckert): t20059623 This should look in the host table and
  // check the hit bit, but that currently requires grabbing the main
  // lock and opens up the possibility of bg thread getting stuck
  // behind update thread.  For now, stub this out to return true and
  // work on adding a better way to communicate hit bit + stale entry
  // garbage collection.
  return true;
}

void BcmSwitch::exitFatal() const {
  utilCreateDir(platform_->getCrashInfoDir());
  dumpState(platform_->getCrashHwStateFile());
  callback_->exitFatal();
}

void BcmSwitch::processChangedAggregatePort(
    const std::shared_ptr<AggregatePort>& oldAggPort,
    const std::shared_ptr<AggregatePort>& newAggPort) {
  CHECK_EQ(oldAggPort->getID(), newAggPort->getID());

  XLOG(DBG2) << "reprogramming AggregatePort " << oldAggPort->getID();
  trunkTable_->programTrunk(oldAggPort, newAggPort);
}

void BcmSwitch::processAddedAggregatePort(
    const std::shared_ptr<AggregatePort>& aggPort) {
  XLOG(DBG2) << "creating AggregatePort " << aggPort->getID() << " with "
             << aggPort->subportsCount() << " ports";
  trunkTable_->addTrunk(aggPort);
}

void BcmSwitch::processRemovedAggregatePort(
    const std::shared_ptr<AggregatePort>& aggPort) {
  XLOG(DBG2) << "deleting AggregatePort " << aggPort->getID();
  trunkTable_->deleteTrunk(aggPort);
}

void BcmSwitch::processLoadBalancerChanges(const StateDelta& delta) {
  forEachChanged(
      delta.getLoadBalancersDelta(),
      &BcmSwitch::processChangedLoadBalancer,
      &BcmSwitch::processAddedLoadBalancer,
      &BcmSwitch::processRemovedLoadBalancer,
      this);
}

void BcmSwitch::processChangedLoadBalancer(
    const std::shared_ptr<LoadBalancer>& oldLoadBalancer,
    const std::shared_ptr<LoadBalancer>& newLoadBalancer) {
  CHECK_EQ(oldLoadBalancer->getID(), newLoadBalancer->getID());

  XLOG(DBG2) << "reprogramming LoadBalancer " << oldLoadBalancer->getID();
  rtag7LoadBalancer_->programLoadBalancer(oldLoadBalancer, newLoadBalancer);
}

void BcmSwitch::processAddedLoadBalancer(
    const std::shared_ptr<LoadBalancer>& loadBalancer) {
  XLOG(DBG2) << "creating LoadBalancer " << loadBalancer->getID();
  rtag7LoadBalancer_->addLoadBalancer(loadBalancer);
}

void BcmSwitch::processRemovedLoadBalancer(
    const std::shared_ptr<LoadBalancer>& loadBalancer) {
  XLOG(DBG2) << "deleting LoadBalancer " << loadBalancer->getID();
  rtag7LoadBalancer_->deleteLoadBalancer(loadBalancer);
}

bool BcmSwitch::isControlPlaneQueueNameChanged(
    const shared_ptr<ControlPlane>& oldCPU,
    const shared_ptr<ControlPlane>& newCPU) {
  if ((oldCPU->getQueues().size() != newCPU->getQueues().size())) {
    return true;
  }

  for (const auto& newQueue : newCPU->getQueues()) {
    auto oldQueue = oldCPU->getQueues().at(newQueue->getID());
    if (newQueue->getName() != oldQueue->getName()) {
      return true;
    }
  }
  return false;
}

void BcmSwitch::processChangedControlPlaneQueues(
    const shared_ptr<ControlPlane>& oldCPU,
    const shared_ptr<ControlPlane>& newCPU) {
  // first make sure queue settings changes applied
  for (const auto newQueue : newCPU->getQueues()) {
    if (oldCPU->getQueues().size() > 0 &&
        *(oldCPU->getQueues().at(newQueue->getID())) == *newQueue) {
      continue;
    }
    XLOG(DBG1) << "New cos queue settings on cpu queue "
               << static_cast<int>(newQueue->getID());
    controlPlane_->setupQueue(*newQueue);
  }

  if (isControlPlaneQueueNameChanged(oldCPU, newCPU)) {
    controlPlane_->getQueueManager()->setupQueueCounters(newCPU->getQueues());
  }
}

void BcmSwitch::processChangedRxReasonToQueueEntries(
    const shared_ptr<ControlPlane>& oldCPU,
    const shared_ptr<ControlPlane>& newCPU) {
  const auto& oldReasonToQueue = oldCPU->getRxReasonToQueue();
  const auto& newReasonToQueue = newCPU->getRxReasonToQueue();
  for (int index = 0;
       index < std::max(newReasonToQueue.size(), oldReasonToQueue.size());
       ++index) {
    if (index >= oldReasonToQueue.size()) { // added
      controlPlane_->setReasonToQueueEntry(index, newReasonToQueue[index]);
    } else if (index >= newReasonToQueue.size()) { // deleted
      controlPlane_->deleteReasonToQueueEntry(index);
    } else if (oldReasonToQueue[index] != newReasonToQueue[index]) { // changed
      controlPlane_->setReasonToQueueEntry(index, newReasonToQueue[index]);
    }
  }
}

void BcmSwitch::processMirrorChanges(const StateDelta& delta) {
  forEachChanged(
      delta.getMirrorsDelta(),
      &BcmMirrorTable::processChangedMirror,
      &BcmMirrorTable::processAddedMirror,
      &BcmMirrorTable::processRemovedMirror,
      writableBcmMirrorTable());
}

void BcmSwitch::clearPortStats(
    const std::unique_ptr<std::vector<int32_t>>& ports) {
  bcmStatUpdater_->clearPortStats(ports);
}

std::vector<PrbsLaneStats> BcmSwitch::getPortAsicPrbsStats(int32_t portId) {
  return bcmStatUpdater_->getPortAsicPrbsStats(portId);
}

void BcmSwitch::clearPortAsicPrbsStats(int32_t portId) {
  bcmStatUpdater_->clearPortAsicPrbsStats(portId);
}

std::vector<PrbsLaneStats> BcmSwitch::getPortGearboxPrbsStats(
    int32_t portId,
    phy::Side side) {
  return portTable_->getBcmPort(portId)->getPlatformPort()->getGearboxPrbsStats(
      side);
}

void BcmSwitch::clearPortGearboxPrbsStats(int32_t portId, phy::Side side) {
  portTable_->getBcmPort(portId)->getPlatformPort()->clearGearboxPrbsStats(
      side);
}

void BcmSwitch::dumpState(const std::string& path) const {
  auto stateString = gatherSdkState();
  if (stateString.length() > 0) {
    folly::writeFile(stateString, path.c_str());
  }
}

void BcmSwitch::processChangedLabelForwardingInformationBase(
    const StateDelta& delta) {
  forEachChanged(
      delta.getLabelForwardingInformationBaseDelta(),
      &BcmSwitch::processChangedLabelForwardingEntry,
      &BcmSwitch::processAddedLabelForwardingEntry,
      &BcmSwitch::processRemovedLabelForwardingEntry,
      this);
}

void BcmSwitch::processAddedLabelForwardingEntry(
    const std::shared_ptr<LabelForwardingEntry>& addedEntry) {
  writableLabelMap()->processAddedLabelSwitchAction(
      addedEntry->getID(), addedEntry->getLabelNextHop());
}

void BcmSwitch::processRemovedLabelForwardingEntry(
    const std::shared_ptr<LabelForwardingEntry>& deletedEntry) {
  writableLabelMap()->processRemovedLabelSwitchAction(deletedEntry->getID());
}

void BcmSwitch::processChangedLabelForwardingEntry(
    const std::shared_ptr<LabelForwardingEntry>& /*oldEntry*/,
    const std::shared_ptr<LabelForwardingEntry>& newEntry) {
  writableLabelMap()->processChangedLabelSwitchAction(
      newEntry->getID(), newEntry->getLabelNextHop());
}

bool BcmSwitch::isAlpmEnabled() const {
  return BcmAPI::getConfigValue(kAlpmSetting);
}

BcmSwitch::MmuState BcmSwitch::queryMmuState() const {
  auto lossless = BcmAPI::getConfigValue("mmu_lossless");
  if (!lossless) {
    return MmuState::UNKNOWN;
  }
  return std::string(lossless) == "0x1" ? MmuState::MMU_LOSSLESS
                                        : MmuState::MMU_LOSSY;
}

void BcmSwitch::l2LearningCallback(
    int unit,
    bcm_l2_addr_t* l2Addr,
    int operation,
    void* userData) {
  auto* bcmSw = static_cast<BcmSwitch*>(userData);
  DCHECK_EQ(bcmSw->getUnit(), unit);
  bcmSw->l2LearningUpdateReceived(l2Addr, operation);
}

/*
 * TD2 and TH
 * ----------
 *
 * Typical Learning workflow:
 *  - ASIC encounters unknown source MAC+vlan.
 *  - ASIC creates a PENDING L2 entry for the MAC+vlan and generates callback.
 *  - In response, wedge_agent will VALIDATE the L2 entry.
 *
 * Typical Aging workflow:
 *  - SDK thread ages out a stale MAC+vlan entry.
 *  - This triggers a callback of type DELETE.
 *  - The MAC+vlan that is aged out would be VALIDATED as when the entry is
 *    learned, in resposne to PENDING ADD callback, wedge_agent would have
 *    VALIDATED the entry.
 *
 * The initial implementation (which used wrong BCM API to register callbacks,
 * and got L2 mod fifo to fill up), in addition to PENDING ADD callback, when
 * PENDING entry got VALIDATED, wedge_agent received DELETE for PENDING and ADD
 * for VALIDATED. Thus, wedge_agent had an explicit logic to ignore DELETE
 * callback for * PENDING and ADD callback for VALIDATED.
 *
 * However, with correct BCM API to register callbacks, we no longer get DELETE
 * for PENDING and ADD for VALIDATED, and thus those need not be ignored here
 * (Broadcom case CS9347300).
 *
 * Furthermore, there is a legitimate case where wedge_agent receives ADD for
 * VALIDATED, and thus wedge_agent cannot ignore this callback but must handle
 * it viz. Mac Move.
 *
 * Typical Mac Move workflow:
 *  - MAC+vlan is learned on a port p1 (say) using typical Learning workflow.
 *  - ASIC encounters the same source MAC+vlan on a different port p2 (say).
 *  - In response, the MAC is moved from port p1 to p2 and two callbacks are
 *    generated viz. DELETE for VALIDATED entry on p1, followed by ADD for
 *    VALIDATED entry on p2.
 *
 * TH3
 * ---
 *
 *  For TH3, there is no notion of PENDING
 *
 *  The entries are learned as VALIDATED and thus the first callback agent
 *  receives is for VALIDATED ADD entry, and wedge_agent must process it.
 *
 *  In response, the wedge_agent will program the already VALIDATED entry
 *  again. This has no effect and does not generate additional callback.
 *
 *  The Mac Move wofkflow for TH3 is same as that for TD2 and TH3 i.e. DELETE
 *  on old port and ADD on new port both for VALIDATED MACs.
 */
void BcmSwitch::l2LearningUpdateReceived(
    bcm_l2_addr_t* l2Addr,
    int operation) noexcept {
  if (l2Addr && isL2OperationOfInterest(operation)) {
    callback_->l2LearningUpdateReceived(
        createL2Entry(l2Addr, isL2EntryPending(l2Addr)),
        getL2EntryUpdateType(operation));
  }
}

void BcmSwitch::setupCos() {
  cosManager_.reset(new BcmCosManager(this));
  controlPlane_.reset(new BcmControlPlane(this));

  // set up cpu queue stats counter
  controlPlane_->getQueueManager()->setupQueueCounters();
}

std::unique_ptr<BcmRxPacket> BcmSwitch::createRxPacket(bcm_pkt_t* pkt) {
  return std::make_unique<FbBcmRxPacket>(pkt, this);
}

bool BcmSwitch::isRxThreadRunning() const {
  return bcm_rx_active(unit_);
}

void BcmSwitch::stopLinkscanThread() {
  // Disable the linkscan thread
  auto rv = bcm_linkscan_enable_set(unit_, 0);
  CHECK(BCM_SUCCESS(rv)) << "failed to stop BcmSwitch linkscan thread "
                         << bcm_errmsg(rv);
  if (linkScanBottomHalfThread_) {
    linkScanBottomHalfEventBase_.runInEventBaseThreadAndWait(
        [this] { linkScanBottomHalfEventBase_.terminateLoopSoon(); });
    linkScanBottomHalfThread_->join();
  }
}

void BcmSwitch::createAclGroup() {
  // Install the master ACL group here, whose content may change overtime
  createFPGroup(
      unit_,
      getAclQset(getPlatform()->getAsic()->getAsicType()),
      FLAGS_acl_gid,
      FLAGS_acl_g_pri);
}

void BcmSwitch::dropDhcpPackets() {
  // Don't flood broadcast DHCP packets to other hosts in the vlan. We
  // should still trap the packet to the cpu in this case though.
  auto rv = bcm_switch_control_set(unit_, bcmSwitchDhcpPktDrop, 1);
  bcmCheckError(rv, "failed to set DHCP packet dropping");
}

void BcmSwitch::setL3MtuFailPackets() {
  // trap packets for which L3 MTU check fails
  auto rv = bcm_switch_control_set(unit_, bcmSwitchL3MtuFailToCpu, 1);
  bcmCheckError(rv, "Failed to set L3 MTU fail to CPU");
}

void BcmSwitch::initFieldProcessor() const {
  auto rv = bcm_field_init(unit_);
  bcmCheckError(rv, "failed to set up ContentAware processing");
}

void BcmSwitch::initMirrorModule() const {
  auto rv = bcm_switch_control_set(unit_, bcmSwitchDirectedMirroring, 1);
  bcmCheckError(rv, "Failed to set Switch Directed Mirroring");
  rv = bcm_mirror_init(unit_);
  bcmCheckError(rv, "failed to set up Mirroring");
  rv = bcm_mirror_mode_set(unit_, BCM_MIRROR_L2);
  bcmCheckError(rv, "Failed to set mirror mode");
}

void BcmSwitch::initMplsModule() const {
  auto rv = bcm_mpls_init(unit_);
  bcmCheckError(rv, "failed to set up label switching");
}

void BcmSwitch::setupFPGroups() {
  for (auto grpId : {FLAGS_acl_gid}) {
    auto gid = static_cast<bcm_field_group_t>(grpId);
    XLOG(DBG1) << "Setting up FP group : " << gid;
    if (gid == FLAGS_acl_gid) {
      createAclGroup();
    } else {
      throw FbossError("Unknown group id : ", gid);
    }
  }
}

bool BcmSwitch::haveMissingOrQSetChangedFPGroups() const {
  std::map<std::pair<int32_t, int32_t>, bcm_field_qset_t> grp2Qset = {
      {{FLAGS_acl_gid, FLAGS_acl_g_pri},
       getAclQset(getPlatform()->getAsic()->getAsicType())},
  };
  for (const auto& grpAndQset : grp2Qset) {
    auto gid = static_cast<bcm_field_group_t>(grpAndQset.first.first);
    auto qset = grpAndQset.second;
    if (!fpGroupExists(unit_, gid)) {
      XLOG(DBG1) << " FP group : " << gid << " does not exist";
      return true;
    } else if (!FPGroupDesiredQsetCmp(unit_, gid, qset).hasDesiredQset()) {
      XLOG(INFO) << " FP group : " << gid << " has a changed QSET";
      return true;
    }
  }
  XLOG(DBG1) << " All FP groups have desired QSETs ";
  return false;
}

bcm_gport_t BcmSwitch::getCpuGPort() const {
  return BCM_GPORT_LOCAL_CPU;
}

// TODO(petr): this is here only because bcmRxReasonSampleSource and the like
// are not yet open sourced.
bool BcmSwitch::handleSflowPacket(bcm_pkt_t* pkt) noexcept {
  auto bcmPkt = reinterpret_cast<const bcm_pkt_t*>(pkt);

  bool ingressSample =
      _SHR_RX_REASON_GET(bcmPkt->rx_reasons, bcmRxReasonSampleSource);
  bool egressSample =
      _SHR_RX_REASON_GET(bcmPkt->rx_reasons, bcmRxReasonSampleDest);
  if (!ingressSample && !egressSample) {
    return false;
  }

  // Assemble packet metadata
  SflowPacketInfo info;

  auto currentTime = std::chrono::high_resolution_clock::now();
  auto duration =
      duration_cast<std::chrono::nanoseconds>(currentTime.time_since_epoch());
  *info.timestamp_ref()->seconds_ref() =
      duration_cast<std::chrono::seconds>(duration).count();
  *info.timestamp_ref()->nanoseconds_ref() =
      duration_cast<std::chrono::nanoseconds>(
          duration % std::chrono::seconds(1))
          .count();
  *info.ingressSampled_ref() = ingressSample;
  *info.egressSampled_ref() = egressSample;
  *info.srcPort_ref() = pkt->src_port;
  *info.dstPort_ref() = pkt->dest_port;
  *info.vlan_ref() = pkt->vlan;

  auto snapLen = std::min(kMaxSflowSnapLen, (unsigned int)(pkt->pkt_data->len));

  XLOG(DBG6) << "sFlow captured packet of size " << pkt->pkt_data->len;
  XLOG(DBG6) << "Packet dump following (max 128 bytes):\n "
             << folly::hexDump(pkt->pkt_data->data, snapLen);

  {
    std::string packetData(pkt->pkt_data->data, pkt->pkt_data->data + snapLen);
    *info.packetData_ref() = std::move(packetData);
  }

  // Print it for debugging
  XLOG(DBG6) << "sFlowSample: (" << *info.timestamp_ref()->seconds_ref() << ','
             << *info.timestamp_ref()->nanoseconds_ref() << ','
             << *info.ingressSampled_ref() << ',' << *info.egressSampled_ref()
             << ',' << *info.srcPort_ref() << ',' << *info.dstPort_ref() << ','
             << *info.vlan_ref() << ',' << info.packetData_ref()->length()
             << ")\n";

  sFlowExporterTable_->sendToAll(info);

  // If it is only here because of sFlow, we're done
  if ((pkt->rx_reason ^ bcmRxReasonSampleSource ^ bcmRxReasonSampleDest) == 0) {
    return true;
  }

  return false;
}

std::string BcmSwitch::gatherSdkState() const {
  if (!platform_->isBcmShellSupported()) {
    XLOG(INFO) << "Cannot dump SDK state since the platform does not support "
                  "bcm shell";
    return "";
  }
  BcmLogBuffer logBuffer;
  printDiagCmd("portstat");
  printDiagCmd("l2 show");
  printDiagCmd("l3 l3table show");
  printDiagCmd("l3 ip6host show");
  printDiagCmd("l3 defip show");
  printDiagCmd("l3 ip6route show");
  printDiagCmd("l3 intf show");
  printDiagCmd("l3 egress show");
  printDiagCmd("l3 multipath show");
  printDiagCmd("trunk show");
  return logBuffer.getBuffer()->moveToFbString().toStdString();
}

/*
 * BroadView is currently only supported in certain BCM SDK version. We will
 * remove this (and move to Bcm) once all 6.3.x BCM SDK is removed.
 */
std::unique_ptr<PacketTraceInfo> BcmSwitch::getPacketTrace(
    std::unique_ptr<MockRxPacket> pkt) const {
  // Check if the port actually exists.
  if (!getPortTable()->portExists(pkt->getSrcPort())) {
    XLOG(ERR) << "Cannot getPacketTrace from non-existing port";
    return nullptr;
  }

  // Construct the arguments for bcm_switch_pkt_trace_info_get()
  unsigned int options = 0; // Unused now
  auto port = getPortTable()->getBcmPortId(pkt->getSrcPort());
  auto packetLength = pkt->getLength();
  auto packetData = pkt->buf()->writableData();
  bcm_switch_pkt_trace_info_t bcmPacketTraceInfo;
  auto rv = bcm_switch_pkt_trace_info_get(
      unit_, options, port, packetLength, packetData, &bcmPacketTraceInfo);
  bcmCheckError(rv, "Failed to get packet trace info (no support on Trident2)");

  // Parse bcmPacketTraceInfo
  std::unique_ptr<PacketTraceInfo> packetTraceInfo =
      std::make_unique<PacketTraceInfo>();
  transformPacketTraceInfo(
      this,
      bcmPacketTraceInfo, /* bcm_switch_pkt_trace_info_t */
      *packetTraceInfo /* fboss::PacketTraceInfo */
  );
  return packetTraceInfo;
}

static int _addL2Entry(int /*unit*/, bcm_l2_addr_t* l2addr, void* user_data) {
  L2EntryThrift entry;
  auto* cookie =
      static_cast<std::pair<BcmSwitch*, std::vector<L2EntryThrift>*>*>(
          user_data);
  auto [hw, l2Table] = *cookie;
  *entry.mac_ref() = folly::sformat(
      "{0:02x}:{1:02x}:{2:02x}:{3:02x}:{4:02x}:{5:02x}",
      l2addr->mac[0],
      l2addr->mac[1],
      l2addr->mac[2],
      l2addr->mac[3],
      l2addr->mac[4],
      l2addr->mac[5]);
  *entry.vlanID_ref() = l2addr->vid;
  *entry.port_ref() = 0;
  if (l2addr->flags & BCM_L2_TRUNK_MEMBER) {
    entry.trunk_ref() = hw->getTrunkTable()->getAggregatePortId(l2addr->tgid);
    XLOG(DBG6) << "L2 entry: Mac:" << *entry.mac_ref()
               << " Vid:" << *entry.vlanID_ref()
               << " Trunk: " << entry.trunk_ref().value_or({});
  } else {
    *entry.port_ref() = l2addr->port;
    XLOG(DBG6) << "L2 entry: Mac:" << *entry.mac_ref()
               << " Vid:" << *entry.vlanID_ref()
               << " Port: " << *entry.port_ref();
  }

  *entry.l2EntryType_ref() = (l2addr->flags & BCM_L2_PENDING)
      ? L2EntryType::L2_ENTRY_TYPE_PENDING
      : L2EntryType::L2_ENTRY_TYPE_VALIDATED;

  // If classID is not set, this field is set to 0 by default.
  // Set in L2EntryThrift only if set to non-default
  if (l2addr->group != 0) {
    entry.classID_ref() = l2addr->group;
  }

  l2Table->push_back(entry);
  return 0;
}

void BcmSwitch::fetchL2Table(std::vector<L2EntryThrift>* l2Table) const {
  auto cookie = std::make_pair(this, l2Table);
  int rv = bcm_l2_traverse(unit_, _addL2Entry, &cookie);
  bcmCheckError(rv, "bcm_l2_traverse failed");
}

void BcmSwitch::processChangedAcl(
    const std::shared_ptr<AclEntry>& oldAcl,
    const std::shared_ptr<AclEntry>& newAcl) {
  // Unfortunately, we cannot modify a field entry due to BCM limitation
  XLOG(DBG3) << "processChangedAcl, ACL=" << oldAcl->getID();
  processRemovedAcl(oldAcl);
  processAddedAcl(newAcl);
}

void BcmSwitch::processRemovedAcl(const std::shared_ptr<AclEntry>& acl) {
  XLOG(DBG3) << "processRemovedAcl, ACL=" << acl->getID();
  aclTable_->processRemovedAcl(acl);
}

void BcmSwitch::processAddedAcl(const std::shared_ptr<AclEntry>& acl) {
  XLOG(DBG3) << "processAddedAcl, ACL=" << acl->getID();
  aclTable_->processAddedAcl(FLAGS_acl_gid, acl);
}

void BcmSwitch::forceLinkscanOn(bcm_pbmp_t ports) {
  if (!(flags_ & LINKSCAN_REGISTERED)) {
    XLOG(INFO) << "Linkscan not registered, skipping force scan";
    return;
  }
  // will immediately scan ports in the passed in port bitmap. Useful
  // for guaranteeing link status is updated during initialization.
  auto rv = bcm_linkscan_update(unit_, ports);
  bcmCheckError(rv, "failed initial scan of link status");
}

bool BcmSwitch::isValidLabelForwardingEntry(
    const LabelForwardingEntry* entry) const {
  if (!isValidLabeledNextHopSet(
          platform_, entry->getLabelNextHop().getNextHopSet())) {
    return false;
  }

  for (auto client : AllClientIDs()) {
    const auto* entryForClient = entry->getEntryForClient(client);
    if (!entryForClient) {
      continue;
    }
    if (!isValidLabeledNextHopSet(platform_, entryForClient->getNextHopSet())) {
      return false;
    }
  }
  return true;
}

void BcmSwitch::processControlPlaneChanges(const StateDelta& delta) {
  const auto controlPlaneDelta = delta.getControlPlaneDelta();
  const auto& oldCPU = controlPlaneDelta.getOld();
  const auto& newCPU = controlPlaneDelta.getNew();

  processChangedControlPlaneQueues(oldCPU, newCPU);
  if (oldCPU->getQosPolicy() != newCPU->getQosPolicy()) {
    controlPlane_->setupIngressQosPolicy(newCPU->getQosPolicy());
  }
  processChangedRxReasonToQueueEntries(oldCPU, newCPU);

  // COPP ACL will be handled just as regular ACL in processAclChanges()
}

void BcmSwitch::disableHotSwap() const {
  if (getPlatform()->getAsic()->isSupported(HwAsic::Feature::HOT_SWAP)) {
    switch (getPlatform()->getAsic()->getAsicType()) {
      case HwAsic::AsicType::ASIC_TYPE_FAKE:
      case HwAsic::AsicType::ASIC_TYPE_TRIDENT2:
      case HwAsic::AsicType::ASIC_TYPE_TOMAHAWK:
        // For TD2 and TH, we patch the SDK to disable hot swap,
        break;
      case HwAsic::AsicType::ASIC_TYPE_TOMAHAWK3:
      case HwAsic::AsicType::ASIC_TYPE_TOMAHAWK4: {
        auto rv = bcm_switch_control_set(unit_, bcmSwitchPcieHotSwapDisable, 1);
        bcmCheckError(rv, "Failed to disable hotswap");
      } break;
      case HwAsic::AsicType::ASIC_TYPE_TAJO:
      case HwAsic::AsicType::ASIC_TYPE_MOCK:
        CHECK(0) << " Invalid ASIC type";
    }
  }
}

bool BcmSwitch::isL2EntryPending(const bcm_l2_addr_t* l2Addr) {
  return l2Addr->flags & BCM_L2_PENDING;
}

bool BcmSwitch::isValidPortQueueUpdate(
    const std::vector<std::shared_ptr<PortQueue>>& /*oldPortQueueConfig*/,
    const std::vector<std::shared_ptr<PortQueue>>& newPortQueueConfig) const {
  if (newPortQueueConfig.empty()) {
    return true;
  }

  std::map<TrafficClass, int> tcToQueue;
  // validate that a traffic class has only one associated queue
  for (const auto& portQueue : newPortQueueConfig) {
    auto trafficClass = portQueue->getTrafficClass()
        ? portQueue->getTrafficClass().value()
        : static_cast<TrafficClass>(portQueue->getID());
    if (trafficClass > BCM_PRIO_MAX || trafficClass < BCM_PRIO_MIN) {
      return false;
    }
    if (tcToQueue.find(trafficClass) != tcToQueue.end()) {
      // this traffic class has already been mapped to some queue
      return false;
    }
    tcToQueue.emplace(trafficClass, portQueue->getID());
  }
  return true;
}

bool BcmSwitch::isValidPortQosPolicyUpdate(
    const std::shared_ptr<Port>& /*oldPort*/,
    const std::shared_ptr<Port>& newPort,
    const std::shared_ptr<SwitchState>& newState) const {
  auto portQosPolicyName = newPort->getQosPolicy();
  if (!portQosPolicyName.has_value() ||
      portQosPolicyName.value() ==
          newState->getDefaultDataPlaneQosPolicy()->getName()) {
    // either no policy or global default poliicy is provided
    return true;
  }
  auto qosPolicy = newState->getQosPolicy(portQosPolicyName.value());
  // qos policy for port must not have exp, since this is not supported
  return qosPolicy->getExpMap().to().empty() &&
      qosPolicy->getExpMap().from().empty();
}

} // namespace facebook::fboss
