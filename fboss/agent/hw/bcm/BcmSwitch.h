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

#include <folly/dynamic.h>
#include <folly/io/async/EventBase.h>
#include <gtest/gtest_prod.h>
#include <optional>
#include "fboss/agent/HwSwitch.h"
#include "fboss/agent/L2Entry.h"
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/hw/bcm/BcmPlatform.h"
#include "fboss/agent/types.h"

#include <boost/container/flat_map.hpp>
#include <memory>
#include <mutex>
#include <thread>

extern "C" {
#include <bcm/error.h>
#include <bcm/l2.h>
#include <bcm/port.h>
#include <bcm/rx.h>
#include <bcm/types.h>
}

namespace facebook::fboss {

class AclEntry;
class AggregatePort;
class ArpEntry;
class BcmAclTable;
class BcmControlPlane;
class BcmCosManager;
class BcmEgress;
class BcmEgressManager;
class BcmHostKey;
class BcmHostTable;
class BcmIntf;
class BcmIntfTable;
class BcmL3NextHop;
class BcmLabeledHostKey;
class BcmMplsNextHop;
class BcmMultiPathNextHopTable;
class BcmNeighborTable;
template <class K, class V>
class BcmNextHopTable;
class BcmPortTable;
class BcmQosPolicyTable;
class BcmRouteTable;
class BcmRxPacket;
class BcmStatUpdater;
class BcmSwitchEventCallback;
class BcmTrunkTable;
class BcmUnit;
class BcmWarmBootCache;
class BcmWarmBootHelper;
class BcmRtag7LoadBalancer;
class BcmSflowExporterTable;
class LabelForwardingEntry;
class LoadBalancer;
class PacketTraceInfo;
class SflowCollector;
class MockRxPacket;
class Interface;
class Port;
class PortStats;
class QosPolicy;
class Vlan;
class VlanMap;
class Mirror;
class BcmMirror;
class BcmMirrorTable;
class ControlPlane;
class BcmBstStatsMgr;
class BcmLabelMap;
class BcmSwitchSettings;
class BcmMacTable;
class PortQueue;
class BcmQcmManager;

/*
 * Virtual interface to BcmSwitch, primarily for mocking/testing
 */
class BcmSwitchIf : public HwSwitch {
 public:
  using HwSwitch::HwSwitch;
  /*
   * Flush internal tables w/o but remain attached to ASIC/Unit
   */
  virtual void resetTables() = 0;

  virtual BcmPlatform* getPlatform() const = 0;

  virtual std::unique_ptr<PacketTraceInfo> getPacketTrace(
      std::unique_ptr<MockRxPacket> pkt) const = 0;

  virtual bool isRxThreadRunning() const = 0;

  virtual int getUnit() const = 0;

  virtual const BcmPortTable* getPortTable() const = 0;
  virtual BcmPortTable* writablePortTable() const = 0;

  virtual const BcmIntfTable* getIntfTable() const = 0;

  virtual const BcmHostTable* getHostTable() const = 0;

  virtual const BcmEgressManager* getEgressManager() const = 0;

  virtual const BcmNeighborTable* getNeighborTable() const = 0;

  virtual const BcmNextHopTable<BcmHostKey, BcmL3NextHop>* getL3NextHopTable()
      const = 0;

  virtual BcmNextHopTable<BcmHostKey, BcmL3NextHop>* writableL3NextHopTable()
      const = 0;

  virtual const BcmNextHopTable<BcmLabeledHostKey, BcmMplsNextHop>*
  getMplsNextHopTable() const = 0;

  virtual BcmNextHopTable<BcmLabeledHostKey, BcmMplsNextHop>*
  writableMplsNextHopTable() const = 0;

  virtual const BcmMultiPathNextHopTable* getMultiPathNextHopTable() const = 0;

  virtual BcmMultiPathNextHopTable* writableMultiPathNextHopTable() const = 0;

  virtual const BcmAclTable* getAclTable() const = 0;

  virtual const BcmQosPolicyTable* getQosPolicyTable() const = 0;

  virtual const BcmTrunkTable* getTrunkTable() const = 0;

  virtual BcmStatUpdater* getStatUpdater() const = 0;

  virtual BcmControlPlane* getControlPlane() const = 0;

  virtual bcm_if_t getDropEgressId() const = 0;

  virtual bcm_if_t getToCPUEgressId() const = 0;

  virtual BcmCosManager* getCosMgr() const = 0;

  virtual BcmHostTable* writableHostTable() const = 0;

  virtual BcmEgressManager* writableEgressManager() const = 0;

  virtual BcmAclTable* writableAclTable() const = 0;

  virtual BcmWarmBootCache* getWarmBootCache() const = 0;

  virtual const BcmMirrorTable* getBcmMirrorTable() const = 0;

  virtual BcmMirrorTable* writableBcmMirrorTable() const = 0;

  virtual BcmLabelMap* writableLabelMap() const = 0;

  virtual std::string gatherSdkState() const = 0;

  virtual void dumpState(const std::string& path) const = 0;

  virtual const BcmSwitchSettings* getSwitchSettings() const = 0;
};

/*
 * BcmSwitch is a HwSwitch implementation for systems that use a single
 * Broadcom ASIC.
 */
class BcmSwitch : public BcmSwitchIf {
 public:
  enum class MmuState { UNKNOWN, MMU_LOSSLESS, MMU_LOSSY };
  using HwSwitch::FeaturesDesired;
  /*
   * Construct a new BcmSwitch.
   *
   * With this constructor, BcmSwitch will fully own the BCM SDK.
   * When init() is called, it will initialize the SDK, then find and
   * initialize the first switching ASIC.
   */
  explicit BcmSwitch(
      BcmPlatform* platform,
      uint32_t featuresDesired =
          (FeaturesDesired::PACKET_RX_DESIRED |
           FeaturesDesired::LINKSCAN_DESIRED));

  ~BcmSwitch() override;

  /*
   * Flush tables tracking ASIC state. Without releasing the ASIC/unit for
   * destruction/detaching. Primarily used for testing, where we flush state
   * and then try to recreate it by mimicking a warmboot sequence
   */
  void resetTables() override;

  /*
   * Initialize the BcmSwitch.
   */
  HwInitResult init(Callback* callback) override;

  void runBcmScript(const std::string& filename) const;

  void unregisterCallbacks() override;

  BcmPlatform* getPlatform() const override {
    return platform_;
  }
  MmuState getMmuState() const {
    return mmuState_;
  }
  uint64_t getMMUCellBytes() const {
    return mmuCellBytes_;
  }
  uint64_t getMMUBufferBytes() const {
    return mmuBufferBytes_;
  }

  std::unique_ptr<TxPacket> allocatePacket(uint32_t size) const override;
  bool sendPacketSwitchedAsync(std::unique_ptr<TxPacket> pkt) noexcept override;
  bool sendPacketOutOfPortAsync(
      std::unique_ptr<TxPacket> pkt,
      PortID portID,
      std::optional<uint8_t> queue = std::nullopt) noexcept override;

  bool sendPacketSwitchedSync(std::unique_ptr<TxPacket> pkt) noexcept override;
  bool sendPacketOutOfPortSync(
      std::unique_ptr<TxPacket> pkt,
      PortID portID,
      std::optional<uint8_t> queue = std::nullopt) noexcept override;
  std::unique_ptr<PacketTraceInfo> getPacketTrace(
      std::unique_ptr<MockRxPacket> pkt) const override;

  bool isRxThreadRunning() const override;

  int getUnit() const override {
    return unit_;
  }

  static bcm_vrf_t getBcmVrfId(RouterID routerId) {
    static_assert(
        sizeof(bcm_vrf_t) == sizeof(RouterID),
        "Size of bcm_vrf_t must equal to size of RouterID");
    return static_cast<bcm_vrf_t>(routerId);
  }
  static RouterID getRouterId(bcm_vrf_t vrf) {
    return RouterID(vrf);
  }
  static bcm_if_t getBcmIntfId(InterfaceID intfId) {
    static_assert(
        sizeof(bcm_if_t) == sizeof(InterfaceID),
        "Size of bcm_if_t must equal to size of InterfaceID");
    return static_cast<bcm_if_t>(intfId);
  }
  static VlanID getVlanId(bcm_vlan_t vlan) {
    return VlanID(vlan);
  }
  const BcmPortTable* getPortTable() const override {
    return portTable_.get();
  }
  BcmPortTable* writablePortTable() const override {
    return portTable_.get();
  }
  const BcmIntfTable* getIntfTable() const override {
    return intfTable_.get();
  }
  const BcmHostTable* getHostTable() const override {
    return hostTable_.get();
  }
  const BcmEgressManager* getEgressManager() const override {
    return egressManager_.get();
  }
  const BcmNeighborTable* getNeighborTable() const override {
    return neighborTable_.get();
  }
  const BcmNextHopTable<BcmHostKey, BcmL3NextHop>* getL3NextHopTable()
      const override {
    return l3NextHopTable_.get();
  }
  BcmNextHopTable<BcmHostKey, BcmL3NextHop>* writableL3NextHopTable()
      const override {
    return l3NextHopTable_.get();
  }
  const BcmNextHopTable<BcmLabeledHostKey, BcmMplsNextHop>*
  getMplsNextHopTable() const override {
    return mplsNextHopTable_.get();
  }
  BcmNextHopTable<BcmLabeledHostKey, BcmMplsNextHop>* writableMplsNextHopTable()
      const override {
    return mplsNextHopTable_.get();
  }

  const BcmMultiPathNextHopTable* getMultiPathNextHopTable() const override {
    return multiPathNextHopTable_.get();
  }
  BcmMultiPathNextHopTable* writableMultiPathNextHopTable() const override {
    return multiPathNextHopTable_.get();
  }

  const BcmQosPolicyTable* getQosPolicyTable() const override {
    return qosPolicyTable_.get();
  }
  const BcmAclTable* getAclTable() const override {
    return aclTable_.get();
  }
  BcmStatUpdater* getStatUpdater() const override {
    return bcmStatUpdater_.get();
  }
  const BcmTrunkTable* getTrunkTable() const override {
    return trunkTable_.get();
  }

  const BcmSwitchSettings* getSwitchSettings() const override {
    return switchSettings_.get();
  }

  bool isPortUp(PortID port) const override;

  bcm_if_t getDropEgressId() const override;
  bcm_if_t getToCPUEgressId() const override;

  BcmControlPlane* getControlPlane() const override {
    return controlPlane_.get();
  }

  // The following function will modify the object. In particular, the return
  // state will be published (to indicate what has actually been applied). If
  // everything from delta was successfully applied, then the "new" state in
  // delta will be published.
  //
  // Lock has to be performed in the function.
  std::shared_ptr<SwitchState> stateChanged(const StateDelta& delta) override;

  /*
   * gracefulExit performs the requisite cleanup
   * for doing a warm boot next time around if
   * supported. Also signal our ability to do a
   * warm boot by creating warm boot file.
   *
   * Requires getting the lock since we don't want
   * state changes while we are calling cleanup
   * shutdown apis in the BCM sdk.
   */
  void gracefulExit(folly::dynamic& switchState) override;

  /*
   * BcmSwitch state as folly::dynamic
   * For now we only dump Host table.
   */
  folly::dynamic toFollyDynamic() const override;

  folly::F14FastMap<std::string, HwPortStats> getPortStats() const override;

  uint64_t getDeviceWatermarkBytes() const override;

  /*
   * Wrapper functions to register and unregister a BCM event callbacks.  These
   * just forward the call.
   */
  std::shared_ptr<BcmSwitchEventCallback> registerSwitchEventCallback(
      bcm_switch_event_t eventID,
      std::shared_ptr<BcmSwitchEventCallback> callback);
  void unregisterSwitchEventCallback(bcm_switch_event_t eventID);

  BcmCosManager* getCosMgr() const override {
    return cosManager_.get();
  };

  void fetchL2Table(std::vector<L2EntryThrift>* l2Table) const override;

  BcmHostTable* writableHostTable() const override {
    return hostTable_.get();
  }
  BcmEgressManager* writableEgressManager() const override {
    return egressManager_.get();
  }
  BcmAclTable* writableAclTable() const override {
    return aclTable_.get();
  }
  BcmWarmBootCache* getWarmBootCache() const override {
    return warmBootCache_.get();
  }

  BcmRouteTable* writableRouteTable() const {
    return routeTable_.get();
  }
  const BcmRouteTable* routeTable() const {
    return routeTable_.get();
  }

  const BcmMirrorTable* getBcmMirrorTable() const override {
    return mirrorTable_.get();
  }
  BcmMirrorTable* writableBcmMirrorTable() const override {
    return mirrorTable_.get();
  }

  BcmLabelMap* writableLabelMap() const override {
    return labelMap_.get();
  }

  std::string gatherSdkState() const override;

  /**
   * Log the hardware state for the switch
   */
  void dumpState(const std::string& path) const override;

  /**
   * Log the hardware state for the switch
   */
  void dumpDebugState(const std::string& path) const override {
    dumpState(path);
  }

  /**
   * Set the aging timer for MAC address aging
   */
  void setMacAging(std::chrono::seconds agingInterval);

  /*
   * Handle a fatal crash.
   */
  void exitFatal() const override;

  /*
   * Returns true if the neighbor entry for the passed in ip
   * has been hit.
   */
  bool getAndClearNeighborHit(RouterID vrf, folly::IPAddress& ip) override;

  bool getPortFECEnabled(PortID port) const override;

  cfg::PortSpeed getPortMaxSpeed(PortID port) const override;

  bool isValidStateUpdate(const StateDelta& delta) const override;

  bcm_gport_t getCpuGPort() const;
  /*
   * Calls linkStateChanged. Invoked by linkscan thread
   * on BCM ASIC as well as explicitly by tests.
   */
  static void
  linkscanCallback(int unit, bcm_port_t port, bcm_port_info_t* info);

  BootType getBootType() const override {
    return bootType_;
  }

  BcmBstStatsMgr* getBstStatsMgr() const {
    return bstStatsMgr_.get();
  }

  BcmQcmManager* getBcmQcmMgr() const {
    return qcmManager_.get();
  }

  /**
   * Runs a diag cmd on the corresponding unit
   */
  void printDiagCmd(const std::string& cmd) const;

  /*
   * Returns whether ALPM has been enabled via the sdk
   */
  bool isAlpmEnabled() const;
  /*
   * Clear statistics for a list of ports.
   */
  void clearPortStats(
      const std::unique_ptr<std::vector<int32_t>>& ports) override;

  std::vector<PrbsLaneStats> getPortAsicPrbsStats(int32_t portId) override;
  void clearPortAsicPrbsStats(int32_t portId) override;

  std::vector<PrbsLaneStats> getPortGearboxPrbsStats(
      int32_t portId,
      phy::Side side) override;
  void clearPortGearboxPrbsStats(int32_t portId, phy::Side side) override;
  /*
   * Friend tests. We want the abilty to test private methods
   * without comprimising encapsulation for code generally.
   * To that end make tests friends, but no one else
   */
  FRIEND_TEST(BcmTest, fpNoMissingOrQsetChangedGrpsPostInit);

  /*
   * Private callback called by the Broadcom API. Dispatches to
   * BcmSwitch::l2AddrUpdteReceived
   */
  static void l2LearningCallback(
      int unit,
      bcm_l2_addr_t* l2Addr,
      int operation,
      void* userData);

  /*
   * Private callback called by BcmSwitch::l2AddrCallBack. Dispatches to
   * callback_->l2LearningUpdateReceived
   */
  void l2LearningUpdateReceived(bcm_l2_addr_t* l2Addr, int operation) noexcept;

  /*
   * Callback handlers for bcm_l2_traverse that generate L2 table update
   * callbacks from software. These are used to maintain SwitchState's MAC
   * Table across warmboot, L2LearningMode changes etc.
   */
  static int addL2TableCb(int unit, bcm_l2_addr_t* l2Addr, void* userData);
  static int
  addL2TableCbForPendingOnly(int unit, bcm_l2_addr_t* l2Addr, void* userData);
  static int deleteL2TableCb(int unit, bcm_l2_addr_t* l2Addr, void* userData);

  bool isPortEnabled(PortID port) const;

 private:
  enum Flags : uint32_t { RX_REGISTERED = 0x01, LINKSCAN_REGISTERED = 0x02 };
  enum DeltaType { ADDED, REMOVED, CHANGED };

  // Forbidden copy constructor and assignment operator
  BcmSwitch(BcmSwitch const&) = delete;
  BcmSwitch& operator=(BcmSwitch const&) = delete;

  /*
   * Handle SwitchRunState changes
   */
  void switchRunStateChangedImpl(SwitchRunState newState) override;

  /*
   * Update all statistics.
   */
  void updateStatsImpl(SwitchStats* switchStats) override;

  /*
   * Get default state switch is in on a cold boot
   */
  std::shared_ptr<SwitchState> getColdBootSwitchState() const;
  std::shared_ptr<SwitchState> applyAndGetWarmBootSwitchState();

  void setupToCpuEgress();

  std::unique_ptr<BcmRxPacket> createRxPacket(bcm_pkt_t* pkt);
  void changeDefaultVlan(VlanID id);

  void processChangedVlan(
      const std::shared_ptr<Vlan>& oldVlan,
      const std::shared_ptr<Vlan>& newVlan);
  void processAddedVlan(const std::shared_ptr<Vlan>& vlan);
  void preprocessRemovedVlan(const std::shared_ptr<Vlan>& vlan);
  void processRemovedVlan(const std::shared_ptr<Vlan>& vlan);

  void processChangedIntf(
      const std::shared_ptr<Interface>& oldIntf,
      const std::shared_ptr<Interface>& newIntf);
  void processAddedIntf(const std::shared_ptr<Interface>& intf);
  void processRemovedIntf(const std::shared_ptr<Interface>& intf);

  void processNeighborDelta(
      const StateDelta& delta,
      std::shared_ptr<SwitchState>* appliedState,
      DeltaType optype);
  template <typename AddrT>
  void processNeighborTableDelta(
      const StateDelta& stateDelta,
      std::shared_ptr<SwitchState>* appliedState,
      DeltaType optype);

  template <typename DELTA, typename ParentClassT>
  void processNeighborEntryDelta(
      const DELTA& delta,
      std::shared_ptr<SwitchState>* appliedState);
  template <typename NeighborEntryT>
  void processAddedNeighborEntry(const NeighborEntryT* addedEntry);
  template <typename NeighborEntryT>
  void processChangedNeighborEntry(
      const NeighborEntryT* oldEntry,
      const NeighborEntryT* newEntry);
  template <typename NeighborEntryT>
  void processRemovedNeighborEntry(const NeighborEntryT* removedEntry);

  template <typename NeighborEntryT>
  void processAddedAndChangedNeighbor(
      const BcmHostKey& key,
      const BcmIntf* intf,
      const NeighborEntryT* entry);

  void processDisabledPorts(const StateDelta& delta);
  void processEnabledPorts(const StateDelta& delta);
  void processAddedPorts(const StateDelta& delta);
  void processChangedPorts(const StateDelta& delta);
  void pickupLinkStatusChanges(const StateDelta& delta);
  void reconfigurePortGroups(const StateDelta& delta);

  template <typename RouteT>
  void processChangedRoute(
      const RouterID& id,
      const std::shared_ptr<RouteT>& oldRoute,
      const std::shared_ptr<RouteT>& newRoute);
  template <typename RouteT>
  void processAddedRoute(
      const RouterID& id,
      const std::shared_ptr<RouteT>& route);
  template <typename RouteT>
  void processRemovedRoute(
      const RouterID id,
      const std::shared_ptr<RouteT>& route);
  void processRemovedRoutes(const StateDelta& delta);
  void processAddedChangedRoutes(
      const StateDelta& delta,
      std::shared_ptr<SwitchState>* appliedState);
  template <typename AddrT>
  void processRouteTableDelta(
      const StateDelta& delta,
      std::shared_ptr<SwitchState>* appliedState);
  template <typename AddrT>
  bool isRouteUpdateValid(const StateDelta& delta) const;
  void processRemovedFibRoutes(const StateDelta& delta);
  void processAddedChangedFibRoutes(
      const StateDelta& delta,
      std::shared_ptr<SwitchState>* appliedState);

  void processQosChanges(const StateDelta& delta);

  void processAclChanges(const StateDelta& delta);
  void processChangedAcl(
      const std::shared_ptr<AclEntry>& oldAcl,
      const std::shared_ptr<AclEntry>& newAcl);
  void processAddedAcl(const std::shared_ptr<AclEntry>& acl);
  void processRemovedAcl(const std::shared_ptr<AclEntry>& acl);

  void processAggregatePortChanges(const StateDelta& delta);
  void processChangedAggregatePort(
      const std::shared_ptr<AggregatePort>& oldAggPort,
      const std::shared_ptr<AggregatePort>& newAggPort);
  void processAddedAggregatePort(const std::shared_ptr<AggregatePort>& aggPort);
  void processRemovedAggregatePort(
      const std::shared_ptr<AggregatePort>& aggPort);

  void processLoadBalancerChanges(const StateDelta& delta);
  void processChangedLoadBalancer(
      const std::shared_ptr<LoadBalancer>& oldLoadBalancer,
      const std::shared_ptr<LoadBalancer>& newLoadBalancer);
  void processAddedLoadBalancer(
      const std::shared_ptr<LoadBalancer>& loadBalancer);
  void processRemovedLoadBalancer(
      const std::shared_ptr<LoadBalancer>& loadBalancer);

  std::shared_ptr<SwitchState> stateChangedImpl(const StateDelta& delta);

  void processSflowSamplingRateChanges(const StateDelta& delta);
  void processSflowCollectorChanges(const StateDelta& delta);
  void processChangedSflowCollector(
      const std::shared_ptr<SflowCollector>& oldCollector,
      const std::shared_ptr<SflowCollector>& newCollector);
  void processAddedSflowCollector(
      const std::shared_ptr<SflowCollector>& collector);
  void processRemovedSflowCollector(
      const std::shared_ptr<SflowCollector>& collector);

  void processControlPlaneChanges(const StateDelta& delta);

  void processMirrorChanges(const StateDelta& delta);

  void processAddedLabelForwardingEntry(
      const std::shared_ptr<LabelForwardingEntry>& addedEntry);
  void processRemovedLabelForwardingEntry(
      const std::shared_ptr<LabelForwardingEntry>& deletedEntry);
  void processChangedLabelForwardingEntry(
      const std::shared_ptr<LabelForwardingEntry>& oldEntry,
      const std::shared_ptr<LabelForwardingEntry>& newEntry);
  void processChangedLabelForwardingInformationBase(const StateDelta& delta);
  bool isValidLabelForwardingEntry(const LabelForwardingEntry* entry) const;

  void processSwitchSettingsChanged(const StateDelta& delta);

  void processMacTableChanges(const StateDelta& delta);

  /*
   * linkStateChangedHwNotLocked is in the call chain started by link scan
   * thread while invoking our link state handler. Link scan thread
   * holds its internal lock for this unit here (LC_LOCK(unit) - BcmUnitLock
   * from here on), while calling this. This in turn means we can't acquire
   * BcmSwitch::lock_ in this call back else we risk getting into a deadlock.
   * Consider, we want to apply a change to h/w and BcmSwitch::stateChanged
   * function gets called. stateChanged then acquires BcmSwitch::lock_ and calls
   * into Bcm code. At this point imagine a port comes up and linkscan
   * handler notices this and acquires BcmUnitLock and invokes our handler.
   * We would now try to acquire BcmSwitch::lock_ but block since update
   * thread holds that. Meanwhile BCM function called from update thread would
   * try to acquire BcmUnitLock and block since the link scan thread is
   * holding that lock.
   * Back traces from deadlocked process here
   * https://phabricator.fb.com/P20042479
   */
  void linkStateChangedHwNotLocked(bcm_port_t port, bool up);

  /*
   * For any actions that require a lock or might need to
   * be delayed, we do not perform those in linkStateChanged.
   * Acquiring locks in the link scan call back call chain
   * can lead to deadlocks (see comments above linkStateChangedHwNotLocked).
   * While waiting in the link scan call back can cause subsequent
   * link up/down notifications to be delayed, leading to packet
   * loss in case of back to back port up/down events.
   *
   */
  void linkStateChanged(PortID port, bool up);

  /*
   * Private callback called by the Broadcom API. Dispatches to
   * BcmSwitch::packetReceived.
   */
  static bcm_rx_t packetRxCallback(int unit, bcm_pkt_t* pkt, void* cookie);
  /*
   * Private callback called by BcmSwitch::packetRxCallback. Dispatches to
   * callback_->packetReceived.
   */
  bcm_rx_t packetReceived(bcm_pkt_t* pkt) noexcept;

  /*
   * Update global statistics.
   */
  void updateGlobalStats();

  /*
   * Drop IPv6 Router Advertisements.
   */
  void dropIPv6RAs();

  /*
   * Drop DHCP packets that are sent to us.
   * stopBufferStatCollection();
   */
  void dropDhcpPackets();

  /*
   * Process packets for which L3 MTU check fails
   */
  void setL3MtuFailPackets();

  /**
   * Copy IPv6 link local multicast packets to CPU
   */
  void copyIPv6LinkLocalMcastPackets();

  /*
   * Create ACL group
   */
  void createAclGroup();
  /*
   * Create slow protocols MAC group
   */
  void createSlowProtocolsGroup();
  /*
   * During warm boot, check for missing FP Groups or FP groups
   * for which QSETs changed.
   */
  bool haveMissingOrQSetChangedFPGroups() const;
  void setupFPGroups();

  /*
   * Forces a linkscan pass on the provided ports.
   */
  void forceLinkscanOn(bcm_pbmp_t ports);

  /*
   * Configure rate limiting of packets sent to the CPU.
   */
  void configureRxRateLimiting();

  /*
   * Stop linkscan thread. This should only be done on shutdown.
   */
  void stopLinkscanThread();

  /*
   * Handle a potential sFlow trapped packet.
   *
   * Returns true if it is only an sFlow sample.
   * Returns false if it is there for another reason as well.
   */
  bool handleSflowPacket(bcm_pkt_t* pkt) noexcept;

  /**
   * Exports the sdk version we build against.
   */
  void exportSdkVersion() const;

  void initFieldProcessor() const;

  void initMirrorModule() const;

  void initMplsModule() const;

  /**
   * Setup COS manager
   */
  void setupCos();

  /*
   * Setup linkscan unless its explicitly disabled via featuresDesired_ flag
   */
  void setupLinkscan();
  /*
   * Setup packet RX unless its explicitly disabled via featuresDesired_ flag
   */
  void setupPacketRx();

  /*
   * Check if state, speed update for this port port would
   * be permissible in the hardware.
   * Right now we check for valid speed and port state update
   * given the constraints of lanes on the physical
   * port group/QSFP. More checks can be added as needed.
   */
  bool isValidPortUpdate(
      const std::shared_ptr<Port>& oldPort,
      const std::shared_ptr<Port>& newPort,
      const std::shared_ptr<SwitchState>& newState) const;

  /* check that given port queue traffic classes are valid */
  bool isValidPortQueueUpdate(
      const std::vector<std::shared_ptr<PortQueue>>& oldPortQueueConfig,
      const std::vector<std::shared_ptr<PortQueue>>& newPortQueueConfig) const;

  /* check that feasible qos policy is associated with port */
  bool isValidPortQosPolicyUpdate(
      const std::shared_ptr<Port>& oldPort,
      const std::shared_ptr<Port>& newPort,
      const std::shared_ptr<SwitchState>& newState) const;

  MmuState queryMmuState() const;
  void exportDeviceBufferUsage();

  /*
   * Return true if any of the port's queue names changed, false otherwise.
   */
  bool isPortQueueNameChanged(
      const std::shared_ptr<Port>& oldPort,
      const std::shared_ptr<Port>& newPort);

  /*
   * Process changes to port queues
   */
  void processChangedPortQueues(
      const std::shared_ptr<Port>& oldPort,
      const std::shared_ptr<Port>& newPort);

  /*
   * Process enabled port queues
   */
  void processEnabledPortQueues(const std::shared_ptr<Port>& port);

  /*
   * Returns true if a CPU queue name has changed, false otherwise.
   */
  bool isControlPlaneQueueNameChanged(
      const std::shared_ptr<ControlPlane>& oldCPU,
      const std::shared_ptr<ControlPlane>& newCPU);

  /*
   * Process changes to control plane queues
   */
  void processChangedControlPlaneQueues(
      const std::shared_ptr<ControlPlane>& oldCPU,
      const std::shared_ptr<ControlPlane>& newCPU);

  void processChangedRxReasonToQueueEntries(
      const std::shared_ptr<ControlPlane>& oldCPU,
      const std::shared_ptr<ControlPlane>& newCPU);
  /*
   * Disable hotswap setting
   */
  void disableHotSwap() const;

  bool isL2EntryPending(const bcm_l2_addr_t* l2Addr);

  /*
   * Member variables
   */
  BcmPlatform* platform_{nullptr};
  Callback* callback_{nullptr};
  int unit_{-1};
  uint32_t flags_{0};
  MmuState mmuState_{MmuState::UNKNOWN};
  uint64_t mmuBufferBytes_{0};
  uint64_t mmuCellBytes_{0};
  std::unique_ptr<BcmWarmBootCache> warmBootCache_;
  std::unique_ptr<BcmPortTable> portTable_;
  std::unique_ptr<BcmEgress> toCPUEgress_;
  std::unique_ptr<BcmIntfTable> intfTable_;
  std::unique_ptr<BcmHostTable> hostTable_;
  std::unique_ptr<BcmEgressManager> egressManager_;
  std::unique_ptr<BcmNeighborTable> neighborTable_;
  std::unique_ptr<BcmNextHopTable<BcmHostKey, BcmL3NextHop>> l3NextHopTable_;
  std::unique_ptr<BcmNextHopTable<BcmLabeledHostKey, BcmMplsNextHop>>
      mplsNextHopTable_;
  std::unique_ptr<BcmMultiPathNextHopTable> multiPathNextHopTable_;
  std::unique_ptr<BcmLabelMap> labelMap_;
  std::unique_ptr<BcmRouteTable> routeTable_;
  std::unique_ptr<BcmQosPolicyTable> qosPolicyTable_;
  std::unique_ptr<BcmAclTable> aclTable_;
  std::unique_ptr<BcmStatUpdater> bcmStatUpdater_;
  std::unique_ptr<BcmCosManager> cosManager_;
  std::unique_ptr<BcmTrunkTable> trunkTable_;
  std::unique_ptr<BcmSflowExporterTable> sFlowExporterTable_;
  std::unique_ptr<BcmControlPlane> controlPlane_;
  std::unique_ptr<BcmRtag7LoadBalancer> rtag7LoadBalancer_;
  std::unique_ptr<BcmMirrorTable> mirrorTable_;
  std::unique_ptr<BcmBstStatsMgr> bstStatsMgr_;

  std::unique_ptr<std::thread> linkScanBottomHalfThread_;
  folly::EventBase linkScanBottomHalfEventBase_;

  std::unique_ptr<BcmSwitchSettings> switchSettings_;

  std::unique_ptr<BcmMacTable> macTable_;

  std::unique_ptr<BcmUnit> unitObject_;
  BootType bootType_{BootType::UNINITIALIZED};
  int64_t bstStatsUpdateTime_{0};
  std::unique_ptr<BcmQcmManager> qcmManager_;

  /*
   * Lock to synchronize access to all BCM* data structures
   */
  std::mutex lock_;
};

} // namespace facebook::fboss
