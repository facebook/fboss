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
#include "fboss/agent/hw/mlnx/MlnxHostIfc.h"
#include "fboss/agent/hw/mlnx/MlnxPortTable.h"
#include "fboss/agent/hw/mlnx/MlnxTxPacket.h"
#include "fboss/agent/hw/mlnx/MlnxError.h"

#include "fboss/agent/packet/PktUtil.h"

extern "C" {
#include <sx/sdk/sx_api_host_ifc.h>
#include <sx/sdk/sx_lib_host_ifc.h>
}

namespace {

using TrapInfo = std::pair<sx_trap_id_t, sx_trap_action_t>;

const sx_trap_id_t kDefaultTrapGroup[] {
  SX_TRAP_ID_PUDE,
};

const TrapInfo kL2Traps[] = {
  // LLDP protocol
  {SX_TRAP_ID_ETH_L2_LLDP, SX_TRAP_ACTION_TRAP_2_CPU},
  // DHCP
  {SX_TRAP_ID_ETH_L2_DHCP, SX_TRAP_ACTION_TRAP_2_CPU},
};

const TrapInfo kL3ControlTraps[] = {
  // ARP protocol
  {SX_TRAP_ID_ARP_REQUEST, SX_TRAP_ACTION_MIRROR_2_CPU},
  {SX_TRAP_ID_ARP_RESPONSE, SX_TRAP_ACTION_MIRROR_2_CPU},
  // link local
  {SX_TRAP_ID_IPV6_LINK_LOCAL_DST, SX_TRAP_ACTION_TRAP_2_CPU},
  {SX_TRAP_ID_IPV6_ALL_NODES_LINK, SX_TRAP_ACTION_TRAP_2_CPU},
  {SX_TRAP_ID_IPV6_ALL_ROUTERS_LINK, SX_TRAP_ACTION_TRAP_2_CPU},
  // NDP protocol
  {SX_TRAP_ID_IPV6_NEIGHBOR_ADVERTISEMENT, SX_TRAP_ACTION_MIRROR_2_CPU},
  {SX_TRAP_ID_IPV6_NEIGHBOR_SOLICITATION, SX_TRAP_ACTION_MIRROR_2_CPU},
  {SX_TRAP_ID_IPV6_ROUTER_ADVERTISEMENT, SX_TRAP_ACTION_MIRROR_2_CPU},
  {SX_TRAP_ID_IPV6_ROUTER_SOLICITATION, SX_TRAP_ACTION_MIRROR_2_CPU},
  {SX_TRAP_ID_IPV6_REDIRECTION, SX_TRAP_ACTION_MIRROR_2_CPU},
  // DHCP
  {SX_TRAP_ID_IPV4_DHCP, SX_TRAP_ACTION_MIRROR_2_CPU},
  {SX_TRAP_ID_IPV6_DHCP, SX_TRAP_ACTION_MIRROR_2_CPU},
};

const TrapInfo kL3Traps[] = {
  // UC routes based traps
  {SX_TRAP_ID_L3_UC_IP_BASE, SX_TRAP_ACTION_TRAP_2_CPU},
  // Host miss traps. This will trigger ARP/NDP resolution in SW
  {SX_TRAP_ID_HOST_MISS_IPV4, SX_TRAP_ACTION_TRAP_2_CPU},
  {SX_TRAP_ID_HOST_MISS_IPV6, SX_TRAP_ACTION_TRAP_2_CPU},
  // IP2ME packet. Packets destinated to our interface IP will be trapped
  {SX_TRAP_ID_IP2ME, SX_TRAP_ACTION_TRAP_2_CPU},
  {SX_TRAP_ID_ROUTER_NEIGH_ACTIVITY, SX_TRAP_ACTION_MIRROR_2_CPU},
  // Arises whenever there is a need to resolve a next hop ip
  {SX_TRAP_ID_NEED_TO_RESOLVE_ARP, SX_TRAP_ACTION_MIRROR_2_CPU},
  // Errors
  {SX_TRAP_ID_ETH_L3_MTUERROR, SX_TRAP_ACTION_TRAP_2_CPU},
  {SX_TRAP_ID_ETH_L3_TTLERROR, SX_TRAP_ACTION_TRAP_2_CPU},
  {SX_TRAP_ID_ETH_L3_LBERROR, SX_TRAP_ACTION_TRAP_2_CPU},
};

const std::tuple<const TrapInfo*, size_t, sx_trap_priority_t> kTrapGroups[] = {
  {kL2Traps, ARRAY_SIZE(kL2Traps), SX_TRAP_PRIORITY_LOW},
  {kL3Traps, ARRAY_SIZE(kL3Traps), SX_TRAP_PRIORITY_LOW},
  {kL3ControlTraps, ARRAY_SIZE(kL3ControlTraps), SX_TRAP_PRIORITY_HIGH},
};

}

namespace facebook { namespace fboss {

MlnxUserChannel::MlnxUserChannel(MlnxSwitch* hw) :
  EventHandler(),
  hw_(hw),
  handle_(hw_->getHandle()),
  swid_(hw_->getDefaultSwid()) {
  sx_status_t rc;

  // resize a pkt data to max packet size
  pkt_.resize(kMaxPacketSize);

  // use file descriptor user channel type
  channel_.type = SX_USER_CHANNEL_TYPE_FD;

  // open host ifc in SDK
  rc = sx_api_host_ifc_open(handle_,
    &channel_.channel.fd);
  mlnxCheckSdkFail(rc,
    "sx_api_host_ifc_open",
    "Failed to open user channel");

  // set the file descriptor in EventHandler
  int fd = channel_.channel.fd.fd;
  changeHandlerFD(fd);
}

MlnxUserChannel::~MlnxUserChannel() {
  sx_status_t rc;
  // go over all traps that was registered and
  // unregister them from this user channel
  for (auto trapId: traps_) {
    rc = sx_api_host_ifc_trap_id_register_set(handle_,
      SX_ACCESS_CMD_DEREGISTER,
      swid_,
      trapId,
      &channel_);

    // just log an error if failed
    mlnxLogSxError(rc,
      "sx_api_host_ifc_trap_id_register_set",
      "Failed to de-register trap ",
      trapId);
  }

  try {
    rc = sx_api_host_ifc_close(handle_,
      &channel_.channel.fd);
    mlnxCheckSdkFail(rc,
      "sx_api_host_ifc_close",
      "Failed to close user channel");
  } catch (const FbossError& error) {
    LOG(ERROR) << error.what();
  }
}

void MlnxUserChannel::setRxPacketCallback(const CallbackT& rxCallback) {
  rxCallback_ = rxCallback;
}

void MlnxUserChannel::start() {
  // Check if rxCallback_ was set
  CHECK(rxCallback_)
    << "Cannot start without rx callback function";

  // register handler for receiving events when fd has data to read
  registerHandler(EventHandler::PERSIST | EventHandler::READ);
}

void MlnxUserChannel::stop() {
  // unregister handler if was registered
  unregisterHandler();
}

bool MlnxUserChannel::sendPacketSwitched(
  std::unique_ptr<TxPacket> pkt) const noexcept {
  sx_status_t rc;

  const auto& sxFd = channel_.channel.fd;
  auto buffer = pkt->buf();
  const uint8_t* data = buffer->data();
  auto len = buffer->length();

  if (len < kMinPacketSize) {
    LOG(ERROR) << "Packet size is to small: "
               << "Tried to send " << len << " bytes; "
               << "Minimum frame size " << kMinPacketSize << " bytes";
    return false;
  }

  rc = sx_lib_host_ifc_data_send(&sxFd,
    data,
    len,
    swid_,
    0);
  mlnxLogSxError(rc,
    "sx_lib_host_ifc_data_send",
    "Failed to send packet");

  return (rc == SX_STATUS_SUCCESS);
}

bool MlnxUserChannel::sendPacketOutOfLogPort(std::unique_ptr<TxPacket> pkt,
  sx_port_log_id_t logPort) const noexcept {
  sx_status_t rc;
  bool sent = false;
  const auto& sxFd = channel_.channel.fd;
  auto buffer = pkt->buf();
  const uint8_t* data = buffer->data();
  auto len = buffer->length();

  folly::io::Cursor cursor(pkt->buf());
  auto mac = PktUtil::readMac(&cursor);

  if(mac.isMulticast()) {
    // the same as in sendPacketSwitched
    sent = sendPacketSwitched(std::move(pkt));
  } else {
    // only for unicast
    rc = sx_lib_host_ifc_unicast_ctrl_send(&sxFd,
      data,
      len,
      swid_,
      logPort,
      0);
    mlnxLogSxError(rc,
      "sx_lib_host_ifc_data_send",
      "Failed to send packet out of logical port: ",
      logPort);
    sent = (rc == SX_STATUS_SUCCESS);
  }
  return sent;
}

void MlnxUserChannel::handlerReady(uint16_t /* events */) noexcept {
  sx_status_t rc;

  rxData_.pkt = static_cast<void*>(pkt_.data());
  rxData_.len = kMaxPacketSize;

  rc = sx_lib_host_ifc_recv(
    &channel_.channel.fd,
    rxData_.pkt,
    &rxData_.len,
    &rxData_.recv_info);

  DLOG(INFO) << "Received trap, trap id: "
             << rxData_.recv_info.trap_id;

  if (rc == SX_STATUS_SUCCESS) {
    // invoke the callback
    rxCallback_(rxData_);
  } else {
    // log the error instead of throw
    mlnxLogSxError(rc,
      "sx_lib_host_ifc_recv",
      "Failed while receiving event info");
  }
}

void MlnxUserChannel::registerTrap(sx_trap_id_t trapId) {
  sx_status_t rc;
  if (!SX_TRAP_ID_CHECK_RANGE(trapId)) {
    throw FbossError("Not valid trap id ", trapId);
  }
  rc = sx_api_host_ifc_trap_id_register_set(handle_,
    SX_ACCESS_CMD_REGISTER,
    swid_,
    trapId,
    &channel_);
  mlnxCheckSdkFail(rc,
    "sx_api_host_ifc_trap_id_register_set",
    "Failed to register trap ",
    trapId);
  traps_.push_back(trapId);
}

// initialize static variable nextTrapGroupId
// Start trap group ids from 1
sx_trap_group_t MlnxTrapGroup::nextTrapGroupId = 1;

MlnxTrapGroup::MlnxTrapGroup(MlnxSwitch* hw,
  sx_trap_priority_t prio) :
  hw_(hw),
  handle_(hw_->getHandle()) {
  sx_status_t rc;

  trapGroupId_ = MlnxTrapGroup::nextTrapGroupId++;

  static const auto maxTrapGroupId = hw_->getRmLimits().hw_trap_groups_num_max;

  CHECK_LE(trapGroupId_, maxTrapGroupId) << "Invalid trap group id";

  trapGroupAttrs_.truncate_mode = SX_TRUNCATE_MODE_DISABLE;
  trapGroupAttrs_.control_type =  SX_CONTROL_TYPE_DEFAULT;
  trapGroupAttrs_.add_timestamp = 0;
  trapGroupAttrs_.prio = prio;

  rc = sx_api_host_ifc_trap_group_set(handle_,
    hw_->getDefaultSwid(),
    trapGroupId_,
    &trapGroupAttrs_);
  mlnxCheckSdkFail(rc,
    "sx_api_host_ifc_trap_group_set",
    "Failed to add a trap group id ",
    trapGroupId_,
    " for swid ",
    hw_->getDefaultSwid());
  DLOG(INFO) << "Trap Group with @id " << (int)trapGroupId_;
}

void MlnxTrapGroup::addTrapId(sx_trap_id_t trapId,
  sx_trap_action_t action) {
  sx_status_t rc;

  if (!SX_TRAP_ID_CHECK_RANGE(trapId)) {
    throw FbossError("Not valid trap id ", trapId);
  }
  rc = sx_api_host_ifc_trap_id_set(handle_,
    hw_->getDefaultSwid(),
    trapId,
    trapGroupId_,
    action);
  mlnxCheckSdkFail(rc,
    "sx_api_host_ifc_trap_id_set",
    "Failed to set a trap id ",
    trapId);
}

MlnxHostIfc::MlnxHostIfc(MlnxSwitch* hw) :
  hw_(hw){
  // Create a user channel for Tx packets
  txChannel_ = std::make_unique<MlnxUserChannel>(hw_);

  // Create a user channel for Rx packets
  rxChannel_ = std::make_unique<MlnxUserChannel>(hw_);
  eventsChannel_ = std::make_unique<MlnxUserChannel>(hw_);

  // PUDE and other events is already in default trap group created in SDK
  for (int idx = 0; idx < ARRAY_SIZE(kDefaultTrapGroup); ++idx) {
    eventsChannel_->registerTrap(kDefaultTrapGroup[idx]);
  }

  // Go over trap groups array
  for (int idx = 0; idx < ARRAY_SIZE(kTrapGroups); ++idx) {
    const auto& trapGroupInfo = kTrapGroups[idx];
    const TrapInfo* trapGroupTraps;
    size_t trapsArraySize;
    sx_trap_priority_t trapPrio;

    std::tie(trapGroupTraps, trapsArraySize, trapPrio) = trapGroupInfo;
    // Create a trap group in SDK
    auto group = std::make_unique<MlnxTrapGroup>(hw_, trapPrio);

    // Go over traps in trap group
    for (int iidx = 0; iidx < trapsArraySize; ++iidx) {
      const auto& trapInfo = trapGroupTraps[iidx];
      const auto& trapId = trapInfo.first;
      const auto& trapAction = trapInfo.second;

      DLOG(INFO) << "Adding trap : trap @id "
                 << trapId
                 << " @action: "
                 << (int)trapAction;

      // add trap to the trap group
      group->addTrapId(trapId, trapAction);
      // register to the user channel
      rxChannel_->registerTrap(trapId);
    }

    trapGroups_.emplace_back(std::move(group));
  }
}

MlnxHostIfc::~MlnxHostIfc() {
  stop();
}

void MlnxHostIfc::setRxPacketCallback(const CallbackT& rxCallback) {
  rxChannel_->setRxPacketCallback(rxCallback);
  eventsChannel_->setRxPacketCallback(rxCallback);
}

void MlnxHostIfc::start() {
  rxChannel_->attachEventBase(&hostIfcEventBase_);
  eventsChannel_->attachEventBase(&hostIfcEventBase_);

  // start handlers
  eventsChannel_->start();
  rxChannel_->start();

  // start the thread and run an evb loop
  hostIfcBackgroundThread_ = boost::thread([&] () {
    hostIfcEventBase_.loop();
  });
}

void MlnxHostIfc::stop() {
  // unregister event handler
  rxChannel_->stop();
  eventsChannel_->stop();
  // terminate an event base loop
  hostIfcEventBase_.terminateLoopSoon();
  // wait for rxThread_ to return
  hostIfcBackgroundThread_.join();
}

bool MlnxHostIfc::sendPacketSwitched(
  std::unique_ptr<TxPacket> pkt) const noexcept {
  return txChannel_->sendPacketSwitched(std::move(pkt));
}

bool MlnxHostIfc::sendPacketOutOfPort(std::unique_ptr<TxPacket> pkt,
  PortID id) const noexcept {
  sx_port_log_id_t logPort;
  try {
    logPort = hw_->getMlnxPortTable()->getMlnxLogPort(id);
  } catch (const FbossError& error) {
    // case if no port with port id @id exists
    LOG(ERROR) << "Cannot send packet out of port @id "
               << id
               << " reason: "
               << error.what();
    // return false meaning that packet send failed
    return false;
  }
  return txChannel_->sendPacketOutOfLogPort(std::move(pkt), logPort);
}

}} // facebook::fboss
