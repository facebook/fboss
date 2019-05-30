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
#include "fboss/agent/hw/bcm/BcmMultiPathNextHop.h"
#include "fboss/agent/hw/bcm/BcmPort.h"
#include "fboss/agent/hw/bcm/BcmTrunk.h"
#include "fboss/agent/state/NeighborEntry.h"
#include "fboss/agent/types.h"

#include <boost/container/flat_map.hpp>
#include <boost/container/flat_set.hpp>

namespace facebook { namespace fboss {

class BcmEcmpEgress;
class BcmEgress;
class BcmSwitchIf;
class BcmHostTable;
class BcmHostReference;

class BcmHostEgress : public boost::noncopyable {
 public:
  enum class BcmHostEgressType { OWNED, REFERENCED };

  explicit BcmHostEgress(opennsl_if_t egressId)
      : type_(BcmHostEgressType::REFERENCED), referencedEgressId_(egressId) {}
  explicit BcmHostEgress(std::unique_ptr<BcmEgressBase> egress)
      : type_(BcmHostEgressType::OWNED), ownedEgress_(std::move(egress)) {}

  BcmHostEgressType type() const {
    return type_;
  }

  BcmEgressBase* getOwnedEgressPtr() const {
    CHECK(type_ == BcmHostEgressType::OWNED);
    return ownedEgress_.get();
  }

  opennsl_if_t getEgressId() const {
    CHECK_NE((ownedEgress_ != nullptr), (referencedEgressId_.hasValue()));
    return ownedEgress_
        ? ownedEgress_->getID() : referencedEgressId_.value();
  }

 private:
  BcmHostEgressType type_;
  std::unique_ptr<BcmEgressBase> ownedEgress_;
  folly::Optional<opennsl_if_t> referencedEgressId_;
};

class BcmHost {
 public:
  BcmHost(const BcmSwitchIf* hw, BcmHostKey key)
      : hw_(hw), key_(std::move(key)) {}

  virtual ~BcmHost();
  bool isProgrammed() const {
    // Cannot use addedInHW_, as v6 link-local will not be added
    // to the HW. Instead, check the egressId_.
    return (getEgressId() != BcmEgressBase::INVALID);
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
    if (!egress_) {
      return BcmEgressBase::INVALID;
    }
    return egress_->getEgressId();
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

  const BcmHostKey& getHostKey() const {
    return key_;
  }
  int getLookupClassId() const {
    return lookupClassId_;
  }
  void setLookupClassId(int lookupClassId) {
    lookupClassId_ = lookupClassId;
  }
  static int getLookupClassFromL3Host(const opennsl_l3_host_t& host);
  static std::string l3HostToString(const opennsl_l3_host_t& host);
  static bool matchLookupClass(
    const opennsl_l3_host_t& newHost,
    const opennsl_l3_host_t& existingHost);
  PortDescriptor portDescriptor() const;

  bool isProgrammedToDrop() const {
    return action_ == DROP;
  }
  bool isProgrammedToCPU() const {
    return action_ == TO_CPU;
  }
  bool isProgrammedToNextHops() const {
    return action_ == NEXTHOPS;
  }

  BcmEgress* getEgress() const {
    return (egress_ &&
            egress_->type() == BcmHostEgress::BcmHostEgressType::OWNED)
        ? static_cast<BcmEgress*>(egress_->getOwnedEgressPtr())
        : nullptr;
  }

 private:
  // no copy or assignment
  BcmHost(BcmHost const &) = delete;
  BcmHost& operator=(BcmHost const &) = delete;
  void program(opennsl_if_t intf, const folly::MacAddress *mac,
               opennsl_port_t port, RouteForwardAction action);
  void initHostCommon(opennsl_l3_host_t *host) const;
  bool isTrunk() const;

  void setLookupClassToL3Host(opennsl_l3_host_t* host) const;

  std::unique_ptr<BcmEgress> createEgress();
  const BcmSwitchIf* hw_;
  BcmHostKey key_;
  // Port that the corresponding egress object references.
  // Only set for actual host entries that point a non
  // drop/CPU egress object. Set to 0 for host routes as
  // well
  opennsl_if_t port_{0};
  opennsl_trunk_t trunk_{BcmTrunk::INVALID};
  bool addedInHW_{false}; // if added to the HW host(ARP) table or not
  int lookupClassId_{0}; // DST Lookup Class
  RouteForwardAction action_{DROP};
  std::unique_ptr<BcmHostEgress> egress_;
};

class BcmHostTable {
 public:
  template <typename KeyT, typename HostT>
  using HostMap = boost::container::
      flat_map<KeyT, std::pair<std::unique_ptr<HostT>, uint32_t>>;

  explicit BcmHostTable(const BcmSwitchIf* hw);
  virtual ~BcmHostTable();

  // throw an exception if not found
  BcmHost* getBcmHost(const BcmHostKey& key) const;
  BcmMultiPathNextHop* getBcmMultiPathNextHop(
      const BcmMultiPathNextHopKey& key) const;

  // return nullptr if not found
  BcmHost* getBcmHostIf(const BcmHostKey& key) const noexcept;
  BcmMultiPathNextHop* getBcmMultiPathNextHopIf(
      const BcmMultiPathNextHopKey& key) const noexcept;

  int getNumBcmHost() const {
    return hosts_.size();
  }
  int getNumBcmMultiPathNextHop() const {
    return ecmpHosts_.size();
  }

  /*
   * The following functions will modify the object. They rely on the global
   * HW update lock in BcmSwitch::lock_ for the protection.
   *
   * BcmHostTable maintains a reference counter for each
   * BcmHost/BcmMultiPathNextHop entry allocated.
   */
  /**
   * Allocates a new BcmHost/BcmMultiPathNextHop if no such one exists. For the
   * existing entry, incRefOrCreateBcmHost() will increase the reference counter
   * by 1.
   *
   * When a new BcmHost is created, the programming to HW is not performed,
   * until explicit BcmHost::program() or BcmHost::programToCPU() is called.
   *
   * @return The BcmHost/BcmMultiPathNextHop pointer just created or found.
   */
  BcmHost* incRefOrCreateBcmHost(const BcmHostKey& hostKey);
  BcmMultiPathNextHop* incRefOrCreateBcmMultiPathNextHop(
      const BcmMultiPathNextHopKey& key);

  /**
   * Decrease an existing BcmHost/BcmMultiPathNextHop entry's reference counter
   * by 1. Only until the reference counter is 0, the
   * BcmHost/BcmMultiPathNextHop entry is deleted.
   *
   * @return nullptr, if the BcmHost/BcmMultiPathNextHop entry is deleted
   * @retrun the BcmHost/BcmMultiPathNextHop object that has reference counter
   *         decreased by 1, but the object is still valid as it is
   *         still referred in somewhere else
   */
  BcmHost* derefBcmHost(const BcmHostKey& key) noexcept;
  BcmMultiPathNextHop* derefBcmMultiPathNextHop(
      const BcmMultiPathNextHopKey& key) noexcept;

  uint32_t getReferenceCount(const BcmHostKey& key) const noexcept;
  uint32_t getReferenceCount(const BcmMultiPathNextHopKey& key) const noexcept;

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


  void programHostsToTrunk(
      const BcmHostKey& key,
      opennsl_if_t intf,
      const MacAddress& mac,
      opennsl_trunk_t trunk);
  void programHostsToPort(
      const BcmHostKey& key,
      opennsl_if_t intf,
      const MacAddress& mac,
      opennsl_port_t port);
  void programHostsToCPU(const BcmHostKey& key, opennsl_if_t intf);

 private:
  const BcmSwitchIf* hw_{nullptr};

  uint32_t numEcmpEgressProgrammed_{0};

  boost::container::flat_map<
      opennsl_if_t,
      std::pair<std::unique_ptr<BcmEgressBase>, uint32_t>>
      egressMap_;

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
  HostMap<BcmMultiPathNextHopKey, BcmMultiPathNextHop> ecmpHosts_;
};

class BcmHostReference {
 public:
  ~BcmHostReference();

  BcmHost* getBcmHost();

  BcmMultiPathNextHop* getBcmMultiPathNextHop();

  opennsl_if_t getEgressId();

  static std::unique_ptr<BcmHostReference> get(BcmSwitch* hw, BcmHostKey key);

  static std::unique_ptr<BcmHostReference> get(
      BcmSwitch* hw,
      BcmMultiPathNextHopKey key);

  static std::unique_ptr<BcmHostReference>
  get(BcmSwitch* hw, opennsl_vrf_t vrf, RouteNextHopSet nexthops);

 private:
  BcmHostReference(BcmSwitch* hw, BcmHostKey key);
  BcmHostReference(BcmSwitch* hw, BcmMultiPathNextHopKey key);
  BcmHostReference(const BcmHostReference&) = delete;
  BcmHostReference(BcmHostReference&&) = delete;
  BcmHostReference& operator=(const BcmHostReference&) = delete;
  BcmHostReference& operator=(const BcmHostReference&&) = delete;

  BcmSwitch* hw_{nullptr};
  folly::Optional<BcmHostKey> hostKey_;
  folly::Optional<BcmMultiPathNextHopKey> ecmpHostKey_;
  BcmHost* host_{nullptr};
  BcmMultiPathNextHop* ecmpHost_{nullptr};
};

class BcmNeighborTable {
 public:
  explicit BcmNeighborTable(BcmSwitch* hw) : hw_(hw) {}
  BcmHost* registerNeighbor(const BcmHostKey& neighbor);
  BcmHost* unregisterNeighbor(const BcmHostKey& neighbor);
  BcmHost* getNeighbor(const BcmHostKey& neighbor) const;
  BcmHost* getNeighborIf(const BcmHostKey& neighbor) const;

 private:
  BcmSwitch* hw_;
  boost::container::flat_map<BcmHostKey, std::unique_ptr<BcmHostReference>>
      neighborHostReferences_;
};
}}
