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

#include "fboss/agent/HwSwitch.h"
#include "fboss/agent/PacketObserver.h"
#include "fboss/agent/RestartTimeTracker.h"
#include "fboss/agent/SwSwitchRouteUpdateWrapper.h"
#include "fboss/agent/Utils.h"
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/gen-cpp2/switch_state_types.h"
#include "fboss/agent/if/gen-cpp2/ctrl_types.h"
#include "fboss/agent/rib/RoutingInformationBase.h"
#include "fboss/agent/state/StateUpdate.h"
#include "fboss/agent/types.h"
#include "fboss/lib/ThreadHeartbeat.h"
#include "fboss/lib/link_snapshots/SnapshotManager-defs.h"
#include "fboss/lib/phy/gen-cpp2/phy_types.h"

#include <folly/IntrusiveList.h>
#include <folly/Range.h>
#include <folly/SpinLock.h>
#include <folly/ThreadLocal.h>
#include <folly/io/async/EventBase.h>
#include <optional>

#include <atomic>
#include <chrono>
#include <memory>
#include <mutex>
#include <thread>
#include <type_traits>

namespace facebook::fboss {

class ArpHandler;
class InterfaceStats;
class IPv4Handler;
class IPv6Handler;
class LinkAggregationManager;
class LldpManager;
class MPLSHandler;
class PktCaptureManager;
class Platform;
class Port;
class PortDescriptor;
class PortStats;
class PortUpdateHandler;
class RxPacket;
class SwitchState;
class SwitchStats;
class StateDelta;
class NeighborUpdater;
class PacketLogger;
class RouteUpdateLogger;
class StateObserver;
class TunManager;
class MirrorManager;
template <size_t interval>
class PhySnapshotManager;
class AclNexthopHandler;
class LookupClassUpdater;
class LookupClassRouteUpdater;
class MacTableManager;
class ResolvedNexthopMonitor;
class ResolvedNexthopProbeScheduler;
class StaticL2ForNeighborObserver;
class MKAServiceManager;
template <typename AddressT>
class Route;
class Interface;
class FsdbSyncer;
class TeFlowNexthopHandler;
class DsfSubscriber;

enum class SwitchFlags : int {
  DEFAULT = 0,
  ENABLE_TUN = 1,
  ENABLE_LLDP = 2,
  PUBLISH_STATS = 4,
  ENABLE_LACP = 8,
  ENABLE_MACSEC = 16,
};

inline SwitchFlags operator|(SwitchFlags lhs, SwitchFlags rhs) {
  using BackingType = std::underlying_type_t<SwitchFlags>;
  return static_cast<SwitchFlags>(
      static_cast<BackingType>(lhs) | static_cast<BackingType>(rhs));
}

inline SwitchFlags& operator|=(SwitchFlags& lhs, SwitchFlags rhs) {
  lhs = lhs | rhs;
  return lhs;
}

inline bool operator&(SwitchFlags lhs, SwitchFlags rhs) {
  using BackingType = std::underlying_type_t<SwitchFlags>;
  return (static_cast<BackingType>(lhs) & static_cast<BackingType>(rhs)) != 0;
}

/*
 * A software representation of a switch.
 *
 * This represents an entire switch in the network (as opposed to a single
 * switch ASIC).
 *
 * SwSwitch is the primary entry point into the FBOSS controller.  It provides
 * all of the hardware-independent logic for switching and routing packets.  It
 * must be used in conjunction with a HwSwitch object, which provides an
 * interface to the switch hardware.
 */
class SwSwitch : public HwSwitch::Callback {
 public:
  typedef std::function<std::shared_ptr<SwitchState>(
      const std::shared_ptr<SwitchState>&)>
      StateUpdateFn;

  typedef std::function<void(const StateDelta&)> StateUpdatedCallback;

  using AllThreadsSwitchStats =
      folly::ThreadLocalPtr<SwitchStats, SwSwitch>::Accessor;

  explicit SwSwitch(std::unique_ptr<Platform> platform);
  ~SwSwitch() override;

  HwSwitch* getHw() const {
    return hw_;
  }

  const Platform* getPlatform() const {
    return platform_.get();
  }
  Platform* getPlatform() {
    return platform_.get();
  }

  TunManager* getTunManager() {
    return tunMgr_.get();
  }

  /**
   * Return the vlan where the CPU sits
   *
   * This vlan ID is used to encode the l2 vlan info when CPU sends traffic
   * through the HW.
   * Note: It does not mean the HW will send the packet with this vlan value.
   * For example, Broadcom HW will overwrite this value based on its egress
   * programming.
   */
  VlanID getCPUVlan() const {
    return VlanID(4095);
  }

  /*
   * Initialize the switch.
   *
   * This initializes the underlying hardware, and populates the current
   * SwitchState to reflect the hardware state.
   *
   * Note that this function is generally slow, and may take many seconds to
   * complete.
   *
   * Param TunManager can be passed to init method. It allows
   * using any custom TunManager instead of default (useful in testing). This
   * will be used only when ENABLE_TUN is present in flags param. Pass nullptr
   * if you don't want custom behaviour for TunManager (default one will be
   * created).
   *
   * Param optional SwitchFlags has has the following flags defined:
   *
   * ENABLE_TUN: enables interface sync to system.
   * ENABLE_LLDP: enables periodically sending LLDP packets
   * PUBLISH_STATS: if set, we will publish the boot type (graceful or
   *                   otherwise) after we initialize the hardware.
   * DEFAULT: None of the above flags are set.
   *
   */
  void init(
      std::unique_ptr<TunManager> tunMgr,
      SwitchFlags flags = SwitchFlags::DEFAULT);

  bool isFullyInitialized() const;

  bool isInitialized() const;

  bool isFullyConfigured() const;

  bool isConfigured() const;

  bool isExiting() const;

  void updateLldpStats();

  void updateStats();

  std::tuple<folly::dynamic, state::WarmbootState> gracefulExitState() const;

  /*
   * Get a pointer to the current switch state.
   *
   * There are actually two states, applied and desired. In absence of hw
   * update failures, these states should be the same. However when they
   * do differ, return appliedState to the callers, since that's what
   * we applied to HW.
   */
  std::shared_ptr<SwitchState> getState() const {
    return getAppliedState();
  }
  /**
   * Schedule an update to the switch state.
   *
   *  @param  update
   *          The update to be enqueued
   *  @return bool whether the update was queued or not
   * This schedules the specified StateUpdate to be invoked in the update
   * thread in order to update the SwitchState.
   *
   */
  bool updateState(std::unique_ptr<StateUpdate> update);

  /**
   * Schedule an update to the switch state.
   *
   * @param name  A name to identify the source of this update.  This is
   *              primarily used for logging and debugging purposes.
   * @param fn    The function that will prepare the new SwitchState.
   * @return bool whether the update was queued or not
   * The StateUpdateFn takes a single argument -- the current SwitchState
   * object to modify.  It should return a new SwitchState object, or null if
   * it decides that no update needs to be performed.
   *
   * Note that the update function will not be called immediately--it will be
   * invoked later from the update thread.  Therefore if you supply a lambda
   * with bound arguments, make sure that any bound arguments will still be
   * valid later when the function is invoked.  (e.g., Don't capture local
   * variables from your current call frame by reference.)
   *
   * The StateUpdateFn must not throw any exceptions.
   *
   * The update thread may choose to batch updates in some cases--if it has
   * multiple update functions to run it may run them all at once and only
   * send a single update notification to the HwSwitch and other update
   * subscribers.  Therefore the StateUpdateFn may be called with an
   * unpublished SwitchState in some cases.
   */
  bool updateState(folly::StringPiece name, StateUpdateFn fn);

  /**
   * Schedule an update to the switch state.
   *
   * @param name  A name to identify the source of this update.  This is
   *              primarily used for logging and debugging purposes.
   * @param fn    The function that will prepare the new SwitchState.
   *
   * A version of updateState that prevents the update handling queue from
   * coalescing this update with later updates. This should rarely be used,
   * but can be used when there is an update that MUST be seen by the hw
   * implementation, even if the inverse update is immediately applied.
   */
  void updateStateNoCoalescing(folly::StringPiece name, StateUpdateFn fn);

  /*
   * A version of updateState() that doesn't return until the update has been
   * applied.
   *
   * This should only be called in situations where it is safe to block the
   * current thread until the operation completes.
   *
   */
  void updateStateBlocking(folly::StringPiece name, StateUpdateFn fn);

  /*
   * A version of updateState() that reports back failures in applying state
   * to Hw. Such updates are marked as blocking, non coalescing and
   * transactional (if HwSwitch supports transactions). This will guarantee that
   * if desired state update could not be applied FbossHwUpdateError exception
   * will be thrown in the caller's thread.
   *
   * Note though that its upto the HwSwitch implementation to decide which state
   * upate failures it can protect against. For things HwSwitch does not protect
   * against it may just fail hard,
   *
   */
  void updateStateWithHwFailureProtection(
      folly::StringPiece name,
      StateUpdateFn fn);

  /**
   * Apply config from the config file (specified in 'config' flag).
   *
   *  @param  reason
   *          What is the reson for applying config. This will be printed for
   *          logging purposes.
   */
  void applyConfig(const std::string& reason, bool reload = false);
  /**
   * Apply passed in config - used primarily by tests.
   *
   *  @param  reason
   *          What is the reson for applying config. This will be printed for
   *          logging purposes.
   */
  void applyConfig(
      const std::string& reason,
      const cfg::SwitchConfig& newConfig);

  /*
   * Get config applied information which include last config applied time(ms).
   * NOTE: If no config has ever been applied, the default timestamp is 0.
   */
  ConfigAppliedInfo getConfigAppliedInfo() const {
    return *configAppliedInfo_.rlock();
  }

  /*
   * Registers an observer of all state updates. An observer will be notified of
   * all state updates that occur and all classes that care about state updates
   * should register using this api.
   *
   * The only required method for observers is stateUpdated and observers can
   * count on this always being called from the update thread.
   */
  void registerStateObserver(StateObserver* observer, const std::string name);
  void unregisterStateObserver(StateObserver* observer);

  /*
   * Signal to the switch that initial config is applied.
   * The switch may then use this to start certain functions
   * which make sense only after the initial config has been
   * applied. As an example it makes sense to start the packet
   * receive only after applying the initial config, else in
   * case of a warm boot this causes us to receive packets tagged
   * with a vlan which software switch state does not even know
   * about. OTOH in case of a cold boot it causes host entries
   * to be created in the wrong (default VLAN).
   */
  void initialConfigApplied(
      const std::chrono::steady_clock::time_point& startTime);
  /*
   * Get the SwitchStats for the current thread.
   *
   * This object should only be used from the current thread.  It should never
   * be stored and used in other threads.
   */
  SwitchStats* stats() {
    SwitchStats* s = stats_.get();
    if (s) {
      return s;
    }
    return createSwitchStats();
  }

  /**
   * Get all SwitchStats for all threads
   */
  AllThreadsSwitchStats getAllThreadsSwitchStats() {
    return stats_.accessAllThreads();
  }

  /*
   * Construct and destroy a client to dump packets to the packet distribution
   * service.
   */
  void constructPushClient(uint16_t port);

  void destroyPushClient();

  /*
   * Send a kill message to the distribution process.
   */
  void killDistributionProcess();

  /*
   * Check if the passed in state is valid.
   * For now we just check for the new port speeds being valid.
   * This could be extended as needed
   */
  bool isValidStateUpdate(const StateDelta& delta) const;

  /*
   * Get the PortStats for the specified port.
   *
   * Note that this returns a thread-local object specific to the current
   * thread.
   */
  PortStats* portStats(PortID port);

  /*
   * Get the InterfaceStats for the specified interface.
   *
   * Note that this returns a thread-local object specific to the current
   * thread.
   */
  InterfaceStats* interfaceStats(InterfaceID intf);

  /*
   * Get Internal PhyInfo
   */
  std::map<PortID, const phy::PhyInfo> getIPhyInfo(
      std::vector<PortID>& portIDs);

  /*
   * Get PortStatus for all the ports.
   */
  std::map<int32_t, PortStatus> getPortStatus();

  /*
   * Get PortStatus of the specified port.
   */
  PortStatus getPortStatus(PortID port);

  /*
   * Get Product Information.
   */
  void getProductInfo(ProductInfo& productInfo) const;

  /*
   * Get the PortStats for the ingress port of this packet.
   */
  PortStats* portStats(const RxPacket* pkt);
  PortStats* portStats(const std::unique_ptr<RxPacket>& pkt) {
    return portStats(pkt.get());
  }

  /*
   * Get the EventBase for the background thread
   */
  folly::EventBase* getBackgroundEvb() {
    return &backgroundEventBase_;
  }

  /*
   * Get the EventBase over which LacpController and LacpMachines should execute
   */
  folly::EventBase* getLacpEvb() {
    return &lacpEventBase_;
  }

  /*
   * Get the EventBase for the update thread
   */
  folly::EventBase* getUpdateEvb() {
    return &updateEventBase_;
  }

  /*
   * Get the EventBase for Arp/Ndp Cache
   */
  folly::EventBase* getNeighborCacheEvb() {
    return &neighborCacheEventBase_;
  }

  /**
   * Do the packet received callback, and throw exception if there is an error
   * in the handling of packet.
   *
   * This is usually not called for regular packets. Useful for testing. For
   * normal calls, see packetReceived() callback from HwSwitch.
   */
  void packetReceivedThrowExceptionOnError(std::unique_ptr<RxPacket> pkt);

  // HwSwitch::Callback methods
  void packetReceived(std::unique_ptr<RxPacket> pkt) noexcept override;
  void linkStateChanged(
      PortID port,
      bool up,
      std::optional<phy::LinkFaultStatus> iPhyFaultStatus =
          std::nullopt) override;
  void pfcWatchdogStateChanged(
      const PortID& portId,
      const bool deadlockDetected) override;
  void l2LearningUpdateReceived(
      L2Entry l2Entry,
      L2EntryUpdateType l2EntryUpdateType) override;
  void exitFatal() const noexcept override;

  uint32_t getEthernetHeaderSize() const;

  /*
   * Allocate a new TxPacket.
   */
  std::unique_ptr<TxPacket> allocatePacket(uint32_t size);

  /**
   * Allocate a TxPacket, which is used to send out through HW
   *
   * The caller of the function provides the minimum size of space needed
   * in the packet to store L3 packet. The function adds L2 header size and
   * also makes sure packet size meeting minimum packet size.
   *
   * The IOBuf returned through the packet will have some headroom reserved
   * already which can be used to write L2 header. The caller is expected
   * to write the L3 contents starting from writableTail().
   *
   * @param l3Len L3 packet size
   * @return the unique pointer to a tx packet
   */
  std::unique_ptr<TxPacket> allocateL3TxPacket(uint32_t l3Len);

  /**
   * All FBOSS Network Control packets should use this API to send out
   */
  void sendNetworkControlPacketAsync(
      std::unique_ptr<TxPacket> pkt,
      std::optional<PortDescriptor> port) noexcept;

  void sendPacketOutOfPortAsync(
      std::unique_ptr<TxPacket> pkt,
      PortID portID,
      std::optional<uint8_t> queue = std::nullopt) noexcept;

  void sendPacketOutOfPortAsync(
      std::unique_ptr<TxPacket> pkt,
      AggregatePortID aggPortID,
      std::optional<uint8_t> queue = std::nullopt) noexcept;

  /*
   * Send a packet, using switching logic to send it out the correct port(s)
   * for the specified VLAN and destination MAC.
   */
  void sendPacketSwitchedAsync(std::unique_ptr<TxPacket> pkt) noexcept;

  /**
   * Send out L3 packet through HW
   *
   * The L3 packet is supposed to be stored starting from pkt->buf()->data(),
   * whose length is provided through pkt->buf()->length().
   *
   * The caller of the function has to make sure the IOBuf in the packet has
   * enough headroom (EthHdr::SIZE) to store the L2 header. Also the
   * IOBuf is bigger enough to hold a minimum size of packet (68). The packet
   * allocated by allocateL3TxPacket() is guaranteed for the above requirements.
   * If any of the above reuqirements is not met, the packet will be dropped.
   *
   * The function will prepend the L2 header to the L3 packet before it is
   * sent out. All (ipv6) link-local unicast and multicast packets are forwarded
   * onto VLAN corresponding to Host interface from which it is received.
   * Link-local communication doesn't go through L3 table lookups, hence it will
   * work regardless of L3 lookup tables state.
   *
   * @param pkt The packet that has L3 packet stored to send out
   */
  void sendL3Packet(
      std::unique_ptr<TxPacket> pkt,
      std::optional<InterfaceID> ifID = std::nullopt) noexcept;

  /**
   * method to send out a packet from HW to host.
   *
   * @return true The packet is sent to host
   *         false The packet is dropped due to errors
   */
  bool sendPacketToHost(InterfaceID dstIfID, std::unique_ptr<RxPacket> pkt);

  /**
   * Get the ArpHandler object.
   *
   * The ArpHandler returned is owned by the SwSwitch, and is only valid as
   * long as the SwSwitch object.
   */
  ArpHandler* getArpHandler() {
    return arp_.get();
  }

  /**
   * Get the IPv6Handler object.
   *
   * The IPv6Handler returned is owned by the SwSwitch, and is only valid as
   * long as the SwSwitch object.
   */
  IPv6Handler* getIPv6Handler() {
    return ipv6_.get();
  }

  /**
   * Get the NeighborUpdater object.
   *
   * The NeighborUpdater returned is owned by the SwSwitch, and is only valid as
   * long as the SwSwitch object.
   */
  NeighborUpdater* getNeighborUpdater() {
    return nUpdater_.get();
  }

  /*
   * Get the PktCaptureManager object.
   */
  PktCaptureManager* getCaptureMgr() {
    return pcapMgr_.get();
  }

  PacketObservers* getPacketObservers() {
    return pktObservers_.get();
  }

  /*
   * Get the LldpManager object
   */
  LldpManager* getLldpMgr() {
    return lldpManager_.get();
  }
#if FOLLY_HAS_COROUTINES
  /*
   *
   */
  MKAServiceManager* getMKAServiceMgr() {
    return mkaServiceManager_.get();
  }
#endif
  /*
   * Get the PacketLogger object
   */
  PacketLogger* getPacketLogger() {
    return packetLogger_.get();
  }

  /*
   * Get the RouteUpdateLogger object
   */
  RouteUpdateLogger* getRouteUpdateLogger() {
    return routeUpdateLogger_.get();
  }

  LinkAggregationManager* getLagManager() {
    return lagManager_.get();
  }

  ResolvedNexthopMonitor* getResolvedNexthopMonitor() {
    return resolvedNexthopMonitor_.get();
  }

  LookupClassUpdater* getLookupClassUpdater() {
    return lookupClassUpdater_.get();
  }

  LookupClassRouteUpdater* getLookupClassRouteUpdater() {
    return lookupClassRouteUpdater_.get();
  }

  /*
   * RIB and switch state need to be kept in sync,
   * so only expose const/non write access to rib
   * All writes to rib must go through SwSwitch or
   * through the well defined RouteUpdateWrapper
   * abstraction
   */
  RoutingInformationBase* getRib() const {
    return rib_.get();
  }

  SwSwitchRouteUpdateWrapper getRouteUpdater() {
    return SwSwitchRouteUpdateWrapper(this, rib_.get());
  }

  /*
   * Gets the flags the SwSwitch was initialized with.
   */
  SwitchFlags getFlags() const {
    return flags_;
  }

  /*
   * Allow hardware to perform any cleanup needed to gracefully restart the
   * agent before we exit application.
   */
  void gracefulExit();

  BootType getBootType() const {
    return bootType_;
  }

  /*
   * Helper function for enabling route update logging
   */
  void logRouteUpdates(
      const std::string& addr,
      uint8_t mask,
      const std::string& identifier);

  void stopLoggingRouteUpdates(const std::string& identifier);

  /*
   * Register a function that will send notifications about the port status.
   * Only one port status listener is supported, and calling this multiple
   * times will overwrite the current listener.
   */
  void registerNeighborListener(
      std::function<void(
          const std::vector<std::string>& added,
          const std::vector<std::string>& deleted)> callback);

  void invokeNeighborListener(
      const std::vector<std::string>& added,
      const std::vector<std::string>& deleted);

  /*
   * Returns true if the arp/ndp entry for the passed in ip has been hit.
   */
  bool getAndClearNeighborHit(RouterID vrf, folly::IPAddress ip);

  const std::string& getConfigStr() const {
    return curConfigStr_;
  }
  const cfg::SwitchConfig& getConfig() const {
    return curConfig_;
  }
  AdminDistance clientIdToAdminDistance(int clientId) const;
  void publishRxPacket(RxPacket* packet, uint16_t ethertype);
  void publishTxPacket(TxPacket* packet, uint16_t ethertype);

  /*
   * Clear PortStats of the specified port.
   */
  void clearPortStats(const std::unique_ptr<std::vector<int32_t>>& ports);

  std::vector<PrbsLaneStats> getPortAsicPrbsStats(int32_t portId);
  void clearPortAsicPrbsStats(int32_t portId);

  std::vector<PrbsLaneStats> getPortGearboxPrbsStats(
      int32_t portId,
      phy::Side side);
  void clearPortGearboxPrbsStats(int32_t portId, phy::Side side);
  SwitchRunState getSwitchRunState() const;

  std::vector<prbs::PrbsPolynomial> getPortPrbsPolynomials(
      int32_t /* portId */);
  prbs::InterfacePrbsState getPortPrbsState(PortID /* portId */);

  template <typename AddressT>
  std::shared_ptr<Route<AddressT>> longestMatch(
      std::shared_ptr<SwitchState> state,
      const AddressT& address,
      RouterID vrf);

  ResolvedNexthopProbeScheduler* getResolvedNexthopProbeScheduler() {
    return resolvedNexthopProbeScheduler_.get();
  }

  void setFibSyncTimeForClient(ClientID clientId);

  FsdbSyncer* fsdbSyncer() {
    return fsdbSyncer_.get();
  }
  /*
   * Public use only in tests
   */
  void stop(bool revertToMinAlpmState = false);

  void publishPhyInfoSnapshots(PortID portID) const;

  bool sendArpRequestHelper(
      std::shared_ptr<Interface> intf,
      std::shared_ptr<SwitchState> state,
      folly::IPAddressV4 source,
      folly::IPAddressV4 target);

  bool sendNdpSolicitationHelper(
      std::shared_ptr<Interface> intf,
      std::shared_ptr<SwitchState> state,
      const folly::IPAddressV6& target);

  std::map<std::string, HwTeFlowStats> getTeFlowStats();

 private:
  void updateStateBlockingImpl(
      folly::StringPiece name,
      StateUpdateFn fn,
      int stateUpdateBehavior);

  /*
   * Applied state corresponds to what was successfully applied
   * to h/w
   */
  std::shared_ptr<SwitchState> getAppliedState() const {
    std::unique_lock guard(stateLock_);
    return appliedStateDontUseDirectly_;
  }

  typedef folly::IntrusiveList<StateUpdate, &StateUpdate::listHook_>
      StateUpdateList;

  // Forbidden copy constructor and assignment operator
  SwSwitch(SwSwitch const&) = delete;
  SwSwitch& operator=(SwSwitch const&) = delete;

  /*
   * Update the current states.
   */
  void setStateInternal(std::shared_ptr<SwitchState> newAppliedState);

  void setDesiredState(std::shared_ptr<SwitchState> newDesiredState);

  void publishInitTimes(std::string name, const float& time);
  void updatePortInfo();
  void updateRouteStats();
  void updateTeFlowStats();
  void publishSwitchInfo(const HwInitResult& hwInitRet);
  void setSwitchRunState(SwitchRunState desiredState);
  SwitchStats* createSwitchStats();
  void handlePacket(std::unique_ptr<RxPacket> pkt);

  void updatePtpTcCounter();
  static void handlePendingUpdatesHelper(SwSwitch* sw);
  void handlePendingUpdates();
  std::shared_ptr<SwitchState> applyUpdate(
      const std::shared_ptr<SwitchState>& oldState,
      const std::shared_ptr<SwitchState>& newState,
      bool isTransaction);

  void startThreads();
  void stopThreads();
  void threadLoop(folly::StringPiece name, folly::EventBase* eventBase);

  /*
   * Helpers to add/remove state observers. These should only be
   * called from the update thread, if the update thread is running.
   */
  bool stateObserverRegistered(StateObserver* observer);
  void addStateObserver(StateObserver* observer, const std::string& name);
  void removeStateObserver(StateObserver* observer);

  /*
   * File where switch state gets dumped on exit
   */
  std::string getSwitchStateFile() const;

  void dumpBadStateUpdate(
      const std::shared_ptr<SwitchState>& oldState,
      const std::shared_ptr<SwitchState>& newState) const;

  /*
   * Notifies all the observers that a state update occured.
   */
  void notifyStateObservers(const StateDelta& delta);

  void logLinkStateEvent(PortID port, bool up);

  void logSwitchRunStateChange(
      SwitchRunState oldState,
      SwitchRunState newState);

  void onSwitchRunStateChange(SwitchRunState newState);

  // Sets the counter that tracks port status
  void setPortStatusCounter(PortID port, bool up);

  void updateConfigAppliedInfo();

  std::string curConfigStr_;
  cfg::SwitchConfig curConfig_;

  // The HwSwitch object.  This object is owned by the Platform.
  HwSwitch* hw_;
  std::unique_ptr<Platform> platform_;
  std::atomic<SwitchRunState> runState_{SwitchRunState::UNINITIALIZED};
  folly::ThreadLocalPtr<SwitchStats, SwSwitch> stats_;
  /**
   * The object to sync the interfaces to the system. This pointer could
   * be nullptr if interface sync is not enabled during init()
   */
  std::unique_ptr<TunManager> tunMgr_;

  /*
   * A list of pending state updates to be applied.
   */
  folly::SpinLock pendingUpdatesLock_;
  StateUpdateList pendingUpdates_;

  /*
   * The current switch state represented as :  appliedState,
   * as in  what is actually applied in the hardware.
   *
   *
   * BEWARE: You generally shouldn't access these states directly, even
   * internally within SwSwitch private methods.  These should only be accessed
   * while holding stateLock_.
   *
   * You almost certainly should call getAppliedState() setStateInternal()
   * instead of directly accessing appliedState
   *
   * This intentionally has an awkward name so people won't forget and try to
   * directly access this pointer.
   */
  std::shared_ptr<SwitchState> appliedStateDontUseDirectly_;
  mutable folly::SpinLock stateLock_;

  /*
   * A thread for performing various background tasks.
   */
  std::unique_ptr<std::thread> backgroundThread_;
  folly::EventBase backgroundEventBase_;
  std::shared_ptr<ThreadHeartbeat> bgThreadHeartbeat_;

  /*
   * A thread for processing packets received from
   * host (linux) that may need to be sent out of
   * ASIC front panel ports
   */
  std::unique_ptr<std::thread> packetTxThread_;
  folly::EventBase packetTxEventBase_;
  std::shared_ptr<ThreadHeartbeat> packetTxThreadHeartbeat_;

  /*
   * A thread for sending packets to the distribution process
   */
  std::shared_ptr<std::thread> pcapDistributionThread_;
  folly::EventBase pcapDistributionEventBase_;

  /*
   * A thread for processing SwitchState updates.
   */
  std::unique_ptr<std::thread> updateThread_;
  folly::EventBase updateEventBase_;
  std::shared_ptr<ThreadHeartbeat> updThreadHeartbeat_;

  /*
   * A thread dedicated to LACP processing.
   */
  std::unique_ptr<std::thread> lacpThread_;
  folly::EventBase lacpEventBase_;
  std::shared_ptr<ThreadHeartbeat> lacpThreadHeartbeat_;

  /*
   * A thread dedicated to Arp and Ndp cache entry processing.
   */
  std::unique_ptr<std::thread> neighborCacheThread_;
  folly::EventBase neighborCacheEventBase_;
  std::shared_ptr<ThreadHeartbeat> neighborCacheThreadHeartbeat_;

  /*
   * A thread dedicated to monitor above thread heartbeats
   */
  std::unique_ptr<ThreadHeartbeatWatchdog> heartbeatWatchdog_;

  /*
   * A callback for listening to neighbors coming and going.
   */
  std::mutex neighborListenerMutex_;
  std::function<void(
      const std::vector<std::string>& added,
      const std::vector<std::string>& deleted)>
      neighborListener_{nullptr};

  /*
   * The list of classes to notify on a state update. This container should only
   * be accessed/modified from the update thread. This removes the need for
   * locking when we access the container during a state update.
   */
  std::map<StateObserver*, std::string> stateObservers_;
  std::unique_ptr<PacketObservers> pktObservers_;

  std::unique_ptr<ArpHandler> arp_;
  std::unique_ptr<IPv4Handler> ipv4_;
  std::unique_ptr<IPv6Handler> ipv6_;
  std::unique_ptr<NeighborUpdater> nUpdater_;
  std::unique_ptr<PktCaptureManager> pcapMgr_;
  std::unique_ptr<MirrorManager> mirrorManager_;
  std::unique_ptr<MPLSHandler> mplsHandler_;
  std::unique_ptr<PacketLogger> packetLogger_;
  std::unique_ptr<RouteUpdateLogger> routeUpdateLogger_;
  std::unique_ptr<LinkAggregationManager> lagManager_;
  std::unique_ptr<ResolvedNexthopMonitor> resolvedNexthopMonitor_;
  std::unique_ptr<ResolvedNexthopProbeScheduler> resolvedNexthopProbeScheduler_;
  std::unique_ptr<RoutingInformationBase> rib_{nullptr};

  BootType bootType_{BootType::UNINITIALIZED};
  std::unique_ptr<LldpManager> lldpManager_;
  std::unique_ptr<PortUpdateHandler> portUpdateHandler_;
  SwitchFlags flags_{SwitchFlags::DEFAULT};

  std::unique_ptr<LookupClassUpdater> lookupClassUpdater_;
  std::unique_ptr<LookupClassRouteUpdater> lookupClassRouteUpdater_;
  std::unique_ptr<StaticL2ForNeighborObserver> staticL2ForNeighborObserver_;
  std::unique_ptr<MacTableManager> macTableManager_;
#if FOLLY_HAS_COROUTINES
  std::unique_ptr<MKAServiceManager> mkaServiceManager_;
#endif

  static constexpr auto kIphySnapshotIntervalSeconds = 1;
  // Collecting phy Info is currently inefficient on some platforms. Instead of
  // collecting them every second, tune down the frequency to only collect once
  // every update_phy_info_interval_s seconds (default to be 10).
  int phyInfoUpdateTime_{0};

  std::unique_ptr<PhySnapshotManager<kIphySnapshotIntervalSeconds>>
      phySnapshotManager_;
  std::unique_ptr<AclNexthopHandler> aclNexthopHandler_;
  std::unique_ptr<FsdbSyncer> fsdbSyncer_;
  std::unique_ptr<TeFlowNexthopHandler> teFlowNextHopHandler_;
  std::unique_ptr<DsfSubscriber> dsfSubscriber_;

  folly::Synchronized<ConfigAppliedInfo> configAppliedInfo_;
  std::optional<std::chrono::time_point<std::chrono::steady_clock>>
      publishedStatsToFsdbAt_;
};

} // namespace facebook::fboss
