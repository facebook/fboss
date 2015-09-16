/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#pragma once

#include "fboss/agent/HighresCounterUtil.h"
#include "fboss/agent/types.h"
#include "fboss/agent/if/gen-cpp2/FbossCtrl.h"
#include <folly/IPAddress.h>

#include <memory>
#include <utility>

namespace facebook { namespace fboss {

class SwitchState;
class SwitchStats;
class StateDelta;
class RxPacket;
class TxPacket;

/*
 * HwSwitch contains the hardware-specific switching logic.
 *
 * Like SwSwitch, there is a single HwSwitch for the entire controller.  It
 * represents the switching hardware for the entire chassis, not just a single
 * switch ASIC.  In a multi-chip chassis, there would still be a single
 * HwSwitch for the entire chassis.
 *
 * HwSwitch is a pure virtual interface, and separate implementations must be
 * provided for each different switch topology that we support.  For instance,
 * we may have different implementations for single-chip Broadcom platforms,
 * multi-chip Broadcom platforms, etc.  A mock, software only implementation
 * may also be provided for testing on development servers with no actual
 * hardware switching ASIC.
 *
 * At a minimum, a HwSwitch implementation must provides access to the switch
 * ports, and provides methods for sending and receiving packets via these
 * ports.
 *
 * In addition, most switch implementations also implement packet forwarding in
 * hardware, so HwSwitch also provides mechanisms for programming the hardware
 * forwarding tables.  However, this functionality is not required--even if a
 * HwSwitch implementation does not implement hardware-based forwarding, the
 * SwSwitch implementation would be able to handle all packets in software
 * (albeit more slowly).
 */
class HwSwitch {
 public:
  class Callback {
   public:
    virtual ~Callback() {}

    /*
     * packetReceived() is invoked by the HwSwitch when a trapped packet is
     * received.
     */
    virtual void packetReceived(std::unique_ptr<RxPacket> pkt) noexcept = 0;

    /*
     * linkStateChanged() is invoked by the HwSwitch whenever the link
     * status changes on a port.
     */
    virtual void linkStateChanged(PortID port, bool up) noexcept = 0;

    /*
     * Used to notify the SwSwitch of a fatal error so the implementation can
     * provide special behavior when a crash occurs.
     */
    virtual void exitFatal() const noexcept = 0;
  };

  HwSwitch() {}
  virtual ~HwSwitch() {}

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
   */
  virtual std::pair<std::shared_ptr<SwitchState>, BootType>
    init(Callback* callback) = 0;


  /*
   * Tells the hw switch to unregister the callback and to stop calling
   * packetReceived and linkStateChanged. This is mainly used during exit
   * as once the SwSwitch starts exiting, it can no longer guarantee that
   * it can handle packets or link state changed events correctly.
   */
  virtual void unregisterCallbacks() = 0;

  /*
   * Apply a state change to the hardware.
   *
   * stateChanged() is called whenever the switch state changes.
   * This is called immediately after updating the state variable in SwSwitch.
   */
  virtual void stateChanged(const StateDelta& delta) = 0;

  /*
   * Allocate a new TxPacket.
   */
  virtual std::unique_ptr<TxPacket> allocatePacket(uint32_t size) = 0;

  /*
   * Send a packet, using switching logic to send it out the correct port(s)
   * for the specified VLAN and destination MAC.
   *
   * @return If the packet is successfully sent to HW.
   */
  virtual bool sendPacketSwitched(std::unique_ptr<TxPacket> pkt) noexcept = 0;

  /*
   * Send a packet, using switching logic to send it out the correct port(s)
   * for the specified VLAN and destination MAC.
   *
   * @return If the packet is successfully sent to HW.
   */
  virtual bool sendPacketOutOfPort(std::unique_ptr<TxPacket> pkt,
                                   PortID portID) noexcept = 0;

  /*
   * Allows hardware-specific code to record switch statistics.
   */
  virtual void updateStats(SwitchStats* switchStats) = 0;

  /*
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
  virtual int getHighresSamplers(
      HighresSamplerList* samplers,
      const folly::StringPiece namespaceString,
      const std::set<folly::StringPiece>& counterSet) = 0;

  /*
   * Allow hardware to perform any warm boot related cleanup
   * before we exit the application.
   */
  virtual void gracefulExit() = 0;

  /*
   * Allow hardware to start any services that can only be
   * safely started after initial config has been applied.
   */
  virtual void initialConfigApplied() = 0;

   /*
   * Tell warm boot code we are done programming
   * the h/w. Warm boot cache then uses this to
   * to delete entries unreferenced entries h/w tables
   */
  virtual void clearWarmBootCache() = 0;

  /*
   * Defines the exit behavior when there is a crash. Allows implementations
   * to add functionality to help with debugging crashes.
   */
  virtual void exitFatal() const = 0;

  /*
   * Get port operational state
   */
  virtual bool isPortUp(PortID port) const = 0;

  /*
   * Returns true if the arp/ndp entry for the passed in ip/intf has been hit
   * since the last call to getAndClearNeighborHit.
   */
  virtual bool getAndClearNeighborHit(RouterID vrf,
                                      folly::IPAddress& ip) = 0;

 private:
  // Forbidden copy constructor and assignment operator
  HwSwitch(HwSwitch const &) = delete;
  HwSwitch& operator=(HwSwitch const &) = delete;
};

}} // facebook::fboss
