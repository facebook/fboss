/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "NeighborUpdater.h"
#include "fboss/agent/FbossError.h"
#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/ArpHandler.h"
#include "fboss/agent/IPv6Handler.h"
#include "fboss/agent/NexthopToRouteCount.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/state/VlanMap.h"
#include "fboss/agent/state/Vlan.h"
#include "fboss/agent/state/ArpTable.h"
#include "fboss/agent/state/NdpTable.h"
#include "fboss/agent/state/StateDelta.h"
#include "fboss/agent/state/Interface.h"
#include "fboss/agent/state/RouteForwardInfo.h"
#include "fboss/agent/ArpCache.h"
#include "fboss/agent/NdpCache.h"

#include <folly/futures/Future.h>
#include <list>
#include <mutex>
#include <string>
#include <vector>
#include <boost/container/flat_map.hpp>

using std::chrono::seconds;
using std::shared_ptr;
using boost::container::flat_map;
using folly::Future;
using folly::Unit;
using folly::IPAddress;
using folly::MacAddress;
using folly::IPAddressV4;
using folly::IPAddressV6;

namespace facebook { namespace fboss {

class UnresolvedNhopsProber : private folly::AsyncTimeout {
 public:
  explicit UnresolvedNhopsProber(SwSwitch *sw) :
    AsyncTimeout(sw->getBackgroundEVB()),
    sw_(sw),
    // Probe every 5 secs (make it faster ?)
    interval_(5) {}

  static void start(UnresolvedNhopsProber* me) {
    me->scheduleTimeout(me->interval_);
  }
  static void stop(UnresolvedNhopsProber* me) {
    delete me;
  }

  void stateChanged(const StateDelta& delta) {
    std::lock_guard<std::mutex> g(lock_);
    nhops2RouteCount_.stateChanged(delta);
  }

  void timeoutExpired() noexcept override;

 private:
  // Need lock since we may get called from both the update
  // thread (stateChanged) and background thread (timeoutExpired)
  std::mutex lock_;
  SwSwitch *sw_{nullptr};
  NexthopToRouteCount nhops2RouteCount_;
  seconds interval_;
};

void UnresolvedNhopsProber::timeoutExpired() noexcept {
  std::lock_guard<std::mutex> g(lock_);
  auto state = sw_->getState();
  for (const auto& ridAndNhopsRefCounts : nhops2RouteCount_) {
    for (const auto& nhopAndRefCount : ridAndNhopsRefCounts.second) {
      const auto& nhop = nhopAndRefCount.first;
      auto intf = state->getInterfaces()->getInterfaceIf(nhop.intf);
      if (!intf) {
        continue; // interface got unconfigured
      }
      // Probe all nexthops for which either don't have a L2 entry
      // or the entry is not resolved (port == 0). Note that we do
      // not exclude pending entries here since in case of recursive
      // routes we might get packets with destination set to prefix
      // that needs to be resolved recursively. In ARP and NDP code
      // we do not do route lookup when deciding to send ARP/NDP requests.
      // So we would only try to ARP/NDP for the destination if it
      // is in one of the interface subnets (which it won't be else
      // we won't have needed recursive resolution). So ARP/NDP for
      // all unresolved next hops. We could also consider doing route
      // lookups in ARP/NDP code, but by probing all unresolved next
      // hops we effectively do the same thing, since the next hops
      // probed come from after the route was (recursively) resolved.
      auto vlan = state->getVlans()->getVlanIf(intf->getVlanID());
      CHECK(vlan); // must have vlan for configrued inteface
      if (nhop.nexthop.isV4()) {
        auto nhop4 = nhop.nexthop.asV4();
        auto arpEntry = vlan->getArpTable()->getEntryIf(nhop4);
        if (!arpEntry || arpEntry->getPort() == 0) {
          VLOG(2) <<" Sending probe for unresolved next hop: " << nhop4;
          ArpHandler::sendArpRequest(sw_, vlan, nhop4);
        }
      } else {
        auto nhop6 = nhop.nexthop.asV6();
        auto ndpEntry = vlan->getNdpTable()->getEntryIf(nhop6);
        if (!ndpEntry || ndpEntry->getPort() == 0) {
          VLOG(2) <<" Sending probe for unresolved next hop: " << nhop6;
          IPv6Handler::sendNeighborSolicitation(sw_, nhop6, vlan);
        }
      }
    }
  }
  scheduleTimeout(interval_);

}

NeighborUpdater::NeighborUpdater(SwSwitch* sw)
    : AutoRegisterStateObserver(sw, "NeighborUpdater"),
      sw_(sw),
      unresolvedNhopsProber_(new UnresolvedNhopsProber(sw)) {

  bool ret = sw_->getBackgroundEVB()->runInEventBaseThread(
    UnresolvedNhopsProber::start, unresolvedNhopsProber_);
  if (!ret) {
    delete unresolvedNhopsProber_;
    throw FbossError("failed to start unresolved next hops prober");
  }
}

NeighborUpdater::~NeighborUpdater() {
  // Stop the prober now
  std::function<void()> stopProber = [=]() {
    UnresolvedNhopsProber::stop(unresolvedNhopsProber_);
  };

  Future<Unit> f = via(sw_->getBackgroundEVB())
    .then(stopProber)
    .onError([=] (const std::exception& e) {
          LOG (FATAL) << "Failed to stop unresolved next hops prober ";
        });

  // wait for prober to stop
  f.get();

  // reset the map of caches. This should call the destructors of
  // each NeighborCache and block until everything is stopped.
  caches_.clear();
}

shared_ptr<ArpCache> NeighborUpdater::getArpCacheFor(VlanID vlan) {
  std::lock_guard<std::mutex> g(cachesMutex_);
  return getArpCacheInternal(vlan);
}

shared_ptr<NdpCache> NeighborUpdater::getNdpCacheFor(VlanID vlan) {
  std::lock_guard<std::mutex> g(cachesMutex_);
  return getNdpCacheInternal(vlan);
}

void NeighborUpdater::getArpCacheData(std::vector<ArpEntryThrift>& arpTable) {
  std::list<ArpEntryThrift> entries;
  {
    std::lock_guard<std::mutex> g(cachesMutex_);
    for (auto it = caches_.begin(); it != caches_.end(); ++it) {
      entries.splice(entries.end(), it->second->arpCache->getArpCacheData());
    }
  }
  arpTable.reserve(entries.size());
  arpTable.insert(arpTable.begin(),
                  std::make_move_iterator(std::begin(entries)),
                  std::make_move_iterator(std::end(entries)));
}

void NeighborUpdater::getNdpCacheData(std::vector<NdpEntryThrift>& ndpTable) {
  std::list<NdpEntryThrift> entries;
  {
    std::lock_guard<std::mutex> g(cachesMutex_);
    for (auto it = caches_.begin(); it != caches_.end(); ++it) {
      entries.splice(entries.end(), it->second->ndpCache->getNdpCacheData());
    }
  }
  ndpTable.reserve(entries.size());
  ndpTable.insert(ndpTable.begin(),
                  std::make_move_iterator(std::begin(entries)),
                  std::make_move_iterator(std::end(entries)));
}

shared_ptr<ArpCache> NeighborUpdater::getArpCacheInternal(VlanID vlan) {
  auto res = caches_.find(vlan);
  if (res == caches_.end()) {
    throw FbossError("Tried to get Arp cache non-existent vlan ", vlan);
  }
  return res->second->arpCache;
}
shared_ptr<NdpCache> NeighborUpdater::getNdpCacheInternal(VlanID vlan) {
  auto res = caches_.find(vlan);
  if (res == caches_.end()) {
    throw FbossError("Tried to get Ndp cache non-existent vlan ", vlan);
  }
  return res->second->ndpCache;
}

void NeighborUpdater::sentNeighborSolicitation(VlanID vlan,
                                               IPAddressV6 ip) {
  auto cache = getNdpCacheFor(vlan);
  cache->sentNeighborSolicitation(ip);
}

void NeighborUpdater::receivedNdpMine(VlanID vlan,
                                      IPAddressV6 ip,
                                      MacAddress mac,
                                      PortID port,
                                      ICMPv6Type type) {
  auto cache = getNdpCacheFor(vlan);
  cache->receivedNdpMine(ip, mac, port, type);
}

void NeighborUpdater::receivedNdpNotMine(VlanID vlan,
                                         IPAddressV6 ip,
                                         MacAddress mac,
                                         PortID port,
                                         ICMPv6Type type) {
  auto cache = getNdpCacheFor(vlan);
  cache->receivedNdpNotMine(ip, mac, port, type);
}

void NeighborUpdater::sentArpRequest(VlanID vlan,
                                     IPAddressV4 ip) {
  auto cache = getArpCacheFor(vlan);
  cache->sentArpRequest(ip);
}

void NeighborUpdater::receivedArpMine(VlanID vlan,
                                      IPAddressV4 ip,
                                      MacAddress mac,
                                      PortID port,
                                      ArpOpCode op) {
  auto cache = getArpCacheFor(vlan);
  cache->receivedArpMine(ip, mac, port, op);
}

void NeighborUpdater::receivedArpNotMine(VlanID vlan,
                                         IPAddressV4 ip,
                                         MacAddress mac,
                                         PortID port,
                                         ArpOpCode op) {
  auto cache = getArpCacheFor(vlan);
  cache->receivedArpNotMine(ip, mac, port, op);
}

void NeighborUpdater::portDown(PortID port) {
  for (auto vlanCaches : caches_) {
    auto arpCache = vlanCaches.second->arpCache;
    arpCache->portDown(port);

    auto ndpCache = vlanCaches.second->ndpCache;
    ndpCache->portDown(port);
  }
}

// expects the cachesMutex_ to be held
bool NeighborUpdater::flushEntryImpl(VlanID vlan, IPAddress ip) {
  if (ip.isV4()) {
    auto cache = getArpCacheInternal(vlan);
    return cache->flushEntryBlocking(ip.asV4());
  }
  auto cache = getNdpCacheInternal(vlan);
  return cache->flushEntryBlocking(ip.asV6());
}

uint32_t NeighborUpdater::flushEntry(VlanID vlan, IPAddress ip) {
  // clone caches_ so we don't need to hold the lock while flushing entries
  boost::container::flat_map<VlanID, std::shared_ptr<NeighborCaches>> caches;
  {
    std::lock_guard<std::mutex> g(cachesMutex_);
    caches = caches_;
  }

  uint32_t count{0};
  if (vlan == VlanID(0)) {
    for (auto it = caches.begin(); it != caches.end(); ++it) {
      if (flushEntryImpl(it->first, ip)) {
        ++count;
      }
    }
  } else {
    if (flushEntryImpl(vlan, ip)) {
      ++count;
    }
  }
  return count;
}

void NeighborUpdater::stateUpdated(const StateDelta& delta) {
  CHECK(sw_->getUpdateEVB()->inRunningEventBaseThread());
  for (const auto& entry : delta.getVlansDelta()) {
    sendNeighborUpdates(entry);
    auto oldEntry = entry.getOld();
    auto newEntry = entry.getNew();

    if (!newEntry) {
      vlanDeleted(oldEntry.get());
    } else if (!oldEntry) {
      vlanAdded(delta.newState().get(), newEntry.get());
    } else {
      vlanChanged(oldEntry.get(), newEntry.get());
    }
  }
  unresolvedNhopsProber_->stateChanged(delta);
}

template<typename T>
void collectPresenceChange(const T& delta,
                          std::vector<std::string>* added,
                          std::vector<std::string>* deleted) {
  for (const auto& entry : delta) {
    auto oldEntry = entry.getOld();
    auto newEntry = entry.getNew();
    if (oldEntry && !newEntry) {
      if (oldEntry->nonZeroPort()) {
        deleted->push_back(folly::to<std::string>(oldEntry->getIP()));
      }
    } else if (newEntry && !oldEntry) {
      if (newEntry->nonZeroPort()) {
        added->push_back(folly::to<std::string>(newEntry->getIP()));
      }
    } else {
      if (oldEntry->zeroPort() && newEntry->nonZeroPort()) {
        // Entry was resolved, add it
        added->push_back(folly::to<std::string>(newEntry->getIP()));
      } else if (oldEntry->nonZeroPort() && newEntry->zeroPort()) {
        // Entry became unresolved, prune it
        deleted->push_back(folly::to<std::string>(oldEntry->getIP()));
      }
    }
  }
}

void NeighborUpdater::sendNeighborUpdates(const VlanDelta& delta) {
  std::vector<std::string> added;
  std::vector<std::string> deleted;
  collectPresenceChange(delta.getArpDelta(), &added, &deleted);
  collectPresenceChange(delta.getNdpDelta(), &added, &deleted);
  if (!(added.empty() && deleted.empty())) {
    sw_->invokeNeighborListener(added, deleted);
  }
}

void NeighborUpdater::vlanAdded(const SwitchState* state, const Vlan* vlan) {
  CHECK(sw_->getUpdateEVB()->inRunningEventBaseThread());

  auto vlanID = vlan->getID();
  auto vlanName = vlan->getName();

  auto intfID = vlan->getInterfaceID();
  auto caches
      = std::make_shared<NeighborCaches>(sw_, state, vlanID, vlanName, intfID);

  // We need to populate the caches from the SwitchState when a vlan is added
  // After this, we no longer process Arp or Ndp deltas for this vlan.
  caches->arpCache->repopulate(vlan->getArpTable());
  caches->ndpCache->repopulate(vlan->getNdpTable());

  {
    std::lock_guard<std::mutex> g(cachesMutex_);
    caches_.emplace(vlan->getID(), std::move(caches));
  }
}

void NeighborUpdater::vlanDeleted(const Vlan* vlan) {
  CHECK(sw_->getUpdateEVB()->inRunningEventBaseThread());
  std::shared_ptr<NeighborCaches> removedEntry;
  {
    std::lock_guard<std::mutex> g(cachesMutex_);
    auto iter = caches_.find(vlan->getID());
    if (iter != caches_.end()) {
      // we copy the entry over so we don't destroy the
      // caches while holding the lock.
      removedEntry = std::move(iter->second);
      caches_.erase(iter);
    } else {
      // TODO(aeckert): May want to fatal here when a cache doesn't exist for a
      // specific vlan. Need to make sure that caches are correctly created for
      // the initial SwitchState to avoid false positives
      VLOG(0) << "Deleted Vlan with no corresponding NeighborCaches";
    }
  }
}

void NeighborUpdater::vlanChanged(const Vlan* oldVlan, const Vlan* newVlan) {
  if (newVlan->getInterfaceID() == oldVlan->getInterfaceID()
      && newVlan->getName().compare(oldVlan->getName()) == 0) {
    // For now we only care about changes to the interfaceID and VlanName
    return;
  }

  CHECK(sw_->getUpdateEVB()->inRunningEventBaseThread());
  {
    std::lock_guard<std::mutex> g(cachesMutex_);
    auto iter = caches_.find(newVlan->getID());
    if (iter != caches_.end()) {
      auto intfID = newVlan->getInterfaceID();
      iter->second->arpCache->setIntfID(intfID);
      iter->second->ndpCache->setIntfID(intfID);
      auto vlanName = newVlan->getName();
      iter->second->arpCache->setVlanName(vlanName);
      iter->second->ndpCache->setVlanName(vlanName);
    } else {
      // TODO(aeckert): May want to fatal here when a cache doesn't exist for a
      // specific vlan. Need to make sure that caches are correctly created for
      // the initial SwitchState to avoid false positives
      VLOG(0) << "Changed Vlan with no corresponding NeighborCaches";
    }
  }
}

}} // facebook::fboss
