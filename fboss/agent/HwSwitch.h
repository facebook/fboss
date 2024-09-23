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
#include "fboss/agent/gen-cpp2/switch_state_types.h"
#include "fboss/agent/hw/gen-cpp2/hardware_stats_types.h"
#include "fboss/agent/if/gen-cpp2/FbossCtrl.h"
#include "fboss/agent/if/gen-cpp2/MultiSwitchCtrl.h"
#include "fboss/agent/if/gen-cpp2/ctrl_types.h"
#include "fboss/agent/if/gen-cpp2/multiswitch_ctrl_handlers.h"
#include "fboss/agent/rib/RoutingInformationBase.h"
#include "fboss/agent/state/DeltaFunctions.h"
#include "fboss/agent/types.h"
#include "fboss/lib/HwWriteBehavior.h"

#include "fboss/agent/HwSwitchCallback.h"

#include <folly/IPAddress.h>
#include <folly/ThreadLocal.h>
#include <optional>

#include <memory>
#include <utility>

namespace folly {
struct dynamic;
}

DECLARE_bool(flowletStatsEnable);
DECLARE_int32(update_voq_stats_interval_s);

namespace facebook::fboss {

class SwitchState;
class SwitchStats;
class StateDelta;
class RxPacket;
class TxPacket;
class L2Entry;
class HwSwitchFb303Stats;

template <typename Delta, typename Mgr>
void checkUnsupportedDelta(const Delta& delta, Mgr& mgr) {
  DeltaFunctions::forEachChanged(
      delta,
      [&](const auto& oldNode, const auto& newNode) {
        mgr.processChanged(oldNode, newNode);
      },
      [&](const auto& newNode) { mgr.processAdded(newNode); },
      [&](const auto& oldNode) { mgr.processRemoved(oldNode); });
}

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
  using Callback = HwSwitchCallback;
  using StateChangedFn =
      std::function<std::shared_ptr<SwitchState>(const StateDelta& delta)>;

  enum FeaturesDesired : uint32_t {
    PACKET_RX_DESIRED = 0x01,
    LINKSCAN_DESIRED = 0x02,
    TAM_EVENT_NOTIFY_DESIRED = 0x04,
  };

  explicit HwSwitch(
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
  HwInitResult init(
      Callback* callback,
      const std::shared_ptr<SwitchState>& state,
      bool failHwCallsOnWarmboot);

  /* initialize hardware switch but do not apply warm boot state */
  HwInitResult initLight(Callback* callback, bool failHwCallsOnWarmboot);

  cfg::SwitchType getSwitchType() const {
    return switchType_;
  }
  std::optional<int64_t> getSwitchId() const {
    return switchId_;
  }
  SwitchID getSwitchID() const {
    if (!switchId_) {
      return SwitchID(0);
    }
    return SwitchID(*switchId_);
  }
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
  std::shared_ptr<SwitchState> stateChanged(
      const StateDelta& delta,
      const HwWriteBehaviorRAII& behavior =
          HwWriteBehaviorRAII(HwWriteBehavior::WRITE));

  virtual std::shared_ptr<SwitchState> stateChangedImpl(
      const StateDelta& delta) = 0;

  virtual std::shared_ptr<SwitchState> stateChangedTransaction(
      const StateDelta& delta,
      const HwWriteBehaviorRAII& behavior =
          HwWriteBehaviorRAII(HwWriteBehavior::WRITE));
  virtual void rollback(const StateDelta& delta) noexcept;

  virtual bool transactionsSupported() const {
    return false;
  }
  /*
   * Check if a state update would be permissible on the HW,
   * without making any actual changes on the HW.
   */
  virtual bool isValidStateUpdate(const StateDelta& delta) const = 0;

  /*
   *  Use to run diagnostic shell commands on various switches.
   */
  virtual void printDiagCmd(const std::string& cmd) const = 0;

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
  void updateStats();

  multiswitch::HwSwitchStats getHwSwitchStats();

  virtual folly::F14FastMap<std::string, HwPortStats> getPortStats() const = 0;
  virtual CpuPortStats getCpuPortStats() const = 0;

  virtual void fetchL2Table(std::vector<L2EntryThrift>* l2Table) const = 0;

  void updateAllPhyInfo();
  std::map<PortID, phy::PhyInfo> getAllPhyInfo() const {
    return lastPhyInfo_.copy();
  }
  virtual const std::map<PortID, FabricEndpoint>& getFabricConnectivity()
      const = 0;
  virtual std::vector<PortID> getSwitchReachability(
      SwitchID switchId) const = 0;
  virtual std::map<std::string, HwSysPortStats> getSysPortStats() const = 0;
  virtual FabricReachabilityStats getFabricReachabilityStats() const = 0;
  virtual TeFlowStats getTeFlowStats() const = 0;
  virtual HwSwitchDropStats getSwitchDropStats() const = 0;
  virtual HwFlowletStats getHwFlowletStats() const = 0;
  virtual std::vector<EcmpDetails> getAllEcmpDetails() const = 0;
  virtual HwSwitchWatermarkStats getSwitchWatermarkStats() const = 0;
  virtual HwResourceStats getResourceStats() const = 0;

  /*
   * Get latest device watermark bytes
   */
  virtual uint64_t getDeviceWatermarkBytes() const = 0;
  /*
   * Allow hardware to perform any warm boot related cleanup
   * before we exit the application.
   */
  void gracefulExit();

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
   * currently used only for BCM
   */
  virtual bool portExists(PortID /*port*/) const {
    return true;
  }

  virtual phy::FecMode getPortFECMode(PortID /* unused */) const {
    return phy::FecMode::NONE;
  }

  /*
   * Clear port stats for specified port
   */
  virtual void clearPortStats(
      const std::unique_ptr<std::vector<int32_t>>& ports) = 0;

  virtual std::vector<phy::PrbsLaneStats> getPortAsicPrbsStats(
      PortID /*portId*/) {
    return std::vector<phy::PrbsLaneStats>();
  }
  virtual void clearPortAsicPrbsStats(PortID /*portId*/) {}

  virtual std::vector<prbs::PrbsPolynomial> getPortPrbsPolynomials(
      int32_t /* portId */) {
    return std::vector<prbs::PrbsPolynomial>();
  }

  virtual prbs::InterfacePrbsState getPortPrbsState(PortID /* portId */) {
    return prbs::InterfacePrbsState();
  }

  virtual BootType getBootType() const = 0;

  virtual cfg::PortSpeed getPortMaxSpeed(PortID /* port */) const = 0;

  uint32_t getFeaturesDesired() const {
    return featuresDesired_;
  }

  std::string getDebugDump() const;
  virtual void dumpDebugState(const std::string& path) const = 0;
  virtual std::string listObjects(
      const std::vector<HwObjectType>& types,
      bool cached) const = 0;

  HwSwitchFb303Stats* getSwitchStats() const;

  uint32_t generateDeterministicSeed(LoadBalancerID loadBalancerID);

  virtual uint32_t generateDeterministicSeed(
      LoadBalancerID loadBalancerID,
      folly::MacAddress mac) const = 0;

  std::shared_ptr<SwitchState> getProgrammedState() const;
  fsdb::OperDelta stateChanged(
      const fsdb::OperDelta& delta,
      const HwWriteBehaviorRAII& behavior =
          HwWriteBehaviorRAII(HwWriteBehavior::WRITE));
  fsdb::OperDelta stateChangedTransaction(
      const fsdb::OperDelta& delta,
      const HwWriteBehaviorRAII& behavior =
          HwWriteBehaviorRAII(HwWriteBehavior::WRITE));

  void ensureConfigured(folly::StringPiece function) const;
  void ensureVoqOrFabric(folly::StringPiece function) const;

  bool isFullyConfigured() const;

  virtual void syncLinkStates() = 0;
  virtual void syncLinkActiveStates() = 0;
  virtual void syncLinkConnectivity() = 0;
  virtual void syncSwitchReachability() = 0;

  virtual AclStats getAclStats() const = 0;

  virtual std::shared_ptr<SwitchState> reconstructSwitchState() const = 0;

  virtual void injectSwitchReachabilityChangeNotification() = 0;

 protected:
  void setProgrammedState(const std::shared_ptr<SwitchState>& state);

  virtual void initialStateApplied() = 0;

 private:
  HwInitResult initLightImpl(Callback* callback, bool failHwCallsOnWarmboot);
  virtual HwInitResult initImpl(
      Callback* callback,
      BootType bootType,
      bool failHwCallsOnWarmboot) = 0;
  virtual void switchRunStateChangedImpl(SwitchRunState newState) = 0;

  virtual void updateStatsImpl() = 0;

  virtual void gracefulExitImpl() = 0;

  virtual std::map<PortID, phy::PhyInfo> updateAllPhyInfoImpl() = 0;

  std::shared_ptr<SwitchState> getMinAlpmState(
      RoutingInformationBase* rib,
      const std::shared_ptr<SwitchState>& state);

  std::shared_ptr<SwitchState> programMinAlpmState(RoutingInformationBase* rib);
  std::shared_ptr<SwitchState> programMinAlpmState(
      RoutingInformationBase* rib,
      StateChangedFn func);

  HwWriteBehaviorRAII getWarmBootWriteBehavior(
      bool failHwCallsOnWarmboot) const;
  uint32_t featuresDesired_;
  SwitchRunState runState_{SwitchRunState::UNINITIALIZED};

  // Forbidden copy constructor and assignment operator
  HwSwitch(HwSwitch const&) = delete;
  HwSwitch& operator=(HwSwitch const&) = delete;
  // mutable to allow for lazy init from getter. This is needed to
  // create the var in the thread local storage (TLS) of the calling
  // thread and cannot be created up front.
  mutable folly::ThreadLocalPtr<HwSwitchFb303Stats> hwSwitchStats_;
  cfg::SwitchType switchType_{cfg::SwitchType::NPU};
  std::optional<int64_t> switchId_;

  folly::Synchronized<std::shared_ptr<SwitchState>> programmedState_;

  // Collecting phy Info is currently inefficient on some platforms. Instead of
  // collecting them every second, tune down the frequency to only collect once
  // every update_phy_info_interval_s seconds (default to be 10).
  int phyInfoUpdateTime_{0};
  folly::Synchronized<std::map<PortID, phy::PhyInfo>> lastPhyInfo_;
};

} // namespace facebook::fboss
