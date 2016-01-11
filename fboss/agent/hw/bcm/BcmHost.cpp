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

#include "fboss/agent/state/Interface.h"
#include "fboss/agent/hw/bcm/BcmError.h"
#include "fboss/agent/hw/bcm/BcmSwitch.h"
#include "fboss/agent/hw/bcm/BcmEgress.h"
#include "fboss/agent/hw/bcm/BcmIntf.h"
#include "fboss/agent/hw/bcm/BcmPort.h"
#include "fboss/agent/hw/bcm/BcmWarmBootCache.h"

namespace facebook { namespace fboss {

using std::unique_ptr;
using std::shared_ptr;
using folly::MacAddress;
using folly::IPAddress;

BcmHost::BcmHost(const BcmSwitch* hw, opennsl_vrf_t vrf, const IPAddress& addr,
    opennsl_if_t referenced_egress)
      : hw_(hw), vrf_(vrf), addr_(addr),
      egressId_(referenced_egress) {
  if (referenced_egress != BcmEgressBase::INVALID) {
    hw_->writableHostTable()->incEgressReference(egressId_);
  }
}

void BcmHost::initHostCommon(opennsl_l3_host_t *host) const {
  opennsl_l3_host_t_init(host);
  if (addr_.isV4()) {
    host->l3a_ip_addr = addr_.asV4().toLongHBO();
  } else {
    memcpy(&host->l3a_ip6_addr, addr_.asV6().toByteArray().data(),
           sizeof(host->l3a_ip6_addr));
    host->l3a_flags |= OPENNSL_L3_IP6;
  }
  host->l3a_vrf = vrf_;
  host->l3a_intf = getEgressId();
}

void BcmHost::addBcmHost(bool isMultipath) {
  if (added_) {
    return;
  }
  opennsl_l3_host_t host;
  initHostCommon(&host);
  if (isMultipath) {
    host.l3a_flags |= OPENNSL_L3_MULTIPATH;
  }
  const auto warmBootCache = hw_->getWarmBootCache();
  auto vrfIp2HostCitr = warmBootCache->findHost(vrf_, addr_);
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
      return flagsEqual && existingHost.l3a_vrf == newHost.l3a_vrf &&
        existingHost.l3a_intf == newHost.l3a_intf;
    };
    if (!equivalent(host, vrfIp2HostCitr->second)) {
      LOG (FATAL) << "Host entries should never change ";
    } else {
      VLOG(1) << "Host entry for : " << addr_ << " already exists";
    }
    warmBootCache->programmed(vrfIp2HostCitr);
  } else {
    VLOG(3) << "Adding host entry for : " << addr_;
    auto rc = opennsl_l3_host_add(hw_->getUnit(), &host);
    bcmCheckError(rc, "failed to program L3 host object for ", addr_.str(),
      " @egress ", getEgressId());
    VLOG(3) << "created L3 host object for " << addr_.str()
    << " @egress " << getEgressId();

  }
  added_ = true;
}
void BcmHost::program(opennsl_if_t intf, const MacAddress* mac,
                      opennsl_port_t port, RouteForwardAction action) {
  unique_ptr<BcmEgress> createdEgress{nullptr};
  BcmEgress* egress{nullptr};
  // get the egress object and then update it with the new MAC
  if (egressId_ == BcmEgressBase::INVALID) {
    createdEgress = folly::make_unique<BcmEgress>(hw_);
    egress = createdEgress.get();
  } else {
    egress = dynamic_cast<BcmEgress*>(
        hw_->writableHostTable()->getEgressObjectIf(egressId_));
  }
  CHECK(egress);
  if (mac) {
    egress->program(intf, vrf_, addr_, *mac, port);
  } else {
    if (action == DROP) {
      egress->programToDrop(intf, vrf_, addr_);
    } else {
      egress->programToCPU(intf, vrf_, addr_);
    }
  }
  if (createdEgress) {
    egressId_ = createdEgress->getID();
    hw_->writableHostTable()->insertBcmEgress(std::move(createdEgress));
  }

  // if no host was added already, add one pointing to the egress object
  if (!added_) {
    addBcmHost();
  }
}


BcmHost::~BcmHost() {
  if (!added_) {
    return;
  }
  opennsl_l3_host_t host;
  initHostCommon(&host);
  auto rc = opennsl_l3_host_delete(hw_->getUnit(), &host);
  bcmLogFatal(rc, hw_, "failed to delete L3 host object for ", addr_.str());
  VLOG(3) << "deleted L3 host object for " << addr_.str();
  hw_->writableHostTable()->derefEgress(egressId_);
}

BcmEcmpHost::BcmEcmpHost(const BcmSwitch *hw, opennsl_vrf_t vrf,
                         const RouteForwardNexthops& fwd)
    : hw_(hw), vrf_(vrf) {
  CHECK_GT(fwd.size(), 0);
  BcmHostTable *table = hw_->writableHostTable();
  opennsl_if_t paths[fwd.size()];
  RouteForwardNexthops prog;
  prog.reserve(fwd.size());
  SCOPE_FAIL {
    for (const auto& nhop : prog) {
      table->derefBcmHost(vrf, nhop.nexthop);
    }
  };
  // allocate a BcmHost object for each path in this ECMP
  int total = 0;
  for (const auto& nhop : fwd) {
    auto host = table->incRefOrCreateBcmHost(vrf, nhop.nexthop);
    auto ret = prog.emplace(nhop.intf, nhop.nexthop);
    CHECK(ret.second);
    // TODO:
    // Ideally, we should have the nexthop resolved already and programmed in
    // HW. If not, SW can preemptively trigger neighbor discovery and then
    // do the HW programming. For now, we program the egress object to punt
    // to CPU. Any traffic going to CPU will trigger the neighbor discovery.
    if (!host->isProgrammed()) {
      const auto intf = hw->getIntfTable()->getBcmIntf(nhop.intf);
      host->programToCPU(intf->getBcmIfId());
    }
    paths[total++] = host->getEgressId();
  }
  if (total == 1) {
    // just one path. No BcmEcmpEgress object this case.
    egressId_ = paths[0];
  } else {
    auto ecmp = folly::make_unique<BcmEcmpEgress>(hw);
    ecmp->program(paths, fwd.size());
    egressId_ = ecmp->getID();
    ecmpEgressId_ = egressId_;
    hw_->writableHostTable()->insertBcmEgress(std::move(ecmp));
  }
  fwd_ = std::move(prog);
}

BcmEcmpHost::~BcmEcmpHost() {
  // Deref ECMP egress first since the ECMP egress entry holds references
  // to egress entries.
  if (ecmpEgressId_ != BcmEgressBase::INVALID) {
    VLOG(3) << "Decremented reference for egress object for " << fwd_;
    hw_->writableHostTable()->derefEgress(ecmpEgressId_);
  }
  BcmHostTable *table = hw_->writableHostTable();
  for (const auto& nhop : fwd_) {
    table->derefBcmHost(vrf_, nhop.nexthop);
  }
}

BcmHostTable::BcmHostTable(const BcmSwitch *hw) : hw_(hw) {
}

BcmHostTable::~BcmHostTable() {
}

template<typename KeyT, typename HostT, typename... Args>
HostT* BcmHostTable::incRefOrCreateBcmHost(
    HostMap<KeyT, HostT>* map, const KeyT& key, Args... args) {
  auto ret = map->emplace(key, std::make_pair(nullptr, 1));
  auto& iter = ret.first;
  if (!ret.second) {
    // there was an entry already there
    iter->second.second++;  // increase the reference counter
    return iter->second.first.get();
  }
  SCOPE_FAIL {
    map->erase(iter);
  };
  auto newHost = folly::make_unique<HostT>(hw_, key.first, key.second, args...);
  auto hostPtr = newHost.get();
  iter->second.first = std::move(newHost);
  return hostPtr;
}

BcmHost* BcmHostTable::incRefOrCreateBcmHost(
    opennsl_vrf_t vrf, const IPAddress& addr) {
  return incRefOrCreateBcmHost(&hosts_, std::make_pair(vrf, addr));
}

BcmHost* BcmHostTable::incRefOrCreateBcmHost(
    opennsl_vrf_t vrf, const IPAddress& addr, opennsl_if_t egressId) {
  return incRefOrCreateBcmHost(&hosts_, std::make_pair(vrf, addr), egressId);
}

BcmEcmpHost* BcmHostTable::incRefOrCreateBcmEcmpHost(
    opennsl_vrf_t vrf, const RouteForwardNexthops& fwd) {
  return incRefOrCreateBcmHost(&ecmpHosts_, std::make_pair(vrf, fwd));
}

template<typename KeyT, typename HostT, typename... Args>
HostT* BcmHostTable::getBcmHostIf(const HostMap<KeyT, HostT>* map,
                                  Args... args) const {
  KeyT key{args...};
  auto iter = map->find(key);
  if (iter == map->end()) {
    return nullptr;
  }
  return iter->second.first.get();
}

BcmHost* BcmHostTable::getBcmHostIf(opennsl_vrf_t vrf,
                                    const IPAddress& addr) const {
  return getBcmHostIf(&hosts_, vrf, addr);
}

BcmHost* BcmHostTable::getBcmHost(
    opennsl_vrf_t vrf, const IPAddress& addr) const {
  auto host = getBcmHostIf(vrf, addr);
  if (!host) {
    throw FbossError("Cannot find BcmHost vrf=", vrf, " addr=", addr);
  }
  return host;
}

BcmEcmpHost* BcmHostTable::getBcmEcmpHostIf(
    opennsl_vrf_t vrf, const RouteForwardNexthops& fwd) const {
  return getBcmHostIf(&ecmpHosts_, vrf, fwd);
}

BcmEcmpHost* BcmHostTable::getBcmEcmpHost(
    opennsl_vrf_t vrf, const RouteForwardNexthops& fwd) const {
  auto host = getBcmEcmpHostIf(vrf, fwd);
  if (!host) {
    throw FbossError("Cannot find BcmEcmpHost vrf=", vrf, " fwd=", fwd);
  }
  return host;
}

template<typename KeyT, typename HostT, typename... Args>
HostT* BcmHostTable::derefBcmHost(HostMap<KeyT, HostT>* map,
                                  Args... args) noexcept {
  KeyT key{args...};
  auto iter = map->find(key);
  if (iter == map->end()) {
    return nullptr;
  }
  auto& entry = iter->second;
  CHECK_GT(entry.second, 0);
  if (--entry.second == 0) {
    map->erase(iter);
    return nullptr;
  }
  return entry.first.get();
}

BcmHost* BcmHostTable::derefBcmHost(
    opennsl_vrf_t vrf, const IPAddress& addr) noexcept {
  return derefBcmHost(&hosts_, vrf, addr);
}

BcmEcmpHost* BcmHostTable::derefBcmEcmpHost(
    opennsl_vrf_t vrf, const RouteForwardNexthops& fwd) noexcept {
  return derefBcmHost(&ecmpHosts_, vrf, fwd);
}

BcmEgressBase* BcmHostTable::incEgressReference(opennsl_if_t egressId) {
  auto it = egressMap_.find(egressId);
  CHECK(it != egressMap_.end());
  it->second.second++;
  return it->second.first.get();
}

BcmEgressBase* BcmHostTable::derefEgress(opennsl_if_t egressId) {
  auto it = egressMap_.find(egressId);
  CHECK(it != egressMap_.end());
  CHECK_GT(it->second.second, 0);
  if (--it->second.second == 0) {
    egressMap_.erase(egressId);
    return nullptr;
  }
  return it->second.first.get();
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
  auto ret = egressMap_.emplace(id, std::make_pair(std::move(egress), 1));
  CHECK(ret.second);
}
}}
