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
#pragma once
#include "fboss/agent/hw/mlnx/MlnxSwitch.h"
#include "fboss/agent/hw/mlnx/MlnxRxPacket.h"

#include "fboss/agent/types.h"

#include <folly/io/async/EventHandler.h>

extern "C" {
#include <sx/sdk/sx_api.h>
#include <sx/sdk/sx_swid.h>
}

#include <vector>
#include <memory>
#include <atomic>

using folly::EventHandler;

namespace facebook { namespace fboss {

class TxPacket;

class MlnxSwitch;
class MlnxRxPacket;

// Callback functio type
using CallbackT = std::function<void (const RxDataStruct&)>;

/**
 * Abstracts the user channel for receiving events/packets
 */
class MlnxUserChannel : public EventHandler {
 public:
  /**
   * ctor, opens an user channel in SDK for tx and rx packets
   *
   * @param MlnxSwitch
   */
  MlnxUserChannel(MlnxSwitch* hw);

  /**
   * dtor, unregisters all registered traps, closes the user channel
   */
  ~MlnxUserChannel();

  /**
   * Sets the rx callback function, that would be called when
   * the user channel will be able to read packet date from FD
   *
   * @param rxCallback Callback function
   */
  void setRxPacketCallback(const CallbackT& rxCallback);

  /**
   * Run the event base threads for receiving packets
   */
  void start();

  /**
   * Stops the running event base threads
   */
  void stop();

  /**
   * handlerReady method is inherited from EventHandler class.
   * It is called when the FD is available to read/write operations
   *
   * @param events Events bitmask (not used)
   */
  void handlerReady(uint16_t /* events */) noexcept;

  /**
   * Registers trap for this user channel
   *
   * @param trapId SDK trap ID
   */
  void registerTrap(sx_trap_id_t trapId);

  /**
   * Tries to send packet through the pipeline
   *
   * @param pkt Unique pointer to TxPacket
   * @ret true if send successfully, otherwise false
   */
  bool sendPacketSwitched(std::unique_ptr<TxPacket> pkt) const noexcept;

  /**
   * Tries to send packet bypassing the pipeline, specifying the egress port
   * NOTE: if the packet is multicast sendPacketSwitched will be called
   *
   * @param pkt Unique pointer to TxPacket
   * @param id logical port id in SDK
   * @ret true if send successfully, otherwise false
   */
  bool sendPacketOutOfLogPort(std::unique_ptr<TxPacket> pkt,
    sx_port_log_id_t id) const noexcept;

 private:
  // forbidden copy constructor and assignment operator
  MlnxUserChannel(const MlnxUserChannel&) = delete;
  MlnxUserChannel& operator=(const MlnxUserChannel&) = delete;

  // private fields
  MlnxSwitch* hw_ {nullptr};
  sx_api_handle_t handle_ {SX_API_INVALID_HANDLE};
  sx_swid_t swid_;
  sx_user_channel_t channel_;
  std::vector<sx_trap_id_t> traps_;
  std::vector<uint8_t> pkt_;
  RxDataStruct rxData_;
  CallbackT rxCallback_ {nullptr};
};

/**
 * Class with static methods to manage trap group in SDK
 */
class MlnxTrapGroup {
 public:

  /**
   * Creates a trap group in SDK
   *
   * @param priority Priority value for traps in this group
   * @ret Group Id
   */
   MlnxTrapGroup(MlnxSwitch* hw, sx_trap_priority_t priority);

   /**
    * No specific actions required to delete trap group.
    * Sdk remove the groups on shutdown,
    * the driver flush them and close all rdqs
    */
   ~MlnxTrapGroup() = default;

   /**
    * Returns trap group id
    *
    * @ret trap group id
    */
   sx_trap_group_t getGroupId() const;

  /**
   * Add a trap with id @trapId to group with id @groupId
   *
   * @param groupId Trap group id
   * @param trapId Trap id
   * @param action Trap action
   */
  void addTrapId(sx_trap_id_t trapId,
    sx_trap_action_t action);

 private:
  // forbidden copy ctor and assignment operator
  MlnxTrapGroup(const MlnxTrapGroup&) = delete;
  MlnxTrapGroup& operator=(const MlnxTrapGroup&) = delete;

  // trap group id counter
  static sx_trap_group_t nextTrapGroupId;

  // private fields
  MlnxSwitch* hw_ {nullptr};
  sx_api_handle_t handle_ {SX_API_INVALID_HANDLE};
  sx_trap_group_t trapGroupId_;
  sx_trap_group_attributes_t trapGroupAttrs_;
};

/**
 * This class provide functionality needed by MlnxSwitch class
 * to receive from and send to HW packets
 */
class MlnxHostIfc {
 public:
  /**
   * ctor, creates user channels for rx and tx packets
   * and creates trap groups in SDK
   *
   * @param hw Pointer to MlnxSwitch object
   */
  MlnxHostIfc(MlnxSwitch* hw);

  /**
   * dtpr, stops all running threads if they were running
   */
  ~MlnxHostIfc();

  /**
   * Sets the rx callback function, that would be called when
   * the user channel will be able to read packet date from FD
   *
   * @param rxCallback Callback function
   */
  void setRxPacketCallback(const CallbackT& rxCallback);

  /**
   * Starts the event base threads for receiving packets
   */
  void start();

  /**
   * Stops the running event base threads
   */
  void stop();

  /**
   * Send packet through pipeline
   *
   * @param pkt Unique pointer to TxPacket
   * @ret true if send successfully, otherwise false
   */
  bool sendPacketSwitched(std::unique_ptr<TxPacket> pkt) const noexcept;

  /**
   * Send packet bypassing the pipeline, specifying the egress port
   *
   * @param pkt Unique pointer to TxPacket
   * @param id FBOSS Port ID to send packet from
   * @ret true if send successfully, otherwise false
   */
  bool sendPacketOutOfPort(std::unique_ptr<TxPacket> pkt,
    PortID id) const noexcept;

 private:
  // forbidden copy constructor and assignment operator
  MlnxHostIfc(const MlnxHostIfc&) = delete;
  MlnxHostIfc& operator=(const MlnxHostIfc&) = delete;

  MlnxSwitch* hw_ {nullptr};
  // A user channel for sending packets
  std::unique_ptr<MlnxUserChannel> txChannel_;
  // A user channel for receiving packets
  std::unique_ptr<MlnxUserChannel> rxChannel_;
  std::unique_ptr<MlnxUserChannel> eventsChannel_;

  // thread for receiving packets
  // and calling callback->packetReceived()
  boost::thread hostIfcBackgroundThread_;
  folly::EventBase hostIfcEventBase_;

  // trap groups
  std::vector<std::unique_ptr<MlnxTrapGroup>> trapGroups_;
};

}} // facebook::fboss
