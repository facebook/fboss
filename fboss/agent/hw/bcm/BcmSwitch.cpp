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

#include <sys/stat.h>
#include <sys/types.h>

#include <boost/cast.hpp>
#include <boost/foreach.hpp>

#include <folly/Hash.h>
#include <folly/Memory.h>
#include <folly/ScopeGuard.h>
#include <folly/Conv.h>
#include <folly/FileUtil.h>
#include "fboss/agent/Constants.h"
#include "fboss/agent/FbossError.h"
#include "fboss/agent/SwitchStats.h"
#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/Utils.h"
#include "fboss/agent/hw/bcm/BcmAPI.h"
#include "fboss/agent/hw/bcm/BcmError.h"
#include "fboss/agent/hw/bcm/BcmIntf.h"
#include "fboss/agent/hw/bcm/BcmPlatform.h"
#include "fboss/agent/hw/bcm/BcmPort.h"
#include "fboss/agent/hw/bcm/BcmPortGroup.h"
#include "fboss/agent/hw/bcm/BcmPortTable.h"
#include "fboss/agent/hw/bcm/BcmHost.h"
#include "fboss/agent/hw/bcm/BcmRoute.h"
#include "fboss/agent/hw/bcm/BcmRxPacket.h"
#include "fboss/agent/hw/bcm/BcmSwitchEventManager.h"
#include "fboss/agent/hw/bcm/BcmSwitchEventCallback.h"
#include "fboss/agent/hw/bcm/BcmTxPacket.h"
#include "fboss/agent/hw/bcm/BcmUnit.h"
#include "fboss/agent/hw/bcm/BcmWarmBootCache.h"
#include "fboss/agent/state/AclEntry.h"
#include "fboss/agent/state/ArpEntry.h"
#include "fboss/agent/state/DeltaFunctions.h"
#include "fboss/agent/state/Interface.h"
#include "fboss/agent/state/InterfaceMap.h"
#include "fboss/agent/state/NodeMapDelta.h"
#include "fboss/agent/state/NodeMapDelta-defs.h"
#include "fboss/agent/state/Port.h"
#include "fboss/agent/state/PortMap.h"
#include "fboss/agent/state/StateDelta.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/state/Vlan.h"
#include "fboss/agent/state/VlanMap.h"
#include "fboss/agent/state/VlanMapDelta.h"
#include "fboss/agent/state/Route.h"
#include "fboss/agent/state/RouteTableRib.h"
#include "fboss/agent/state/RouteTable.h"
#include "fboss/agent/state/RouteTableMap.h"
#include "fboss/agent/state/RouteDelta.h"
#include "fboss/agent/SysError.h"

extern "C" {
#include <opennsl/link.h>
#include <opennsl/port.h>
#include <opennsl/stg.h>
#include <opennsl/switch.h>
#include <opennsl/vlan.h>
}

using folly::make_unique;
using std::chrono::seconds;
using std::chrono::duration_cast;
using std::unique_ptr;
using std::make_pair;
using std::make_shared;
using std::shared_ptr;
using std::string;

using facebook::fboss::DeltaFunctions::forEachChanged;
using facebook::fboss::DeltaFunctions::forEachAdded;
using facebook::fboss::DeltaFunctions::forEachRemoved;
using folly::IPAddress;

DEFINE_int32(linkscan_interval_us, 250000,
             "The Broadcom linkscan interval");
DEFINE_bool(flexports, false,
            "Load the agent with flexport support enabled");

enum : uint8_t {
  kRxCallbackPriority = 1,
};

namespace {
constexpr auto kHostTable = "hostTable";
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
}

namespace facebook { namespace fboss {
/*
 * Get current port speed from BCM SDK and convert to
 * cfg::PortSpeed
 */
cfg::PortSpeed BcmSwitch::getPortSpeed(PortID port) const {
  int portSpeed;
  auto ret = opennsl_port_speed_get(unit_, port, &portSpeed);
  bcmCheckError(ret, "failed to get current speed for port", port);
  return cfg::PortSpeed(portSpeed);
}

cfg::PortSpeed BcmSwitch::getMaxPortSpeed(PortID port) const {
  int maxPortSpeed = 0;
  auto ret = opennsl_port_speed_max(unit_, port, &maxPortSpeed);
  bcmCheckError(ret, "failed to get max speed for port", port);
  return cfg::PortSpeed(maxPortSpeed);
}

BcmSwitch::BcmSwitch(BcmPlatform *platform, HashMode hashMode)
  : platform_(platform),
    hashMode_(hashMode),
    portTable_(new BcmPortTable(this)),
    intfTable_(new BcmIntfTable(this)),
    hostTable_(new BcmHostTable(this)),
    routeTable_(new BcmRouteTable(this)),
    aclTable_(new BcmAclTable()),
    warmBootCache_(new BcmWarmBootCache(this)) {

  // Start switch event manager so critical events will be handled.
  switchEventManager_.reset(new BcmSwitchEventManager(this));

  // Register parity error callback handler.
  switchEventManager_->registerSwitchEventCallback(
    OPENNSL_SWITCH_EVENT_PARITY_ERROR,
    make_shared<BcmSwitchEventParityErrorCallback>());
  dumpConfigMap(BcmAPI::getHwConfig(), platform->getHwConfigDumpFile());

  exportSdkVersion();
}

BcmSwitch::BcmSwitch(BcmPlatform *platform, unique_ptr<BcmUnit> unit)
  : BcmSwitch(platform) {
  unitObject_ = std::move(unit);
  unit_ = unitObject_->getNumber();
}

BcmSwitch::~BcmSwitch() {
  if (unitObject_) {
    unregisterCallbacks();
  }
}

unique_ptr<BcmUnit> BcmSwitch::releaseUnit() {
  std::lock_guard<std::mutex> g(lock_);

  unregisterCallbacks();

  // Destroy all of our member variables that track state,
  // to make sure they clean up their state now before we reset unit_.
  switchEventManager_.reset();
  warmBootCache_.reset();
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

  unit_ = -1;
  unitObject_->setCookie(nullptr);
  return std::move(unitObject_);
}

void BcmSwitch::unregisterCallbacks() {
  if (flags_ & RX_REGISTERED) {
    opennsl_rx_stop(unit_, nullptr);
    auto rv = opennsl_rx_unregister(unit_, packetRxCallback,
        kRxCallbackPriority);
    CHECK(OPENNSL_SUCCESS(rv)) <<
        "failed to unregister BcmSwitch rx callback: " << opennsl_errmsg(rv);
  }
  // Note that we don't explicitly call opennsl_linkscan_detach() here--
  // this call is not thread safe and should only be called from the main
  // thread.  However, opennsl_detach() / _opennsl_shutdown() will clean up the
  // linkscan module properly.
  if (flags_ & LINKSCAN_REGISTERED) {
    auto rv = opennsl_linkscan_unregister(unit_, linkscanCallback);
    CHECK(OPENNSL_SUCCESS(rv)) << "failed to unregister BcmSwitch linkscan "
      "callback: " << opennsl_errmsg(rv);

    disableLinkscan();
  }
}

void BcmSwitch::ecmpHashSetup() {
  int arg;
  int rv;

  // enable preprocessing for both hash module A and B
  rv = opennsl_switch_control_set(unit_,
      opennslSwitchHashField0PreProcessEnable, 1);
  bcmCheckError(rv, "failed to enable pre-processing A");
  rv = opennsl_switch_control_set(unit_,
      opennslSwitchHashField1PreProcessEnable, 1);
  bcmCheckError(rv, "failed to enable pre-processing B");

  // set the seed
  // We do not want the traffic flow hashing is changed cross process restart,
  // therefore We need to generate consistent seeds for this box.
  // Here, we use the local MAC base (lower 32b as they are mostly NIC ID) to
  // generate two different hashes to set to seed0 and seed1.
  auto mac64 = platform_->getLocalMac().u64HBO();
  uint32_t mac32 = static_cast<uint32_t>(mac64 & 0xFFFFFFFF);
  rv = opennsl_switch_control_set(unit_, opennslSwitchHashSeed0,
                              folly::hash::jenkins_rev_mix32(mac32));
  bcmCheckError(rv, "failed to set hash seed 0");
  rv = opennsl_switch_control_set(unit_, opennslSwitchHashSeed1,
                              folly::hash::twang_32from64(mac64));
  bcmCheckError(rv, "failed to set hash seed 1");

  // First, field selection:
  // For both IPv4 and IPv6, depending on whether this is full mode
  // or half mode hash we use for hashing either
  // a) src IP, dst IP, src port, and dst port - full mode
  // b) src IP and dst IP - half mode
  // We alternate b/w full and half modes in layers of n/w tiers
  // So if tier n uses full mode, tier n + 1 would use half mode and
  // so on. This is done to avoid hash polarization
  arg = OPENNSL_HASH_FIELD_IP4SRC_LO | OPENNSL_HASH_FIELD_IP4SRC_HI
    | OPENNSL_HASH_FIELD_IP4DST_LO | OPENNSL_HASH_FIELD_IP4DST_HI;
  if (hashMode_ == FULL_HASH) {
    // We are using full mode hash, add src, dst ports
    arg |= OPENNSL_HASH_FIELD_SRCL4 | OPENNSL_HASH_FIELD_DSTL4;
  }
  rv = opennsl_switch_control_set(unit_, opennslSwitchHashIP4Field0, arg);
  bcmCheckError(rv, "failed to config field selection");
  rv = opennsl_switch_control_set(unit_, opennslSwitchHashIP4Field1, arg);
  bcmCheckError(rv, "failed to config field selection");
  rv = opennsl_switch_control_set(unit_, opennslSwitchHashIP4TcpUdpField0, arg);
  bcmCheckError(rv, "failed to config field selection");
  rv = opennsl_switch_control_set(unit_, opennslSwitchHashIP4TcpUdpField1, arg);
  bcmCheckError(rv, "failed to config field selection");
  rv = opennsl_switch_control_set(
      unit_, opennslSwitchHashIP4TcpUdpPortsEqualField0, arg);
  bcmCheckError(rv, "failed to config field selection");
  rv = opennsl_switch_control_set(
      unit_, opennslSwitchHashIP4TcpUdpPortsEqualField1, arg);
  bcmCheckError(rv, "failed to config field selection");
  // v6
  arg = OPENNSL_HASH_FIELD_IP6SRC_LO | OPENNSL_HASH_FIELD_IP6SRC_HI
    | OPENNSL_HASH_FIELD_IP6DST_LO | OPENNSL_HASH_FIELD_IP6DST_HI;
  if (hashMode_ == FULL_HASH) {
    // We are using full mode hash, add src, dst ports
    arg |= OPENNSL_HASH_FIELD_SRCL4 | OPENNSL_HASH_FIELD_DSTL4;
  }
  rv = opennsl_switch_control_set(unit_, opennslSwitchHashIP6Field0, arg);
  bcmCheckError(rv, "failed to config field selection");
  rv = opennsl_switch_control_set(unit_, opennslSwitchHashIP6Field1, arg);
  bcmCheckError(rv, "failed to config field selection");
  rv = opennsl_switch_control_set(unit_, opennslSwitchHashIP6TcpUdpField0, arg);
  bcmCheckError(rv, "failed to config field selection");
  rv = opennsl_switch_control_set(unit_, opennslSwitchHashIP6TcpUdpField1, arg);
  bcmCheckError(rv, "failed to config field selection");
  rv = opennsl_switch_control_set(
      unit_, opennslSwitchHashIP6TcpUdpPortsEqualField0, arg);
  bcmCheckError(rv, "failed to config field selection");
  rv = opennsl_switch_control_set(
      unit_, opennslSwitchHashIP6TcpUdpPortsEqualField1, arg);
  bcmCheckError(rv, "failed to config field selection");

  // Second: hash algorithm.
  // Always use CRC-16 using CCITT polynomial
  arg = OPENNSL_HASH_FIELD_CONFIG_CRC16CCITT;
  rv = opennsl_switch_control_set(unit_, opennslSwitchHashField0Config, arg);
  bcmCheckError(rv, "failed to config CRC16-CCITT to A0");
  rv = opennsl_switch_control_set(unit_, opennslSwitchHashField0Config1, arg);
  bcmCheckError(rv, "failed to config CRC16-CCITT to A1");
  rv = opennsl_switch_control_set(unit_, opennslSwitchHashField1Config, arg);
  bcmCheckError(rv, "failed to config CRC16-CCITT to B0");
  rv = opennsl_switch_control_set(unit_, opennslSwitchHashField1Config1, arg);
  bcmCheckError(rv, "failed to config CRC16-CCITT to B1");

  // Third, choose the ECMP set to use.
  // Use the default offset (0), which means HASH_A0 will be selected.
  // (refer: opennsl_esw_switch_control_port_set())
  rv = opennsl_switch_control_set(unit_, opennslSwitchECMPHashSet0Offset, 0);
  bcmCheckError(rv, "failed to set ECMP set 0");

  configureAdditionalEcmpHashSets();

  // Take default for RTAG7 hash control
  rv = opennsl_switch_control_set(unit_, opennslSwitchHashSelectControl, 0);
  bcmCheckError(rv, "failed to set default RTAG7 hash control");

  // Enable macro flow hash
  rv = opennsl_switch_control_set(unit_,
      opennslSwitchEcmpMacroFlowHashEnable, 1);
  if (rv == OPENNSL_E_UNAVAIL) {
    LOG(INFO) << "Macro flow feature is not available on this hw";
  } else {
    bcmCheckError(rv, "failed to enable macro flow hash");
  }

  // Enable RTAG7 hash
  arg = OPENNSL_HASH_CONTROL_ECMP_ENHANCE;
  rv = opennsl_switch_control_set(unit_, opennslSwitchHashControl, arg);
  bcmCheckError(rv, "failed to enable RTAG7");
}

folly::dynamic BcmSwitch::gracefulExit() {
  std::lock_guard<std::mutex> g(lock_);
  folly::dynamic hwSwitch = toFollyDynamic();
  unitObject_->detach();
  unitObject_.reset();
  return hwSwitch;
}

folly::dynamic BcmSwitch::toFollyDynamic() const {
  folly::dynamic hwSwitch = folly::dynamic::object;
  // For now we only serialize Host table
  hwSwitch[kHostTable] = hostTable_->toFollyDynamic();
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
  opennsl_port_config_t pcfg;
  auto rv = opennsl_port_config_get(unit_, &pcfg);
  bcmCheckError(rv, "failed to get port configuration");
  // On cold boot all ports are in Vlan 1
  auto vlanMap = make_shared<VlanMap>();
  auto vlan = make_shared<Vlan>(VlanID(1), "InitVlan");
  Vlan::MemberPorts memberPorts;
  opennsl_port_t idx;
  OPENNSL_PBMP_ITER(pcfg.port, idx) {
    PortID portID = portTable_->getPortId(idx);
    string name = folly::to<string>("port", portID);
    bootState->registerPort(portID, name);
    memberPorts.insert(make_pair(PortID(idx), false));
  }
  vlan->setPorts(memberPorts);
  bootState->addVlan(vlan);
  return bootState;
}

std::shared_ptr<SwitchState> BcmSwitch::getWarmBootSwitchState() const {
  auto warmBootState = getColdBootSwitchState();
  for (auto port:  *warmBootState->getPorts()) {
    int portEnabled;
    auto ret = opennsl_port_enable_get(unit_, port->getID(), &portEnabled);
    bcmCheckError(ret, "Failed to get current state for port", port->getID());
    port->setState(portEnabled == 1 ? cfg::PortState::UP: cfg::PortState::DOWN);

    auto speed = getPortSpeed(port->getID());
    auto maxSpeed = getMaxPortSpeed(port->getID());
    if (speed == maxSpeed) {
      speed = cfg::PortSpeed::DEFAULT;
    } else if (speed > maxSpeed) {
      LOG(FATAL) << "Invalid port speed:" << (int) speed << " for port:" << port
                 << " max:" << (int) maxSpeed;
    }
    port->setSpeed(speed);
    port->setMaxSpeed(maxSpeed);
  }
  warmBootState->resetIntfs(warmBootCache_->reconstructInterfaceMap());
  warmBootState->resetVlans(warmBootCache_->reconstructVlanMap());
  return warmBootState;
}

std::pair<std::shared_ptr<SwitchState>, BootType>
BcmSwitch::init(Callback* callback) {
  std::lock_guard<std::mutex> g(lock_);

  // Create unitObject_ before doing anything else.
  if (!unitObject_) {
    unitObject_ = BcmAPI::initOnlyUnit();
  }
  unit_ = unitObject_->getNumber();
  unitObject_->setCookie(this);
  callback_ = callback;

  // Initialize the switch.
  if (!unitObject_->isAttached()) {
    unitObject_->attach(platform_->getWarmBootDir());
  }

  LOG(INFO) << "Initializing BcmSwitch for unit " << unit_;

  platform_->onUnitAttach();

  // Additional switch configuration
  auto state = make_shared<SwitchState>();
  opennsl_port_config_t pcfg;
  auto rv = opennsl_port_config_get(unit_, &pcfg);
  bcmCheckError(rv, "failed to get port configuration");

  auto bootType = unitObject_->bootType();
  CHECK(bootType != BootType::UNINITIALIZED);
  auto warmBoot = bootType == BootType::WARM_BOOT;

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
  } else {
    LOG (INFO) << "Performing warm boot ";
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
  // Trap IPv6 Neighbor Discovery Protocol (NDP) packets.
  // TODO: We may want to trap NDP on a per-port or per-VLAN basis.
  rv = opennsl_switch_control_set(unit_, opennslSwitchNdPktToCpu, 1);
  bcmCheckError(rv, "failed to set NDP trapping");
  // Setup hash functions for ECMP
  ecmpHashSetup();

  initFieldProcessor(warmBoot);

  createAclGroup();
  dropDhcpPackets();
  dropIPv6RAs();
  configureRxRateLimiting();

  // enable IPv4 and IPv6 on CPU port
  opennsl_port_t idx;
  OPENNSL_PBMP_ITER(pcfg.cpu, idx) {
    rv = opennsl_port_control_set(unit_, idx, opennslPortControlIP4, 1);
    bcmCheckError(rv, "failed to enable IPv4 on cpu port ", idx);
    rv = opennsl_port_control_set(unit_, idx, opennslPortControlIP6, 1);
    bcmCheckError(rv, "failed to enable IPv6 on cpu port ", idx);
    VLOG(2) << "Enabled IPv4/IPv6 on CPU port " << idx;
  }

  // verify the drop egress ID is really dropping
  BcmEgress::verifyDropEgress(unit_);

  if (warmBoot) {
    // This needs to be done after we have set
    // opennslSwitchL3EgressMode else the egress ids
    // in the host table don't show up correctly.
    warmBootCache_->populate();
  }

  // create an egress object for ToCPU
  toCPUEgress_ = make_unique<BcmEgress>(this);
  toCPUEgress_->programToCPU();

  portTable_->initPorts(&pcfg, warmBoot);

  bcmCheckError(rv, "failed to set linkscan ports");
  rv = opennsl_linkscan_register(unit_, linkscanCallback);
  bcmCheckError(rv, "failed to register for linkscan events");
  flags_ |= LINKSCAN_REGISTERED;
  rv = opennsl_linkscan_enable_set(unit_, FLAGS_linkscan_interval_us);
  bcmCheckError(rv, "failed to enable linkscan");

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
  if (warmBoot) {
    auto warmBootState = getWarmBootSwitchState();
    stateChangedImpl(StateDelta(make_shared<SwitchState>(), warmBootState));
    hostTable_->warmBootHostEntriesSynced();
    return std::make_pair(warmBootState, bootType);
  }
  return std::make_pair(getColdBootSwitchState(), bootType);
}

void BcmSwitch::initialConfigApplied() {
  std::lock_guard<std::mutex> g(lock_);
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
  //
  // TODO: Configure rate limiting for packets sent to the CPU
  //
  // Start the Broadcom packet RX API.
  if (!isRxThreadRunning()) {
    rv = opennsl_rx_start(unit_, nullptr);
  }
  bcmCheckError(rv, "failed to start broadcom packet rx API");
}

void BcmSwitch::stateChanged(const StateDelta& delta) {
  // Take the lock before modifying any objects
  std::lock_guard<std::mutex> g(lock_);
  stateChangedImpl(delta);
}

void BcmSwitch::stateChangedImpl(const StateDelta& delta) {
  // TODO: This function contains high-level logic for how to apply the
  // StateDelta, and isn't particularly hardware-specific.  I plan to refactor
  // it, and move it out into a common helper class that can be shared by
  // many different HwSwitch implementations.

  // As the first step, disable ports that are now disabled.
  // This ensures that we immediately stop forwarding traffic on these ports.
  processDisabledPorts(delta);

  // remove all routes to be deleted
  processRemovedRoutes(delta);

  // delete all interface not existing anymore. that should stop
  // all traffic on that interface now
  forEachRemoved(delta.getIntfsDelta(), &BcmSwitch::processRemovedIntf, this);

  // Add all new VLANs, and modify VLAN port memberships.
  // We don't actually delete removed VLANs at this point, we simply remove
  // all members from the VLAN.  This way any ports that ingres packets to this
  // VLAN will still switch use this VLAN until we get the new VLAN fully
  // configured.
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

  configureCosQMappingForLocalInterfaces(delta);

  // Any ARP changes
  processArpChanges(delta);

  // Any ACL changes
  processAclChanges(delta);

  // Process any new routes or route changes
  processAddedChangedRoutes(delta);

  // Reconfigure port groups in case we are changing between using a port as
  // 1, 2 or 4 ports. Only do this if flexports are enabled
  if (FLAGS_flexports) {
    reconfigurePortGroups(delta);
  }

  // Update speed and ingressVlan of ports if needed.
  processChangedPorts(delta);

  // As the last step, enable newly enabled ports.
  // Doing this as the last step ensures that we only start forwarding traffic
  // once the ports are correctly configured.
  processEnabledPorts(delta);
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
      if (!oldPort->isDisabled() && newPort->isDisabled()) {
        auto bcmPort = portTable_->getBcmPort(newPort->getID());
        bcmPort->disable(newPort);
      }
    });
}

void BcmSwitch::processEnabledPorts(const StateDelta& delta) {
  forEachChanged(delta.getPortsDelta(),
    [&] (const shared_ptr<Port>& oldPort, const shared_ptr<Port>& newPort) {
      if (oldPort->isDisabled() && !newPort->isDisabled()) {
        auto bcmPort = portTable_->getBcmPort(newPort->getID());
        bcmPort->enable(newPort);
      }
    });
}

void BcmSwitch::processChangedPorts(const StateDelta& delta) {
  forEachChanged(delta.getPortsDelta(),
    [&] (const shared_ptr<Port>& oldPort, const shared_ptr<Port>& newPort) {
      auto speedChanged = oldPort->getSpeed() != newPort->getSpeed();
      auto vlanChanged = oldPort->getIngressVlan() != newPort->getIngressVlan();

      if (speedChanged || vlanChanged) {
        auto bcmPort = portTable_->getBcmPort(newPort->getID());
        bcmPort->program(newPort);
      }
    });
}

void BcmSwitch::reconfigurePortGroups(const StateDelta& delta) {
  // This logic is a bit messy. We could encode some notion of port
  // groups into the swith state somehow so it is easy to generate
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
      auto enabled = oldPort->isDisabled() && !newPort->isDisabled();
      auto speedChanged = oldPort->getSpeed() != newPort->getSpeed();

      if (speedChanged || enabled) {
        // this may require reconfiguring lanes. First we check if
        // this is a valid configuration. We could be smarter here
        // and only check once per port group instead of once per
        // changed port in a port group, but keeping it simple for now.
        auto bcmPort = portTable_->getBcmPort(newPort->getID());
        auto portGroup = bcmPort->getPortGroup();
        if (!portGroup) {
          // no port group for this port.
          return;
        } else if (!portGroup->validConfiguration(newState)) {
          // Fail hard for now. We should add this to a pre-update
          // check instead in the future.
          throw FbossError("Invalid port configuration passed in");
        }
        portGroup->reconfigureIfNeeded(newState);
      }
    });
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

  VLOG(2) << "updating VLAN " << newVlan->getID() << ": " <<
    numAdded << " ports added, " << numRemoved << " ports removed";
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
  VLOG(2) << "creating VLAN " << vlan->getID() << " with " <<
    vlan->getPorts().size() << " ports";

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
      VLOG(1) << "updating VLAN " << vlan->getID() << " with " <<
        vlan->getPorts().size() << " ports";
      auto oldVlan = vlan->clone();
      warmBootCache_->fillVlanPortInfo(oldVlan.get());
      processChangedVlan(oldVlan, vlan);
    } else {
      VLOG (1) <<" Vlan : "<<vlan->getID()<<" already exists ";
    }
    warmBootCache_->programmed(vlanItr);
  } else {
      VLOG(1) << "creating VLAN " << vlan->getID() << " with " <<
      vlan->getPorts().size() << " ports";
      auto rv = opennsl_vlan_create(unit_, vlan->getID());
      bcmCheckError(rv, "failed to add VLAN ", vlan->getID());
      rv = opennsl_vlan_port_add(unit_, vlan->getID(), pbmp, ubmp);
      bcmCheckError(rv, "failed to add members to new VLAN ", vlan->getID());
  }
}

void BcmSwitch::preprocessRemovedVlan(const shared_ptr<Vlan>& vlan) {
  // Remove all ports from this VLAN at this phase.
  VLOG(2) << "preparing to remove VLAN " << vlan->getID();
  auto rv = opennsl_vlan_gport_delete_all(unit_, vlan->getID());
  bcmCheckError(rv, "failed to remove members from VLAN ", vlan->getID());
}

void BcmSwitch::processRemovedVlan(const shared_ptr<Vlan>& vlan) {
  VLOG(2) << "removing VLAN " << vlan->getID();

  auto rv = opennsl_vlan_destroy(unit_, vlan->getID());
  bcmCheckError(rv, "failed to remove VLAN ", vlan->getID());
}

void BcmSwitch::processChangedIntf(const shared_ptr<Interface>& oldIntf,
                                   const shared_ptr<Interface>& newIntf) {
  CHECK_EQ(oldIntf->getID(), newIntf->getID());
  VLOG(2) << "changing interface " << oldIntf->getID();
  intfTable_->programIntf(newIntf);
}

void BcmSwitch::processAddedIntf(const shared_ptr<Interface>& intf) {
  VLOG(2) << "adding interface " << intf->getID();
  intfTable_->addIntf(intf);
}

void BcmSwitch::processRemovedIntf(const shared_ptr<Interface>& intf) {
  VLOG(2) << "deleting interface " << intf->getID();
  intfTable_->deleteIntf(intf);
}

void BcmSwitch::processAclChanges(const StateDelta& delta) {
  forEachChanged(
    delta.getAclsDelta(),
    &BcmSwitch::processChangedAcl,
    &BcmSwitch::processAddedAcl,
    &BcmSwitch::processRemovedAcl,
    this);
}

template<typename DELTA>
void BcmSwitch::processNeighborEntryDelta(const DELTA& delta) {
  const auto* oldEntry = delta.getOld().get();
  const auto* newEntry = delta.getNew().get();

  const BcmIntf *intf;
  opennsl_vrf_t vrf;
  auto getIntfAndVrf = [&](InterfaceID id) {
    intf = getIntfTable()->getBcmIntf(id);
    vrf = getBcmVrfId(intf->getInterface()->getRouterID());
  };

  if (!oldEntry) {
    getIntfAndVrf(newEntry->getIntfID());
    auto host = hostTable_->incRefOrCreateBcmHost(
      vrf, IPAddress(newEntry->getIP()));

    if (newEntry->isPending()) {
      VLOG(3) << "adding pending neighbor entry to " << newEntry->getIP().str();
      host->programToDrop(intf->getBcmIfId());
    } else {
      VLOG(3) << "adding neighbor entry " << newEntry->getIP().str()
              << " to " << newEntry->getMac().toString();
      host->program(intf->getBcmIfId(), newEntry->getMac(),
                    getPortTable()->getBcmPortId(newEntry->getPort()));
    }
  } else if (!newEntry) {
    VLOG(3) << "deleting neighbor entry " << oldEntry->getIP().str();
    getIntfAndVrf(oldEntry->getIntfID());
    auto host = hostTable_->derefBcmHost(vrf, IPAddress(oldEntry->getIP()));
    if (host) {
      host->programToCPU(intf->getBcmIfId());
    }
  } else {
    CHECK_EQ(oldEntry->getIP(), newEntry->getIP());
    VLOG(3) << "changing neighbor entry " << oldEntry->getIP().str()
            << " to " << newEntry->getMac().toString();
    getIntfAndVrf(newEntry->getIntfID());
    auto host = hostTable_->getBcmHost(vrf, IPAddress(newEntry->getIP()));
    if (newEntry->isPending()) {
      host->programToDrop(intf->getBcmIfId());
    } else {
      host->program(intf->getBcmIfId(), newEntry->getMac(),
          getPortTable()->getBcmPortId(newEntry->getPort()));
    }
  }
}

void BcmSwitch::processArpChanges(const StateDelta& delta) {
  for (const auto& vlanDelta : delta.getVlansDelta()) {
    for (const auto& arpDelta : vlanDelta.getArpDelta()) {
      processNeighborEntryDelta(arpDelta);
    }
    for (const auto& ndpDelta : vlanDelta.getNdpDelta()) {
      processNeighborEntryDelta(ndpDelta);
    }
  }
}

template <typename RouteT>
void BcmSwitch::processChangedRoute(const RouterID id,
                                    const shared_ptr<RouteT>& oldRoute,
                                    const shared_ptr<RouteT>& newRoute) {
  VLOG(3) << "changing to route entry @ vrf " << id << " from old: "
    << oldRoute->str() << "to new: " << newRoute->str();
  // if the new route is not resolved, delete it instead of changing it
  if (!newRoute->isResolved()) {
    VLOG(1) << "Non-resolved route HW programming is skipped";
    processRemovedRoute(id, oldRoute);
  } else {
    routeTable_->addRoute(getBcmVrfId(id), newRoute.get());
  }
}

template <typename RouteT>
void BcmSwitch::processAddedRoute(const RouterID id,
                                  const shared_ptr<RouteT>& route) {
  VLOG(3) << "adding route entry @ vrf " << id << " " << route->str();
  // if the new route is not resolved, ignore it
  if (!route->isResolved()) {
    VLOG(1) << "Non-resolved route HW programming is skipped";
    return;
  }
  routeTable_->addRoute(getBcmVrfId(id), route.get());
}

template <typename RouteT>
void BcmSwitch::processRemovedRoute(const RouterID id,
                                    const shared_ptr<RouteT>& route) {
  VLOG(3) << "removing route entry @ vrf " << id << " " << route->str();
  if (!route->isResolved()) {
    VLOG(1) << "Non-resolved route HW programming is skipped";
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

void BcmSwitch::processAddedChangedRoutes(const StateDelta& delta) {
  for (auto const& rtDelta : delta.getRouteTablesDelta()) {
    if (!rtDelta.getNew()) {
      // no new route table, must not added or changed route, skip
      continue;
    }
    RouterID id = rtDelta.getNew()->getID();
    forEachChanged(
        rtDelta.getRoutesV4Delta(),
        &BcmSwitch::processChangedRoute<RouteV4>,
        &BcmSwitch::processAddedRoute<RouteV4>,
        [&](BcmSwitch *, RouterID, const shared_ptr<RouteV4>&) {},
        this,
        id);
    forEachChanged(
        rtDelta.getRoutesV6Delta(),
        &BcmSwitch::processChangedRoute<RouteV6>,
        &BcmSwitch::processAddedRoute<RouteV6>,
        [&](BcmSwitch *, RouterID, const shared_ptr<RouteV6>&) {},
        this,
        id);
  }
}

void BcmSwitch::linkscanCallback(int unit,
                                 opennsl_port_t bcmPort,
                                 opennsl_port_info_t* info) {
  try {
    BcmUnit* unitObj = BcmAPI::getUnit(unit);
    BcmSwitch* sw = static_cast<BcmSwitch*>(unitObj->getCookie());
    sw->linkStateChangedHwNotLocked(bcmPort, info);
  } catch (const std::exception& ex) {
    LOG(ERROR) << "unhandled exception while processing linkscan callback "
      << "for unit " << unit << " port " << bcmPort << ": "
      << folly::exceptionStr(ex);
  }
}

void BcmSwitch::linkStateChangedHwNotLocked(opennsl_port_t bcmPortId,
    opennsl_port_info_t* info) {
  portTable_->setPortStatus(bcmPortId, info->linkstatus);
  // TODO: We should eventually define a more robust hardware independent
  // LinkStatus enum, so we can expose more detailed information to to the
  // callback about why the link is down.
  bool up = info->linkstatus == OPENNSL_PORT_LINK_STATUS_UP;
  if (!up) {
    // For port up events we wait till ARP/NDP entries
    // are re resolved after port up before adding them
    // back. Adding them earlier leads to packet loss.
    hostTable_->linkDownHwNotLocked(bcmPortId);
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
  unique_ptr<BcmRxPacket> bcmPkt;
  try {
    bcmPkt = createRxPacket(pkt);
  } catch (const std::exception& ex) {
    LOG(ERROR) << "failed to allocated BcmRxPacket for receive handling: " <<
      folly::exceptionStr(ex);
    return OPENNSL_RX_NOT_HANDLED;
  }
  callback_->packetReceived(std::move(bcmPkt));
  return OPENNSL_RX_HANDLED_OWNED;
}

bool BcmSwitch::sendPacketSwitched(unique_ptr<TxPacket> pkt) noexcept {
  unique_ptr<BcmTxPacket> bcmPkt(
      boost::polymorphic_downcast<BcmTxPacket*>(pkt.release()));

  auto rv = BcmTxPacket::sendAsync(std::move(bcmPkt));
  return OPENNSL_SUCCESS(rv);
}

bool BcmSwitch::sendPacketOutOfPort(unique_ptr<TxPacket> pkt,
                                    PortID portID) noexcept {
  unique_ptr<BcmTxPacket> bcmPkt(
      boost::polymorphic_downcast<BcmTxPacket*>(pkt.release()));
  bcmPkt->setDestModPort(getPortTable()->getBcmPortId(portID));
  VLOG(4) << "sendPacketOutOfPort for" <<
    getPortTable()->getBcmPortId(portID);
  auto rv = BcmTxPacket::sendAsync(std::move(bcmPkt));
  return OPENNSL_SUCCESS(rv);
}

void BcmSwitch::updateStats(SwitchStats *switchStats) {
  // Update thread-local switch statistics.
  updateThreadLocalSwitchStats(switchStats);
  // Update thread-local per-port statistics.
  PortStatsMap* portStatsMap = switchStats->getPortStats();
  BOOST_FOREACH(auto& it, *portStatsMap) {
    PortID portID = it.first;
    PortStats *portStats = it.second.get();
    updateThreadLocalPortStats(portID, portStats);
  }
  // Update global statistics.
  updateGlobalStats();
}

void BcmSwitch::updateThreadLocalSwitchStats(SwitchStats *switchStats) {
  // TODO
}

void BcmSwitch::updateThreadLocalPortStats(PortID portID,
    PortStats *portStats) {
  // TODO
}

void BcmSwitch::updateGlobalStats() {
  portTable_->updatePortStats();
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

bool BcmSwitch::getAndClearNeighborHit(RouterID vrf,
                                       folly::IPAddress& ip) {
  std::lock_guard<std::mutex> g(lock_);
  auto host = hostTable_->getBcmHostIf(vrf, ip);
  if (!host) {
    return false;
  }
  return host->getAndClearHitBit();
}

void BcmSwitch::exitFatal() const {
  dumpState();
  callback_->exitFatal();
}

}} // facebook::fboss
