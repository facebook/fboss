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
#include <bcm/l3.h>
#include <bcm/types.h>
}

#include <folly/IPAddress.h>
#include <folly/MacAddress.h>
#include <folly/SpinLock.h>
#include <folly/dynamic.h>
#include "fboss/agent/hw/bcm/BcmEgress.h"
#include "fboss/agent/hw/bcm/BcmHostKey.h"
#include "fboss/agent/hw/bcm/BcmMultiPathNextHop.h"
#include "fboss/agent/hw/bcm/BcmPort.h"
#include "fboss/agent/hw/bcm/BcmPortDescriptor.h"
#include "fboss/agent/hw/bcm/BcmTrunk.h"
#include "fboss/agent/state/NeighborEntry.h"
#include "fboss/agent/types.h"
#include "fboss/lib/RefMap.h"

namespace facebook::fboss {

class BcmEcmpEgress;
class BcmEgress;
class BcmSwitchIf;
class BcmHostTable;

class BcmHostEgress : public boost::noncopyable {
 public:
  enum class BcmHostEgressType { OWNED, REFERENCED };

  explicit BcmHostEgress(bcm_if_t egressId)
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

  bcm_if_t getEgressId() const {
    CHECK_NE((ownedEgress_ != nullptr), (referencedEgressId_.has_value()));
    return ownedEgress_ ? ownedEgress_->getID() : referencedEgressId_.value();
  }

 private:
  BcmHostEgressType type_;
  std::unique_ptr<BcmEgressBase> ownedEgress_;
  std::optional<bcm_if_t> referencedEgressId_;
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
  void program(
      bcm_if_t intf,
      folly::MacAddress mac,
      bcm_port_t port,
      std::optional<cfg::AclLookupClass> classID = std::nullopt) {
    return program(intf, &mac, port, NEXTHOPS, classID);
  }
  void programToCPU(
      bcm_if_t intf,
      std::optional<cfg::AclLookupClass> classID = std::nullopt) {
    return program(intf, nullptr, 0, TO_CPU, classID);
  }
  void programToDrop(bcm_if_t intf) {
    return program(intf, nullptr, 0, DROP);
  }
  void programToTrunk(bcm_if_t intf, folly::MacAddress mac, bcm_trunk_t trunk);

  void setEgressId(bcm_if_t eid);

  bcm_if_t getEgressId() const {
    if (!egress_) {
      return BcmEgressBase::INVALID;
    }
    return egress_->getEgressId();
  }

  bool getAndClearHitBit() const;
  void addToBcmHostTable(bool isMultipath = false, bool replace = false);

  bool isPortOrTrunkSet() const {
    return egressPort_.has_value();
  }
  bcm_gport_t getSetPortAsGPort() const {
    if (!egressPort_) {
      return BcmPort::asGPort(0);
    }
    return egressPort_->asGPort();
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
  static int getLookupClassFromL3Host(const bcm_l3_host_t& host);
  static std::string l3HostToString(const bcm_l3_host_t& host);
  static bool matchLookupClass(
      const bcm_l3_host_t& newHost,
      const bcm_l3_host_t& existingHost);
  std::optional<BcmPortDescriptor> getEgressPortDescriptor() const;

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

  bool isAddedInHw() const {
    return addedInHW_;
  }

 private:
  // no copy or assignment
  BcmHost(BcmHost const&) = delete;
  BcmHost& operator=(BcmHost const&) = delete;
  void program(
      bcm_if_t intf,
      const folly::MacAddress* mac,
      bcm_port_t port,
      RouteForwardAction action,
      std::optional<cfg::AclLookupClass> classID = std::nullopt);

  void initHostCommon(bcm_l3_host_t* host) const;

  void setLookupClassToL3Host(bcm_l3_host_t* host) const;

  std::unique_ptr<BcmEgress> createEgress();
  const BcmSwitchIf* hw_;
  BcmHostKey key_;
  // Port that the corresponding egress object references.
  // Only set for actual host entries that point a non
  // drop/CPU egress object. Set to 0 for host routes as
  // well
  std::optional<BcmPortDescriptor> egressPort_;
  bool addedInHW_{false}; // if added to the HW host(ARP) table or not
  int lookupClassId_{0}; // DST Lookup Class
  RouteForwardAction action_{DROP};
  std::unique_ptr<BcmHostEgress> egress_;
};

class BcmHostTable {
 public:
  using HostRefMap = FlatRefMap<BcmHostKey, BcmHost>;
  using HostConstIterator = typename HostRefMap::MapType::const_iterator;
  explicit BcmHostTable(const BcmSwitchIf* hw);
  virtual ~BcmHostTable();

  // throw an exception if not found
  BcmHost* getBcmHost(const BcmHostKey& key) const;

  // return nullptr if not found
  BcmHost* getBcmHostIf(const BcmHostKey& key) const noexcept;

  int getNumBcmHost() const {
    return hosts_.size();
  }

  uint32_t getReferenceCount(const BcmHostKey& key) const noexcept;

  /*
   * Host entries from warm boot cache synced
   */
  void warmBootHostEntriesSynced();

  /*
   * Release all host entries. Should only
   * be called when we are about to reset/destroy
   * the host table
   */
  void releaseHosts() {
    hosts_.clear();
  }

  void programHostsToTrunk(
      const BcmHostKey& key,
      bcm_if_t intf,
      const MacAddress& mac,
      bcm_trunk_t trunk);
  void programHostsToPort(
      const BcmHostKey& key,
      bcm_if_t intf,
      const MacAddress& mac,
      bcm_port_t port,
      std::optional<cfg::AclLookupClass> classID = std::nullopt);
  void programHostsToCPU(
      const BcmHostKey& key,
      bcm_if_t intf,
      std::optional<cfg::AclLookupClass> classID = std::nullopt);

  std::shared_ptr<BcmHost> refOrEmplace(const BcmHostKey& key);

  HostConstIterator begin() const {
    return hosts_.begin();
  }

  HostConstIterator end() const {
    return hosts_.end();
  }

 private:
  const BcmSwitchIf* hw_{nullptr};

  HostRefMap hosts_;
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
  boost::container::flat_map<BcmHostKey, std::shared_ptr<BcmHost>>
      neighborHosts_;
};

} // namespace facebook::fboss
