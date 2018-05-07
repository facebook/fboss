/* 
 * Copyright (c) Mellanox Technologies. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the names of the copyright holders nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * Alternatively, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") version 2 as published by the Free
 * Software Foundation.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#include "fboss/agent/hw/mlnx/MlnxSwitch.h"
#include "fboss/agent/hw/mlnx/MlnxPlatform.h"
#include "fboss/agent/hw/mlnx/MlnxPlatformPort.h"
#include "fboss/agent/hw/mlnx/MlnxInitHelper.h"
#include "fboss/agent/hw/mlnx/MlnxDevice.h"
#include "fboss/agent/hw/mlnx/MlnxPort.h"
#include "fboss/agent/hw/mlnx/MlnxHostIfc.h"
#include "fboss/agent/hw/mlnx/MlnxVrf.h"
#include "fboss/agent/hw/mlnx/MlnxIntf.h"
#include "fboss/agent/hw/mlnx/MlnxRoute.h"
#include "fboss/agent/hw/mlnx/MlnxNeigh.h"
#include "fboss/agent/hw/mlnx/MlnxRxPacket.h"
#include "fboss/agent/hw/mlnx/MlnxTxPacket.h"
#include "fboss/agent/hw/mlnx/MlnxPortTable.h"
#include "fboss/agent/hw/mlnx/MlnxIntfTable.h"
#include "fboss/agent/hw/mlnx/MlnxRouteTable.h"
#include "fboss/agent/hw/mlnx/MlnxNeighTable.h"
#include "fboss/agent/hw/mlnx/MlnxUtils.h"
#include "fboss/agent/hw/mlnx/MlnxError.h"

#include "fboss/agent/state/StateDelta.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/state/SwitchState-defs.h"
#include "fboss/agent/SwitchStats.h"
#include "fboss/agent/SwSwitch.h"

// FIXME:
// ETHERTYPE_ARP and ETHERTYPE_IPV6 defined in some of standard linux files
// which is included with sx_* headers.
// This leads to compilation errors because ArpHandler and IPv6Handler
// are trying to redefine them
#undef ETHERTYPE_ARP
#undef ETHERTYPE_IPV6
#include "fboss/agent/NeighborUpdater.h"
#define ETHERTYPE_ARP 0x0806
#define ETHERTYPE_IPV6 0x86dd

#include "fboss/agent/state/Port.h"
#include "fboss/agent/state/DeltaFunctions.h"

#include <vector>
#include <iterator>
#include <algorithm>

extern "C" {
#include <sx/sdk/sx_api_dbg.h>
#include <sx/sdk/sx_api_fdb.h>
#include <sx/sdk/sx_api_vlan.h>
#include <sx/sdk/sx_api_mstp.h>
}

using std::make_unique;
using std::make_shared;
using std::make_pair;
using std::shared_ptr;
using std::unique_ptr;
using std::string;

using namespace std::chrono;

DEFINE_string(sdk_dump_location,
  "/tmp/sdkdump",
  "Path for sdk dump on fatal exit");

namespace facebook { namespace fboss {

const auto kSdkDumpFile = FLAGS_sdk_dump_location.c_str();

MlnxSwitch::MlnxSwitch(MlnxPlatform* platform) :
  platform_(platform),
  portTable_(make_unique<MlnxPortTable>(this)),
  intfTable_(make_unique<MlnxIntfTable>(this)),
  routeTable_(make_unique<MlnxRouteTable>(this)),
  neighTable_(make_unique<MlnxNeighTable>(this)) {
}

MlnxSwitch::~MlnxSwitch() {
}

std::shared_ptr<SwitchState> MlnxSwitch::getColdBootState() {
  auto bootState = make_shared<SwitchState>();

  // On cold boot all ports are in Vlan 1
  auto vlanMap = make_shared<VlanMap>();
  auto vlan = make_shared<Vlan>(VlanID(1), "initVlan");
  Vlan::MemberPorts memberPorts;

  for(const auto& kv: *portTable_) {
    PortID portId = kv.first;
    auto mlnxPort = kv.second.get();
    auto name = folly::to<string>("Port", portId);

    auto swPort = make_shared<Port>(portId, name);
    swPort->setSpeed(mlnxPort->getSpeed());
    bootState->addPort(swPort);

    memberPorts.insert(make_pair(portId, false));
  }
  vlan->setPorts(memberPorts);
  bootState->addVlan(vlan);
  return bootState;
}

HwInitResult MlnxSwitch::init(HwSwitch::Callback* callback) {
  HwInitResult ret;
  callback_ = callback;
  sx_status_t rc;

  std::lock_guard<std::mutex> g(lock_);

  // get the instance of MlnxInitHelper
  // perform all HW and SDK initialization
  MlnxInitHelper* initHelper = MlnxInitHelper::get();

  try {
    steady_clock::time_point begin = steady_clock::now();

    // Perform SDK initialization
    initHelper->init();

    ret.initializedTime =
      duration_cast<duration<float>>(steady_clock::now() - begin).count();

    // set needed variables from initHelper
    handle_ = initHelper->getHandle();
    swid_ = initHelper->getDefaultSwid();

    device_ = initHelper->getSingleDevice(this);
    device_->init();

    LOG(INFO) << "Hardware initialized";

    LOG(INFO) << "Performing cold boot";

    portTable_->initPorts(false);

    // Set the spanning tree state of all ports to forwarding.
    // TODO: Eventually the spanning tree state should be part of the Port
    // state, and this should be handled in applyConfig().
    //
    // Spanning tree group settings
    // TODO: This should eventually be done as part of applyConfig()
    for (const auto& portPair: *portTable_) {
      auto log_port = portPair.second->getLogPort();
      rc = sx_api_rstp_port_state_set(handle_,
        log_port,
        SX_MSTP_INST_PORT_STATE_FORWARDING);
      mlnxCheckSdkFail(rc,
        "sx_api_rstp_port_state_set",
        "Failed to set RSTP state to forwarding for port",
        portPair.first);
    }

    // init router module in SDK
    initHelper->initRouter();

    hostIfc_ = make_unique<MlnxHostIfc>(this);

    // Create default VRF (The VRF with RouterID 0)
    vrf_ = make_unique<MlnxVrf>(this, RouterID(0));

    ret.bootTime =
        duration_cast<duration<float>>(steady_clock::now() - begin).count();

    // no warm boot support yet
    ret.bootType = BootType::COLD_BOOT;
    ret.switchState = getColdBootState();

  } catch(const FbossError& exception) {
    LOG(ERROR) << exception.what();
    exitFatal();
    throw exception;
  }
  return ret;
}

std::shared_ptr<SwitchState> MlnxSwitch::stateChanged(
  const StateDelta& delta) {
  // hold a lock while updating HW state
  std::lock_guard<std::mutex> g(lock_);

  auto appliedState = delta.newState();

  // process which ports will be disabled
  // This ensures that we immediately stop traffic on these ports
  DeltaFunctions::forEachChanged(delta.getPortsDelta(),
    &MlnxSwitch::processDisabledPorts,
    this);

  // remove routes before other configuration
  processRemovedRoutes(delta);

  // remove neighbors
  processRemovedNeighbors(delta);

  // remove interfaces
  DeltaFunctions::forEachRemoved(delta.getIntfsDelta(),
    &MlnxSwitch::processRemovedIntf,
    this);

  // change default vlan.
  // This affects the pvid on those ports
  // that has no pvid set. Here it is useless
  // as we need to specify ingressVlan configuration
  // for each port in JSON.
  if (delta.oldState()->getDefaultVlan() !=
    delta.newState()->getDefaultVlan()) {
    changeDefaultVlan(delta.newState()->getDefaultVlan());
  }

  // process added/changed/removed vlans
  DeltaFunctions::forEachChanged(delta.getVlansDelta(),
    &MlnxSwitch::processChangedVlan,
    &MlnxSwitch::processAddedVlan,
    &MlnxSwitch::processRemovedVlan,
    this);

  // process changed ports
  DeltaFunctions::forEachChanged(delta.getPortsDelta(),
    &MlnxSwitch::processChangedPorts,
    this);

  // process enabled ports
  DeltaFunctions::forEachChanged(delta.getPortsDelta(),
    &MlnxSwitch::processEnabledPorts,
    this);

  // process added and changed interfaces
  DeltaFunctions::forEachChanged(delta.getIntfsDelta(),
    &MlnxSwitch::processChangedIntf,
    &MlnxSwitch::processAddedIntf,
    [&] (MlnxSwitch*,
      const std::shared_ptr<Interface>&) {},
    this);

  // added and changed neighbors before route updates
  processAddedChangedNeighbors(delta, &appliedState);

  // added and changed routes
  processAddedChangedRoutes(delta, &appliedState);

  appliedState->publish();
  return appliedState;
}

void MlnxSwitch::changeDefaultVlan(VlanID vlan) {
  sx_status_t rc;
  sx_vid_t vid = static_cast<sx_vid_t>(vlan);

  rc = sx_api_vlan_default_vid_set(handle_,
    swid_, vid);
  mlnxCheckSdkFail(rc, "sx_api_vlan_default_vid_set");
  LOG(INFO) << "Default VLAN changed to " << vlan;
}

void MlnxSwitch::processDisabledPorts(const shared_ptr<Port>& oldPort,
    const shared_ptr<Port>& newPort) {
  if (oldPort->isEnabled() && !newPort->isEnabled()) {
    auto mlnxPort = portTable_->getMlnxPort(newPort->getID());
    LOG(INFO) << "Disabling port " << oldPort->getID();
    mlnxPort->disable();
  }
}

void MlnxSwitch::processEnabledPorts(const shared_ptr<Port>& oldPort,
    const shared_ptr<Port>& newPort) {
  if (!oldPort->isEnabled() && newPort->isEnabled()) {
    auto mlnxPort = portTable_->getMlnxPort(newPort->getID());
    LOG(INFO) << "Enabling port " << oldPort->getID();
    mlnxPort->enable(newPort);
  }
}

void MlnxSwitch::processChangedPorts(const shared_ptr<Port>& oldPort,
    const shared_ptr<Port>& newPort) {
  auto mlnxPort = portTable_->getMlnxPort(newPort->getID());

  auto speedChanged = oldPort->getSpeed() != newPort->getSpeed();
  auto vlanChanged = oldPort->getIngressVlan() != newPort->getIngressVlan();
  auto fecChanged = oldPort->getFEC() != newPort->getFEC();
  auto pauseChanged = oldPort->getPause() != newPort->getPause();
  auto sflowChanged =
    (oldPort->getSflowIngressRate() != newPort->getSflowIngressRate()) ||
    (oldPort->getSflowEgressRate() != newPort->getSflowEgressRate());

  if (speedChanged) {
    mlnxPort->setSpeed(newPort->getSpeed());
  }

  if (vlanChanged) {
    mlnxPort->setIngressVlan(newPort->getIngressVlan());
  }

  if (fecChanged) {
    mlnxPort->setFECMode(newPort->getFEC());
  }

  if (pauseChanged) {
    mlnxPort->setPortPauseSetting(newPort->getPause());
  }

  if (sflowChanged) {
    LOG(WARNING) << "Sflow not suppoted yet";
    // TODO(stepanb): sflow implementation
  }
}

void MlnxSwitch::processChangedVlan(const shared_ptr<Vlan>& oldVlan,
  const shared_ptr<Vlan>& newVlan) {
  using MemberPortType = std::pair<PortID, VlanFields::PortInfo>;
  Vlan::MemberPorts addedPorts;
  Vlan::MemberPorts removedPorts;
  const auto& oldPorts = oldVlan->getPorts();
  const auto& newPorts = newVlan->getPorts();

  auto addedPortToVlanOrChangedTaggedFieldCompFunc = [] (
    const MemberPortType& portA,
    const MemberPortType& portB) -> bool {
    bool result = false;
    const auto& vlanPortInfoA = portA.second;
    const auto& vlanPortInfoB = portB.second;

    // compare port IDs
    if (portA.first != portB.first) {
      result = (portA.first < portB.first);
    } else {
      // compare also by tagged field
      // to handle changes of emitTag vlan port property
      result = (vlanPortInfoA.tagged != vlanPortInfoB.tagged);
    }

    return result;
  };

  auto removedPortFromVlanCompFunc = [] (
    const MemberPortType& portA,
    const MemberPortType& portB) -> bool {
    // compare port IDs
    return (portA.first < portB.first);
  };

  // get added ports
  std::set_difference(newPorts.begin(), newPorts.end(),
    oldPorts.begin(), oldPorts.end(),
    std::inserter(addedPorts, addedPorts.begin()),
    addedPortToVlanOrChangedTaggedFieldCompFunc
  );
  // get removed ports
  std::set_difference(oldPorts.begin(), oldPorts.end(),
    newPorts.begin(), newPorts.end(),
    std::inserter(removedPorts, removedPorts.begin()),
    removedPortFromVlanCompFunc
  );

  // process these changes
  portTable_->removePortsFromVlan(removedPorts, oldVlan->getID());
  portTable_->addPortsToVlan(addedPorts, newVlan->getID());
}

void MlnxSwitch::processAddedVlan(const shared_ptr<Vlan>& addedVlan) {
  LOG(INFO) << "processing added VLAN " << addedVlan->getID();
  const auto& memberPorts = addedVlan->getPorts();

  // no need to allocate new vlan in SDK
  // 4k have been already created during the init

  portTable_->addPortsToVlan(memberPorts, addedVlan->getID());
}

void MlnxSwitch::processRemovedVlan(const shared_ptr<Vlan>& removedVlan) {
  LOG(INFO) << "processing removed VLAN " << removedVlan->getID();
  const auto& memberPorts = removedVlan->getPorts();

  // no need to remove vlan from SDK
  // we have 4k vlans in vlan DB
  // only remove member ports from this vlan

  portTable_->removePortsFromVlan(memberPorts, removedVlan->getID());
}

void MlnxSwitch::processAddedIntf(const std::shared_ptr<Interface>& intf) {
  intfTable_->addIntf(intf);
}

void MlnxSwitch::processChangedIntf(const std::shared_ptr<Interface>& oldIntf,
  const std::shared_ptr<Interface>& newIntf) {
  intfTable_->changeIntf(oldIntf, newIntf);

}

void MlnxSwitch::processRemovedIntf(const std::shared_ptr<Interface>& intf) {
  intfTable_->deleteIntf(intf);
}

template<typename RouteT>
void MlnxSwitch::processAddedRoute(RouterID vrf,
  std::shared_ptr<SwitchState>* appliedState,
  const std::shared_ptr<RouteT>& swRoute) {
  if (!swRoute->isResolved()) {
    LOG(INFO) << "Non-resolved route HW programming skipped";
    return;
  }
  try {
    routeTable_->addRoute(vrf, swRoute);
  } catch(const MlnxError& error) {
    rethrowIfHwNotFull(error);
    LOG(ERROR) << error.what();
    using AddrT = typename RouteT::Addr;
    // revert new route entry in SwSwitch
    SwitchState::revertNewRouteEntry<AddrT>(vrf,
      swRoute,
      nullptr,
      appliedState);
  }
}

template<typename RouteT>
void MlnxSwitch::processRemovedRoute(RouterID vrf,
  const std::shared_ptr<RouteT>& swRoute) {
  if (!swRoute->isResolved()) {
    LOG(INFO) << "Non-resolved route wasn't added to HW";
    return;
  }
  routeTable_->deleteRoute(vrf, swRoute);
}

template<typename RouteT>
void MlnxSwitch::processChangedRoute(RouterID vrf,
  std::shared_ptr<SwitchState>* appliedState,
  const std::shared_ptr<RouteT>& oldRoute,
  const std::shared_ptr<RouteT>& newRoute) {
  try {
    if (!oldRoute->isResolved() && newRoute->isResolved()) {
      LOG(INFO) << "Route became resolved, adding to HW";
      // now the route is resolved, we can add it to the table
      routeTable_->addRoute(vrf, newRoute);
    } else if (oldRoute->isResolved() && !newRoute->isResolved()) {
      // the route was resolved and now it is not, we should delete
      // it from the table
      LOG(INFO) << "Route became unresolved, deleting from HW";
      routeTable_->deleteRoute(vrf, newRoute);
    } else if (oldRoute->isResolved() && newRoute->isResolved()) {
      LOG(INFO) << "Route has changed, programming change to HW";
      routeTable_->changeRoute(vrf, oldRoute, newRoute);
    } else {
      LOG(INFO) << "Non-resolved route HW programming skipped";
    }
  } catch(const MlnxError& error) {
    rethrowIfHwNotFull(error);
    LOG(ERROR) << error.what();
    // revert new route entry in SwSwitch
    SwitchState::revertNewRouteEntry(vrf,
      newRoute,
      oldRoute,
      appliedState);
  }
}

void MlnxSwitch::processRemovedRoutes(const StateDelta& delta) {
  auto rtDeltas = delta.getRouteTablesDelta();
  for (const auto& rtDelta: rtDeltas) {
    if (!rtDelta.getOld()) {
      // no old table, no removed routes
      continue;
    }
    RouterID id = rtDelta.getOld()->getID();
    DeltaFunctions::forEachRemoved(rtDelta.getRoutesV4Delta(),
      &MlnxSwitch::processRemovedRoute<RouteV4>,
      this,
      id);
    DeltaFunctions::forEachRemoved(rtDelta.getRoutesV6Delta(),
      &MlnxSwitch::processRemovedRoute<RouteV6>,
      this,
      id);
  }
}

void MlnxSwitch::processAddedChangedRoutes(const StateDelta& delta,
  std::shared_ptr<SwitchState>* appliedState) {
  auto rtDeltas = delta.getRouteTablesDelta();

  for (const auto& rtDelta: rtDeltas) {
    if (!rtDelta.getNew()) {
      // no new route table, must not have added or changed route, skip
      continue;
    }
    RouterID id = rtDelta.getNew()->getID();
    DeltaFunctions::forEachChanged(rtDelta.getRoutesV4Delta(),
      &MlnxSwitch::processChangedRoute<RouteV4>,
      &MlnxSwitch::processAddedRoute<RouteV4>,
      [&](MlnxSwitch*,
          const RouterID&,
          std::shared_ptr<SwitchState>*,
          const std::shared_ptr<RouteV4>&) {},
      this,
      id,
      appliedState);
    DeltaFunctions::forEachChanged(rtDelta.getRoutesV6Delta(),
      &MlnxSwitch::processChangedRoute<RouteV6>,
      &MlnxSwitch::processAddedRoute<RouteV6>,
      [&](MlnxSwitch*,
          const RouterID&,
          std::shared_ptr<SwitchState>*,
          const std::shared_ptr<RouteV6>&) {},
      this,
      id,
      appliedState);
  }
}

template<typename NeighEntryT, typename NeighTableT>
void MlnxSwitch::processAddedNeighbor(
  std::shared_ptr<SwitchState>* appliedState,
  const std::shared_ptr<NeighEntryT>& swNeigh) {
  const folly::IPAddress& ip = swNeigh->getIP();
  if (swNeigh->isPending() ||
    (ip.isV6() && ip.asV6().isLinkLocal())) {
    // 1. Do not program to HW unresolved neighbors
    //    It relies on SX_TRAP_ID_HOST_MISS_IPV4/6
    // 2. Do not program to HW ipv6 link local addresses
    //    NOTE: v4 is programmed (see fboss/agent/SwSwitch.cpp#L1441)
    LOG(INFO) << "Pending or link-local neighbor entry "
                 "programming to HW skipped";
    return;
  }
  try {
    // swNeighbor is resolved, add to the table
    neighTable_->addNeigh(swNeigh);
  } catch (const MlnxError& error) {
    rethrowIfHwNotFull(error);
    LOG(ERROR) << "Error adding neighbor entry "
               << error.what();
    SwitchState::revertNewNeighborEntry<NeighEntryT, NeighTableT>(swNeigh,
      nullptr,
      appliedState);
  }
}

template<typename NeighEntryT>
void MlnxSwitch::processRemovedNeighbor(
  const std::shared_ptr<NeighEntryT>& swNeigh) {
  const folly::IPAddress& ip = swNeigh->getIP();
  if (swNeigh->isPending() ||
    (ip.isV6() && ip.asV6().isLinkLocal())) {
    // neighbor was not programmed to hardware, skip deleting
    return;
  }
  neighTable_->deleteNeigh(swNeigh);
}

template<typename NeighEntryT, typename NeighTableT>
void MlnxSwitch::processChangedNeighbor(
  std::shared_ptr<SwitchState>* appliedState,
  const std::shared_ptr<NeighEntryT>& oldNeigh,
  const std::shared_ptr<NeighEntryT>& newNeigh) {
  const folly::IPAddress& oldIp = oldNeigh->getIP();
  // no sdk api to change existing neighbor entry
  // just remove old and add new entry

  // if old neighbor entry is not pending and address is not link-local
  // then this entry was programmed to HW, need to delete
  if (!oldNeigh->isPending() &&
    !(oldIp.isV6() && oldIp.asV6().isLinkLocal())) {
    // neighbor was not programmed to hardware, skip deleting
    neighTable_->deleteNeigh(oldNeigh);
  }

  const folly::IPAddress& newIp = oldNeigh->getIP();
  try {
    // if new is not pending and address is not link-local
    // program to HW
    if (newNeigh->isPending() ||
      (newIp.isV6() && newIp.asV6().isLinkLocal())) {
      LOG(INFO) << "Pending or link-local neighbor entry "
                   "programming to HW skipped";
      return;
    }
    neighTable_->addNeigh(newNeigh);
  } catch (const MlnxError& error) {
    rethrowIfHwNotFull(error);
    LOG(ERROR) << "Error changing neighbor entry "
               << error.what();
    SwitchState::revertNewNeighborEntry<NeighEntryT, NeighTableT>(newNeigh,
      oldNeigh,
      appliedState);
  }
}

void MlnxSwitch::processRemovedNeighbors(const StateDelta& delta) {
  const auto& vlanDeltas = delta.getVlansDelta();
  for (const auto& vlanDelta: vlanDeltas) {
    const auto arpDelta = vlanDelta.getArpDelta();
    const auto ndpDelta = vlanDelta.getNdpDelta();
    DeltaFunctions::forEachRemoved(arpDelta,
      &MlnxSwitch::processRemovedNeighbor<ArpEntry>,
      this);
    DeltaFunctions::forEachRemoved(ndpDelta,
      &MlnxSwitch::processRemovedNeighbor<NdpEntry>,
      this);
  }
}

void MlnxSwitch::processAddedChangedNeighbors(const StateDelta& delta,
  std::shared_ptr<SwitchState>* appliedState) {
  const auto& vlanDeltas = delta.getVlansDelta();
  // arp/ndp tables are stored per vlan
  for (const auto& vlanDelta: vlanDeltas) {
    const auto arpDelta = vlanDelta.getArpDelta();
    const auto ndpDelta = vlanDelta.getNdpDelta();

    // process arp delta
    DeltaFunctions::forEachChanged(arpDelta,
      &MlnxSwitch::processChangedNeighbor<ArpEntry, ArpTable>,
      &MlnxSwitch::processAddedNeighbor<ArpEntry, ArpTable>,
      [&] (MlnxSwitch*,
        std::shared_ptr<SwitchState>*,
        const std::shared_ptr<ArpEntry>&) {}, // stubbed-out remove callback
      this,
      appliedState);

    // process ndp delta
    DeltaFunctions::forEachChanged(ndpDelta,
      &MlnxSwitch::processChangedNeighbor<NdpEntry, NdpTable>,
      &MlnxSwitch::processAddedNeighbor<NdpEntry, NdpTable>,
      [&] (MlnxSwitch*,
        std::shared_ptr<SwitchState>*,
        const std::shared_ptr<NdpEntry>&) {}, // stubbed-out remove callback
      this,
      appliedState);
  }
}

std::unique_ptr<TxPacket> MlnxSwitch::allocatePacket(uint32_t size) {
  return make_unique<MlnxTxPacket>(size);
}

bool MlnxSwitch::sendPacketSwitched(
  std::unique_ptr<TxPacket> pkt) noexcept {
  return hostIfc_->sendPacketSwitched(std::move(pkt));
}

bool MlnxSwitch::sendPacketOutOfPort(
  std::unique_ptr<TxPacket> pkt,
  PortID portId) noexcept {
  return hostIfc_->sendPacketOutOfPort(std::move(pkt), portId);
}

folly::dynamic MlnxSwitch::toFollyDynamic() const {
  // TODO(stepanb): implement serialization methods for Mlnx* objects
  return folly::dynamic::object;
}

bool MlnxSwitch::isPortUp(PortID portID) const {
  auto mlnxPort = portTable_->getMlnxPort(portID);
  return mlnxPort->isUp();
}

bool MlnxSwitch::getPortFECConfig(PortID portId) const {
  auto mlnxPort = portTable_->getMlnxPort(portId);
  return mlnxPort->getOperFECMode() == cfg::PortFEC::ON;
}

void MlnxSwitch::initialConfigApplied() {
  try {
    std::lock_guard<std::mutex> g(lock_);
    // callback lambda
    auto rxCallback = [this] (const RxDataStruct& rxData) {
      onReceivedHostIfcEvent(rxData);
    };
    // set callback for rx
    hostIfc_->setRxPacketCallback(rxCallback);
    // register handler and start rx thread
    hostIfc_->start();
  } catch (const FbossError& error) {
    LOG(ERROR) << error.what();
    unregisterCallbacks();
    exitFatal();
    throw error;
  }
}

sx_api_handle_t MlnxSwitch::getHandle() const {
  return handle_;
}

sx_chip_types_t MlnxSwitch::getChipType() const {
  return MlnxInitHelper::get()->getChipType();
}

const rm_resources_t& MlnxSwitch::getRmLimits() const {
  return MlnxInitHelper::get()->getRmLimits();
}

sx_swid_t MlnxSwitch::getDefaultSwid() const {
  return swid_;
}

const MlnxPortTable* MlnxSwitch::getMlnxPortTable() const {
  return portTable_.get();
}

const MlnxIntfTable* MlnxSwitch::getMlnxIntfTable() const {
  return intfTable_.get();
}

const MlnxVrf* MlnxSwitch::getDefaultMlnxVrf() const {
  return vrf_.get();
}

std::vector<sx_port_attributes_t> MlnxSwitch::getMlnxPortsAttr() {
  return device_->getPortAttrs();
}

const SwSwitch* MlnxSwitch::getSwSwitch() const {
  return boost::polymorphic_downcast<SwSwitch*>(callback_);
}

void MlnxSwitch::fetchL2Table(
  std::vector<L2EntryThrift>* l2Table)  {
  sx_status_t rc;

  sx_fdb_uc_mac_addr_params_t mac_entry_arr[SX_FDB_MAX_GET_ENTRIES];
  sx_fdb_uc_mac_addr_params_t key_p;
  sx_fdb_uc_key_filter_t key_filter_p;
  uint32_t data_cnt = SX_FDB_MAX_GET_ENTRIES;

  // This function can return only SX_FDB_MAX_GET_ENTRIES
  // records at once
  rc = sx_api_fdb_uc_mac_addr_get(handle_,
    swid_,
    SX_ACCESS_CMD_GET_FIRST,
    SX_FDB_UC_ALL,
    &key_p,
    &key_filter_p,
    mac_entry_arr,
    &data_cnt);
  mlnxCheckSdkFail(rc,
    "sx_api_fdb_uc_mac_addr_get",
    "Cannot retrieve fdb entries");
  while (data_cnt > 0) {
    for (int i = 0; i < data_cnt; ++i) {
      auto mac = utils::sdkMacAddressToFolly(
        mac_entry_arr[i].mac_addr).toString();
      auto vid = mac_entry_arr[i].fid_vid;
      auto log_port = mac_entry_arr[i].log_port;
      L2EntryThrift fdbEntry;
      fdbEntry.mac = mac;
      // In case of created interfaces on switch
      // there are static fdb entries with mac set
      // to interfaces mac and port set to cpu port
      // ignore those entries for now
      if (log_port) {
        fdbEntry.port = portTable_->getPortID(log_port);
      } else {
        fdbEntry.port = PortID(0);
      }
      fdbEntry.vlanID = VlanID(vid);
      l2Table->push_back(fdbEntry);
    }
    key_p.fid_vid = mac_entry_arr[data_cnt-1].fid_vid;
    key_p.mac_addr = mac_entry_arr[data_cnt-1].mac_addr;
    rc = sx_api_fdb_uc_mac_addr_get(handle_,
      swid_,
      SX_ACCESS_CMD_GETNEXT,
      SX_FDB_UC_ALL,
      &key_p,
      &key_filter_p,
      mac_entry_arr,
      &data_cnt);
    mlnxCheckSdkFail(rc,
      "sx_api_fdb_uc_mac_addr_get",
      "Cannot retrieve fdb entries");
  }
}

void MlnxSwitch::gracefulExit(folly::dynamic& /* switchState */) {
  const auto& initHelper = MlnxInitHelper::get();
  LOG(INFO) << "Graceful exiting";
  std::lock_guard<std::mutex> g(lock_);

  // disable all ports on graceful exit
  for (auto& port: *portTable_) {
    port.second->disable();
  }

  // Delete host ifc object
  hostIfc_.reset();

  // delete all routes
  routeTable_.reset();

  // delete all neighbors
  neighTable_.reset();

  // delete L3 interfaces
  intfTable_.reset();

  // Delete default VRF
  vrf_.reset();

  initHelper->deinitRouter();

  // deinitialize device
  // unmap all ports
  device_->deinit();
  LOG(INFO) << " [ Exit ] Devices deinitialized";

  initHelper->deinit();
  LOG(INFO) << " [ Exit ] sdk deinitialized";
}

void MlnxSwitch::exitFatal() const {
  sx_status_t rc;
  const auto& initHelper = MlnxInitHelper::get();

  LOG(ERROR) << "Fatal. Exiting...";

  rc = sx_api_dbg_generate_dump(handle_, kSdkDumpFile);
  mlnxLogSxError(rc,
    "sx_api_dbg_generate_dump",
    "Failed to generate dump file from sdk");
  LOG_IF(INFO, rc == SX_STATUS_SUCCESS) << "Sdk dump generated at "
                                        << kSdkDumpFile;

  initHelper->deinit();
  callback_->exitFatal();
}

void MlnxSwitch::unregisterCallbacks() {
  // unregister event handlers and stop rx threads
  hostIfc_->stop();
}

void MlnxSwitch::onReceivedHostIfcEvent(
  const RxDataStruct& rxData) const noexcept {
  auto trapId = rxData.recv_info.trap_id;
  // port up, down events
  if (trapId == SX_TRAP_ID_PUDE) {
    onReceivedPUDEvent(rxData);
  } else if (trapId == SX_TRAP_ID_NEED_TO_RESOLVE_ARP) {
    onReceivedNeedToResolveArp(rxData);
  } else {
    std::unique_ptr<MlnxRxPacket> rxPacket;
    try {
      rxPacket = make_unique<MlnxRxPacket>(this, rxData);
    } catch (const FbossError& error) {
      LOG(ERROR) << "Failed to create rxPacket, reason: "
                 << error.what();
      return;
    }
    // send to SwSwitch
    callback_->packetReceived(std::move(rxPacket));
  }
}

void MlnxSwitch::onReceivedPUDEvent(
  const RxDataStruct& rxData) const noexcept {
  // retrieve logical port and operative state from receive info structure
  auto logPort = rxData.recv_info.event_info.pude.log_port;
  auto operState = rxData.recv_info.event_info.pude.oper_state;
  // get FBOSS portID
  PortID portId;
  try {
    portId = portTable_->getPortID(logPort);
  } catch (const FbossError& error) {
    LOG(ERROR) << "Port up/down event received for non existing port";
    // don't want to rethrow exception from rx thread, return intead
    return;
  }
  bool up = (operState == SX_PORT_OPER_STATUS_UP ? true : false);
  // notify SwSwitch
  callback_->linkStateChanged(portId, up);
}

void MlnxSwitch::onReceivedNeedToResolveArp(
  const RxDataStruct& rxData) const noexcept {
  // get pointer to SwSwitch
  SwSwitch* sw = boost::polymorphic_downcast<SwSwitch*>(callback_);

  // convert neighbor address from sdk to folly
  folly::IPAddress neighborAddress;
  try {
    neighborAddress = utils::sdkIpAddressToFolly(
      rxData.recv_info.event_info.need_to_resolve_arp.ip_addr);
  } catch (const FbossError& error) {
    LOG(ERROR) << "Failed to trigger next hop resolution: "
               << error.what();
    return;
  }

  // get neighbor updater from SwSwitch
  auto neighborUpdater = sw->getNeighborUpdater();

  // need to get interface ID and Vlan to send Arp/Ndp
  auto rif = rxData.recv_info.event_info.need_to_resolve_arp.rif;

  InterfaceID intfId;

  try {
    intfId = intfTable_->getInterfaceIdByRif(rif);
  } catch(const FbossError& error) {
    LOG(ERROR) << "Cannot find MlnxIntf object with @rif "
               << static_cast<int>(rif)
               << " while received need to resolve arp event for neighbor: "
               << neighborAddress.str();
    return;
  }

  // need to find Sw Vlan object where found interface exists
  const auto& switchState = sw->getAppliedState();
  const auto& swIntf = switchState->getInterfaces()->getInterface(intfId);
  VlanID vlan = swIntf->getVlanID();

  LOG(INFO) << "Triggering SwSwitch to resolve neighbor: "
            << neighborAddress.str()
            << "; interface id: "
            << intfId;

  // Next two methods don't send arp request, but set pending entry
  // in arp/ndp table if neighbor was not resolved, and doesn't do
  // anything when we already have neighbor entry in the table
  if (neighborAddress.isV4()) {
    neighborUpdater->sentArpRequest(vlan, neighborAddress.asV4());
  } else {
    neighborUpdater->sentNeighborSolicitation(vlan, neighborAddress.asV6());
  }
}

void MlnxSwitch::updateStats(SwitchStats* /* switchStats */) {
  // TODO(stepanb): implement update stats/counters for Mlnx* objects
}

void MlnxSwitch::rethrowIfHwNotFull(const MlnxError& error) {
  if (static_cast<sx_status_t>(error.getRc()) != SX_STATUS_NO_RESOURCES) {
    throw error;
  }
}

}} // facebook::fboss
