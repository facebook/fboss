/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "BcmHost.h"
#include <string>
#include <iostream>

#include <folly/logging/xlog.h>
#include "fboss/agent/Constants.h"
#include "fboss/agent/hw/bcm/BcmEgress.h"
#include "fboss/agent/hw/bcm/BcmError.h"
#include "fboss/agent/hw/bcm/BcmIntf.h"
#include "fboss/agent/hw/bcm/BcmNextHop.h"
#include "fboss/agent/hw/bcm/BcmNextHopTable-defs.h"
#include "fboss/agent/hw/bcm/BcmPort.h"
#include "fboss/agent/hw/bcm/BcmSwitch.h"
#include "fboss/agent/hw/bcm/BcmWarmBootCache.h"
#include "fboss/agent/state/Interface.h"

namespace {
constexpr auto kIntf = "intf";
constexpr auto kPort = "port";
constexpr auto kNextHops = "nexthops";

struct BcmHostKeyComparator {
  using BcmHost = facebook::fboss::BcmHost;
  using BcmHostKey = facebook::fboss::BcmHostKey;
  using BcmLabeledHostKey = facebook::fboss::BcmLabeledHostKey;
  using BcmLabeledHostMap =
      facebook::fboss::BcmHostTable::HostMap<BcmLabeledHostKey, BcmHost>;

  bool operator()(
      const BcmLabeledHostMap::value_type& entry,
      const BcmHostKey& bcmHostkey) {
    return BcmHostKey(
               entry.first.getVrf(), entry.first.addr(), entry.first.intfID()) <
        bcmHostkey;
  }

  bool operator()(
      const BcmHostKey& bcmHostkey,
      const BcmLabeledHostMap::value_type& entry) {
    return bcmHostkey <
        BcmHostKey(
               entry.first.getVrf(), entry.first.addr(), entry.first.intfID());
  }
};
}

namespace facebook { namespace fboss {

std::ostream& operator<<(
    std::ostream& os,
    const facebook::fboss::BcmEcmpHostKey& key) {
  return os << "BcmEcmpHost: " << key.second << "@vrf " << key.first;
}

using std::unique_ptr;
using std::shared_ptr;
using folly::MacAddress;
using folly::IPAddress;

std::string BcmHost::l3HostToString(const opennsl_l3_host_t& host) {
  std::ostringstream os;
  os << "is v6: " << (host.l3a_flags & OPENNSL_L3_IP6 ? "yes" : "no")
    << ", is multipath: "
    << (host.l3a_flags & OPENNSL_L3_MULTIPATH ? "yes": "no")
    << ", vrf: " << host.l3a_vrf << ", intf: " << host.l3a_intf
    << ", lookupClass: " << getLookupClassFromL3Host(host);
  return os.str();
}

void BcmHost::setEgressId(opennsl_if_t eid) {
  if (eid == egressId_) {
    // This could happen for loopback interface route.
    // For example, for the loopback interface address, 1.1.1.1/32.
    // The route's nexthop is 1.1.1.1. We will first create a BcmHost for
    // the nexthop, 1.1.1.1, and assign the egress ID to this BcmHost.
    // Then, the interface route, 1.1.1.1/32, will be represented by the
    // same BcmHost and BcmHost::setEgressId() will be called with the
    // egress ID retrieved from the nexthop BcmHost, which is exactly the same
    // as the BcmHost object.
    return;
  }
  if (egressId_ != BcmEgressBase::INVALID) {
    hw_->writableHostTable()->derefEgress(egressId_);
  }
  hw_->writableHostTable()->incEgressReference(eid);
  XLOG(DBG3) << "set host object for " << key_.str() << " to @egress " << eid
             << " from @egress " << egressId_;
  egressId_ = eid;
}

void BcmHost::initHostCommon(opennsl_l3_host_t *host) const {
  opennsl_l3_host_t_init(host);
  const auto& addr = key_.addr();
  if (addr.isV4()) {
    host->l3a_ip_addr = addr.asV4().toLongHBO();
  } else {
    memcpy(&host->l3a_ip6_addr, addr.asV6().toByteArray().data(),
           sizeof(host->l3a_ip6_addr));
    host->l3a_flags |= OPENNSL_L3_IP6;
  }
  host->l3a_vrf = key_.getVrf();
  host->l3a_intf = getEgressId();
  setLookupClassToL3Host(host);
}

void BcmHost::addToBcmHostTable(bool isMultipath, bool replace) {
  if (addedInHW_ || key_.hasLabel()) {
    return;
  }
  const auto& addr = key_.addr();
  if (addr.isV6() && addr.isLinkLocal()) {
    // For v6 link-local BcmHost, do not add it to the HW table
    return;
  }

  opennsl_l3_host_t host;
  initHostCommon(&host);
  if (isMultipath) {
    host.l3a_flags |= OPENNSL_L3_MULTIPATH;
  }
  if (replace) {
    host.l3a_flags |= OPENNSL_L3_REPLACE;
  }

  bool needToAddInHw = true;
  const auto warmBootCache = hw_->getWarmBootCache();
  auto vrfIp2HostCitr = warmBootCache->findHost(key_.getVrf(), addr);
  if (vrfIp2HostCitr != warmBootCache->vrfAndIP2Host_end()) {
    // Lambda to compare if hosts are equivalent
    auto equivalent =
      [=] (const opennsl_l3_host_t& newHost,
          const opennsl_l3_host_t& existingHost) {
      // Compare the flags we care about, I have seen garbage
      // values set on actual non flag bits when reading entries
      // back on warm boot.
      bool flagsEqual = ((existingHost.l3a_flags & OPENNSL_L3_IP6) ==
          (newHost.l3a_flags & OPENNSL_L3_IP6) &&
          (existingHost.l3a_flags & OPENNSL_L3_MULTIPATH) ==
          (newHost.l3a_flags & OPENNSL_L3_MULTIPATH));
      return flagsEqual &&
             existingHost.l3a_vrf == newHost.l3a_vrf &&
             existingHost.l3a_intf == newHost.l3a_intf &&
             matchLookupClass(host, existingHost);
    };
    const auto& existingHost = vrfIp2HostCitr->second;
    if (equivalent(host, existingHost)) {
      XLOG(DBG1) << "Host entry for " << addr << " already exists";
      needToAddInHw = false;
    } else {
      XLOG(DBG1) << "Different host attributes, addr:" << addr
                 << ", existing: " << l3HostToString(existingHost)
                 << ", new: " << l3HostToString(host)
                 << ", need to replace the existing one";
      // make sure replace flag is set
      host.l3a_flags |= OPENNSL_L3_REPLACE;
    }
  }

  if (needToAddInHw) {
    XLOG(DBG3) << (host.l3a_flags & OPENNSL_L3_REPLACE ? "Replacing" : "Adding")
               << "host entry for : " << addr;
    auto rc = opennsl_l3_host_add(hw_->getUnit(), &host);
    bcmCheckError(rc, "failed to program L3 host object for ", key_.str(),
      " @egress ", getEgressId());
    XLOG(DBG3) << "Programmed L3 host object for " << key_.str() << " @egress "
               << getEgressId();
  }
  // make sure we clear warmboot cache after programming to HW
  if (vrfIp2HostCitr != warmBootCache->vrfAndIP2Host_end()) {
    warmBootCache->programmed(vrfIp2HostCitr);
  }
  addedInHW_ = true;
}

void BcmHost::program(opennsl_if_t intf, const MacAddress* mac,
                      opennsl_port_t port, RouteForwardAction action) {
  unique_ptr<BcmEgress> createdEgress{nullptr};
  BcmEgress* egressPtr{nullptr};
  const auto& addr = key_.addr();
  const auto vrf = key_.getVrf();
  // get the egress object and then update it with the new MAC
  if (egressId_ == BcmEgressBase::INVALID) {
    XLOG(DBG3) << "Host entry for " << key_.str()
               << " does not have an egress, create one.";
    createdEgress = createEgress();
    egressPtr = createdEgress.get();
  } else {
    egressPtr = dynamic_cast<BcmEgress*>(
      hw_->writableHostTable()->getEgressObjectIf(egressId_));
  }
  CHECK(egressPtr);
  if (mac) {
    egressPtr->programToPort(intf, vrf, addr, *mac, port);
  } else {
    if (action == DROP) {
      egressPtr->programToDrop(intf, vrf, addr);
    } else {
      egressPtr->programToCPU(intf, vrf, addr);
    }
  }
  if (createdEgress) {
    egressId_ = createdEgress->getID();
    hw_->writableHostTable()->insertBcmEgress(std::move(createdEgress));
  }

  // if no host was added already, add one pointing to the egress object
  if (!addedInHW_) {
    addToBcmHostTable();
  }

  XLOG(DBG1) << "Updating egress " << egressPtr->getID() << " from "
             << (isTrunk() ? "trunk port " : "physical port ")
             << (isTrunk() ? trunk_ : port_) << " to physical port " << port;

  // TODO(samank): port_ or trunk_ being set is used as a proxy for whether
  // egressId_ is in the set of resolved egresses. We should instead simply
  // consult the set of resolved egresses for this information.
  bool isSet = isPortOrTrunkSet();
  // If ARP/NDP just resolved for this host, we need to inform
  // ecmp egress objects about this egress Id becoming reachable.
  // Consider the case where a port went down, neighbor entry expires
  // and then the port came back up. When the neighbor entry expired,
  // we would have taken it out of the port->egressId mapping. Now even
  // when the port comes back up, we won't have that egress Id mapping
  // there and won't signal ecmp objects to add this back. So when
  // a egress object gets resolved, for all the ecmp objects that
  // have this egress Id, ask them to add it back if they don't
  // already have this egress Id. We do a checked add because if
  // the neighbor entry just expired w/o the port going down we
  // would have never removed it from ecmp egress object.

  // Note that we notify the ecmp group of the paths whenever we get
  // to this point with a nonzero port to associate with an egress
  // mapping. This handles the case where we hit the ecmp shrink code
  // during the initialization process and the port down event is not
  // processed by the SwSwitch correctly.  The SwSwitch is responsible
  // for generating an update for each NeighborEntry after it is
  // initialized to ensure the hw is programmed correctly. By trying
  // to always expand ECMP whenever we get a valid port mapping for a
  // egress ID, we would also signal for ECMP expand when port mapping
  // of a egress ID changes (e.g. on IP Address renumbering). This is
  // however safe since we ECMP expand code handles the case where we
  // try to add a already present egress ID in a ECMP group.
  BcmEcmpEgress::Action ecmpAction;
  if (isSet && !port) {
    /* went down */
    hw_->writableHostTable()->unresolved(egressId_);
    ecmpAction = BcmEcmpEgress::Action::SHRINK;
  } else if (!isSet && port) {
    /* came up */
    hw_->writableHostTable()->resolved(egressId_);
    ecmpAction = BcmEcmpEgress::Action::EXPAND;
  } else if (!isSet && !port) {
    /* stayed down */
    /* unresolved(egressId_); */
    ecmpAction = BcmEcmpEgress::Action::SKIP;
  } else {
    /* stayed up */
    DCHECK(isSet && port);
    /* resolved(egressId_); */
    ecmpAction = BcmEcmpEgress::Action::EXPAND;
  }

  // Update port mapping, for entries marked to DROP or to CPU port gets
  // set to 0, which implies no ports are associated with this entry now.
  hw_->writableHostTable()->updatePortToEgressMapping(
      egressPtr->getID(), getSetPortAsGPort(), BcmPort::asGPort(port));

  hw_->writableHostTable()->egressResolutionChangedHwLocked(
      egressId_, ecmpAction);

  trunk_ = BcmTrunk::INVALID;
  port_ = port;
}

void BcmHost::programToTrunk(opennsl_if_t intf,
                             const MacAddress mac, opennsl_trunk_t trunk) {
  unique_ptr<BcmEgress> createdEgress{nullptr};
  BcmEgress* egress{nullptr};
  // get the egress object and then update it with the new MAC
  if (egressId_ == BcmEgressBase::INVALID) {
    createdEgress = std::make_unique<BcmEgress>(hw_);
    egress = createdEgress.get();
  } else {
    egress = dynamic_cast<BcmEgress*>(
        hw_->writableHostTable()->getEgressObjectIf(egressId_));
  }
  CHECK(egress);

  egress->programToTrunk(intf, key_.getVrf(), key_.addr(), mac, trunk);

  if (createdEgress) {
    egressId_ = createdEgress->getID();
    hw_->writableHostTable()->insertBcmEgress(std::move(createdEgress));
  }

  // if no host was added already, add one pointing to the egress object
  if (!addedInHW_) {
    addToBcmHostTable();
  }

  XLOG(DBG1) << "Updating egress " << egress->getID() << " from "
             << (isTrunk() ? "trunk port " : "physical port ")
             << (isTrunk() ? trunk_ : port_) << " to trunk port " << trunk;

  hw_->writableHostTable()->resolved(egressId_);

  hw_->writableHostTable()->updatePortToEgressMapping(
      egress->getID(), getSetPortAsGPort(), BcmTrunk::asGPort(trunk));

  hw_->writableHostTable()->egressResolutionChangedHwLocked(
      egressId_, BcmEcmpEgress::Action::EXPAND);

  port_ = 0;
  trunk_ = trunk;
}

bool BcmHost::isTrunk() const {
  return trunk_ != BcmTrunk::INVALID;
}

BcmHost::~BcmHost() {
  if (addedInHW_) {
    opennsl_l3_host_t host;
    initHostCommon(&host);
    auto rc = opennsl_l3_host_delete(hw_->getUnit(), &host);
    bcmLogFatal(rc, hw_, "failed to delete L3 host object for ", key_.str());
    XLOG(DBG3) << "deleted L3 host object for " << key_.str();
  } else {
    XLOG(DBG3) << "No need to delete L3 host object for " << key_.str()
               << " as it was not added to the HW before";
  }
  if (egressId_ == BcmEgressBase::INVALID) {
    return;
  }
  if (isPortOrTrunkSet()) {
    hw_->writableHostTable()->unresolved(egressId_);
  }
  // This host mapping just went away, update the port -> egress id mapping
  hw_->writableHostTable()->updatePortToEgressMapping(
      egressId_, getSetPortAsGPort(), BcmPort::asGPort(0));
  hw_->writableHostTable()->egressResolutionChangedHwLocked(
      egressId_,
      isPortOrTrunkSet() ? BcmEcmpEgress::Action::SHRINK
                         : BcmEcmpEgress::Action::SKIP);
  hw_->writableHostTable()->derefEgress(egressId_);
}

std::shared_ptr<BcmNextHop> BcmEcmpHost::refOrEmplaceNextHop(
    const HostKey& key) {
  if (key.hasLabel()) {
    return hw_->writableMplsNextHopTable()->referenceOrEmplaceNextHop(
        folly::poly_cast<facebook::fboss::BcmLabeledHostKey>(key));
  }
  return hw_->writableL3NextHopTable()->referenceOrEmplaceNextHop(
      folly::poly_cast<facebook::fboss::BcmHostKey>(key));
}

BcmEcmpHost::BcmEcmpHost(const BcmSwitchIf *hw,
                         BcmEcmpHostKey key)
    : hw_(hw), vrf_(key.first) {
  auto& fwd = key.second;
  CHECK_GT(fwd.size(), 0);
  BcmHostTable *table = hw_->writableHostTable();
  BcmEcmpEgress::Paths paths;
  std::vector<std::shared_ptr<BcmNextHop>> nexthops;
  // allocate a BcmHost object for each path in this ECMP
  int total = 0;
  for (const auto& nhop : fwd) {
    auto nexthopSharedPtr = refOrEmplaceNextHop(getNextHopKey(vrf_, nhop));
    auto* nexthop = nexthopSharedPtr.get();
    // TODO:
    // Below comment appplies for L3 Nexthop only
    // Ideally, we should have the nexthop resolved already and programmed in
    // HW. If not, SW can preemptively trigger neighbor discovery and then
    // do the HW programming. For now, we program the egress object to punt
    // to CPU. Any traffic going to CPU will trigger the neighbor discovery.
    if (!nexthop->isProgrammed()) {
      const auto intf = hw->getIntfTable()->getBcmIntf(nhop.intf());
      nexthop->programToCPU(intf->getBcmIfId());
    }
    for (int i=0; i<nhop.weight(); ++i) {
      paths.insert(nexthop->getEgressId());
    }
    nexthops.push_back(std::move(nexthopSharedPtr));
  }
  if (paths.size() == 1) {
    // just one path. No BcmEcmpEgress object this case.
    egressId_ = *paths.begin();
  } else {
    auto ecmp = std::make_unique<BcmEcmpEgress>(hw, std::move(paths));
    egressId_ = ecmp->getID();
    ecmpEgressId_ = egressId_;
    hw_->writableHostTable()->insertBcmEgress(std::move(ecmp));
  }
  fwd_ = std::move(fwd);
  nexthops_ = std::move(nexthops);
}

BcmEcmpHost::~BcmEcmpHost() {
  // Deref ECMP egress first since the ECMP egress entry holds references
  // to egress entries.
  XLOG(DBG3) << "Decremented reference for egress object for " << fwd_;
  hw_->writableHostTable()->derefEgress(ecmpEgressId_);
}

BcmHostTable::BcmHostTable(const BcmSwitchIf* hw) : hw_(hw) {
  auto port2EgressIds = std::make_shared<PortAndEgressIdsMap>();
  port2EgressIds->publish();
  setPort2EgressIdsInternal(port2EgressIds);
}

BcmHostTable::~BcmHostTable() {
}

template<typename KeyT, typename HostT>
HostT* BcmHostTable::incRefOrCreateBcmHostImpl(
    HostMap<KeyT, HostT>* map,
    const KeyT& key) {
  auto iter = map->find(key);
  if (iter != map->cend()) {
    // there was an entry already there
    iter->second.second++;  // increase the reference counter
    XLOG(DBG3) << "referenced " << key
               << ". new ref count: " << iter->second.second;
    return iter->second.first.get();
  }
  auto newHost = std::make_unique<HostT>(hw_, key);
  auto hostPtr = newHost.get();
  auto ret = map->emplace(key, std::make_pair(std::move(newHost), 1));
  CHECK_EQ(ret.second, true)
    << "must insert BcmHost/BcmEcmpHost as a new entry in this case";
  XLOG(DBG3) << "created " << key
             << ". new ref count: " << ret.first->second.second;
  return hostPtr;
}

BcmHost* BcmHostTable::incRefOrCreateBcmHost(const HostKey& hostKey) {
  if (!hostKey.hasLabel()) {
    return incRefOrCreateBcmHostImpl(
        &hosts_, folly::poly_cast<BcmHostKey>(hostKey));
  }
  return incRefOrCreateBcmHostImpl(
      &labeledHosts_, folly::poly_cast<BcmLabeledHostKey>(hostKey));
}

BcmEcmpHost* BcmHostTable::incRefOrCreateBcmEcmpHost(
    const BcmEcmpHostKey& key) {
  return incRefOrCreateBcmHostImpl(&ecmpHosts_, key);
}

uint32_t BcmHostTable::getReferenceCount(const BcmEcmpHostKey& key)
    const noexcept {
  return getReferenceCountImpl(&ecmpHosts_, key);
}

uint32_t BcmHostTable::getReferenceCount(const HostKey& key) const noexcept {
  if (!key.hasLabel()) {
    return getReferenceCountImpl(&hosts_, folly::poly_cast<BcmHostKey>(key));
  }
  return getReferenceCountImpl(
      &labeledHosts_, folly::poly_cast<BcmLabeledHostKey>(key));
}

template<typename KeyT, typename HostT>
uint32_t BcmHostTable::getReferenceCountImpl(
    const HostMap<KeyT, HostT>* map,
    const KeyT& key) const noexcept {
  auto iter = map->find(key);
  if (iter == map->cend()) {
    return 0;
  }
  return iter->second.second;
}

template<typename KeyT, typename HostT>
HostT* BcmHostTable::getBcmHostIfImpl(
    const HostMap<KeyT, HostT>* map,
    const KeyT& key) const noexcept {
  auto iter = map->find(key);
  if (iter == map->cend()) {
    return nullptr;
  }
  return iter->second.first.get();
}

BcmHost* BcmHostTable::getBcmHost(const HostKey& key) const {
  auto host = getBcmHostIf(key);
  if (!host) {
    throw FbossError("Cannot find BcmHost key=", key);
  }
  return host;
}

BcmEcmpHost* BcmHostTable::getBcmEcmpHost(
    const BcmEcmpHostKey& key) const {
  auto host = getBcmEcmpHostIf(key);
  if (!host) {
    throw FbossError(
        "Cannot find BcmEcmpHost vrf=", key.first, " fwd=", key.second);
  }
  return host;
}

BcmHost* BcmHostTable::getBcmHostIf(const HostKey& key) const noexcept {
  return !key.hasLabel()
      ? getBcmHostIfImpl(&hosts_, folly::poly_cast<BcmHostKey>(key))
      : getBcmHostIfImpl(
            &labeledHosts_, folly::poly_cast<BcmLabeledHostKey>(key));
}

BcmEcmpHost* BcmHostTable::getBcmEcmpHostIf(
    const BcmEcmpHostKey& key) const noexcept {
  return getBcmHostIfImpl(&ecmpHosts_, key);
}

template<typename KeyT, typename HostT>
HostT* BcmHostTable::derefBcmHostImpl(
    HostMap<KeyT, HostT>* map,
    const KeyT& key) noexcept {
  auto iter = map->find(key);
  if (iter == map->cend()) {
    return nullptr;
  }
  auto& entry = iter->second;
  CHECK_GT(entry.second, 0);
  if (--entry.second == 0) {
    XLOG(DBG3) << "erase host " << key << " from host map";
    /*
     * moving unique ptr outside of ECMP host map before calling an erase
     * this is important for following:
     * Consider for ECMP host, deref leads to 0 ref count, and  ECMP host is
     * erased from its map (without moving ownership out), following events
     * happen
     *
     * 1) BcmEcmpHost's destructor is invoked,
     * 2) In BcmEcmpHost's destructor BcmHost member's ref count are decremented
     * 3) If one of BcmHost ref count leads to 0, then BcmHost is also be
     *    erased from map,
     * 4) BcmHost's destrucor is invoked
     * 5) BcmHost's destructor now wants to update ECMP groups to which it
     *    may belongs
     * 6) BcmHost's destructor iterates over ECMP host map, which now is in
     *    "bad state" (because map.erase has still not returned and iterator is
     *      not yet reset)
     * This activity leads to crash
     */
    auto ptr = std::move(iter->second.first);
    map->erase(iter);
    return nullptr;
  }
  XLOG(DBG3) << "dereferenced host " << key
             << ". new ref count: " << entry.second;
  return entry.first.get();
}

BcmHost* BcmHostTable::derefBcmHost(const HostKey& key) noexcept {
  return !key.hasLabel()
      ? derefBcmHostImpl(&hosts_, folly::poly_cast<BcmHostKey>(key))
      : derefBcmHostImpl(
            &labeledHosts_, folly::poly_cast<BcmLabeledHostKey>(key));
}

BcmEcmpHost* BcmHostTable::derefBcmEcmpHost(
    const BcmEcmpHostKey& key) noexcept {
  return derefBcmHostImpl(&ecmpHosts_, key);
}

BcmEgressBase* BcmHostTable::incEgressReference(opennsl_if_t egressId) {
  if (egressId == BcmEcmpEgress::INVALID ||
      egressId == hw_->getDropEgressId()) {
    return nullptr;
  }
  auto it = egressMap_.find(egressId);
  CHECK(it != egressMap_.end());
  it->second.second++;
  XLOG(DBG3) << "referenced egress " << egressId
             << ". new ref count: " << it->second.second;
  return it->second.first.get();
}

BcmEgressBase* BcmHostTable::derefEgress(opennsl_if_t egressId) {
  if (egressId == BcmEcmpEgress::INVALID ||
      egressId == hw_->getDropEgressId()) {
    return nullptr;
  }
  auto it = egressMap_.find(egressId);
  CHECK(it != egressMap_.end());
  CHECK_GT(it->second.second, 0);
  if (--it->second.second == 0) {
    if (it->second.first->isEcmp()) {
      CHECK(numEcmpEgressProgrammed_ > 0);
      numEcmpEgressProgrammed_--;
    }
    XLOG(DBG3) << "erase egress " << egressId << " from egress map";
    egressMap_.erase(egressId);
    return nullptr;
  }
  XLOG(DBG3) << "dereferenced egress " << egressId
             << ". new ref count: " << it->second.second;
  return it->second.first.get();
}

void BcmHostTable::updatePortToEgressMapping(
    opennsl_if_t egressId,
    opennsl_gport_t oldGPort,
    opennsl_gport_t newGPort) {
  auto newMapping = getPortAndEgressIdsMap()->clone();

  if (BcmPort::isValidLocalPort(oldGPort) ||
      BcmTrunk::isValidTrunkPort(oldGPort)) {
    auto old = newMapping->getPortAndEgressIdsIf(oldGPort);
    CHECK(old);
    auto oldCloned = old->clone();
    oldCloned->removeEgressId(egressId);
    if (oldCloned->empty()) {
      newMapping->removePort(oldGPort);
    } else {
      newMapping->updatePortAndEgressIds(oldCloned);
    }
  }
  if (BcmPort::isValidLocalPort(newGPort) ||
      BcmTrunk::isValidTrunkPort(newGPort)) {
    auto existing = newMapping->getPortAndEgressIdsIf(newGPort);
    if (existing) {
      auto existingCloned = existing->clone();
      existingCloned->addEgressId(egressId);
      newMapping->updatePortAndEgressIds(existingCloned);
    } else {
      PortAndEgressIdsFields::EgressIdSet egressIds;
      egressIds.insert(egressId);
      auto toAdd = std::make_shared<PortAndEgressIds>(newGPort, egressIds);
      newMapping->addPortAndEgressIds(toAdd);
    }
  }
  // Publish and replace with the updated mapping
  newMapping->publish();
  setPort2EgressIdsInternal(newMapping);
}

void BcmHostTable::setPort2EgressIdsInternal(
    std::shared_ptr<PortAndEgressIdsMap> newMap) {
  // This is one of the only two places that should ever directly access
  // portAndEgressIdsDontUseDirectly_.  (getPortAndEgressIdsMap() being the
  // other one.)
  CHECK(newMap->isPublished());
  folly::SpinLockGuard guard(portAndEgressIdsLock_);
  portAndEgressIdsDontUseDirectly_.swap(newMap);
}

const BcmEgressBase* BcmHostTable::getEgressObjectIf(opennsl_if_t egress) const {
  auto it = egressMap_.find(egress);
  return it == egressMap_.end() ? nullptr : it->second.first.get();
}

BcmEgressBase* BcmHostTable::getEgressObjectIf(opennsl_if_t egress) {
  return const_cast<BcmEgressBase*>(
      const_cast<const BcmHostTable*>(this)->getEgressObjectIf(egress));
}

void BcmHostTable::insertBcmEgress(
    std::unique_ptr<BcmEgressBase> egress) {
  auto id = egress->getID();
  XLOG(DBG3) << "insert egress " << id << " into egress map";
  if (egress->isEcmp()) {
    numEcmpEgressProgrammed_++;
  }
  auto ret = egressMap_.emplace(id, std::make_pair(std::move(egress), 1));
  CHECK(ret.second);
}

void BcmHostTable::warmBootHostEntriesSynced() {
  opennsl_port_config_t pcfg;
  auto rv = opennsl_port_config_get(hw_->getUnit(), &pcfg);
  bcmCheckError(rv, "failed to get port configuration");
  // Ideally we should call this only for ports which were
  // down when we went down, but since we don't record that
  // signal up for all up ports.
  XLOG(DBG1) << "Warm boot host entries synced, signalling link "
                "up for all up ports";
  opennsl_port_t idx;
  OPENNSL_PBMP_ITER(pcfg.port, idx) {
    // Some ports might have come up or gone down during
    // the time controller was down. So call linkUp/DownHwLocked
    // for these. We could track this better by just calling
    // linkUp/DownHwLocked only for ports that actually changed
    // state, but thats a minor optimization.
    if (hw_->isPortUp(PortID(idx))) {
      linkUpHwLocked(idx);
    } else {
      linkDownHwLocked(idx);
    }
  }
}

folly::dynamic BcmHost::toFollyDynamic() const {
  folly::dynamic host = folly::dynamic::object;
  host[kVrf] = key_.getVrf();
  host[kIp] = key_.addr().str();
  if (key_.intfID().hasValue()) {
    host[kIntf] = static_cast<uint32_t>(key_.intfID().value());
  }
  host[kPort] = port_;
  host[kEgressId] = egressId_;
  if (egressId_ != BcmEgressBase::INVALID &&
      egressId_ != hw_->getDropEgressId()) {
    host[kEgress] = hw_->getHostTable()
      ->getEgressObjectIf(egressId_)->toFollyDynamic();
  }
  return host;
}

folly::dynamic BcmEcmpHost::toFollyDynamic() const {
  folly::dynamic ecmpHost = folly::dynamic::object;
  ecmpHost[kVrf] = vrf_;
  folly::dynamic nhops = folly::dynamic::array;
  for (const auto& nhop : fwd_) {
    nhops.push_back(nhop.toFollyDynamic());
  }
  ecmpHost[kNextHops] = std::move(nhops);
  ecmpHost[kEgressId] = egressId_;
  ecmpHost[kEcmpEgressId] = ecmpEgressId_;
  if (ecmpEgressId_ != BcmEgressBase::INVALID) {
    ecmpHost[kEcmpEgress] = hw_->getHostTable()
      ->getEgressObjectIf(ecmpEgressId_)->toFollyDynamic();
  }
  return ecmpHost;
}

folly::dynamic BcmHostTable::toFollyDynamic() const {
  folly::dynamic hostsJson = folly::dynamic::array;
  for (const auto& vrfIpAndHost: hosts_) {
    hostsJson.push_back(vrfIpAndHost.second.first->toFollyDynamic());
  }
  folly::dynamic ecmpHostsJson = folly::dynamic::array;
  for (const auto& vrfNhopsAndHost: ecmpHosts_) {
    ecmpHostsJson.push_back(vrfNhopsAndHost.second.first->toFollyDynamic());
  }
  folly::dynamic hostTable = folly::dynamic::object;
  hostTable[kHosts] = std::move(hostsJson);
  hostTable[kEcmpHosts] = std::move(ecmpHostsJson);
  return hostTable;
}

void BcmHostTable::linkStateChangedMaybeLocked(
    opennsl_gport_t gport,
    bool up,
    bool locked) {
  auto portAndEgressIdMapping = getPortAndEgressIdsMap();

  const auto portAndEgressIds =
      portAndEgressIdMapping->getPortAndEgressIdsIf(gport);
  if (!portAndEgressIds) {
    return;
  }
  if (locked) {
    egressResolutionChangedHwLocked(
        portAndEgressIds->getEgressIds(),
        up ? BcmEcmpEgress::Action::EXPAND : BcmEcmpEgress::Action::SHRINK);
  } else {
    CHECK(!up);
    egressResolutionChangedHwNotLocked(
        hw_->getUnit(), portAndEgressIds->getEgressIds(), up);
  }
}

int BcmHostTable::removeAllEgressesFromEcmpCallback(
    int unit,
    opennsl_l3_egress_ecmp_t* ecmp,
    int intfCount,
    opennsl_if_t* intfArray,
    void* userData) {
  // loop over the egresses present in the ecmp group. For each that is
  // in the passed in list of egresses to remove (userData),
  // remove it from the ecmp group.
  EgressIdSet* egressesToRemove = static_cast<EgressIdSet*>(userData);
  for (int i = 0; i < intfCount; ++i) {
    opennsl_if_t egressInHw = intfArray[i];
    if (egressesToRemove->find(egressInHw) != egressesToRemove->end()) {
      BcmEcmpEgress::removeEgressIdHwNotLocked(
          unit, ecmp->ecmp_intf, egressInHw);
    }
  }
  return 0;
}

void BcmHostTable::egressResolutionChangedHwNotLocked(
    int unit,
    const EgressIdSet& affectedEgressIds,
    bool up) {
  CHECK(!up);
  EgressIdSet tmpEgressIds(affectedEgressIds);
  opennsl_l3_egress_ecmp_traverse(
      unit, removeAllEgressesFromEcmpCallback, &tmpEgressIds);
}

void BcmHostTable::egressResolutionChangedHwLocked(
    const EgressIdSet& affectedEgressIds,
    BcmEcmpEgress::Action action) {
  if (action == BcmEcmpEgress::Action::SKIP) {
    return;
  }

  for (const auto& nextHopsAndEcmpHostInfo : ecmpHosts_) {
    auto ecmpId = nextHopsAndEcmpHostInfo.second.first->getEcmpEgressId();
    if (ecmpId == BcmEgressBase::INVALID) {
      continue;
    }
    auto ecmpEgress = static_cast<BcmEcmpEgress*>(getEgressObjectIf(ecmpId));
    // Must find the egress object, we could have done a slower
    // dynamic cast check to ensure that this is the right type
    // our map should be pointing to valid Ecmp egress object for
    // a ecmp egress Id anyways
    CHECK(ecmpEgress);
    for (auto egrId : affectedEgressIds) {
      switch (action) {
        case BcmEcmpEgress::Action::EXPAND:
          ecmpEgress->pathReachableHwLocked(egrId);
          break;
        case BcmEcmpEgress::Action::SHRINK:
          ecmpEgress->pathUnreachableHwLocked(egrId);
          break;
        case BcmEcmpEgress::Action::SKIP:
          break;
        default:
          XLOG(FATAL) << "BcmEcmpEgress::Action matching not exhaustive";
          break;
      }
    }
  }
  /*
   * We may not have done a FIB sync before ports start coming
   * up or ARP/NDP start getting resolved/unresolved. In this case
   * we won't have BcmEcmpHost entries, so we
   * look through the warm boot cache for ecmp egress entries.
   * Conversely post a FIB sync we won't have any ecmp egress IDs
   * in the warm boot cache
   */

  for (const auto& ecmpAndEgressIds :
       hw_->getWarmBootCache()->ecmp2EgressIds()) {
    for (auto path : affectedEgressIds) {
      switch (action) {
        case BcmEcmpEgress::Action::EXPAND:
          BcmEcmpEgress::addEgressIdHwLocked(
              hw_->getUnit(),
              ecmpAndEgressIds.first,
              ecmpAndEgressIds.second,
              path);
          break;
        case BcmEcmpEgress::Action::SHRINK:
          BcmEcmpEgress::removeEgressIdHwLocked(
              hw_->getUnit(), ecmpAndEgressIds.first, path);
          break;
        case BcmEcmpEgress::Action::SKIP:
          break;
        default:
          XLOG(FATAL) << "BcmEcmpEgress::Action matching not exhaustive";
          break;
      }
    }
  }
}

BcmHost* BcmNeighborTable::registerNeighbor(const BcmHostKey& neighbor) {
  auto result = neighborHostReferences_.emplace(
      neighbor, BcmHostReference::get(hw_, neighbor));
  return result.first->second->getBcmHost();
}

BcmHost* FOLLY_NULLABLE
BcmNeighborTable::unregisterNeighbor(const BcmHostKey& neighbor) {
  neighborHostReferences_.erase(neighbor);
  return hw_->getHostTable()->getBcmHostIf(neighbor);
}

BcmHost* BcmNeighborTable::getNeighbor(const BcmHostKey& neighbor) const {
  auto* host = getNeighborIf(neighbor);
  if (!host) {
    throw FbossError("neighbor entry not found for :", neighbor.str());
  }
  return host;
}

BcmHost* FOLLY_NULLABLE
BcmNeighborTable::getNeighborIf(const BcmHostKey& neighbor) const {
  auto iter = neighborHostReferences_.find(neighbor);
  if (iter == neighborHostReferences_.end()) {
    return nullptr;
  }
  return iter->second->getBcmHost();
}

void BcmHostTable::programHostsToTrunk(
    const BcmHostKey& key,
    opennsl_if_t intf,
    const MacAddress& mac,
    opennsl_trunk_t trunk) {
  auto iter = hosts_.find(key);
  if (iter == hosts_.end()) {
    throw FbossError("host not found to program to trunk");
  }
  auto* host = iter->second.first.get();
  host->programToTrunk(intf, mac, trunk);

  // program labeled next hops to the host
  auto ret = std::equal_range(
      std::begin(labeledHosts_),
      std::end(labeledHosts_),
      key,
      BcmHostKeyComparator());
  std::for_each(ret.first, ret.second, [=](auto& entry) {
    auto* labeledHost = entry.second.first.get();
    labeledHost->programToTrunk(intf, mac, trunk);
  });
}

void BcmHostTable::programHostsToPort(
    const BcmHostKey& key,
    opennsl_if_t intf,
    const MacAddress& mac,
    opennsl_port_t port) {
  auto iter = hosts_.find(key);
  if (iter == hosts_.end()) {
    throw FbossError("host not found to program to port");
  }
  auto* host = iter->second.first.get();
  host->program(intf, mac, port);

  // program labeled next hops to the host
  auto ret = std::equal_range(
      std::begin(labeledHosts_),
      std::end(labeledHosts_),
      key,
      BcmHostKeyComparator());
  std::for_each(ret.first, ret.second, [=](auto& entry) {
    auto* labeledHost = entry.second.first.get();
    labeledHost->program(intf, mac, port);
  });
}

void BcmHostTable::programHostsToCPU(const BcmHostKey& key, opennsl_if_t intf) {
  auto iter = hosts_.find(key);
  if (iter != hosts_.end()) {
    auto* host = iter->second.first.get();
    host->programToCPU(intf);
  }

  // program labeled next hops to the host
  auto ret = std::equal_range(
      std::begin(labeledHosts_),
      std::end(labeledHosts_),
      key,
      BcmHostKeyComparator());
  std::for_each(ret.first, ret.second, [=](auto& entry) {
    auto* labeledHost = entry.second.first.get();
    labeledHost->programToCPU(intf);
  });
}

BcmHostReference::BcmHostReference(BcmSwitch* hw, HostKey key)
    : hw_(hw), hostKey_(std::move(key)) {}

BcmHostReference::BcmHostReference(BcmSwitch* hw, BcmEcmpHostKey key)
    : hw_(hw), ecmpHostKey_(std::move(key)) {}

std::unique_ptr<BcmHostReference> BcmHostReference::get(
    BcmSwitch* hw,
    opennsl_vrf_t vrf,
    RouteNextHopSet nexthops) {
  struct _ : public BcmHostReference {
    _(BcmSwitch* hw, opennsl_vrf_t vrf, RouteNextHopSet nexthops)
        : BcmHostReference(hw, std::make_pair(vrf, nexthops)) {}
  };
  return std::make_unique<_>(hw, vrf, std::move(nexthops));
}

std::unique_ptr<BcmHostReference> BcmHostReference::get(
    BcmSwitch* hw,
    HostKey key) {
  struct _ : public BcmHostReference {
    _(BcmSwitch* hw, HostKey key) : BcmHostReference(hw, std::move(key)) {}
  };
  return std::make_unique<_>(hw, std::move(key));
}

std::unique_ptr<BcmHostReference> BcmHostReference::get(
    BcmSwitch* hw,
    BcmEcmpHostKey key) {
  struct _ : public BcmHostReference {
    _(BcmSwitch* hw, BcmEcmpHostKey key)
        : BcmHostReference(hw, std::move(key)) {}
  };
  return std::make_unique<_>(hw, std::move(key));
}

BcmHostReference::~BcmHostReference() {
  if (hostKey_ && host_) {
    hw_->writableHostTable()->derefBcmHost(hostKey_.value());
  }
  if (ecmpHostKey_ && ecmpHost_) {
    hw_->writableHostTable()->derefBcmEcmpHost(ecmpHostKey_.value());
  }
}

BcmHost* BcmHostReference::getBcmHost() {
  if (host_ || !hostKey_) {
    return host_;
  }
  host_ = hw_->writableHostTable()->incRefOrCreateBcmHost(hostKey_.value());
  return host_;
}

BcmEcmpHost* BcmHostReference::getBcmEcmpHost() {
  if (ecmpHost_ || !ecmpHostKey_) {
    return ecmpHost_;
  }
  ecmpHost_ =
      hw_->writableHostTable()->incRefOrCreateBcmEcmpHost(ecmpHostKey_.value());
  return ecmpHost_;
}

opennsl_if_t BcmHostReference::getEgressId() {
  return hostKey_ ? getBcmHost()->getEgressId()
                  : getBcmEcmpHost()->getEgressId();
}
}}
