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
#include "fboss/agent/types.h"
#include "fboss/agent/gen-cpp/switch_config_types.h"
#include "fboss/agent/hw/bcm/gen-cpp2/packettrace_types.h"
#include <folly/dynamic.h>

#include <memory>
#include <mutex>
#include <boost/container/flat_map.hpp>

extern "C" {
#include <opennsl/port.h>
#include <opennsl/rx.h>
#include <opennsl/types.h>
}

namespace facebook { namespace fboss {

struct AclEntryID;
class AclEntry;
class ArpEntry;
class BcmCosManager;
class BcmEgress;
class BcmHostTable;
class BcmIntfTable;
class BcmPlatform;
class BcmPortTable;
class BcmRouteTable;
class BcmRxPacket;
class BcmSwitchEventManager;
class BcmUnit;
class BcmWarmBootCache;
class MockRxPacket;
class Interface;
class Port;
class PortStats;
class Vlan;
class VlanMap;
/*
 * Map AclEntryId to opennsl_field_entry_t
 * opennsl_field_entry_t is not opensourced. Using int for now.
 */
using BcmAclTable =
  boost::container::flat_map<AclEntryID, int>;

/*
 * BcmSwitch is a HwSwitch implementation for systems that use a single
 * Broadcom ASIC.
 */
class BcmSwitch : public HwSwitch {
 public:
   enum HashMode {
     FULL_HASH, // Full hash - use src IP, dst IP, src port, dst port
     HALF_HASH  // Half hash - user src IP, dst IP
   };
  /*
   * Construct a new BcmSwitch.
   *
   * With this constructor, BcmSwitch will fully own the BCM SDK.
   * When init() is called, it will initialize the SDK, then find and
   * initialize the first switching ASIC.
   */
  explicit BcmSwitch(BcmPlatform* platform, HashMode hashMode=FULL_HASH);

  /*
   * Construct a new BcmSwitch for an existing BCM unit.
   *
   * This version assumes the BCM SDK has already been initialized, and uses
   * the specified BCM unit.  The BCM unit is not reset, but is used in its
   * current state.  Note that BcmSwitch::init() must still be called.
   */
  BcmSwitch(BcmPlatform *platform, std::unique_ptr<BcmUnit> unit);

  ~BcmSwitch() override;

  /*
   * Release the BcmUnit used by this BcmSwitch.
   *
   * This returns the BcmUnit used by this switch.  This can be used to
   * destroy the BcmSwitch without also detaching the underlying BcmUnit.
   * Once this method has called no other BcmSwitch methods should be accessed
   * before destroying the BcmSwitch object.
   */
  std::unique_ptr<BcmUnit> releaseUnit();

  /*
   * Initialize the BcmSwitch.
   */
  HwInitResult init(Callback* callback) override;

  void unregisterCallbacks() override;

  void remedyPorts() override;

  BcmPlatform* getPlatform() const {
    return platform_;
  }

  std::unique_ptr<TxPacket> allocatePacket(uint32_t size) override;
  bool sendPacketSwitched(std::unique_ptr<TxPacket> pkt) noexcept override;
  bool sendPacketOutOfPort(std::unique_ptr<TxPacket> pkt,
                           PortID portID) noexcept override;
  std::unique_ptr<PacketTraceInfo> getPacketTrace(
      std::unique_ptr<MockRxPacket> pkt);

  bool isRxThreadRunning();

  int getUnit() const {
    return unit_;
  }

  static opennsl_vrf_t getBcmVrfId(RouterID routerId) {
    static_assert(sizeof(opennsl_vrf_t) == sizeof(RouterID),
                  "Size of opennsl_vrf_t must equal to size of RouterID");
    return static_cast<opennsl_vrf_t>(routerId);
  }
  static RouterID getRouterId(opennsl_vrf_t vrf) {
    return RouterID(vrf);
  }
  static opennsl_if_t getBcmIntfId(InterfaceID intfId) {
    static_assert(sizeof(opennsl_if_t) == sizeof(InterfaceID),
        "Size of opennsl_if_t must equal to size of InterfaceID");
    return static_cast<opennsl_if_t>(intfId);
  }
  static VlanID getVlanId(opennsl_vlan_t vlan) {
    return VlanID(vlan);
  }
  const BcmPortTable* getPortTable() const {
    return portTable_.get();
  }
  const BcmIntfTable* getIntfTable() const {
    return intfTable_.get();
  }
  const BcmHostTable* getHostTable() const {
    return hostTable_.get();
  }
  bool isPortUp(PortID port) const override;

  opennsl_if_t getDropEgressId() const;
  opennsl_if_t getToCPUEgressId() const;

  // The following function will modify the object.
  // Lock has to be performed in the function.
  void stateChanged(const StateDelta& delta) override;

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

  /*
   * Start any services that make sense only
   * after initial configuration has been applied
   */
  void initialConfigApplied() override;
  /*
   * Signal to warm boot cache that this
   * owner is done programming the h/w and we
   * should remove any unclaimed entries that
   * just have owner as their last remaining
   * owner
   */
  void clearWarmBootCache() override;
  /*
   * Update all statistics.
   */
  void updateStats(SwitchStats* switchStats) override;

  /*
   * Get Broadcom-specific samplers.
   *
   * @return     The number of requested counters we can handle.
   * @param[out] samplers         A vector of high-resolution samplers.  We will
   *                              append new samplers to this list.
   * @param[in]  namespaceString  A string respresentation of the current
   *                              counter namespace.
   * @param[in]  counterSet       The set of requested Broadcom counters.
   */
  int getHighresSamplers(
      HighresSamplerList* samplers,
      const folly::StringPiece namespaceString,
      const std::set<folly::StringPiece>& counterSet) override;

  BcmCosManager* getCosMgr() const {
    return cosManager_.get();
  };

  void fetchL2Table(std::vector<L2EntryThrift> *l2Table) override;

  BcmHostTable* writableHostTable() const { return hostTable_.get(); }
  BcmWarmBootCache* getWarmBootCache() const {
    return warmBootCache_.get();
  }

  /**
   * Log the hardware state for the switch
   */
  void dumpState() const;

  /*
   * Handle a fatal crash.
   */
  void exitFatal() const override;

  /*
   * Returns true if the neighbor entry for the passed in ip
   * has been hit.
   */
  bool getAndClearNeighborHit(RouterID vrf,
                              folly::IPAddress& ip) override;

  cfg::PortSpeed getPortSpeed(PortID port) const override;
  cfg::PortSpeed getMaxPortSpeed(PortID port) const override;

  bool isValidStateUpdate(const StateDelta& delta) const override;

 private:
  enum Flags : uint32_t {
    RX_REGISTERED = 0x01,
    LINKSCAN_REGISTERED = 0x02,
  };
  // Forbidden copy constructor and assignment operator
  BcmSwitch(BcmSwitch const &) = delete;
  BcmSwitch& operator=(BcmSwitch const &) = delete;

  /*
   * Get default state switch is in on a cold boot
   */
  std::shared_ptr<SwitchState> getColdBootSwitchState() const;
  /*
   * Get state switch is in on a warm boot. This is not
   * quite the complete state yet, t4155406 is filed for
   * building complete state.
   */
  std::shared_ptr<SwitchState> getWarmBootSwitchState() const;

  std::unique_ptr<BcmRxPacket> createRxPacket(opennsl_pkt_t* pkt);
  void changeDefaultVlan(VlanID id);

  void processChangedVlan(const std::shared_ptr<Vlan>& oldVlan,
                          const std::shared_ptr<Vlan>& newVlan);
  void processAddedVlan(const std::shared_ptr<Vlan>& vlan);
  void preprocessRemovedVlan(const std::shared_ptr<Vlan>& vlan);
  void processRemovedVlan(const std::shared_ptr<Vlan>& vlan);

  void processChangedIntf(const std::shared_ptr<Interface>& oldIntf,
                          const std::shared_ptr<Interface>& newIntf);
  void processAddedIntf(const std::shared_ptr<Interface>& intf);
  void processRemovedIntf(const std::shared_ptr<Interface>& intf);

  template<typename DELTA>
  void processNeighborEntryDelta(const DELTA& delta);
  void processArpChanges(const StateDelta& delta);

  void processDisabledPorts(const StateDelta& delta);
  void processEnabledPorts(const StateDelta& delta);
  void processChangedPorts(const StateDelta& delta);
  void reconfigurePortGroups(const StateDelta& delta);

  template <typename RouteT>
  void processChangedRoute(
      const RouterID id, const std::shared_ptr<RouteT>& oldRoute,
      const std::shared_ptr<RouteT>& newRoute);
  template <typename RouteT>
  void processAddedRoute(
      const RouterID id, const std::shared_ptr<RouteT>& route);
  template <typename RouteT>
  void processRemovedRoute(
      const RouterID id, const std::shared_ptr<RouteT>& route);
  void processRemovedRoutes(const StateDelta& delta);
  void processAddedChangedRoutes(const StateDelta& delta);

  void processAclChanges(const StateDelta& delta);
  void processChangedAcl(const std::shared_ptr<AclEntry>& oldAcl,
                          const std::shared_ptr<AclEntry>& newAcl);
  void processAddedAcl(const std::shared_ptr<AclEntry>& acl);
  void processRemovedAcl(const std::shared_ptr<AclEntry>& acl);

  void stateChangedImpl(const StateDelta& delta);

  /*
   * Calls linkStateChanged below
   */
  static void linkscanCallback(int unit,
                               opennsl_port_t port,
                               opennsl_port_info_t* info);
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
   * Back traces from deadlocked process here https://phabricator.fb.com/P20042479
   */
  void linkStateChangedHwNotLocked(opennsl_port_t port,
      opennsl_port_info_t* info);

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
  static opennsl_rx_t packetRxCallback(int unit, opennsl_pkt_t* pkt,
      void* cookie);
  /*
   * Private callback called by BcmSwitch::packetRxCallback. Dispatches to
   * callback_->packetReceived.
   */
  opennsl_rx_t packetReceived(opennsl_pkt_t* pkt) noexcept;

  /*
   * Update thread-local switch statistics.
   */
  void updateThreadLocalSwitchStats(SwitchStats *switchStats);

  /*
   * Update thread-local per-port statistics.
   */
  void updateThreadLocalPortStats(PortID portID, PortStats *portStats);

  /*
   * Update global statistics.
   */
  void updateGlobalStats();

  /**
   * ECMP hash setup
   */
  void ecmpHashSetup();

  /*
   * Drop IPv6 Router Advertisements.
   */
  void dropIPv6RAs();

  /*
   * Drop DHCP packets that are sent to us.
   */
  void dropDhcpPackets();

  /*
   * Create ACL group
   */
  void createAclGroup();

  /*
   * Configure rate limiting of packets sent to the CPU.
   */
  void configureRxRateLimiting();

  /*
   * Configures any additional ecmp hash sets if applicable.
   */
  void configureAdditionalEcmpHashSets();

  /*
   * Disable linkscan thread. This should only be done on shutdown.
   */
  void disableLinkscan();


  /**
   * Runs a diag cmd on the corresponding unit
   */
  void printDiagCmd(const std::string& cmd) const;

  /**
   * Exports the sdk version we build against.
   */
  void exportSdkVersion() const;

  void initFieldProcessor(bool isWarmBoot) const;

  /**
   * Setup COS manager
   */
  void setupCos();

  /*
   * Cos Q mapping for packets destined to us.
   * Most often these are matched by next hop self
   * as the RX reason and we put it into the right
   * Cos Queue. However if these packets have their
   * TTL set to 1, that gets precedence and such packets
   * destined to us get put in low pri cos queue. Fix that.
   */
   void configureCosQMappingForLocalInterfaces(const StateDelta& delta) const;

   /*
    * Check if state, speed update for this port port would
    * be permissible in the hardware.
    * Right now we check for valid speed and port state update
    * given the constraints of lanes on the physical
    * port group/QSFP. More checks can be added as needed.
    */
  bool isValidPortUpdate(const std::shared_ptr<Port>& oldPort,
    const std::shared_ptr<Port>& newPort,
    const std::shared_ptr<SwitchState>& newState) const;

  /*
   * Member variables
   */
  BcmPlatform* platform_{nullptr};
  Callback* callback_{nullptr};
  std::unique_ptr<BcmUnit> unitObject_;
  int unit_{-1};
  uint32_t flags_{0};
  HashMode hashMode_;
  std::unique_ptr<BcmPortTable> portTable_;
  std::unique_ptr<BcmEgress> toCPUEgress_;
  std::unique_ptr<BcmIntfTable> intfTable_;
  std::unique_ptr<BcmHostTable> hostTable_;
  std::unique_ptr<BcmRouteTable> routeTable_;
  std::unique_ptr<BcmAclTable> aclTable_;
  std::unique_ptr<BcmWarmBootCache> warmBootCache_;
  std::unique_ptr<BcmSwitchEventManager> switchEventManager_;
  std::unique_ptr<BcmCosManager> cosManager_;
  /*
   * Lock to synchronize access to all BCM* data structures
   */
  std::mutex lock_;
};

}} // facebook::fboss
