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

#include "fboss/agent/Platform.h"
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/hw/gen-cpp2/hardware_stats_types.h"
#include "fboss/agent/if/gen-cpp2/FbossCtrl.h"
#include "fboss/agent/types.h"

#include <folly/IPAddress.h>
#include <optional>

#include <memory>
#include <utility>

namespace folly {
struct dynamic;
}

namespace facebook::fboss {

class SwitchState;
class SwitchStats;
class StateDelta;
class RxPacket;
class TxPacket;
class L2Entry;
class HwSwitchStats;
enum class L2EntryUpdateType : uint8_t;

struct HwInitResult {
  std::shared_ptr<SwitchState> switchState{nullptr};
  std::shared_ptr<SwitchState> switchStateDesired{nullptr};
  BootType bootType{BootType::UNINITIALIZED};
  float initializedTime{0.0};
  float bootTime{0.0};
};

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
    virtual void linkStateChanged(PortID port, bool up) = 0;

    /*
     * l2LearningUpdateReceived() is invoked by the HwSwitch when there is
     * changes l2 table.
     */
    virtual void l2LearningUpdateReceived(
        L2Entry l2Entry,
        L2EntryUpdateType l2EntryUpdateType) = 0;

    /*
     * Used to notify the SwSwitch of a fatal error so the implementation can
     * provide special behavior when a crash occurs.
     */
    virtual void exitFatal() const noexcept = 0;
  };

  enum FeaturesDesired : uint32_t {
    PACKET_RX_DESIRED = 0x01,
    LINKSCAN_DESIRED = 0x02
  };

  HwSwitch(
      uint32_t featuresDesired =
          (FeaturesDesired::PACKET_RX_DESIRED |
           FeaturesDesired::LINKSCAN_DESIRED))
      : featuresDesired_(featuresDesired) {}
  virtual ~HwSwitch() {}

  virtual Platform* getPlatform() const = 0;

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
  virtual HwInitResult init(Callback* callback) = 0;

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
   *
   * @ret   The actual state that was applied in the hardware.
   */
  virtual std::shared_ptr<SwitchState> stateChanged(
      const StateDelta& delta) = 0;
  /*
   * Check if a state update would be permissible on the HW,
   * without making any actual changes on the HW.
   */
  virtual bool isValidStateUpdate(const StateDelta& delta) const = 0;

  /*
   * Allocate a new TxPacket.
   */
  virtual std::unique_ptr<TxPacket> allocatePacket(uint32_t size) const = 0;

  /*
   * Send a packet, use switching logic to send it out the correct port(s)
   * for the specified VLAN and destination MAC.
   *
   * @return If the packet is successfully sent to HW.
   */
  virtual bool sendPacketSwitchedAsync(
      std::unique_ptr<TxPacket> pkt) noexcept = 0;

  /*
   * Send a packet, send it out the specified port, use
   * VLAN and destination MAC from packet
   *
   * @return If the packet is successfully sent to HW.
   */
  virtual bool sendPacketOutOfPortAsync(
      std::unique_ptr<TxPacket> pkt,
      PortID portID,
      std::optional<uint8_t> queue = std::nullopt) noexcept = 0;

  /*
   * Send a packet, use switching logic to send it out the correct port(s)
   * for the specified VLAN and destination MAC.
   *
   * @return If the packet is successfully sent to HW.
   */
  virtual bool sendPacketSwitchedSync(
      std::unique_ptr<TxPacket> pkt) noexcept = 0;

  /*
   * Send a packet, send it out the specified port and queue, use
   * VLAN and destination MAC from packet
   *
   * @return If the packet is successfully sent to HW.
   */
  virtual bool sendPacketOutOfPortSync(
      std::unique_ptr<TxPacket> pkt,
      PortID portID,
      std::optional<uint8_t> queue = std::nullopt) noexcept = 0;

  /*
   * Allows hardware-specific code to record switch statistics.
   */
  void updateStats(SwitchStats* switchStats);

  virtual folly::F14FastMap<std::string, HwPortStats> getPortStats() const = 0;

  virtual void fetchL2Table(std::vector<L2EntryThrift>* l2Table) const = 0;

  /*
   * Get latest device watermark bytes
   */
  virtual uint64_t getDeviceWatermarkBytes() const = 0;
  /*
   * Allow hardware to perform any warm boot related cleanup
   * before we exit the application.
   */
  virtual void gracefulExit(folly::dynamic& switchState) = 0;

  /*
   * Get Hw Switch state in a folly::dynamic
   * object. Needed in warm boots and useful during
   * debugging fatal exits.
   */
  virtual folly::dynamic toFollyDynamic() const = 0;

  /*
   * Some HwSwitch (SAI most prominently) implementations require a
   * l2 entry to be created for each resolved neighbor. So provide
   * a boolean knob to control this. Longer term we should just
   * default to doing this for all HwSwitch impls. A l2 entry
   * anyways get created as a result of packet exchange for
   * neighbhor resolution, so being explicit does not hurt
   */
  virtual bool needL2EntryForNeighbor() const {
    return false;
  }
  /*
   * When SwSwitch changes its SwitchRunState, such as when it transitions
   * to INITIALIZED or CONFIGURED, HwSwitch may need to react. For
   * example, clearing the warm boot cache on FIB_SYNCED, or
   * turning on callbacks on INITIALIZED.
   */
  void switchRunStateChanged(SwitchRunState newState);

  SwitchRunState getRunState() const {
    return runState_;
  }

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
   * Get whether the port has set FEC or not
   * TODO(rsher) Consider refactoring this interface to expose
   * ports as first class citizens (hwPort?) and then move all of these
   * functions into the hwPort abstraction
   */
  virtual bool getPortFECEnabled(PortID /* unused */) const {
    return false;
  }

  /*
   * Returns true if the arp/ndp entry for the passed in ip/intf has been hit
   * since the last call to getAndClearNeighborHit.
   */
  virtual bool getAndClearNeighborHit(RouterID vrf, folly::IPAddress& ip) = 0;

  /*
   * Clear port stats for specified port
   */
  virtual void clearPortStats(
      const std::unique_ptr<std::vector<int32_t>>& ports) = 0;

  virtual std::vector<PrbsLaneStats> getPortAsicPrbsStats(int32_t /*portId*/) {
    return std::vector<PrbsLaneStats>();
  }
  virtual void clearPortAsicPrbsStats(int32_t /*portId*/) {}

  virtual std::vector<PrbsLaneStats> getPortGearboxPrbsStats(
      int32_t /*portId*/,
      phy::Side /* side */) {
    return std::vector<PrbsLaneStats>();
  }
  virtual void clearPortGearboxPrbsStats(
      int32_t /*portId*/,
      phy::Side /* side */) {}

  virtual BootType getBootType() const = 0;

  virtual cfg::PortSpeed getPortMaxSpeed(PortID /* port */) const = 0;

  uint32_t getFeaturesDesired() const {
    return featuresDesired_;
  }

  std::string getDebugDump() const;
  virtual void dumpDebugState(const std::string& path) const = 0;

  HwSwitchStats* getSwitchStats() const;

 private:
  virtual void switchRunStateChangedImpl(SwitchRunState newState) = 0;

  virtual void updateStatsImpl(SwitchStats* switchStats) = 0;

  uint32_t featuresDesired_;
  SwitchRunState runState_{SwitchRunState::UNINITIALIZED};

  // Forbidden copy constructor and assignment operator
  HwSwitch(HwSwitch const&) = delete;
  HwSwitch& operator=(HwSwitch const&) = delete;
};

} // namespace facebook::fboss
