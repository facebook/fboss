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

#include "fboss/agent/HwSwitch.h"
#include "fboss/agent/types.h"
#include "fboss/agent/gen-cpp2/switch_config_types.h"

extern "C" {
#include <sx/sdk/sx_api.h>
#include <sx/sdk/sx_swid.h>
#include <sx/sdk/sx_port.h>
#include <sx/sdk/sx_trap_id.h>
}

#include <boost/thread.hpp>

#include <map>
#include <mutex>
#include <memory>

namespace folly {
class MacAddress;
}

namespace facebook { namespace fboss {

struct RxDataStruct;

class Port;
class Vlan;
class Interface;
class SwitchState;
class SwSwitch;

class MlnxPort;
class MlnxDevice;
class MlnxPlatform;
class MlnxPortTable;
class MlnxHostIfc;
class MlnxError;

const uint32_t kMinPacketSize {64};
const uint32_t kMaxPacketSize {SX_HOST_EVENT_BUFFER_SIZE_MAX};

class MlnxSwitch : public HwSwitch {
 public:
  /**
   * Constructs a MlnxSwitch instance
   * All initialization flow is in init()
   *
   * @param platform MlnxPlatform
   */
  MlnxSwitch(MlnxPlatform* platform);

  /**
   * dtor
   */
  virtual ~MlnxSwitch() override;

  /*
   * Initialize the hardware.
   *
   * This is called when the switch first starts, before any configuration has
   * been loaded.
   *
   * This should perform base hardware initialization, and return a new
   * SwitchState object that describes the current hardware state and the
   * type of boot time initialization that was done (warm, cold).  For
   * platforms that support warm reload, the SwitchState should reflect the
   * current state of the hardware.  For platforms that do not support warm
   * reload, the SwitchState should reflect the base configuration after the
   * hardware has been reinitialized.
   *
   * @param callback A pointer to SwSwitch class which is a Callback's child class
   * @return HwInitResult instance which has init/boot time and SwitchState instance
   */
  HwInitResult init(Callback* callback) override;

  /**
   * Apply a state change to the hardware.
   *
   * stateChanged() is called whenever the switch state changes.
   * This is called immediately after updating the state variable in SwSwitch.
   *
   * @param delta StateDelta instance
   * @return The actual state that was applied in the hardware.
   */
  std::shared_ptr<SwitchState> stateChanged(const StateDelta& delta) override;

  /**
   * Allocate a new TxPacket.
   *
   * @param size Packet size
   * @return Unique pointer to TxPacket
   */
  std::unique_ptr<TxPacket> allocatePacket(uint32_t size) override;

  /**
   * Send a packet, using switching logic to send it out the correct port(s)
   * for the specified VLAN and destination MAC.
   *
   * @return If the packet is successfully sent to HW
   */
  bool sendPacketSwitched(std::unique_ptr<TxPacket> pkt) noexcept override;

  /**
   * Send a packet, using switching logic to send it out the correct port(s)
   * for the specified VLAN and destination MAC.
   *
   * @param pkt TxPacket to send
   * @param portID port ID to send from
   * @return If the packet is successfully sent to HW.
   */
  bool sendPacketOutOfPort(
      std::unique_ptr<TxPacket> pkt,
      PortID portId) noexcept override;

  /**
   * Performs graceful exiting
   *
   * @param dump the HwSwitch state
   */
  void gracefulExit(folly::dynamic&) override;

  /**
   * Get HW Switch state in a folly::dynamic object.
   * Needed in warm boots and useful during debugging
   * fatal exits
   *
   * @ret folly::dynamic object with HwSwitch
   */
  folly::dynamic toFollyDynamic() const override;

  /**
   * Tell warm boot code we are done programming
   * the h/w. Warm boot cache then uses this to
   * to delete entries unreferenced entries h/w tables
   */
  void clearWarmBootCache() override {
  }

  /**
   * Invoked right after MlnxSwitch::init()
   *
   * Sets trap groups and starts rx threads
   */
  void initialConfigApplied() override;

  /*
   * Allows hardware-specific code to record switch statistics.
   *
   * @param switchState object to fill in stats
   */
  void updateStats(SwitchStats* /*switchStats*/) override;

  /**
   * Returns a hardware-specific sampler based on a namespace string and list of
   * counters within that namespace.  This assumes that a single sampler
   * instance will never need to handle counters from different namespaces.
   *
   * @return     How many counters were added from this namespace.
   * @param[out] samplers         A vector of high-resolution samplers.  We will
   *                              append new samplers to this list.
   * @param[in]  namespaceString  A string respresentation of the current
   *                              counter namespace.
   * @param[in]  counterSet       The set of requested counters within the
   *                              current namespace.
   */
  int getHighresSamplers(
      HighresSamplerList* /*samplers*/,
      const std::string& /*namespaceString*/,
      const std::set<CounterRequest>& /*counterSet*/) override {
    return 0;
  }

  /**
   * Fetch FDB table from hardware
   *
   * @pararm l2Table A vector of FDB entries
   */
  void fetchL2Table(std::vector<L2EntryThrift>* l2Table) override;

  /**
   * Perform fatal exit
   * This will deinitialize the device and SDK
   */
  void exitFatal() const override;

  /**
   * Tells the hw switch to unregister the callback and to stop calling
   * packetReceived and linkStateChanged. This is mainly used during exit
   * as once the SwSwitch starts exiting, it can no longer guarantee that
   * it can handle packets or link state changed events correctly.
   */
  void unregisterCallbacks() override;

  /**
   * Returns true if the arp/ndp entry for the passed in ip/intf has been hit
   * since the last call to getAndClearNeighborHit.
   */
  bool getAndClearNeighborHit(RouterID /*vrf*/,
      folly::IPAddress& /*ip*/) override {
    return false;
  }

  /**
   * Gets the port operative state by port Id
   *
   * @param portId Port Id
   * @ret true if link is up else false
   */
  bool isPortUp(PortID portId) const override;

  /**
   * Gets the port operative fec config by port Id
   * @param portId Port Id
   * @ret true if fec is on else false
   */
  bool getPortFECConfig(PortID portId) const override;

  /**
   * Check if a state update would be permissible on the HW,
   * without making any actual changes on the HW.
   *
   * @param delta Switch state delta
   * @ret true if valid update
   */
  bool isValidStateUpdate(const StateDelta& /*delta*/) const override {
    return true;
  }

  /**
   * Return pointer to the platform
   *
   * @ret pointer to the MlnxPlatform
   */
  MlnxPlatform* getPlatform() const { return platform_; }

  /**
   * Returns the switch state on cold boot
   *
   * @return SwitchState on cold boot
   *
   */
  std::shared_ptr<SwitchState> getColdBootState();

  /**
   * Returns sdk handle
   *
   * @ret sdk handle
   */
  sx_api_handle_t getHandle() const;

  /**
   * Returns default switch id number which is 0
   *
   * @ret switch id
   */
  sx_swid_t getDefaultSwid() const;

  /**
   * Gets chip type (Spectrum A0/A1, spectrum2, etc)
   *
   * @ret chip type
   */
  sx_chip_types_t getChipType() const;

  /**
   * Gets resource limits for this chip
   *
   * @ret reference to rm limits struct
   */
  const rm_resources_t& getRmLimits() const;

  /**
   * Returns port table
   *
   * @ret pointer to MlnxPortTable
   */
  const MlnxPortTable* getMlnxPortTable() const;

  /**
   * This method return the vector of all ports
   * in the system. It is used by the MlnxPlatform
   * code to generate MlnxPlatformPort & MlnxPort
   * objects based on this vector
   *
   * @return vecot of port attributes
   */
  std::vector<sx_port_attributes_t> getMlnxPortsAttr();

  /**
   * Return callback_ as pointer to SwSwitch
   *
   * @ret pointer to SwSwitch
   */
  const SwSwitch* getSwSwitch() const;

  /**
   * Check whether the mac for interface is valid
   * NOTE: different Mellanox switching chips
   * have different limitations for interface MAC
   *
   * @param mac Interface MAC address to check
   * @ret true if valid otherwise false
   */
  virtual bool isValidMacForInterface(const folly::MacAddress& mac) = 0;

 private:
  // Forbidden copy constructor and assignment operator
  MlnxSwitch(MlnxSwitch const &) = delete;
  MlnxSwitch& operator=(MlnxSwitch const &) = delete;

  /**
   * Callback for received host ifc events/packets
   *
   * @param RxDataStruct Structure that holds data, length and receive info
   */
  void onReceivedHostIfcEvent(const RxDataStruct& rxData) const noexcept;

  /**
   * Callback for received host ifc events/packets
   *
   * @param RxDataStruct Structure that holds data, length and receive info
   */
  void onReceivedPUDEvent(const RxDataStruct& rxData) const noexcept;

  // L2 processing

  // process state changes functions

  /**
   * Process changed port configuration
   *
   * @param oldPort instance
   * @param newPort instance
   */
  void processChangedPorts(const std::shared_ptr<Port>&,
    const std::shared_ptr<Port>&);

  /**
   * Process enabled ports
   *
   * @param oldPort instance
   * @param newPort instance
   */
  void processEnabledPorts(const std::shared_ptr<Port>&,
    const std::shared_ptr<Port>&);

  /**
   * Process disabled ports
   *
   * @param oldPort instance
   * @param newPort instance
   */
  void processDisabledPorts(const std::shared_ptr<Port>&,
    const std::shared_ptr<Port>&);

  /**
   * Process changed vlan configuration
   *
   * @param oldVlan instance
   * @param newVlan instance
   */
  void processChangedVlan(const std::shared_ptr<Vlan>&,
    const std::shared_ptr<Vlan>&);

  /**
   * Process added vlan
   *
   * @param newVlan instance
   */
  void processAddedVlan(const std::shared_ptr<Vlan>&);

  /**
   * Process removed vlan
   *
   * @param removedVlan instance
   */
  void processRemovedVlan(const std::shared_ptr<Vlan>&);

  /**
   * Changes default vlan
   *
   * @vlan new vlan id
   */
  void changeDefaultVlan(VlanID vlan);

  // private fields

  HwSwitch::Callback* callback_{nullptr};
  sx_api_handle_t handle_{SX_API_INVALID_HANDLE};
  MlnxPlatform* platform_{nullptr};
  sx_swid_t swid_;

  std::unique_ptr<MlnxDevice> device_;
  std::unique_ptr<MlnxHostIfc> hostIfc_;

  // tables
  std::unique_ptr<MlnxPortTable> portTable_;

  std::mutex lock_;
};

}} // facebook::fboss
