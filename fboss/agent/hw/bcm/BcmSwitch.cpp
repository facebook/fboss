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

#include <fstream>

#include <boost/cast.hpp>
#include <folly/Conv.h>
#include <folly/FileUtil.h>
#include <folly/Memory.h>
#include <folly/hash/Hash.h>
#include <folly/logging/xlog.h>
#include <optional>
#include "common/time/Time.h"
#include "fboss/agent/Constants.h"
#include "fboss/agent/FbossError.h"
#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/SwitchStats.h"
#include "fboss/agent/Utils.h"
#include "fboss/agent/hw/BufferStatsLogger.h"
#include "fboss/agent/hw/bcm/BcmAPI.h"
#include "fboss/agent/hw/bcm/BcmAclTable.h"
#include "fboss/agent/hw/bcm/BcmAddressFBConvertors.h"
#include "fboss/agent/hw/bcm/BcmBstStatsMgr.h"
#include "fboss/agent/hw/bcm/BcmControlPlane.h"
#include "fboss/agent/hw/bcm/BcmCosManager.h"
#include "fboss/agent/hw/bcm/BcmEgressManager.h"
#include "fboss/agent/hw/bcm/BcmError.h"
#include "fboss/agent/hw/bcm/BcmHost.h"
#include "fboss/agent/hw/bcm/BcmHostKey.h"
#include "fboss/agent/hw/bcm/BcmIntf.h"
#include "fboss/agent/hw/bcm/BcmLabelMap.h"
#include "fboss/agent/hw/bcm/BcmMirrorTable.h"
#include "fboss/agent/hw/bcm/BcmNextHop.h"
#include "fboss/agent/hw/bcm/BcmPlatform.h"
#include "fboss/agent/hw/bcm/BcmPort.h"
#include "fboss/agent/hw/bcm/BcmPortGroup.h"
#include "fboss/agent/hw/bcm/BcmPortTable.h"
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
#include "fboss/agent/hw/bcm/BcmTrunkTable.h"
#include "fboss/agent/hw/bcm/BcmTxPacket.h"
#include "fboss/agent/hw/bcm/BcmUnit.h"
#include "fboss/agent/hw/bcm/BcmWarmBootCache.h"
#include "fboss/agent/hw/bcm/BcmWarmBootHelper.h"
#include "fboss/agent/hw/bcm/BcmWarmBootState.h"
#include "fboss/agent/hw/bcm/gen-cpp2/bcmswitch_constants.h"
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
#include <opennsl/link.h>
#include <opennsl/port.h>
#include <opennsl/stg.h>
#include <opennsl/switch.h>
#include <opennsl/vlan.h>
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

using namespace std::chrono;

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
  if (error.getBcmError() != OPENNSL_E_FULL) {
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
 * Learning generates ADD, aging generaets DELETE, while Mac move results in
 * DELETE followed by ADD.
 * We ASSERT this explicitly via BCM tests.
 */
const std::map<int, facebook::fboss::L2EntryUpdateType>
    kL2AddrUpdateOperationsOfInterest = {
        {OPENNSL_L2_CALLBACK_DELETE,
         facebook::fboss::L2EntryUpdateType::L2_ENTRY_UPDATE_TYPE_DELETE},
        {OPENNSL_L2_CALLBACK_ADD,
         facebook::fboss::L2EntryUpdateType::L2_ENTRY_UPDATE_TYPE_ADD},
};

using facebook::fboss::AggregatePortID;
using facebook::fboss::L2Entry;
using facebook::fboss::L2EntryUpdateType;
using facebook::fboss::macFromBcm;
using facebook::fboss::PortDescriptor;
using facebook::fboss::PortID;
using facebook::fboss::VlanID;

L2Entry createL2Entry(const opennsl_l2_addr_t* l2Addr) {
  CHECK(l2Addr);

  if (!(l2Addr->flags & OPENNSL_L2_TRUNK_MEMBER)) {
    return L2Entry(
        macFromBcm(l2Addr->mac),
        VlanID(l2Addr->vid),
        PortDescriptor(PortID(l2Addr->port)));
  } else {
    return L2Entry(
        macFromBcm(l2Addr->mac),
        VlanID(l2Addr->vid),
        PortDescriptor(AggregatePortID(l2Addr->tgid)));
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

} // namespace

namespace facebook {
namespace fboss {
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
      switchSettings_(new BcmSwitchSettings(this)) {
  exportSdkVersion();
}

BcmSwitch::~BcmSwitch() {
  XLOG(ERR) << "Destroying BcmSwitch";
  resetTables();
  unitObject_->detachAndCleanupSDKUnit();
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
  // Reset warmboot cache last in case Bcm object destructors
  // access it during object deletion.
  warmBootCache_.reset();
}

void BcmSwitch::unregisterCallbacks() {
  if (flags_ & RX_REGISTERED) {
    opennsl_rx_stop(unit_, nullptr);
    auto rv =
        opennsl_rx_unregister(unit_, packetRxCallback, kRxCallbackPriority);
    CHECK(OPENNSL_SUCCESS(rv))
        << "failed to unregister BcmSwitch rx callback: " << opennsl_errmsg(rv);
    flags_ &= ~RX_REGISTERED;
  }
  // Note that we don't explicitly call opennsl_linkscan_detach() here--
  // this call is not thread safe and should only be called from the main
  // thread.  However, opennsl_detach() / _opennsl_shutdown() will clean up the
  // linkscan module properly.
  if (flags_ & LINKSCAN_REGISTERED) {
    auto rv = opennsl_linkscan_unregister(unit_, linkscanCallback);
    CHECK(OPENNSL_SUCCESS(rv)) << "failed to unregister BcmSwitch linkscan "
                                  "callback: "
                               << opennsl_errmsg(rv);

    stopLinkscanThread();
    flags_ &= ~LINKSCAN_REGISTERED;
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

void BcmSwitch::switchRunStateChanged(SwitchRunState newState) {
  std::lock_guard<std::mutex> g(lock_);
  switch (newState) {
    case SwitchRunState::INITIALIZED:
      setupLinkscan();
      setupPacketRx();
      break;
    default:
      break;
  }
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

    auto rv = opennsl_port_learn_get(getUnit(), portID, &port_flags);
    bcmCheckError(rv, "Unable to get L2 Learning flags for port: ", portID);

    if (flags == 0) {
      flags = port_flags;
    } else if (flags != port_flags) {
      throw FbossError(
          "Every port should have same L2 Learning setting by default");
    }
  }

  // This is cold boot, so there cannot be any L2 update callback registered.
  // Thus, OPENNSL_PORT_LEARN_ARL | OPENNSL_PORT_LEARN_FWD should be enough to
  // ascertain HARDWARE as L2 learning mode.
  cfg::L2LearningMode l2LearningMode;
  if (flags == (OPENNSL_PORT_LEARN_ARL | OPENNSL_PORT_LEARN_FWD)) {
    l2LearningMode = cfg::L2LearningMode::HARDWARE;
  } else if (flags == (OPENNSL_PORT_LEARN_ARL | OPENNSL_PORT_LEARN_PENDING)) {
    l2LearningMode = cfg::L2LearningMode::SOFTWARE;
  } else {
    throw FbossError(
        "L2 Learning mode is neither SOFTWARE, nor HARDWARE, flags: ", flags);
  }

  auto switchSettings = make_shared<SwitchSettings>();
  switchSettings->setL2LearningMode(l2LearningMode);
  bootState->resetSwitchSettings(switchSettings);

  opennsl_vlan_t defaultVlan;
  auto rv = opennsl_vlan_default_get(getUnit(), &defaultVlan);
  bcmCheckError(rv, "Unable to get default VLAN");
  bootState->setDefaultVlan(VlanID(defaultVlan));
  // get cpu queue settings
  auto cpu = make_shared<ControlPlane>();
  auto cpuQueues = controlPlane_->getMulticastQueueSettings();
  cpu->resetQueues(cpuQueues);
  bootState->resetControlPlane(cpu);

  // On cold boot all ports are in Vlan 1
  auto vlan = make_shared<Vlan>(VlanID(1), "InitVlan");
  Vlan::MemberPorts memberPorts;
  for (const auto& kv : *portTable_) {
    PortID portID = kv.first;
    BcmPort* bcmPort = kv.second;
    string name = folly::to<string>("port", portID);

    auto swPort = make_shared<Port>(portID, name);
    swPort->setSpeed(bcmPort->getSpeed());
    if (platform_->isCosSupported()) {
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
    newPort->setOperState(bcmPort->isUp());
  }

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
  auto rv = opennsl_linkscan_register(unit_, linkscanCallback);
  bcmCheckError(rv, "failed to register for linkscan events");
  flags_ |= LINKSCAN_REGISTERED;
  rv = opennsl_linkscan_enable_set(unit_, FLAGS_linkscan_interval_us);
  bcmCheckError(rv, "failed to enable linkscan");
  if (getBootType() == BootType::WARM_BOOT) {
    opennsl_port_config_t pcfg;
    rv = opennsl_port_config_get(unit_, &pcfg);
    bcmCheckError(rv, "failed to get port configuration");
    forceLinkscanOn(pcfg.port);
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
      opennsl_l2_age_timer_get(unit_, &currentSeconds),
      "failed to get l2_age_timer");
  if (currentSeconds != targetSeconds) {
    XLOG(DBG1) << "Changing MAC Aging timer from " << currentSeconds << " to "
               << targetSeconds;
    bcmCheckError(
        opennsl_l2_age_timer_set(unit_, targetSeconds),
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
  BcmSwitchEventUtils::initUnit(unit_);
  auto fatalCob = make_shared<BcmSwitchEventUnitFatalErrorCallback>();
  auto nonFatalCob = make_shared<BcmSwitchEventUnitNonFatalErrorCallback>();
  BcmSwitchEventUtils::registerSwitchEventCallback(
      unit_, OPENNSL_SWITCH_EVENT_STABLE_FULL, fatalCob);
  BcmSwitchEventUtils::registerSwitchEventCallback(
      unit_, OPENNSL_SWITCH_EVENT_STABLE_ERROR, fatalCob);
  BcmSwitchEventUtils::registerSwitchEventCallback(
      unit_, OPENNSL_SWITCH_EVENT_UNCONTROLLED_SHUTDOWN, fatalCob);
  BcmSwitchEventUtils::registerSwitchEventCallback(
      unit_, OPENNSL_SWITCH_EVENT_WARM_BOOT_DOWNGRADE, fatalCob);
  BcmSwitchEventUtils::registerSwitchEventCallback(
      unit_, OPENNSL_SWITCH_EVENT_PARITY_ERROR, nonFatalCob);

  // Create bcmStatUpdater to cache the stat ids
  bcmStatUpdater_ = std::make_unique<BcmStatUpdater>(this, isAlpmEnabled());

  XLOG(INFO) << " Is ALPM enabled: " << isAlpmEnabled();
  // Additional switch configuration
  auto state = make_shared<SwitchState>();
  opennsl_port_config_t pcfg;
  auto rv = opennsl_port_config_get(unit_, &pcfg);
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

  rv = opennsl_switch_control_set(unit_, opennslSwitchL3EgressMode, 1);
  bcmCheckError(rv, "failed to set L3 egress mode");
  // Trap IPv4 Address Resolution Protocol (ARP) packets.
  // TODO: We may want to trap ARP on a per-port or per-VLAN basis.
  rv = opennsl_switch_control_set(unit_, opennslSwitchArpRequestToCpu, 1);
  bcmCheckError(rv, "failed to set ARP request trapping");
  rv = opennsl_switch_control_set(unit_, opennslSwitchArpReplyToCpu, 1);
  bcmCheckError(rv, "failed to set ARP reply trapping");
  // Trap IP header TTL or hoplimit 1 to CPU
  rv = opennsl_switch_control_set(unit_, opennslSwitchL3UcastTtl1ToCpu, 1);
  bcmCheckError(rv, "failed to set L3 header error trapping");
  // Trap DHCP packets to CPU
  rv = opennsl_switch_control_set(unit_, opennslSwitchDhcpPktToCpu, 1);
  bcmCheckError(rv, "failed to set DHCP packet trapping");
  // Trap Dest miss
  rv = opennsl_switch_control_set(unit_, opennslSwitchUnknownL3DestToCpu, 1);
  bcmCheckError(rv, "failed to set destination miss trapping");
  rv = opennsl_switch_control_set(unit_, opennslSwitchV6L3DstMissToCpu, 1);
  bcmCheckError(rv, "failed to set IPv6 destination miss trapping");
  // Trap IPv6 Neighbor Discovery Protocol (NDP) packets.
  // TODO: We may want to trap NDP on a per-port or per-VLAN basis.
  rv = opennsl_switch_control_set(unit_, opennslSwitchNdPktToCpu, 1);
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
  opennsl_port_t idx;
  OPENNSL_PBMP_ITER(pcfg.cpu, idx) {
    rv = opennsl_port_control_set(unit_, idx, opennslPortControlIP4, 1);
    bcmCheckError(rv, "failed to enable IPv4 on cpu port ", idx);
    rv = opennsl_port_control_set(unit_, idx, opennslPortControlIP6, 1);
    bcmCheckError(rv, "failed to enable IPv6 on cpu port ", idx);
    XLOG(DBG2) << "Enabled IPv4/IPv6 on CPU port " << idx;
  }

  // verify the drop egress ID is really dropping
  BcmEgress::verifyDropEgress(unit_);

  if (warmBoot) {
    // This needs to be done after we have set
    // opennslSwitchL3EgressMode else the egress ids
    // in the host table don't show up correctly.
    warmBootCache_->populate();
  }
  setupToCpuEgress();
  portTable_->initPorts(&pcfg, warmBoot);

  setupCos();
  configureRxRateLimiting();

  bstStatsMgr_->startBufferStatCollection();

  // Set the spanning tree state of all ports to forwarding.
  // TODO: Eventually the spanning tree state should be part of the Port
  // state, and this should be handled in applyConfig().
  //
  // Spanning tree group settings
  // TODO: This should eventually be done as part of applyConfig()
  opennsl_stg_t stg = 1;
  OPENNSL_PBMP_ITER(pcfg.port, idx) {
    rv = opennsl_stg_stp_set(unit_, stg, idx, OPENNSL_STG_STP_FORWARD);
    bcmCheckError(rv, "failed to set spanning tree state on port ", idx);
  }

  ret.bootType = bootType_;

  if (warmBoot) {
    auto warmBootState = applyAndGetWarmBootSwitchState();
    hostTable_->warmBootHostEntriesSynced();
    ret.switchState = warmBootState;
    // Done with warm boot, clear warm boot cache
    warmBootCache_->clear();
  } else {
    ret.switchState = getColdBootSwitchState();
  }

  setMacAging(std::chrono::seconds(FLAGS_l2AgeTimerSeconds));

  switchSettings_ = std::make_unique<BcmSwitchSettings>(this);

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
  static opennsl_rx_cfg_t rxCfg = {
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
  uint32_t rxFlags = OPENNSL_RCO_F_ALL_COS;
  auto rv = opennsl_rx_register(
      unit_, // int unit
      "fboss_rx", // const char *name
      packetRxCallback, // opennsl_rx_cb_f callback
      kRxCallbackPriority, // uint8 priority
      this, // void *cookie
      rxFlags); // uint32 flags
  bcmCheckError(rv, "failed to register packet rx callback");
  flags_ |= RX_REGISTERED;

  if (!isRxThreadRunning()) {
    rv = opennsl_rx_start(unit_, &rxCfg);
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

  forEachRemoved(delta.getPortsDelta(), [](const auto& /*oldPort*/) {
    throw FbossError("Ports cannot be removed");
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

  processLoadBalancerChanges(delta);

  CHECK(!bothStandAloneRibOrRouteTableRibUsed(delta));

  // remove all routes to be deleted
  processRemovedRoutes(delta);
  processRemovedFibRoutes(delta);

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

  // Any neighbor changes, and modify appliedState if some changes fail to apply
  processNeighborChanges(delta, &appliedState);

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
  return make_unique<BcmTxPacket>(unit_, size);
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

        if (speedChanged || vlanChanged || pauseChanged || sFlowChanged ||
            fecChanged || loopbackChanged || mirrorChanged ||
            qosPolicyChanged || nameChanged) {
          bcmPort->program(newPort);
        }

        if (newPort->getPortQueues().size() != 0 &&
            !platform_->isCosSupported()) {
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

  auto newState = delta.newState();
  forEachChanged(
      delta.getPortsDelta(),
      [&](const shared_ptr<Port>& oldPort, const shared_ptr<Port>& newPort) {
        auto enabled = !oldPort->isEnabled() && newPort->isEnabled();
        auto speedChanged = oldPort->getSpeed() != newPort->getSpeed();
        auto sFlowChanged = (oldPort->getSflowIngressRate() !=
                             newPort->getSflowIngressRate()) ||
            (oldPort->getSflowEgressRate() != newPort->getSflowEgressRate());

        if (enabled || speedChanged || sFlowChanged) {
          if (!isValidPortUpdate(oldPort, newPort, newState)) {
            // Fail hard
            throw FbossError("Invalid port configuration passed in ");
          }
          auto bcmPort = portTable_->getBcmPort(newPort->getID());
          auto portGroup = bcmPort->getPortGroup();
          if (portGroup) {
            portGroup->reconfigureIfNeeded(newState);
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
  return true;
}

template <typename AddrT>
bool BcmSwitch::isRouteUpdateValid(const StateDelta& delta) const {
  using RouteT = Route<AddrT>;

  auto maxLabelStackDepth = getPlatform()->maxLabelStackDepth();
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
      (newState->getMirrors()->size() <= bcmswitch_constants::MAX_MIRRORS_);

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
  auto rv = opennsl_vlan_default_set(unit_, id);
  bcmCheckError(rv, "failed to set default VLAN to ", id);
}

void BcmSwitch::processChangedVlan(
    const shared_ptr<Vlan>& oldVlan,
    const shared_ptr<Vlan>& newVlan) {
  // Update port membership
  opennsl_pbmp_t addedPorts;
  OPENNSL_PBMP_CLEAR(addedPorts);
  opennsl_pbmp_t addedUntaggedPorts;
  OPENNSL_PBMP_CLEAR(addedUntaggedPorts);
  opennsl_pbmp_t removedPorts;
  OPENNSL_PBMP_CLEAR(removedPorts);
  const auto& oldPorts = oldVlan->getPorts();
  const auto& newPorts = newVlan->getPorts();

  auto oldIter = oldPorts.begin();
  auto newIter = newPorts.begin();
  uint32_t numAdded{0};
  uint32_t numRemoved{0};
  while (oldIter != oldPorts.end() || newIter != newPorts.end()) {
    LoopAction action = LoopAction::CONTINUE;
    if (oldIter == oldPorts.end() ||
        (newIter != newPorts.end() && newIter->first < oldIter->first)) {
      // This port was added
      ++numAdded;
      opennsl_port_t bcmPort = portTable_->getBcmPortId(newIter->first);
      OPENNSL_PBMP_PORT_ADD(addedPorts, bcmPort);
      if (!newIter->second.tagged) {
        OPENNSL_PBMP_PORT_ADD(addedUntaggedPorts, bcmPort);
      }
      ++newIter;
    } else if (
        newIter == newPorts.end() ||
        (oldIter != oldPorts.end() && oldIter->first < newIter->first)) {
      // This port was removed
      ++numRemoved;
      opennsl_port_t bcmPort = portTable_->getBcmPortId(oldIter->first);
      OPENNSL_PBMP_PORT_ADD(removedPorts, bcmPort);
      ++oldIter;
    } else {
      ++oldIter;
      ++newIter;
    }
  }

  XLOG(DBG2) << "updating VLAN " << newVlan->getID() << ": " << numAdded
             << " ports added, " << numRemoved << " ports removed";
  if (numRemoved) {
    auto rv = opennsl_vlan_port_remove(unit_, newVlan->getID(), removedPorts);
    bcmCheckError(rv, "failed to remove ports from VLAN ", newVlan->getID());
  }
  if (numAdded) {
    auto rv = opennsl_vlan_port_add(
        unit_, newVlan->getID(), addedPorts, addedUntaggedPorts);
    bcmCheckError(rv, "failed to add ports to VLAN ", newVlan->getID());
  }
}

void BcmSwitch::processAddedVlan(const shared_ptr<Vlan>& vlan) {
  XLOG(DBG2) << "creating VLAN " << vlan->getID() << " with "
             << vlan->getPorts().size() << " ports";

  opennsl_pbmp_t pbmp;
  opennsl_pbmp_t ubmp;
  OPENNSL_PBMP_CLEAR(pbmp);
  OPENNSL_PBMP_CLEAR(ubmp);

  for (const auto& entry : vlan->getPorts()) {
    opennsl_port_t bcmPort = portTable_->getBcmPortId(entry.first);
    OPENNSL_PBMP_PORT_ADD(pbmp, bcmPort);
    if (!entry.second.tagged) {
      OPENNSL_PBMP_PORT_ADD(ubmp, bcmPort);
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
      return OPENNSL_PBMP_EQ(newVlan.allPorts, existingVlan.allPorts) &&
          OPENNSL_PBMP_EQ(newVlan.untagged, existingVlan.untagged);
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
    auto rv = opennsl_vlan_create(unit_, vlan->getID());
    bcmCheckError(rv, "failed to add VLAN ", vlan->getID());
    rv = opennsl_vlan_port_add(unit_, vlan->getID(), pbmp, ubmp);
    bcmCheckError(rv, "failed to add members to new VLAN ", vlan->getID());
  }
}

void BcmSwitch::preprocessRemovedVlan(const shared_ptr<Vlan>& vlan) {
  // Remove all ports from this VLAN at this phase.
  XLOG(DBG2) << "preparing to remove VLAN " << vlan->getID();
  auto rv = opennsl_vlan_gport_delete_all(unit_, vlan->getID());
  bcmCheckError(rv, "failed to remove members from VLAN ", vlan->getID());
}

void BcmSwitch::processRemovedVlan(const shared_ptr<Vlan>& vlan) {
  XLOG(DBG2) << "removing VLAN " << vlan->getID();

  auto rv = opennsl_vlan_destroy(unit_, vlan->getID());
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

template <typename DELTA, typename ParentClassT>
void BcmSwitch::processNeighborEntryDelta(
    const DELTA& delta,
    std::shared_ptr<SwitchState>* appliedState) {
  const auto* oldEntry = delta.getOld().get();
  const auto* newEntry = delta.getNew().get();

  if (!oldEntry) {
    processAddedNeighborEntry(newEntry);
  } else if (!newEntry) {
    processRemovedNeighborEntry(oldEntry);
  } else {
    processChangedNeighborEntry(oldEntry, newEntry);
  }
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

void BcmSwitch::processNeighborChanges(
    const StateDelta& delta,
    std::shared_ptr<SwitchState>* appliedState) {
  processNeighborTableDelta<folly::IPAddressV4>(delta, appliedState);
  processNeighborTableDelta<folly::IPAddressV6>(delta, appliedState);
}

template <typename AddrT>
void BcmSwitch::processNeighborTableDelta(
    const StateDelta& stateDelta,
    std::shared_ptr<SwitchState>* appliedState) {
  using NeighborTableT = std::conditional_t<
      std::is_same<AddrT, folly::IPAddressV4>::value,
      ArpTable,
      NdpTable>;
  using NeighborEntryT = typename NeighborTableT::Entry;
  using NeighborEntryDeltaT = DeltaValue<NeighborEntryT>;
  std::vector<NeighborEntryDeltaT> discardedNeighborEntryDelta;

  for (const auto& vlanDelta : stateDelta.getVlansDelta()) {
    auto oldVlan = vlanDelta.getOld();
    auto newVlan = vlanDelta.getNew();

    for (const auto& delta :
         vlanDelta.template getNeighborDelta<NeighborTableT>()) {
      try {
        processNeighborEntryDelta<NeighborEntryDeltaT, NeighborTableT>(
            delta, appliedState);
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
    opennsl_port_t bcmPort,
    opennsl_port_info_t* info) {
  try {
    BcmUnit* unitObj = BcmAPI::getUnit(unit);
    BcmSwitch* hw = static_cast<BcmSwitch*>(unitObj->getCookie());
    bool up = info->linkstatus == OPENNSL_PORT_LINK_STATUS_UP;

    hw->linkScanBottomHalfEventBase_.runInEventBaseThread(
        [hw, bcmPort, up]() { hw->linkStateChangedHwNotLocked(bcmPort, up); });
  } catch (const std::exception& ex) {
    XLOG(ERR) << "unhandled exception while processing linkscan callback "
              << "for unit " << unit << " port " << bcmPort << ": "
              << folly::exceptionStr(ex);
  }
}

void BcmSwitch::linkStateChangedHwNotLocked(opennsl_port_t bcmPortId, bool up) {
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

opennsl_rx_t
BcmSwitch::packetRxCallback(int unit, opennsl_pkt_t* pkt, void* cookie) {
  auto* bcmSw = static_cast<BcmSwitch*>(cookie);
  DCHECK_EQ(bcmSw->getUnit(), unit);
  DCHECK_EQ(bcmSw->getUnit(), pkt->unit);
  return bcmSw->packetReceived(pkt);
}

opennsl_rx_t BcmSwitch::packetReceived(opennsl_pkt_t* pkt) noexcept {
  // Log it if it's an sFlow sample
  if (handleSflowPacket(pkt)) {
    // It was just here because it was an sFlow packet
    opennsl_rx_free(pkt->unit, pkt);
    return OPENNSL_RX_HANDLED_OWNED;
  }

  // Otherwise, send it to the SwSwitch
  unique_ptr<BcmRxPacket> bcmPkt;
  try {
    bcmPkt = createRxPacket(pkt);
  } catch (const std::exception& ex) {
    XLOG(ERR) << "failed to allocate BcmRxPacket for receive handling: "
              << folly::exceptionStr(ex);
    return OPENNSL_RX_NOT_HANDLED;
  }

  callback_->packetReceived(std::move(bcmPkt));
  return OPENNSL_RX_HANDLED_OWNED;
}

bool BcmSwitch::sendPacketSwitchedAsync(unique_ptr<TxPacket> pkt) noexcept {
  unique_ptr<BcmTxPacket> bcmPkt(
      boost::polymorphic_downcast<BcmTxPacket*>(pkt.release()));
  return OPENNSL_SUCCESS(BcmTxPacket::sendAsync(std::move(bcmPkt)));
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
  return OPENNSL_SUCCESS(BcmTxPacket::sendAsync(std::move(bcmPkt)));
}

bool BcmSwitch::sendPacketSwitchedSync(unique_ptr<TxPacket> pkt) noexcept {
  unique_ptr<BcmTxPacket> bcmPkt(
      boost::polymorphic_downcast<BcmTxPacket*>(pkt.release()));
  return OPENNSL_SUCCESS(BcmTxPacket::sendSync(std::move(bcmPkt)));
}

bool BcmSwitch::sendPacketOutOfPortSync(
    unique_ptr<TxPacket> pkt,
    PortID portID) noexcept {
  unique_ptr<BcmTxPacket> bcmPkt(
      boost::polymorphic_downcast<BcmTxPacket*>(pkt.release()));
  bcmPkt->setDestModPort(getPortTable()->getBcmPortId(portID));
  return OPENNSL_SUCCESS(BcmTxPacket::sendSync(std::move(bcmPkt)));
}

bool BcmSwitch::sendPacketOutOfPortSync(
    unique_ptr<TxPacket> pkt,
    PortID portID,
    uint8_t cos) noexcept {
  unique_ptr<BcmTxPacket> bcmPkt(
      boost::polymorphic_downcast<BcmTxPacket*>(pkt.release()));
  bcmPkt->setCos(cos);

  return sendPacketOutOfPortSync(std::move(bcmPkt), portID);
}

void BcmSwitch::updateStats(SwitchStats* switchStats) {
  // Update thread-local switch statistics.
  updateThreadLocalSwitchStats(switchStats);
  // Update thread-local per-port statistics.
  PortStatsMap* portStatsMap = switchStats->getPortStats();
  for (auto& it : *portStatsMap) {
    PortID portID = it.first;
    PortStats* portStats = it.second.get();
    updateThreadLocalPortStats(portID, portStats);
  }
  // Update global statistics.
  updateGlobalStats();
  // Update cpu or host bound packet stats
  controlPlane_->updateQueueCounters();
}

shared_ptr<BcmSwitchEventCallback> BcmSwitch::registerSwitchEventCallback(
    opennsl_switch_event_t eventID,
    shared_ptr<BcmSwitchEventCallback> callback) {
  return BcmSwitchEventUtils::registerSwitchEventCallback(
      unit_, eventID, callback);
}
void BcmSwitch::unregisterSwitchEventCallback(opennsl_switch_event_t eventID) {
  return BcmSwitchEventUtils::unregisterSwitchEventCallback(unit_, eventID);
}

void BcmSwitch::updateThreadLocalSwitchStats(SwitchStats* /*switchStats*/) {
  // TODO
}

void BcmSwitch::updateThreadLocalPortStats(
    PortID /*portID*/,
    PortStats* /*portStats*/) {
  // TODO
}

void BcmSwitch::updateGlobalStats() {
  portTable_->updatePortStats();
  trunkTable_->updateStats();
  bcmStatUpdater_->updateStats();

  auto now = WallClockUtil::NowInSecFast();
  if ((now - bstStatsUpdateTime_ >= FLAGS_update_bststats_interval_s) ||
      bstStatsMgr_->isFineGrainedBufferStatLoggingEnabled()) {
    bstStatsUpdateTime_ = now;
    bstStatsMgr_->updateStats();
  }
}

opennsl_if_t BcmSwitch::getDropEgressId() const {
  return BcmEgress::getDropEgressId();
}

opennsl_if_t BcmSwitch::getToCPUEgressId() const {
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
    opennsl_l2_addr_t* l2Addr,
    int operation,
    void* userData) {
  auto* bcmSw = static_cast<BcmSwitch*>(userData);
  DCHECK_EQ(bcmSw->getUnit(), unit);
  bcmSw->l2LearningUpdateReceived(l2Addr, operation);
}

void BcmSwitch::l2LearningUpdateReceived(
    opennsl_l2_addr_t* l2Addr,
    int operation) noexcept {
  /*
   * TODO (skhare)
   * Dispatch "valid" callbacks
   */
  if (l2Addr && isL2OperationOfInterest(operation)) {
    callback_->l2LearningUpdateReceived(
        createL2Entry(l2Addr), getL2EntryUpdateType(operation));
  }
}

} // namespace fboss
} // namespace facebook
