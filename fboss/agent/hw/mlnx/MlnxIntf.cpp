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
#include "fboss/agent/hw/mlnx/MlnxIntf.h"
#include "fboss/agent/hw/mlnx/MlnxIntfTable.h"
#include "fboss/agent/hw/mlnx/MlnxSwitch.h"
#include "fboss/agent/hw/mlnx/MlnxVrf.h"
#include "fboss/agent/hw/mlnx/MlnxError.h"
#include "fboss/agent/hw/mlnx/MlnxUtils.h"

#include <folly/Format.h>

extern "C" {
#include <sx/sdk/sx_api_router.h>
}

namespace facebook { namespace fboss {

MlnxIntf::MlnxIntf(MlnxSwitch* hw,
  MlnxIntfTable* intfTable,
  const Interface& swIntf) :
  MlnxIntf {} {
  hw_ = hw;
  handle_ = hw_->getHandle();
  intfTable_ = intfTable;
  id_ = swIntf.getID();
  vrid_ = hw_->getDefaultMlnxVrf()->getSdkVrid();

  // Add interface to HW
  program(swIntf, SX_ACCESS_CMD_ADD);
  // create counters for debugging
  createAndBindCounter();

  // for each assigned interface ip
  // create an IP2ME route
  for (const auto& addr: swIntf.getAddresses()) {
    const auto& intfIp = addr.first;
    programIP2ME(intfIp, SX_ACCESS_CMD_ADD);
    interfaceIpSet_.insert(addr);
    intfTable_->writableIpToInterfaceMap()->emplace(intfIp, this);
  }

  // bring the interface up
  setAdminState(true);

  LOG(INFO) << "Interface created: " << toString();
}

void MlnxIntf::change(const Interface& newIntf) {
  Interface::Addresses addedIps;
  Interface::Addresses removedIps;
  const auto& oldIps = interfaceIpSet_; // old set of assigned IPs
  const auto& newIps = newIntf.getAddresses(); // new set of assigned IPs

  auto oldLogString = toString();

  // take the interface down before changing configuration
  setAdminState(false);

  // Edit interface configuration
  program(newIntf, SX_ACCESS_CMD_EDIT);
  
  // Interface::Addresses is a boost::flat_map which is sorted
  // So we can use std::set_difference to calculate difference
  // and reprogram IP2ME routes

  // get removed IPs
  std::set_difference(oldIps.begin(), oldIps.end(),
    newIps.begin(), newIps.end(),
    std::inserter(removedIps, removedIps.begin()));

  // get added IPs
  std::set_difference(newIps.begin(), newIps.end(),
    oldIps.begin(), oldIps.end(),
    std::inserter(addedIps, addedIps.begin()));

  // process differences

  for(const auto& removedIp: removedIps) {
    const auto& intfIp = removedIp.first;
    programIP2ME(intfIp, SX_ACCESS_CMD_DELETE);
    interfaceIpSet_.erase(intfIp);
    intfTable_->writableIpToInterfaceMap()->erase(intfIp);
  }

  for(const auto& addedIp: addedIps) {
    const auto& intfIp = addedIp.first;
    programIP2ME(intfIp, SX_ACCESS_CMD_ADD);
    interfaceIpSet_.insert(addedIp);
    intfTable_->writableIpToInterfaceMap()->emplace(intfIp, this);
  }

  // bring interface up
  setAdminState(true);

  LOG(INFO) << "Changed interface configuration from "
            << oldLogString
            << "to "
            << toString();

}

void MlnxIntf::createAndBindCounter() {
  sx_status_t rc;

  rc = sx_api_router_counter_set(handle_,
    SX_ACCESS_CMD_CREATE,
    &counter_);
  mlnxCheckSdkFail(rc,
    "sx_api_router_counter_set",
    "Failed to create router counter");
  rc = sx_api_router_interface_counter_bind_set(handle_,
    SX_ACCESS_CMD_BIND,
    counter_,
    rif_);
  mlnxCheckSdkFail(rc,
    "sx_api_router_counter_set",
    "Failed to bind router counter");
  LOG(INFO) << "Counters for interface @id "
            << id_
            << " created; "
            << "SDK counter @id "
            << (int)counter_;
}

void MlnxIntf::unbindAndDestroyCounter() {
  sx_status_t rc;

  rc = sx_api_router_interface_counter_bind_set(handle_,
    SX_ACCESS_CMD_UNBIND,
    counter_,
    rif_);
  mlnxCheckSdkFail(rc,
    "sx_api_router_counter_set",
    "Failed to unbind router counter");
  rc = sx_api_router_counter_set(handle_,
    SX_ACCESS_CMD_DESTROY,
    &counter_);
  mlnxCheckSdkFail(rc,
    "sx_api_router_counter_set",
    "Failed to destroy router counter");
  LOG(INFO) << "Counters for interface @id "
            << id_
            << " destroyed "
            << "SDK counter @id "
            << (int)counter_;
}

void MlnxIntf::program(const Interface& swIntf, sx_access_cmd_t cmd) {
  sx_status_t rc;

  // get interface parameters from SW Interface
  CHECK(swIntf.getRouterID() == 0) << "Only one VRF supported with id 0";
  sx_vlan_id_t vlan = static_cast<sx_vlan_id_t>(swIntf.getVlanID());
  sx_mac_addr_t macAddr;
  const auto& mac = swIntf.getMac();
  uint32_t mtu = swIntf.getMtu();

  // convert MAC string to sx_mac_addr_t structure
  utils::follyMacAddressToSdk(mac, &macAddr);

  if (cmd == SX_ACCESS_CMD_ADD) {
    // validate the MAC
    if (!hw_->isValidMacForInterface(mac)) {
      throw FbossError("Invalid interface MAC address ",
        swIntf.getMac(),
        " for Interface @id ",
        id_);
    }

    interfaceParams_.type = SX_L2_INTERFACE_TYPE_VLAN;
    interfaceParams_.ifc.vlan.vlan = vlan;
    interfaceParams_.ifc.vlan.swid = hw_->getDefaultSwid();
  } else if (cmd == SX_ACCESS_CMD_EDIT) {
    bool vlanChanged = vlan != interfaceParams_.ifc.vlan.vlan;
    bool macChanged = std::memcmp(&macAddr,
      &interfaceAttrs_.mac_addr,
      folly::MacAddress::SIZE);
    if (vlanChanged || macChanged) {
      throw FbossError("Modifying VLAN or MAC on L3 interface @id ",
        id_,
        " is not supported, interface: ",
        toString());
    }
  }

  interfaceAttrs_.mac_addr = macAddr;
  interfaceAttrs_.mtu = mtu;
  interfaceAttrs_.qos_mode = SX_ROUTER_QOS_MODE_NOP;
  interfaceAttrs_.multicast_ttl_threshold = 0;
  interfaceAttrs_.loopback_enable = true;

  rc = sx_api_router_interface_set(handle_,
    cmd,
    vrid_,
    &interfaceParams_,
    &interfaceAttrs_,
    &rif_);
  mlnxCheckSdkFail(rc,
    "sx_api_router_interface_set",
    "Failed to ",
    ((cmd == SX_ACCESS_CMD_ADD) ? "create" : "edit"),
    " interface @id",
    id_,
    ".  ");

  LOG(INFO) << "Interface @id "
            << id_
            << ((cmd == SX_ACCESS_CMD_ADD) ? " created " : " edited ");
}

void MlnxIntf::programIP2ME(const folly::IPAddress& ip,
  sx_access_cmd_t cmd) {
  sx_status_t rc;
  sx_uc_route_data_t routeData;

  // skip if address is link local
  // If several interfaces would share same MAC
  // they will have same IPv6 link local addresses
  // which will cause error if we will try to add
  // existing IP2ME route. However, all IPv6 link local
  // packets will be trapped by SDK trap.
  // No need to add here.
  if (ip.isLinkLocal()) {
    return;
  }

  memset(&routeData, 0, sizeof(routeData));
  routeData.type = SX_UC_ROUTE_TYPE_IP2ME;
  sx_ip_prefix_t networkAddr;

  utils::follyIPPrefixToSdk(ip, &networkAddr);

  rc = sx_api_router_uc_route_set(handle_,
    cmd,
    vrid_,
    &networkAddr,
    &routeData);
  mlnxCheckSdkFail(rc,
    "sx_api_router_uc_route_set",
    "Failed to ",
    (cmd == SX_ACCESS_CMD_ADD ? "Adding " : "Deleting "),
    " IP2ME route for interface @id ",
    id_);

  LOG(INFO) << (cmd == SX_ACCESS_CMD_ADD ? "Added " : "Deleted ")
            << "IP2ME route "
            << ip.str()
            << " @id "
            << id_;
}

void MlnxIntf::setAdminState(bool up) {
  sx_status_t rc;
  sx_router_interface_state_t rifState;
  rifState.ipv4_enable =
    up ? SX_ROUTER_ENABLE_STATE_ENABLE: SX_ROUTER_ENABLE_STATE_DISABLE;
  rifState.ipv6_enable =
    up ? SX_ROUTER_ENABLE_STATE_ENABLE: SX_ROUTER_ENABLE_STATE_DISABLE;
  rifState.ipv4_mc_enable = SX_ROUTER_ENABLE_STATE_DISABLE;
  rifState.ipv6_mc_enable = SX_ROUTER_ENABLE_STATE_DISABLE;
  rifState.mpls_enable = SX_ROUTER_ENABLE_STATE_DISABLE;
  rc = sx_api_router_interface_state_set(handle_,
    rif_,
    &rifState);
  mlnxCheckSdkFail(rc,
    "sx_api_router_interface_state_set",
    "Failed to bring interface @rif",
    rif_,
    (up ? " up" : " down"));
  LOG(INFO) << "Interface @id "
            << id_
            << " brought "
            << (up ? " up" : " down");
}

sx_router_interface_t MlnxIntf::getSdkRif() const {
  return rif_;
}

std::string MlnxIntf::toString() const {
  auto ret = folly::sformat("vlan interface: @rif {:d}, @vlan {:d}, "
     "@mac: {}, @mtu: {:d}, @counter_id {:d}; attached subnets {{",
     rif_, interfaceParams_.ifc.vlan.vlan,
     utils::sdkMacAddressToFolly(interfaceAttrs_.mac_addr).toString(),
     interfaceAttrs_.mtu, counter_);

  for (const auto& subnet: interfaceIpSet_) {
    const auto& subnetIp = subnet.first;
    auto cidrMask = subnet.second;
    folly::toAppend(folly::sformat("{} /{}", subnetIp.str(), cidrMask), &ret);
  }
  folly::toAppend("}}", &ret);
  return ret;
}

MlnxIntf::~MlnxIntf() {
  sx_status_t rc;
  auto logString = toString();
  // take the interface down
  try {
    setAdminState(false);
  } catch (const FbossError& error) {
    LOG(ERROR) << error.what();
  }
  // unbind and destroy interface counters
  try {
    unbindAndDestroyCounter();
  } catch (const FbossError& error) {
    LOG(ERROR) << error.what();
  }
  // delete interface from SDK
  rc = sx_api_router_interface_set(handle_,
    SX_ACCESS_CMD_DELETE,
    vrid_,
    nullptr,
    nullptr,
    &rif_);
  mlnxLogSxError(rc,
    "sx_api_router_interface_set",
    "Failed to destroy interface: ",
    logString);
  // go over set of interface ip addresses and delete IP2ME routes
  for (auto addrIterator = interfaceIpSet_.cbegin();
    addrIterator != interfaceIpSet_.cend(); ) {
    const auto intfIp = addrIterator->first;
    try {
      programIP2ME(intfIp, SX_ACCESS_CMD_DELETE);
      addrIterator = interfaceIpSet_.erase(addrIterator);
    } catch (const FbossError& error) {
      LOG(ERROR) << error.what();
    }
    // delete addresses from interface table
    intfTable_->writableIpToInterfaceMap()->erase(intfIp);
  }
  LOG_IF(INFO, rc == SX_STATUS_SUCCESS) << "Interface removed: "
                                        << logString;
}

}} // facebook::fboss
