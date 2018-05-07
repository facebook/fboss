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
#include "fboss/agent/hw/mlnx/MlnxRxPacket.h"
#include "fboss/agent/hw/mlnx/MlnxTxPacket.h"
#include "fboss/agent/hw/mlnx/MlnxPortTable.h"
#include "fboss/agent/hw/mlnx/MlnxUtils.h"
#include "fboss/agent/hw/mlnx/MlnxError.h"

#include "fboss/agent/state/StateDelta.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/state/SwitchState-defs.h"
#include "fboss/agent/SwitchStats.h"
#include "fboss/agent/SwSwitch.h"

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
  portTable_(make_unique<MlnxPortTable>(this)) {
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

    hostIfc_ = make_unique<MlnxHostIfc>(this);

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

void MlnxSwitch::updateStats(SwitchStats* /* switchStats */) {
  // TODO(stepanb): implement update stats/counters for Mlnx* objects 
}

}} // facebook::fboss
