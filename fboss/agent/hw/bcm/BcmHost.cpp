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
#include "fboss/agent/hw/bcm/BcmEgressManager.h"
#include "fboss/agent/hw/bcm/BcmError.h"
#include "fboss/agent/hw/bcm/BcmIntf.h"
#include "fboss/agent/hw/bcm/BcmMultiPathNextHop.h"
#include "fboss/agent/hw/bcm/BcmNextHop.h"
#include "fboss/agent/hw/bcm/BcmPort.h"
#include "fboss/agent/hw/bcm/BcmPortTable.h"
#include "fboss/agent/hw/bcm/BcmSwitch.h"
#include "fboss/agent/hw/bcm/BcmTrunkTable.h"
#include "fboss/agent/hw/bcm/BcmWarmBootCache.h"
#include "fboss/agent/state/Interface.h"

namespace {
constexpr auto kIntf = "intf";
constexpr auto kPort = "port";
}

namespace facebook { namespace fboss {

std::ostream& operator<<(
    std::ostream& os,
    const facebook::fboss::BcmMultiPathNextHopKey& key) {
  return os << "BcmMultiPathNextHop: " << key.second << "@vrf " << key.first;
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
  if (eid == getEgressId()) {
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

  XLOG(DBG3) << "set host object for " << key_.str() << " to @egress "
             << eid << " from @egress " << getEgressId();
  egress_ = std::make_unique<BcmHostEgress>(eid);
  // in case if both neighbor & host route prefix end up using same host entry
  // next hops referring to it, can't refer to hostRouteEgress
  action_ = DROP;
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
  if (!egress_ || egress_->getEgressId() == BcmEgressBase::INVALID) {
    XLOG(DBG3) << "Host entry for " << key_.str()
               << " does not have an egress, create one.";
    egress_ = std::make_unique<BcmHostEgress>(createEgress());
  }
  egressPtr = getEgress();

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
    hw_->writableEgressManager()->unresolved(getEgressId());
    ecmpAction = BcmEcmpEgress::Action::SHRINK;
  } else if (!isSet && port) {
    /* came up */
    hw_->writableEgressManager()->resolved(getEgressId());
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
  hw_->writableEgressManager()->updatePortToEgressMapping(
      egressPtr->getID(), getSetPortAsGPort(), BcmPort::asGPort(port));

  hw_->writableMultiPathNextHopTable()->egressResolutionChangedHwLocked(
      getEgressId(), ecmpAction);

  trunk_ = BcmTrunk::INVALID;
  port_ = port;
  action_ = action;
}

void BcmHost::programToTrunk(opennsl_if_t intf,
                             const MacAddress mac, opennsl_trunk_t trunk) {
  BcmEgress* egress{nullptr};
  // get the egress object and then update it with the new MAC
  if (!egress_ || egress_->getEgressId() == BcmEgressBase::INVALID) {
    egress_ = std::make_unique<BcmHostEgress>(std::make_unique<BcmEgress>(hw_));
  }
  egress = getEgress();
  CHECK(egress);

  egress->programToTrunk(intf, key_.getVrf(), key_.addr(), mac, trunk);

  // if no host was added already, add one pointing to the egress object
  if (!addedInHW_) {
    addToBcmHostTable();
  }

  XLOG(DBG1) << "Updating egress " << egress->getID() << " from "
             << (isTrunk() ? "trunk port " : "physical port ")
             << (isTrunk() ? trunk_ : port_) << " to trunk port " << trunk;

  hw_->writableEgressManager()->resolved(getEgressId());

  hw_->writableEgressManager()->updatePortToEgressMapping(
      getEgressId(), getSetPortAsGPort(), BcmTrunk::asGPort(trunk));

  hw_->writableMultiPathNextHopTable()->egressResolutionChangedHwLocked(
      getEgressId(), BcmEcmpEgress::Action::EXPAND);

  port_ = 0;
  trunk_ = trunk;
  action_ = NEXTHOPS;
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
  if (getEgressId() == BcmEgressBase::INVALID) {
    return;
  }
  if (isPortOrTrunkSet()) {
    hw_->writableEgressManager()->unresolved(getEgressId());
  }
  // This host mapping just went away, update the port -> egress id mapping
  hw_->writableEgressManager()->updatePortToEgressMapping(
      getEgressId(), getSetPortAsGPort(), BcmPort::asGPort(0));
  hw_->writableMultiPathNextHopTable()->egressResolutionChangedHwLocked(
      getEgressId(),
      isPortOrTrunkSet() ? BcmEcmpEgress::Action::SHRINK
                         : BcmEcmpEgress::Action::SKIP);
}

PortDescriptor BcmHost::portDescriptor() const {
  return isTrunk()
      ? PortDescriptor(hw_->getTrunkTable()->getAggregatePortId(trunk_))
      : PortDescriptor(hw_->getPortTable()->getPortId(port_));
}

BcmHostTable::BcmHostTable(const BcmSwitchIf* hw) : hw_(hw) {}

BcmHostTable::~BcmHostTable() {
}

uint32_t BcmHostTable::getReferenceCount(const BcmHostKey& key) const noexcept {
  return hosts_.referenceCount(key);
}

BcmHost* BcmHostTable::getBcmHost(const BcmHostKey& key) const {
  auto* host = getBcmHostIf(key);
  CHECK(host);
  if (!host) {
    throw FbossError("Cannot find BcmHost key=", key);
  }
  return host;
}

BcmHost* FOLLY_NULLABLE BcmHostTable::getBcmHostIf(const BcmHostKey& key) const
    noexcept {
  return hosts_.getMutable(key);
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
      hw_->writableEgressManager()->linkUpHwLocked(idx);
    } else {
      hw_->writableEgressManager()->linkDownHwLocked(idx);
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
  host[kEgressId] = getEgressId();
  if (getEgressId() != BcmEgressBase::INVALID &&
      egress_->type() == BcmHostEgress::BcmHostEgressType::OWNED) {
    // owned egress, BcmHost entry is not host route entry.
    host[kEgress] = egress_->getOwnedEgressPtr()->toFollyDynamic();
  }
  return host;
}

std::shared_ptr<BcmHost> BcmHostTable::refOrEmplace(const BcmHostKey& key) {
  auto rv = hosts_.refOrEmplace(key, hw_, key);
  if (rv.second) {
    XLOG(DBG3) << "inserted reference to BcmHost " << key.str();
  } else {
    XLOG(DBG3) << "accessed reference to BcmHost " << key.str();
  }
  return rv.first;
}

folly::dynamic BcmHostTable::toFollyDynamic() const {
  folly::dynamic hostsJson = folly::dynamic::array;
  for (const auto& vrfIpAndHost: hosts_) {
    auto host = vrfIpAndHost.second.lock();
    hostsJson.push_back(host->toFollyDynamic());
  }
  folly::dynamic ecmpHostsJson = folly::dynamic::array;
  auto& ecmpHosts = hw_->getMultiPathNextHopTable()->getNextHops();
  for (const auto& vrfNhopsAndHost: ecmpHosts) {
    auto weakPtr = vrfNhopsAndHost.second;
    auto ecmpHost = weakPtr.lock();
    ecmpHostsJson.push_back(ecmpHost->toFollyDynamic());
  }
  folly::dynamic hostTable = folly::dynamic::object;
  hostTable[kHosts] = std::move(hostsJson);
  hostTable[kEcmpHosts] = std::move(ecmpHostsJson);
  return hostTable;
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
  auto* host = getBcmHostIf(key);
  CHECK(host);
  host->programToTrunk(intf, mac, trunk);
  // (TODO) program labeled next hops to the host
}

void BcmHostTable::programHostsToPort(
    const BcmHostKey& key,
    opennsl_if_t intf,
    const MacAddress& mac,
    opennsl_port_t port) {
  auto* host = getBcmHostIf(key);
  CHECK(host);
  host->program(intf, mac, port);
  // (TODO) program labeled next hops to the host
}

void BcmHostTable::programHostsToCPU(const BcmHostKey& key, opennsl_if_t intf) {
  auto* host = getBcmHostIf(key);
  if (host) {
    return host->programToCPU(intf);
  }
  // (TODO) program labeled next hops to the host
}

BcmHostReference::BcmHostReference(BcmSwitch* hw, BcmHostKey key)
    : hw_(hw), hostKey_(std::move(key)) {}

BcmHostReference::BcmHostReference(BcmSwitch* hw, BcmMultiPathNextHopKey key)
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
    BcmHostKey key) {
  struct _ : public BcmHostReference {
    _(BcmSwitch* hw, BcmHostKey key) : BcmHostReference(hw, std::move(key)) {}
  };
  return std::make_unique<_>(hw, std::move(key));
}

std::unique_ptr<BcmHostReference> BcmHostReference::get(
    BcmSwitch* hw,
    BcmMultiPathNextHopKey key) {
  struct _ : public BcmHostReference {
    _(BcmSwitch* hw, BcmMultiPathNextHopKey key)
        : BcmHostReference(hw, std::move(key)) {}
  };
  return std::make_unique<_>(hw, std::move(key));
}

BcmHost* BcmHostReference::getBcmHost() {
  if (host_ || !hostKey_) {
    return host_.get();
  }
  host_ = hw_->writableHostTable()->refOrEmplace(hostKey_.value());
  return host_.get();
}

BcmMultiPathNextHop* BcmHostReference::getBcmMultiPathNextHop() {
  if (ecmpHost_ || !ecmpHostKey_) {
    return ecmpHost_.get();
  }
  ecmpHost_ = hw_->writableMultiPathNextHopTable()->referenceOrEmplaceNextHop(
      ecmpHostKey_.value());
  return ecmpHost_.get();
}

opennsl_if_t BcmHostReference::getEgressId() {
  return hostKey_ ? getBcmHost()->getEgressId()
                  : getBcmMultiPathNextHop()->getEgressId();
}
}}
