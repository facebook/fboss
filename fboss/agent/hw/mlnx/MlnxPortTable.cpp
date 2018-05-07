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
#include "fboss/agent/hw/mlnx/MlnxPortTable.h"
#include "fboss/agent/hw/mlnx/MlnxSwitch.h"
#include "fboss/agent/hw/mlnx/MlnxPlatform.h"
#include "fboss/agent/hw/mlnx/MlnxPlatformPort.h"
#include "fboss/agent/hw/mlnx/MlnxError.h"

#include "fboss/agent/FbossError.h"

extern "C" {
#include <sx/sdk/sx_api_vlan.h>
}

namespace facebook { namespace fboss {

MlnxPortTable::MlnxPortTable(MlnxSwitch* hw):
    hw_(hw) {}

MlnxPortTable::~MlnxPortTable() {}

void MlnxPortTable::initPorts(bool warmBoot) {
  Vlan::MemberPorts portSet {};
  auto mlnxPorts = hw_->getPlatform()->initPorts();
  for(const auto& entry: mlnxPorts) {
    auto log_port = entry.first;
    auto pltfPort = entry.second;
    auto portId = pltfPort->getPortID();

    // create hw port
    auto mlnxPort = std::make_unique<MlnxPort>(hw_, portId, log_port);

    // set platform port for hw port
    mlnxPort->setPlatformPort(pltfPort);

    // init (set port config for cold boot)
    mlnxPort->init(warmBoot);
    insert(mlnxPort);

    // on init all ports in vlan 1, untagged
    portSet.emplace(portId, Vlan::PortInfo(false /* untagged */));
  }

  // set all ports to vlan 1 as requiered for cold boot
  addPortsToVlan(portSet, VlanID(1));
}

void MlnxPortTable::setVlanPorts(
  const Vlan::MemberPorts& vlanPorts,
  VlanID vlan,
  bool add) {
  sx_status_t rc;

  // if vlanPorts is empty no processing needed
  if (vlanPorts.empty()) {
    return;
  }

  sx_vid_t vid = vlan;

  // set command depending if we want to add or delete ports to/from vlan
  sx_access_cmd_t cmd = (add ? SX_ACCESS_CMD_ADD : SX_ACCESS_CMD_DELETE);

  // get SDK handle
  auto handle = hw_->getHandle();

  // a list of sx_vlan_ports_t to pass to sdk
  std::vector<sx_vlan_ports_t> sxVlanPorts;

  // fill sxVlanPorts
  for (const auto& vlanPortEntry: vlanPorts) {
    sx_vlan_ports_t vlanPort;

    PortID id = vlanPortEntry.first;
    const VlanFields::PortInfo& portInfo = vlanPortEntry.second;

    vlanPort.log_port = getMlnxLogPort(id);
    vlanPort.is_untagged = portInfo.tagged ? SX_TAGGED_MEMBER :
      SX_UNTAGGED_MEMBER;

    // add to the vector of vlan ports
    sxVlanPorts.push_back(vlanPort);

    LOG(INFO) << (add ? "Adding" : "Removing")
              << " port " << id
              << (add ? " to" : " from")
              << " vlan " << vlan
              << ((portInfo.tagged == true) ?" tagged" : " untagged");
  }
  // perform SDK call
  rc = sx_api_vlan_ports_set(handle,
    cmd,
    hw_->getDefaultSwid(),
    vid,
    sxVlanPorts.data(),
    sxVlanPorts.size());
  mlnxCheckSdkFail(rc,
    "sx_api_vlan_ports_set",
    "Failed to ",
    SX_ACCESS_CMD_STR(cmd),
    " ports to vlan ",
    vlan);
}

void MlnxPortTable::addPortsToVlan(
  const Vlan::MemberPorts& vlanPorts,
  VlanID vlan) {
  setVlanPorts(vlanPorts, vlan, true);
}

void MlnxPortTable::removePortsFromVlan(
  const Vlan::MemberPorts& vlanPorts,
  VlanID vlan) {
  setVlanPorts(vlanPorts, vlan, false);
}

void MlnxPortTable::insert(std::unique_ptr<MlnxPort>& mlnxPort) {
  auto portId = mlnxPort->getPortId();
  logToMlnxPort_.emplace(mlnxPort->getLogPort(), mlnxPort.get());
  fbossToMlnxPort_.emplace(portId, std::move(mlnxPort));
}

sx_port_log_id_t MlnxPortTable::getMlnxLogPort(PortID id) const {
  auto iter = fbossToMlnxPort_.find(id);
  if (iter == fbossToMlnxPort_.end()) {
    throw FbossError("No such port - ", id);
  }
  return iter->second->getLogPort();
}

PortID MlnxPortTable::getPortID(sx_port_log_id_t log_port) const {
  auto iter = logToMlnxPort_.find(log_port);
  if (iter == logToMlnxPort_.end()) {
    throw FbossError("No such port with log_port ", log_port);
  }
  return iter->second->getPortId();
}

MlnxPort* MlnxPortTable::getMlnxPort(PortID id) const {
  auto iter = fbossToMlnxPort_.find(id);
  if (iter == fbossToMlnxPort_.end()) {
    throw FbossError("No such port with port id ", id);
  }
  return iter->second.get();
}

MlnxPort* MlnxPortTable::getMlnxPort(sx_port_log_id_t log_port) const {
  auto iter = logToMlnxPort_.find(log_port);
  if (iter == logToMlnxPort_.end()) {
    throw FbossError("No such port with log_port ", log_port);
  }
  return iter->second;
}

MlnxPortTable::FbossPortMapT::const_iterator MlnxPortTable::begin() const {
  return fbossToMlnxPort_.begin();
}

MlnxPortTable::FbossPortMapT::const_iterator MlnxPortTable::end() const {
  return fbossToMlnxPort_.end();
}

}}
