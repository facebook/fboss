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

extern "C" {
#include <opennsl/types.h>
#include <opennsl/l3.h>
}

#include <folly/IPAddress.h>
#include <folly/MacAddress.h>
#include <folly/SpinLock.h>
#include <folly/dynamic.h>
#include "fboss/agent/hw/bcm/BcmEgress.h"
#include "fboss/agent/hw/bcm/BcmHostKey.h"
#include "fboss/agent/hw/bcm/BcmPort.h"
#include "fboss/agent/hw/bcm/BcmTrunk.h"
#include "fboss/agent/hw/bcm/PortAndEgressIdsMap.h"
#include "fboss/agent/state/NeighborEntry.h"
#include "fboss/agent/state/RouteNextHopEntry.h"
#include "fboss/agent/types.h"

#include <boost/container/flat_map.hpp>
#include <boost/container/flat_set.hpp>

namespace facebook { namespace fboss {

class BcmEcmpEgress;
class BcmEgress;
class BcmSwitchIf;
class BcmHostTable;

class BcmHost {
 public:
  BcmHost(const BcmSwitchIf* hw, BcmHostKey key)
      : hw_(hw), key_(std::move(key)) {}

  virtual ~BcmHost();
  bool isProgrammed() const {
    // Cannot use addedInHW_, as v6 link-local will not be added
    // to the HW. Instead, check the egressId_.
    return (egressId_ != BcmEgressBase::INVALID);
  }
  /*
   * program* apis get called only when for non host
   * route entries (which provide a L2 mapping for
   * a IP). Here we need to do 2 things
   * a) Add egress entry.
   * b) Add a host entry.
   * For host routes, we only need to do b) since we use
   * a already created egress entry
   */
  void program(opennsl_if_t intf, folly::MacAddress mac,
      opennsl_port_t port) {
    return program(intf, &mac, port, NEXTHOPS);
  }
  void programToCPU(opennsl_if_t intf) {
    return program(intf, nullptr, 0, TO_CPU);
  }
  void programToDrop(opennsl_if_t intf) {
    return program(intf, nullptr, 0, DROP);
  }
  void programToTrunk(
      opennsl_if_t intf,
      folly::MacAddress mac,
      opennsl_trunk_t trunk);

  void setEgressId(opennsl_if_t eid);

  opennsl_if_t getEgressId() const {
    return egressId_;
  }

  bool getAndClearHitBit() const;
  void addToBcmHostTable(bool isMultipath = false, bool replace = false);
  folly::dynamic toFollyDynamic() const;
  // TODO(samank): use getPort() instead of port_ member variable
  opennsl_port_t getPort() const { return port_; }
  bool isPortOrTrunkSet() const {
    return trunk_ != BcmTrunk::INVALID || port_ != 0;
  }
  opennsl_gport_t getSetPortAsGPort() {
    if (trunk_ != BcmTrunk::INVALID) {
      return BcmTrunk::asGPort(trunk_);
    } else {
      return BcmPort::asGPort(port_);
    }
  }

 private:
  // no copy or assignment
  BcmHost(BcmHost const &) = delete;
  BcmHost& operator=(BcmHost const &) = delete;
  void program(opennsl_if_t intf, const folly::MacAddress *mac,
               opennsl_port_t port, RouteForwardAction action);
  void initHostCommon(opennsl_l3_host_t *host) const;
  bool isTrunk() const;
  const BcmSwitchIf* hw_;
  BcmHostKey key_;
  // Port that the corresponding egress object references.
  // Only set for actual host entries that point a non
  // drop/CPU egress object. Set to 0 for host routes as
  // well
  opennsl_if_t port_{0};
  opennsl_trunk_t trunk_{BcmTrunk::INVALID};
  opennsl_if_t egressId_{BcmEgressBase::INVALID};
  bool addedInHW_{false}; // if added to the HW host(ARP) table or not
};

/**
 * Class to abstract ECMP path
 *
 * There are 2 use cases for BCM Ecmp host
 * a) As a collection of BcmHost entries - Unlike BcmHost, in this case
 * BcmEcmpHost does not have its own HW programming. It functions as
 * a SW object which refers to one or multiple BcmHost objects.
 * b) As a object representing a host route. In this case the BcmEcmpHost
 * simply references another egress entry (which maybe either BcmEgress
 * or BcmEcmpEgress).
 */
using BcmEcmpHostKey = std::pair<opennsl_vrf_t, RouteNextHopSet>;

class BcmEcmpHost {
 public:
  BcmEcmpHost(const BcmSwitchIf* hw, BcmEcmpHostKey key);
  virtual ~BcmEcmpHost();
  opennsl_if_t getEgressId() const {
    return egressId_;
  }
  opennsl_if_t getEcmpEgressId() const {
    return ecmpEgressId_;
  }
  folly::dynamic toFollyDynamic() const;
 private:
  const BcmSwitchIf* hw_;
  opennsl_vrf_t vrf_;
  /**
   * The egress ID for this ECMP host
   *
   * If there is only one entry in 'fwd', there will be one BcmHost object
   * created. The egress ID is that host's egress ID.
   * Otherwise, one BcmEcmpEgress object is created. The egress ID is the one
   * from this ECMP egress object. In the latter case, ecmpEgressId_ will also
   * be set and both egressId_ and ecmpEgressId_ will be that of the ecmp egress
   * object.
   */
  opennsl_if_t egressId_{BcmEgressBase::INVALID};
  opennsl_if_t ecmpEgressId_{BcmEgressBase::INVALID};
  RouteNextHopSet fwd_;
};

class BcmHostTable {
 public:
  explicit BcmHostTable(const BcmSwitchIf *hw);
  virtual ~BcmHostTable();

  // throw an exception if not found
  BcmHost* getBcmHost(const BcmHostKey& key) const;
  BcmEcmpHost* getBcmEcmpHost(const BcmEcmpHostKey& key) const;

  // return nullptr if not found
  BcmHost* getBcmHostIf(const BcmHostKey& key) const noexcept;
  BcmEcmpHost* getBcmEcmpHostIf(const BcmEcmpHostKey& key) const noexcept;

  /*
   * The following functions will modify the object. They rely on the global
   * HW update lock in BcmSwitch::lock_ for the protection.
   *
   * BcmHostTable maintains a reference counter for each BcmHost/BcmEcmpHost
   * entry allocated.
   */
  /**
   * Allocates a new BcmHost/BcmEcmpHost if no such one exists. For the existing
   * entry, incRefOrCreateBcmHost() will increase the reference counter by 1.
   *
   * When a new BcmHost is created, the programming to HW is not performed,
   * until explicit BcmHost::program() or BcmHost::programToCPU() is called.
   *
   * @return The BcmHost/BcmEcmpHost pointer just created or found.
   */
  BcmHost* incRefOrCreateBcmHost(const BcmHostKey& hostKey);
  BcmEcmpHost* incRefOrCreateBcmEcmpHost(const BcmEcmpHostKey& key);

  /**
   * Decrease an existing BcmHost/BcmEcmpHost entry's reference counter by 1.
   * Only until the reference counter is 0, the BcmHost/BcmEcmpHost entry
   * is deleted.
   *
   * @return nullptr, if the BcmHost/BcmEcmpHost entry is deleted
   * @retrun the BcmHost/BcmEcmpHost object that has reference counter
   *         decreased by 1, but the object is still valid as it is
   *         still referred in somewhere else
   */
  BcmHost* derefBcmHost(const BcmHostKey& key) noexcept;
  BcmEcmpHost* derefBcmEcmpHost(const BcmEcmpHostKey& key) noexcept;

  uint32_t getReferenceCount(const BcmHostKey& key) const noexcept;
  uint32_t getReferenceCount(const BcmEcmpHostKey& key) const noexcept;

  /*
   * APIs to manage egress objects. Multiple host entries can point
   * to a egress object. Lifetime of these egress objects is thus
   * managed via a reference count of hosts pointing to these
   * egress objects. Once the last host pointing to a egress object
   * goes away, the egress object is deleted.
   */
  void insertBcmEgress(std::unique_ptr<BcmEgressBase> egress);
  BcmEgressBase* incEgressReference(opennsl_if_t egressId);
  BcmEgressBase* derefEgress(opennsl_if_t egressId);
  const BcmEgressBase*  getEgressObjectIf(opennsl_if_t egress) const;
  BcmEgressBase* getEgressObjectIf(opennsl_if_t egress);

  /*
   * Port down handling
   * Look up egress entries going over this port and
   * then remove these from ecmp entries.
   * This is called from the linkscan callback and
   * we don't acquire BcmSwitch::lock_ here. See note above
   * declaration of BcmSwitch::linkStateChangedHwNotLocked which
   * explains why we can't hold this lock here.
   */
  void linkDownHwNotLocked(opennsl_port_t port) {
    linkStateChangedMaybeLocked(
        BcmPort::asGPort(port), false /*down*/, false /*not locked*/);
  }
  void trunkDownHwNotLocked(opennsl_trunk_t trunk) {
    linkStateChangedMaybeLocked(
        BcmTrunk::asGPort(trunk), false /*down*/, false /*not locked*/);
  }
  void linkDownHwLocked(opennsl_port_t port) {
    // Just call the non locked counterpart here.
    // We don't really need the lock for link down
    // handling
    linkDownHwNotLocked(port);
  }
  /*
   * link up handling. Only ever called
   * while holding hw lock.
   */
  void linkUpHwLocked(opennsl_port_t port) {
    linkStateChangedMaybeLocked(
        BcmPort::asGPort(port), true /*up*/, true /*locked*/);
  }
  /*
   * Update port to egressIds mapping
   */
  void updatePortToEgressMapping(
      opennsl_if_t egressId,
      opennsl_port_t oldPort,
      opennsl_port_t newPort);
  /*
   * Get port -> egressIds map
   */
  std::shared_ptr<PortAndEgressIdsMap> getPortAndEgressIdsMap() const {
    folly::SpinLockGuard guard(portAndEgressIdsLock_);
    return portAndEgressIdsDontUseDirectly_;
  }
  /*
   * Serialize toFollyDynamic
   */
  folly::dynamic toFollyDynamic() const;
  /*
   * Host entries from warm boot cache synced
   */
  void warmBootHostEntriesSynced();

  using EgressIdSet = BcmEcmpEgress::EgressIdSet;
  /*
   * Release all host entries. Should only
   * be called when we are about to reset/destroy
   * the host table
   */
  void releaseHosts() {
    ecmpHosts_.clear();
    hosts_.clear();
  }

  bool isResolved(const opennsl_if_t egressId) const {
    return resolvedEgresses_.find(egressId) != resolvedEgresses_.end();
  }
  void resolved(const opennsl_if_t egressId) {
    resolvedEgresses_.insert(egressId);
  }
  void unresolved(const opennsl_if_t egressId) {
    resolvedEgresses_.erase(egressId);
  }

  uint32_t numEcmpEgress() const {
    return numEcmpEgressProgrammed_;
  }

  void egressResolutionChangedHwLocked(
      const EgressIdSet& affectedEgressIds,
      BcmEcmpEgress::Action action);
  void egressResolutionChangedHwLocked(
      opennsl_if_t affectedPath,
      BcmEcmpEgress::Action action) {
    EgressIdSet affectedEgressIds;
    affectedEgressIds.insert(affectedPath);
    egressResolutionChangedHwLocked(affectedEgressIds, action);
  }

 private:
  /*
   * Called both while holding and not holding the hw lock.
   */
  void linkStateChangedMaybeLocked(opennsl_port_t port, bool up, bool locked);
  static void egressResolutionChangedHwNotLocked(
      int unit,
      const EgressIdSet& affectedEgressIds,
      bool up);
  // Callback for traversal in egressResolutionChangedHwNotLocked
  static int removeAllEgressesFromEcmpCallback(
      int unit,
      opennsl_l3_egress_ecmp_t* ecmp, // ecmp object being traversed
      int intfCount, // number of egresses in the ecmp group
      opennsl_if_t* intfArray, // array of egresses in the ecmp group
      void* userData); // egresses we intend to remove from the ecmp group
  void setPort2EgressIdsInternal(std::shared_ptr<PortAndEgressIdsMap> newMap);

  const BcmSwitchIf* hw_{nullptr};

  /*
   * The current port -> egressIds map.
   *
   * BEWARE: You generally shouldn't access this directly, even internally
   * within this class's private methods.  This should only be accessed while
   * holding port2EgressIdsLock_.  You almost certainly should call
   * getPort2EgressIds() or setPort2EgressIdsInternal() instead of directly
   * accessing this.
   *
   * This intentionally has an awkward name so people won't forget and try to
   * directly access this pointer.
   */
  std::shared_ptr<PortAndEgressIdsMap> portAndEgressIdsDontUseDirectly_;
  mutable folly::SpinLock portAndEgressIdsLock_;
  boost::container::flat_set<opennsl_if_t> resolvedEgresses_;
  uint32_t numEcmpEgressProgrammed_{0};

  boost::container::flat_map<
      opennsl_if_t,
      std::pair<std::unique_ptr<BcmEgressBase>, uint32_t>>
      egressMap_;

  template <typename KeyT, typename HostT>
  using HostMap = boost::container::
    flat_map<KeyT, std::pair<std::unique_ptr<HostT>, uint32_t>>;
  template <typename KeyT, typename HostT>
  HostT* incRefOrCreateBcmHostImpl(
      HostMap<KeyT, HostT>* map,
      const KeyT& key);
  template <typename KeyT, typename HostT>
  HostT* getBcmHostIfImpl(
      const HostMap<KeyT, HostT>* map,
      const KeyT& key) const noexcept;
  template <typename KeyT, typename HostT>
  HostT* derefBcmHostImpl(
      HostMap<KeyT, HostT>* map,
      const KeyT& key) noexcept;
  template <typename KeyT, typename HostT>
  uint32_t getReferenceCountImpl(
      const HostMap<KeyT, HostT> *map,
      const KeyT& key) const noexcept;

  HostMap<BcmHostKey, BcmHost> hosts_;
  HostMap<BcmEcmpHostKey, BcmEcmpHost> ecmpHosts_;
};

}}
