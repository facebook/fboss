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
#include <folly/Optional.h>
#include <folly/hash/Hash.h>
#include <folly/logging/xlog.h>
#include "common/time/Time.h"
#include "fboss/agent/Constants.h"
#include "fboss/agent/FbossError.h"
#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/SwitchStats.h"
#include "fboss/agent/Utils.h"
#include "fboss/agent/hw/BufferStatsLogger.h"
#include "fboss/agent/hw/bcm/BcmAPI.h"
#include "fboss/agent/hw/bcm/BcmAclTable.h"
#include "fboss/agent/hw/bcm/BcmControlPlane.h"
#include "fboss/agent/hw/bcm/BcmCosManager.h"
#include "fboss/agent/hw/bcm/BcmError.h"
#include "fboss/agent/hw/bcm/BcmHost.h"
#include "fboss/agent/hw/bcm/BcmHostKey.h"
#include "fboss/agent/hw/bcm/BcmIntf.h"
#include "fboss/agent/hw/bcm/BcmMirrorTable.h"
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
#include "fboss/agent/hw/bcm/BcmTableStats.h"
#include "fboss/agent/hw/bcm/BcmTrunkTable.h"
#include "fboss/agent/hw/bcm/BcmTxPacket.h"
#include "fboss/agent/hw/bcm/BcmUnit.h"
#include "fboss/agent/hw/bcm/BcmWarmBootCache.h"
#include "fboss/agent/hw/bcm/BcmWarmBootHelper.h"
#include "fboss/agent/hw/bcm/BcmBstStatsMgr.h"
#include "fboss/agent/hw/bcm/gen-cpp2/bcmswitch_constants.h"
#include "fboss/agent/state/AclEntry.h"
#include "fboss/agent/state/AggregatePort.h"
#include "fboss/agent/state/ArpEntry.h"
#include "fboss/agent/state/DeltaFunctions.h"
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

using std::make_unique;
using std::chrono::seconds;
using std::chrono::duration_cast;
using std::unique_ptr;
using std::make_pair;
using std::make_shared;
using std::move;
using std::shared_ptr;
using std::string;

using facebook::fboss::DeltaFunctions::forEachChanged;
using facebook::fboss::DeltaFunctions::forEachAdded;
using facebook::fboss::DeltaFunctions::forEachRemoved;
using folly::IPAddress;

using namespace std::chrono;

DEFINE_int32(linkscan_interval_us, 250000,
             "The Broadcom linkscan interval");
DEFINE_bool(flexports, false,
            "Load the agent with flexport support enabled");
DEFINE_int32(
    update_bststats_interval_s,
    60,
    "Update BST stats for ODS interval in seconds");
DEFINE_bool(force_init_fp, true, "Force full field processor initialization");

enum : uint8_t {
  kRxCallbackPriority = 1,
};

namespace {
constexpr auto kHostTable = "hostTable";
constexpr int kLogBcmErrorFreqMs = 3000;
/*
 * Dump map containing switch h/w config as a key, value pair
 * to a file. Create parent directories of file if needed.
 * The actual format of config in the file is JSON for easy
 * serialization/desrialization
 */
void dumpConfigMap(const facebook::fboss::BcmAPI::HwConfigMap& config,
    const std::string& filename) {
  folly::dynamic json = folly::dynamic::object;
  for (const auto& kv : config) {
    json[kv.first] = kv.second;
  }
  folly::writeFile(toPrettyJson(json), filename.c_str());
}

void rethrowIfHwNotFull(const facebook::fboss::BcmError& error) {
  if (error.getBcmError() != OPENNSL_E_FULL) {
    // If this is not because of TCAM being full, rethrow the exception.
    throw error;
  }

  XLOG(WARNING) << error.what();
}

}

namespace facebook { namespace fboss {
/*
 * Get current port speed from BCM SDK and convert to
 * cfg::PortSpeed
 */

bool BcmSwitch::getPortFECEnabled(PortID port) const {
  // relies on getBcmPort() to throw an if not found
  return getPortTable()->getBcmPort(port)->isFECEnabled();
}

BcmSwitch::BcmSwitch(BcmPlatform* platform, uint32_t featuresDesired)
    : platform_(platform),
      featuresDesired_(featuresDesired),
      mmuBufferBytes_(platform->getMMUBufferBytes()),
      mmuCellBytes_(platform->getMMUCellBytes()),
      warmBootCache_(new BcmWarmBootCache(this)),
      portTable_(new BcmPortTable(this)),
      intfTable_(new BcmIntfTable(this)),
      hostTable_(new BcmHostTable(this)),
      routeTable_(new BcmRouteTable(this)),
      qosPolicyTable_(new BcmQosPolicyTable(this)),
      aclTable_(new BcmAclTable(this)),
      trunkTable_(new BcmTrunkTable(this)),
      sFlowExporterTable_(new BcmSflowExporterTable()),
      rtag7LoadBalancer_(new BcmRtag7LoadBalancer(this)),
      mirrorTable_(new BcmMirrorTable(this)),
      bstStatsMgr_(new BcmBstStatsMgr(this)) {
  dumpConfigMap(BcmAPI::getHwConfig(), platform->getHwConfigDumpFile());
  exportSdkVersion();
}

BcmSwitch::BcmSwitch(
    BcmPlatform* platform,
    unique_ptr<BcmUnit> unit,
    uint32_t featuresDesired)
    : BcmSwitch(platform, featuresDesired) {
  unitObject_ = std::move(unit);
  unit_ = unitObject_->getNumber();
}

BcmSwitch::~BcmSwitch() {
  XLOG(ERR) << "Destroying BcmSwitch";
}

void BcmSwitch::resetTablesImpl(std::unique_lock<std::mutex>& /*lock*/) {
  unregisterCallbacks();
  routeTable_.reset();
  // Release host entries before reseting switch's host table
  // entries so that if host try to refer to look up host table
  // via the BCM switch during their destruction the pointer
  // access is still valid.
  hostTable_->releaseHosts();
  hostTable_.reset();
  intfTable_.reset();
  toCPUEgress_.reset();
  portTable_.reset();
  qosPolicyTable_.reset();
  aclTable_->releaseAcls();
  aclTable_.reset();
  mirrorTable_.reset();
  trunkTable_.reset();
  controlPlane_.reset();
  coppAclEntries_.clear();
  rtag7LoadBalancer_.reset();
  bcmStatUpdater_.reset();
  bstStatsMgr_.reset();
  // Reset warmboot cache last in case Bcm object destructors
  // access it during object deletion.
  warmBootCache_.reset();

}

unique_ptr<BcmUnit> BcmSwitch::releaseUnit() {
  std::unique_lock<std::mutex> lk(lock_);

  // Destroy all of our member variables that track state,
  // to make sure they clean up their state now before we reset unit_.
  BcmSwitchEventUtils::resetUnit(unit_);
  resetTablesImpl(lk);
  // We don't maintain a BcmVlan data structure, so the
  // vlans need to be destroyed explicity
  auto rv = opennsl_vlan_destroy_all(unit_);
  bcmCheckError(rv, "failed to destroy all VLANs");
  unit_ = -1;
  unitObject_->setCookie(nullptr);
  return std::move(unitObject_);
}

void BcmSwitch::resetTables() {
  std::unique_lock<std::mutex> lk(lock_);
  resetTablesImpl(lk);
}

void BcmSwitch::initTables(const folly::dynamic& warmBootState) {
  std::lock_guard<std::mutex> g(lock_);
  bcmStatUpdater_ = std::make_unique<BcmStatUpdater>(this, isAlpmEnabled());
  portTable_ = std::make_unique<BcmPortTable>(this);
  qosPolicyTable_ = std::make_unique<BcmQosPolicyTable>(this);
  intfTable_ = std::make_unique<BcmIntfTable>(this);
  hostTable_ = std::make_unique<BcmHostTable>(this);
  routeTable_ = std::make_unique<BcmRouteTable>(this);
  aclTable_ = std::make_unique<BcmAclTable>(this);
  trunkTable_ = std::make_unique<BcmTrunkTable>(this);
  sFlowExporterTable_ = std::make_unique<BcmSflowExporterTable>();
  controlPlane_ = std::make_unique<BcmControlPlane>(this);
  rtag7LoadBalancer_ = std::make_unique<BcmRtag7LoadBalancer>(this);
  mirrorTable_ = std::make_unique<BcmMirrorTable>(this);
  warmBootCache_ = std::make_unique<BcmWarmBootCache>(this);
  warmBootCache_->populate(folly::Optional<folly::dynamic>(warmBootState));
  bstStatsMgr_ = std::make_unique<BcmBstStatsMgr>(this);

  setupToCpuEgress();

  // We should always initPorts for portTable_ during init/initTables, Otherwise
  // portable will be empty.
  opennsl_port_config_t pcfg;
  auto rv = opennsl_port_config_get(unit_, &pcfg);
  bcmCheckError(rv, "failed to get port configuration");
  portTable_->initPorts(&pcfg, true);

  setupCos();
  auto switchState = stateChangedImpl(
      StateDelta(make_shared<SwitchState>(), getWarmBootSwitchState()));
  restorePortSettings(switchState);
  setupLinkscan();
  setupPacketRx();
}

void BcmSwitch::unregisterCallbacks() {
  if (flags_ & RX_REGISTERED) {
    opennsl_rx_stop(unit_, nullptr);
    auto rv = opennsl_rx_unregister(unit_, packetRxCallback,
        kRxCallbackPriority);
    CHECK(OPENNSL_SUCCESS(rv)) <<
        "failed to unregister BcmSwitch rx callback: " << opennsl_errmsg(rv);
    flags_ &= ~RX_REGISTERED;
  }
  // Note that we don't explicitly call opennsl_linkscan_detach() here--
  // this call is not thread safe and should only be called from the main
  // thread.  However, opennsl_detach() / _opennsl_shutdown() will clean up the
  // linkscan module properly.
  if (flags_ & LINKSCAN_REGISTERED) {
    auto rv = opennsl_linkscan_unregister(unit_, linkscanCallback);
    CHECK(OPENNSL_SUCCESS(rv)) << "failed to unregister BcmSwitch linkscan "
      "callback: " << opennsl_errmsg(rv);

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
  unitObject_->detachAndSetupWarmBoot(switchState);
  unitObject_.reset();
  XLOG(INFO)
      << "[Exit] BRCM Graceful Exit time "
      << duration_cast<duration<float>>(steady_clock::now() - begin).count();
}

folly::dynamic BcmSwitch::toFollyDynamic() const {
  folly::dynamic hwSwitch = folly::dynamic::object;
  // For now we only serialize Host table
  hwSwitch[kHostTable] = hostTable_->toFollyDynamic();
  hwSwitch[kIntfTable] = intfTable_->toFollyDynamic();
  hwSwitch[kRouteTable] = routeTable_->toFollyDynamic();
  hwSwitch[kWarmBootCache] = warmBootCache_->toFollyDynamic();
  return hwSwitch;
}

void BcmSwitch::clearWarmBootCache() {
  std::lock_guard<std::mutex> g(lock_);
  warmBootCache_->clear();
}

bool BcmSwitch::isPortUp(PortID port) const {
  int linkStatus;
  opennsl_port_link_status_get(getUnit(), port, &linkStatus);
  return linkStatus == OPENNSL_PORT_LINK_STATUS_UP;
}

std::shared_ptr<SwitchState> BcmSwitch::getColdBootSwitchState() const {
  auto bootState = make_shared<SwitchState>();
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
  return bootState;
}

std::shared_ptr<SwitchState> BcmSwitch::getWarmBootSwitchState() const {
  auto warmBootState = getColdBootSwitchState();
  for (auto port :  *warmBootState->getPorts()) {
    auto bcmPort = portTable_->getBcmPort(port->getID());
    port->setOperState(bcmPort->isUp());
    port->setAdminState(bcmPort->isEnabled() ?
                        cfg::PortState::ENABLED : cfg::PortState::DISABLED);

    XLOG(DBG1) << "Recovered port " << port->getID() << " after warm boot."
               << " AdminState="
               << ((port->isEnabled()) ? "Enabled" : "Disabled")
               << " OperState=" << ((port->isUp()) ? "Up" : "Down");
  }
  warmBootState->resetIntfs(warmBootCache_->reconstructInterfaceMap());
  warmBootState->resetVlans(warmBootCache_->reconstructVlanMap());
  warmBootState->resetRouteTables(warmBootCache_->reconstructRouteTables());
  warmBootState->resetAcls(warmBootCache_->reconstructAclMap());
  warmBootState->resetQosPolicies(warmBootCache_->reconstructQosPolicies());
  warmBootState->resetLoadBalancers(warmBootCache_->reconstructLoadBalancers());
  warmBootState->resetMirrors(warmBootCache_->reconstructMirrors());
  warmBootCache_->reconstructPortMirrors(&warmBootState);
  warmBootCache_->reconstructPortVlans(&warmBootState);
  return warmBootState;
}

void BcmSwitch::runBcmScriptPreAsicInit() const {
  std::string filename = platform_->getScriptPreAsicInit();
  std::ifstream scriptFile(filename);

  if (!scriptFile.good()) {
    return;
  }

  XLOG(INFO) << "Run script " << filename;
  printDiagCmd(folly::to<string>("rcload ", filename));
}

void BcmSwitch::setupLinkscan() {
  if (!(featuresDesired_ & LINKSCAN_DESIRED)) {
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
}

HwInitResult BcmSwitch::init(Callback* callback) {
  HwInitResult ret;

  std::lock_guard<std::mutex> g(lock_);

  steady_clock::time_point begin = steady_clock::now();
  if (!unitObject_) {
    unitObject_ = BcmAPI::initOnlyUnit(platform_);
  }
  unit_ = unitObject_->getNumber();
  unitObject_->setCookie(this);

  bootType_ = platform_->getWarmBootHelper()->canWarmBoot()
      ? BootType::WARM_BOOT
      : BootType::COLD_BOOT;
  auto warmBoot = bootType_ == BootType::WARM_BOOT;
  callback_ = callback;

  // Possibly run pre-init bcm shell script before ASIC init.
  runBcmScriptPreAsicInit();

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

  XLOG(INFO) <<" Is ALPM enabled: " << isAlpmEnabled();
  // Additional switch configuration
  auto state = make_shared<SwitchState>();
  opennsl_port_config_t pcfg;
  auto rv = opennsl_port_config_get(unit_, &pcfg);
  bcmCheckError(rv, "failed to get port configuration");


  if (!warmBoot) {
    LOG (INFO) << " Performing cold boot ";
    // Cold boot - put all ports in Vlan 1
    auto vlan = make_shared<Vlan>(VlanID(1), "InitVlan");
    state->addVlan(vlan);
    Vlan::MemberPorts memberPorts;
    opennsl_port_t idx;
    OPENNSL_PBMP_ITER(pcfg.port, idx) {
      memberPorts.insert(make_pair(PortID(idx), false));
    }
    vlan->setPorts(memberPorts);
    /* initialize mirroring module */
    initMirrorModule();
  } else {
    LOG (INFO) << "Performing warm boot ";

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

  trunkTable_->setupTrunking();
  setupLinkscan();
  // If warm booting, force a scan of all ports. Unfortunately
  // opennsl_enable_set will enable all of the ports and return before
  // the first loop on the link thread has updated the link status of
  // ports. This will guarantee we have performed at least one scan of
  // all ports before proceeding.
  if (warmBoot) {
    forceLinkscanOn(pcfg.port);
  }

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
    auto warmBootState = getWarmBootSwitchState();
    warmBootState =
        stateChangedImpl(StateDelta(make_shared<SwitchState>(), warmBootState));
    restorePortSettings(warmBootState);
    hostTable_->warmBootHostEntriesSynced();
    ret.switchState = warmBootState;
  } else {
    ret.switchState = getColdBootSwitchState();
  }

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
    (16 * 1032),            // packet alloc size (12K packets plus spare)
    16,                     // Packets per chain
    0,                      // Default pkt rate, global (all COS, one unit)
    0,                      // Burst
    {                       // 1 RX channel
      {0, 0, 0, 0},         // Channel 0 is usually TX
      {                     // Channel 1, default RX
        4,                  // DV count (number of chains)
        0,                  // Default pkt rate, DEPRECATED
        0,                  // No flags
        0xff                // COS bitmap channel to receive
      }
    },
    nullptr,                // Use default alloc function
    nullptr,                // Use default free function
    0,                      // flags
    0,                      // Num CPU addrs
    nullptr                 // cpu_addresses
  };

  if (!(featuresDesired_ & PACKET_RX_DESIRED)) {
    XLOG(DBG1) << " Skip settiing up packet RX since its explicitly disabled";
    return;
  }
  // Register our packet handler callback function.
  uint32_t rxFlags = OPENNSL_RCO_F_ALL_COS;
  auto rv = opennsl_rx_register(
      unit_,            // int unit
      "fboss_rx",       // const char *name
      packetRxCallback, // opennsl_rx_cb_f callback
      kRxCallbackPriority, // uint8 priority
      this,             // void *cookie
      rxFlags);         // uint32 flags
  bcmCheckError(rv, "failed to register packet rx callback");
  flags_ |= RX_REGISTERED;

  if (!isRxThreadRunning()) {
    rv = opennsl_rx_start(unit_, &rxCfg);
  }
  bcmCheckError(rv, "failed to start broadcom packet rx API");
}

void BcmSwitch::initialConfigApplied() {
  std::lock_guard<std::mutex> g(lock_);
  setupPacketRx();
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
  auto appliedState = delta.newState();
  // TODO: This function contains high-level logic for how to apply the
  // StateDelta, and isn't particularly hardware-specific.  I plan to refactor
  // it, and move it out into a common helper class that can be shared by
  // many different HwSwitch implementations.

  // As the first step, disable ports that are now disabled.
  // This ensures that we immediately stop forwarding traffic on these ports.
  processDisabledPorts(delta);

  processLoadBalancerChanges(delta);

  // remove all routes to be deleted
  processRemovedRoutes(delta);

  // delete all interface not existing anymore. that should stop
  // all traffic on that interface now
  forEachRemoved(delta.getIntfsDelta(), &BcmSwitch::processRemovedIntf, this);

  // Add all new VLANs, and modify VLAN port memberships.
  // We don't actually delete removed VLANs at this point, we simply remove
  // all members from the VLAN.  This way any ports that ingress packets to this
  // VLAN will still use this VLAN until we get the new VLAN fully configured.
  forEachChanged(delta.getVlansDelta(),
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
  reconfigureCoPP(delta);

  // Any neighbor changes, and modify appliedState if some changes fail to apply
  processNeighborChanges(delta, &appliedState);

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

  processAggregatePortChanges(delta);

  // Reconfigure port groups in case we are changing between using a port as
  // 1, 2 or 4 ports. Only do this if flexports are enabled
  if (FLAGS_flexports) {
    reconfigurePortGroups(delta);
  }

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

unique_ptr<TxPacket> BcmSwitch::allocatePacket(uint32_t size) {
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
  forEachChanged(delta.getPortsDelta(),
    [&] (const shared_ptr<Port>& oldPort, const shared_ptr<Port>& newPort) {
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
    bcmPort->setupQueue(queue);
  }
}

void BcmSwitch::processEnabledPorts(const StateDelta& delta) {
  forEachChanged(delta.getPortsDelta(),
    [&] (const shared_ptr<Port>& oldPort, const shared_ptr<Port>& newPort) {
      if (!oldPort->isEnabled() && newPort->isEnabled()) {
        auto bcmPort = portTable_->getBcmPort(newPort->getID());
        bcmPort->enable(newPort);
        processEnabledPortQueues(newPort);
      }
    });
}

bool BcmSwitch::isPortQueueNameChanged(
    const shared_ptr<Port>& oldPort,
    const shared_ptr<Port>& newPort) {
  if (oldPort->getPortQueues().size() != newPort->getPortQueues().size()) {
    return true;
  }

  for (const auto& newQueue : newPort->getPortQueues()) {
    auto oldQueue = oldPort->getPortQueues().at(newQueue->getID());
    if (oldQueue->getName() != newQueue->getName()) {
      return true;
    }
  }

  return false;
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
    bcmPort->setupQueue(newQueue);
  }
}

void BcmSwitch::processChangedPorts(const StateDelta& delta) {
  forEachChanged(delta.getPortsDelta(),
    [&] (const shared_ptr<Port>& oldPort, const shared_ptr<Port>& newPort) {
      auto id = newPort->getID();
      auto bcmPort = portTable_->getBcmPort(id);
      if (oldPort->getName() != newPort->getName()) {
        bcmPort->updateName(newPort->getName());
      }

      if (platform_->isCosSupported() &&
          isPortQueueNameChanged(oldPort, newPort)) {
        bcmPort->getQueueManager()->setupQueueCounters(
            newPort->getPortQueues());
      }

      if (!oldPort->isEnabled() && !newPort->isEnabled()) {
        // No need to process changes on disabled ports. We will pick up changes
        // should the port ever become enabled.
        return;
      }

      auto speedChanged = oldPort->getSpeed() != newPort->getSpeed();
      XLOG_IF(DBG1, speedChanged) << "New speed on port " << id;

      auto vlanChanged = oldPort->getIngressVlan() != newPort->getIngressVlan();
      XLOG_IF(DBG1, vlanChanged) << "New ingress vlan on port " << id;

      auto pauseChanged = oldPort->getPause() != newPort->getPause();
      XLOG_IF(DBG1, pauseChanged) << "New pause settings on port " << id;

      auto sFlowChanged =
          (oldPort->getSflowIngressRate() != newPort->getSflowIngressRate()) ||
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

      if (speedChanged || vlanChanged || pauseChanged || sFlowChanged ||
          fecChanged || loopbackChanged || mirrorChanged || qosPolicyChanged) {
        bcmPort->program(newPort);
      }

      if (newPort->getPortQueues().size() != 0 &&
          !platform_->isCosSupported()) {
        throw FbossError("Changing settings for cos queues not supported on ",
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
  forEachChanged(delta.getPortsDelta(),
    [&] (const shared_ptr<Port>& oldPort, const shared_ptr<Port>& newPort) {
      auto enabled = !oldPort->isEnabled() && newPort->isEnabled();
      auto speedChanged = oldPort->getSpeed() != newPort->getSpeed();
      auto sFlowChanged =
          (oldPort->getSflowIngressRate() != newPort->getSflowIngressRate()) ||
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

bool BcmSwitch::isValidPortUpdate(const shared_ptr<Port>& oldPort,
    const shared_ptr<Port>& newPort,
    const shared_ptr<SwitchState>& newState) const {
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

bool BcmSwitch::isValidStateUpdate(const StateDelta& delta) const {
  auto newState = delta.newState();
  auto isValid = true;

  forEachChanged(delta.getPortsDelta(),
      [&] (const shared_ptr<Port>& oldPort, const shared_ptr<Port>& newPort) {
      if (isValid && !isValidPortUpdate(oldPort, newPort, newState)) {
          isValid = false;
      }
  });
  isValid = isValid &&
      (newState->getMirrors()->size() <= bcmswitch_constants::MAX_MIRRORS_);

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

  return isValid;
}

void BcmSwitch::changeDefaultVlan(VlanID id) {
  auto rv = opennsl_vlan_default_set(unit_, id);
  bcmCheckError(rv, "failed to set default VLAN to ", id);
}

void BcmSwitch::processChangedVlan(const shared_ptr<Vlan>& oldVlan,
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
    } else if (newIter == newPorts.end() ||
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
    auto rv = opennsl_vlan_port_add(unit_, newVlan->getID(),
                                addedPorts, addedUntaggedPorts);
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

void BcmSwitch::processChangedIntf(const shared_ptr<Interface>& oldIntf,
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
  if (!platform_->areAclsSupported()) {
    // certain platforms may not support acls fully.
    return;
  }

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
  using EntryT = typename DELTA::Node;
  const auto* oldEntry = delta.getOld().get();
  const auto* newEntry = delta.getNew().get();

  const BcmIntf *intf;
  opennsl_vrf_t vrf;
  auto getIntfAndVrf = [&](InterfaceID id) {
    intf = getIntfTable()->getBcmIntf(id);
    vrf = getBcmVrfId(intf->getInterface()->getRouterID());
  };

  auto program = [&](std::string op, BcmHost* host) {
    CHECK(newEntry);

    auto isTrunk = newEntry->getPort().isAggregatePort();

    XLOG(DBG3) << op << " neighbor entry " << newEntry->getIP().str() << " to "
               << newEntry->getMac().toString();

    if (isTrunk) {
      auto trunk = newEntry->getPort().aggPortID();
      host->programToTrunk(
          intf->getBcmIfId(),
          newEntry->getMac(),
          getTrunkTable()->getBcmTrunkId(trunk));
    } else {
      auto port = newEntry->getPort().phyPortID();
      host->program(
          intf->getBcmIfId(),
          newEntry->getMac(),
          getPortTable()->getBcmPortId(port));
    }
  };

  if (!oldEntry) {
    getIntfAndVrf(newEntry->getIntfID());
    auto host = hostTable_->incRefOrCreateBcmHost(
        BcmHostKey(vrf, IPAddress(newEntry->getIP()), newEntry->getIntfID()));

    if (newEntry->isPending()) {
      XLOG(DBG3) << "adding pending neighbor entry to "
                 << newEntry->getIP().str();
      try {
        host->programToCPU(intf->getBcmIfId());
      } catch (const BcmError& error) {
        rethrowIfHwNotFull(error);
        SwitchState::revertNewNeighborEntry<EntryT, ParentClassT>(
            delta.getNew(), nullptr, appliedState);
      }
    } else {
      try {
        program("adding", host);
      } catch (const BcmError& error) {
        rethrowIfHwNotFull(error);
        SwitchState::revertNewNeighborEntry<EntryT, ParentClassT>(
            delta.getNew(), nullptr, appliedState);
      }
    }
  } else if (!newEntry) {
    XLOG(DBG3) << "deleting neighbor entry " << oldEntry->getIP().str();
    getIntfAndVrf(oldEntry->getIntfID());
    auto host = hostTable_->derefBcmHost(
        BcmHostKey(vrf, IPAddress(oldEntry->getIP()), oldEntry->getIntfID()));
    if (host) {
        host->programToCPU(intf->getBcmIfId());
        // This should not fail. Not catching exceptions. If the delete fails,
        // it is probably not because of TCAM being full.
    }
  } else {
    CHECK_EQ(oldEntry->getIP(), newEntry->getIP());
    getIntfAndVrf(newEntry->getIntfID());
    auto host = hostTable_->getBcmHost(
        BcmHostKey(vrf, IPAddress(newEntry->getIP()), newEntry->getIntfID()));
    try {
      if (newEntry->isPending()) {
        XLOG(DBG3) << "changing neighbor entry " << oldEntry->getIP().str()
                   << " to pending";
        host->programToCPU(intf->getBcmIfId());
      } else {
        program("changing", host);
      }
    } catch (const BcmError& error) {
      rethrowIfHwNotFull(error);
      SwitchState::revertNewNeighborEntry<EntryT, ParentClassT>(
          delta.getOld(), delta.getNew(), appliedState);
    }
  }
}

void BcmSwitch::processNeighborChanges(
    const StateDelta& delta,
    std::shared_ptr<SwitchState>* appliedState) {
  for (const auto& vlanDelta : delta.getVlansDelta()) {
    for (const auto& arpDelta : vlanDelta.getArpDelta()) {
      using DeltaT = DeltaValue<ArpEntry>;
      processNeighborEntryDelta<DeltaT, ArpTable>(arpDelta, appliedState);
    }
    for (const auto& ndpDelta : vlanDelta.getNdpDelta()) {
      using DeltaT = DeltaValue<NdpEntry>;
      processNeighborEntryDelta<DeltaT, NdpTable>(ndpDelta, appliedState);
    }
  }
}

template <typename RouteT>
void BcmSwitch::processChangedRoute(
    const RouterID& id,
    std::shared_ptr<SwitchState>* appliedState,
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
    try {
      routeTable_->addRoute(getBcmVrfId(id), newRoute.get());
    } catch (const BcmError& error) {
      rethrowIfHwNotFull(error);
      SwitchState::revertNewRouteEntry(id, newRoute, oldRoute, appliedState);
    }
  }
}

template <typename RouteT>
void BcmSwitch::processAddedRoute(
    const RouterID& id,
    std::shared_ptr<SwitchState>* appliedState,
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
  try {
    routeTable_->addRoute(getBcmVrfId(id), route.get());
  } catch (const BcmError& error) {
    rethrowIfHwNotFull(error);
    using AddrT = typename RouteT::Addr;
    SwitchState::revertNewRouteEntry<AddrT>(id, route, nullptr, appliedState);
  }
}

template <typename RouteT>
void BcmSwitch::processRemovedRoute(const RouterID id,
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

void BcmSwitch::processAddedChangedRoutes(
    const StateDelta& delta,
    std::shared_ptr<SwitchState>* appliedState) {
  for (auto const& rtDelta : delta.getRouteTablesDelta()) {
    if (!rtDelta.getNew()) {
      // no new route table, must not have added or changed route, skip
      continue;
    }
    RouterID id = rtDelta.getNew()->getID();
    forEachChanged(
        rtDelta.getRoutesV4Delta(),
        &BcmSwitch::processChangedRoute<RouteV4>,
        &BcmSwitch::processAddedRoute<RouteV4>,
        [&](BcmSwitch*,
            const RouterID&,
            std::shared_ptr<SwitchState>*,
            const shared_ptr<RouteV4>&) {},
        this,
        id,
        appliedState);
    forEachChanged(
        rtDelta.getRoutesV6Delta(),
        &BcmSwitch::processChangedRoute<RouteV6>,
        &BcmSwitch::processAddedRoute<RouteV6>,
        [&](BcmSwitch*,
            const RouterID&,
            std::shared_ptr<SwitchState>*,
            const shared_ptr<RouteV6>&) {},
        this,
        id,
        appliedState);
  }
}

void BcmSwitch::linkscanCallback(
    int unit,
    opennsl_port_t bcmPort,
    opennsl_port_info_t* info) {
  try {
    BcmUnit* unitObj = BcmAPI::getUnit(unit);
    BcmSwitch* sw = static_cast<BcmSwitch*>(unitObj->getCookie());
    bool up = info->linkstatus == OPENNSL_PORT_LINK_STATUS_UP;

    sw->linkScanBottomHalfEventBase_.runInEventBaseThread(
        [sw, bcmPort, up]() { sw->linkStateChangedHwNotLocked(bcmPort, up); });
  } catch (const std::exception& ex) {
    XLOG(ERR) << "unhandled exception while processing linkscan callback "
              << "for unit " << unit << " port " << bcmPort << ": "
              << folly::exceptionStr(ex);
  }
}

void BcmSwitch::linkStateChangedHwNotLocked(
    opennsl_port_t bcmPortId,
    bool up) {
  CHECK(linkScanBottomHalfEventBase_.inRunningEventBaseThread());

  if (!up) {
    auto trunk = trunkTable_->linkDownHwNotLocked(bcmPortId);
    if (trunk != BcmTrunk::INVALID) {
      XLOG(INFO) << "Shrinking ECMP entries egressing over trunk " << trunk;
      hostTable_->trunkDownHwNotLocked(trunk);
    }
    hostTable_->linkDownHwNotLocked(bcmPortId);
  } else {
    // For port up events we wait till ARP/NDP entries
    // are re resolved after port up before adding them
    // back. Adding them earlier leads to packet loss.
  }
  callback_->linkStateChanged(portTable_->getPortId(bcmPortId), up);
}

opennsl_rx_t BcmSwitch::packetRxCallback(int unit, opennsl_pkt_t* pkt,
    void* cookie) {
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
    XLOG(ERR) << "failed to allocated BcmRxPacket for receive handling: "
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
    folly::Optional<uint8_t> cos) noexcept {
  unique_ptr<BcmTxPacket> bcmPkt(
      boost::polymorphic_downcast<BcmTxPacket*>(pkt.release()));
  bcmPkt->setDestModPort(getPortTable()->getBcmPortId(portID));
  if (cos) {
    bcmPkt->setCos(*cos);
  }
  XLOG(DBG4) << "sendPacketOutOfPortAsync for"
             << getPortTable()->getBcmPortId(portID);
  return OPENNSL_SUCCESS(BcmTxPacket::sendAsync(std::move(bcmPkt)));
}

bool BcmSwitch::sendPacketSwitchedSync(unique_ptr<TxPacket> pkt) noexcept {
  unique_ptr<BcmTxPacket> bcmPkt(
      boost::polymorphic_downcast<BcmTxPacket*>(pkt.release()));
  return OPENNSL_SUCCESS(BcmTxPacket::sendSync(std::move(bcmPkt)));
}

bool BcmSwitch::sendPacketOutOfPortSync(unique_ptr<TxPacket> pkt,
                                    PortID portID) noexcept {
  unique_ptr<BcmTxPacket> bcmPkt(
      boost::polymorphic_downcast<BcmTxPacket*>(pkt.release()));
  bcmPkt->setDestModPort(getPortTable()->getBcmPortId(portID));
  XLOG(DBG4) << "sendPacketOutOfPortSync for"
             << getPortTable()->getBcmPortId(portID);
  return OPENNSL_SUCCESS(BcmTxPacket::sendSync(std::move(bcmPkt)));
}

bool BcmSwitch::sendPacketOutOfPortSync(unique_ptr<TxPacket> pkt,
                                        PortID portID,
                                       uint8_t cos) noexcept {
  unique_ptr<BcmTxPacket> bcmPkt(
      boost::polymorphic_downcast<BcmTxPacket*>(pkt.release()));
  bcmPkt->setCos(cos);

  return sendPacketOutOfPortSync(std::move(bcmPkt), portID);
}

void BcmSwitch::updateStats(SwitchStats *switchStats) {
  // Update thread-local switch statistics.
  updateThreadLocalSwitchStats(switchStats);
  // Update thread-local per-port statistics.
  PortStatsMap* portStatsMap = switchStats->getPortStats();
  for (auto& it : *portStatsMap) {
    PortID portID = it.first;
    PortStats *portStats = it.second.get();
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
  return BcmSwitchEventUtils::registerSwitchEventCallback(unit_, eventID,
                                                          callback);
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
  for (const auto newQueue: newCPU->getQueues()) {
    if (oldCPU->getQueues().size() > 0 &&
        *(oldCPU->getQueues().at(newQueue->getID())) == *newQueue) {
      continue;
    }
    XLOG(DBG1) << "New cos queue settings on cpu queue "
               << static_cast<int>(newQueue->getID());
    controlPlane_->setupQueue(newQueue);
  }

  if (isControlPlaneQueueNameChanged(oldCPU, newCPU)) {
    controlPlane_->getQueueManager()->setupQueueCounters(newCPU->getQueues());
  }
}

void BcmSwitch::processControlPlaneChanges(const StateDelta& delta) {
  const auto controlPlaneDelta = delta.getControlPlaneDelta();
  const auto& oldCPU = controlPlaneDelta.getOld();
  const auto& newCPU = controlPlaneDelta.getNew();

  processChangedControlPlaneQueues(oldCPU, newCPU);
  if (oldCPU->getQosPolicy() != newCPU->getQosPolicy()) {
    controlPlane_->setupIngressQosPolicy(newCPU->getQosPolicy());
  }
  // TODO(joseph5wu) Add reason-port mapping and cpu acls
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

void BcmSwitch::restorePortSettings(const std::shared_ptr<SwitchState>& state) {
  /*
  Dumped Switch state(newState) already has ports. However, state delta
  processing only handles enabled/disabled ports & changed ports. It does not
  adds ports to oldState. That is what led to hack this function in.

  After warmboot, there may be some settings of BcmPort which may never get
  applied, because of above. In that case, restore those settings.

  TODO - handle ports in state delta processing, do not not handle init/creating
  BcmPort outside of state delta processing, instead incorporate processing of
  added/removed ports in state delta handling. Doing this will render this
  function unnecessary and can be removed.
  */
  for (auto port : *state->getPorts()) {
    if (port->getIngressMirror()) {
      portTable_->getBcmPort(port->getID())
          ->setIngressPortMirror(port->getIngressMirror().value());
    }
    if (port->getEgressMirror()) {
      portTable_->getBcmPort(port->getID())
          ->setEgressPortMirror(port->getEgressMirror().value());
    }
  }
}
}} // facebook::fboss
