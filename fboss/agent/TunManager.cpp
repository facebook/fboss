/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "TunManager.h"

extern "C" {
#include <netlink/route/link.h>
#include <netlink/route/addr.h>
#include <netlink/route/route.h>
#include <netlink/route/rule.h>
#include <net/if.h>
}

#include "fboss/agent/RxPacket.h"
#include "fboss/agent/NlError.h"
#include "fboss/agent/TunIntf.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/state/InterfaceMap.h"
#include "fboss/agent/state/Interface.h"
#include <folly/io/async/EventBase.h>

#include <boost/container/flat_set.hpp>

namespace facebook { namespace fboss {

using folly::IPAddress;
using folly::EventBase;

TunManager::TunManager(SwSwitch *sw, EventBase *evb) : sw_(sw), evb_(evb) {
  sock_ = nl_socket_alloc();
  if (!sock_) {
    throw FbossError("failed to allocate libnl socket");
  }
  auto error = nl_connect(sock_, NETLINK_ROUTE);
  nlCheckError(error, "failed to connect netlink socket to NETLINK_ROUTE");
}

TunManager::~TunManager() {
  if (observingState_) {
    sw_->unregisterStateObserver(this);
  }

  stop();
  nl_close(sock_);
  nl_socket_free(sock_);
}

int TunManager::getMinMtu(std::shared_ptr<SwitchState> state) {
  int minMtu = INT_MAX;
  auto intfMap = state->getInterfaces();
  for (const auto& intf : intfMap->getAllNodes()) {
    minMtu = std::min(intf.second->getMtu(), minMtu);
  }
  return minMtu == INT_MAX ? 1500 : minMtu;
}

void TunManager::addIntf(RouterID rid, const std::string& name, int ifIdx) {
  auto ret = intfs_.emplace(rid, nullptr);
  if (!ret.second) {
    throw FbossError("Duplicate interface ", name, " in router ", rid);
  }
  SCOPE_FAIL {
    intfs_.erase(ret.first);
  };
  ret.first->second.reset(
      new TunIntf(sw_, evb_, name, rid, ifIdx, getMinMtu(sw_->getState())));
}

void TunManager::addIntf(RouterID rid, const Interface::Addresses& addrs) {
  auto ret = intfs_.emplace(rid, nullptr);
  if (!ret.second) {
    throw FbossError("Duplicate interface for router ", rid);
  }
  SCOPE_FAIL {
    intfs_.erase(ret.first);
  };
  auto intf = folly::make_unique<TunIntf>(
      sw_, evb_, rid, addrs, getMinMtu(sw_->getState()));
  SCOPE_FAIL {
    intf->setDelete();
  };
  const auto& name = intf->getName();
  auto index = intf->getIfIndex();
  // bring up the interface so that we can add the default route next step
  bringupIntf(name, index);
  // create a new route table for this rid
  addRouteTable(index, rid);
  // add all addresses
  for (const auto& addr : addrs) {
    addTunAddress(name, rid, index, addr.first, addr.second);
  }
  ret.first->second = std::move(intf);
}

void TunManager::removeIntf(RouterID rid) {
  auto iter = intfs_.find(rid);
  if (iter == intfs_.end()) {
    throw FbossError("Cannot find to be delete interface for router ", rid);
  }
  auto& intf = iter->second;
  // remove the route table
  removeRouteTable(intf->getIfIndex(), intf->getRouterId());
  intf->setDelete();
  intfs_.erase(iter);
}

void TunManager::addProbedAddr(int ifIndex, const IPAddress& addr,
                               uint8_t mask) {
  for (auto& intf : intfs_) {
    if (intf.second->getIfIndex() == ifIndex) {
      intf.second->addAddress(addr, mask);
      return;
    }
  }
  // This function is called for all interface addresses discovered from
  // the host. Since we only create TunIntf object for TUN interfaces,
  // it is normal we cannot find the interface matching the ifIndex provided.
  VLOG(3) << "Cannot find interface @ index " << ifIndex
          << " for probed address " << addr.str() << "/"
          << static_cast<int>(mask);
}

void TunManager::bringupIntf(const std::string& name, int ifIndex) {
  // TODO: We need to change the interface status based on real HW
  // interface status (up/down). Make them up all the time for now.
  auto link = rtnl_link_alloc();
  SCOPE_EXIT { rtnl_link_put(link); };
  if (!link) {
    throw FbossError("Failed to allocate link to bring up");
  }
  rtnl_link_set_ifindex(link, ifIndex);
  rtnl_link_set_family(link, AF_UNSPEC);
  rtnl_link_set_flags(link, IFF_UP);
  auto error = rtnl_link_change(sock_, link, link, 0);
  nlCheckError(error, "Failed to bring up interface ", name,
                " @ index ", ifIndex);
  LOG(INFO) << "Brought up interface " << name << " @ index " << ifIndex;
}

inline int TunManager::getTableId(RouterID rid) const {
  // Kernel only supports up to 256 tables. The last few are used by kernel
  // as main, default, and local. We start with table base (100) downwards.
  const int tidBase = 100;
  CHECK_GT(tidBase - rid, 0);
  return tidBase - rid;
}

void TunManager::addRemoveTable(int ifIdx, RouterID rid, bool add) {
  // We just store default routes (one for IPv4 and one for IPv6) in each route
  // table.
  const folly::IPAddress addrs[] = {
    IPAddress{"0.0.0.0"},       // v4 default
    IPAddress{"::0"},           // v6 default
  };

  for (const auto& addr : addrs) {
    auto route = rtnl_route_alloc();
    SCOPE_EXIT { rtnl_route_put(route); };

    auto error = rtnl_route_set_family(route, addr.family());
    nlCheckError(error, "Failed to set family to", addr.family());
    rtnl_route_set_table(route, getTableId(rid));
    rtnl_route_set_scope(route, RT_SCOPE_UNIVERSE);
    rtnl_route_set_protocol(route, RTPROT_FBOSS);
    error = rtnl_route_set_type(route, RTN_UNICAST);
    nlCheckError(error, "Failed to set type to ", RTN_UNICAST);
    auto destaddr = nl_addr_build(addr.family(),
       const_cast<unsigned char *>(addr.bytes()), addr.byteCount());
    if (!destaddr) {
      throw FbossError("Failed to build destination address for ",  addr);
    }
    SCOPE_EXIT { nl_addr_put(destaddr); };
    nl_addr_set_prefixlen(destaddr, 0);
    error = rtnl_route_set_dst(route, destaddr);
    nlCheckError(error, "Failed to set destination route to ", addr);
    auto nexthop = rtnl_route_nh_alloc();
    if (!nexthop) {
      throw FbossError("Failed to allocate nexthop");
    }
    rtnl_route_nh_set_ifindex(nexthop, ifIdx);
    rtnl_route_add_nexthop(route, nexthop);
    if (add) {
      error = rtnl_route_add(sock_, route, NLM_F_REPLACE);
    } else {
      error = rtnl_route_delete(sock_, route, 0);
    }
    nlCheckError(error, "Failed to ", add ? "add" : "remove",
                  " default route ", addr, " @ index ", ifIdx,
                  " in table ", getTableId(rid), " for router ", rid);
    LOG(INFO) << (add ? "Added" : "Removed") << " default route " << addr
              << " @ index " << ifIdx << " in table " << getTableId(rid)
              << " for router " << rid;
  }
}

void TunManager::addRemoveSourceRouteRule(
    RouterID rid, folly::IPAddress addr, bool add) {
  auto rule = rtnl_rule_alloc();
  SCOPE_EXIT { rtnl_rule_put(rule); };
  rtnl_rule_set_family(rule, addr.family());
  rtnl_rule_set_table(rule, getTableId(rid));
  rtnl_rule_set_action(rule, FR_ACT_TO_TBL);
  auto sourceaddr = nl_addr_build(addr.family(),
    const_cast<unsigned char *>(addr.bytes()), addr.byteCount());
  if (!sourceaddr) {
    throw FbossError("Failed to build destination address for ",  addr);
  }
  SCOPE_EXIT { nl_addr_put(sourceaddr); };
  nl_addr_set_prefixlen(sourceaddr, addr.bitCount());
  auto error = rtnl_rule_set_src(rule, sourceaddr);
  nlCheckError(error, "Failed to set destination route to ", addr);

  if (add) {
    error = rtnl_rule_add(sock_, rule, NLM_F_REPLACE);
  } else {
    error = rtnl_rule_delete(sock_, rule, 0);
  }
  nlCheckError(error, "Failed to ", add ? "add" : "remove",
                " rule for address ", addr,
                " to lookup table ", getTableId(rid),
                " for router ", rid);
  LOG(INFO) << (add ? "Added" : "Removed") << " rule for address " << addr
            << " to lookup table " << getTableId(rid) << " for router " << rid;
}

void TunManager::addRemoveTunAddress(
    const std::string& name, uint32_t ifIndex,
    folly::IPAddress addr, uint8_t mask, bool add) {
  auto tunaddr = rtnl_addr_alloc();
  SCOPE_EXIT { rtnl_addr_put(tunaddr); };
  if (!tunaddr) {
    throw FbossError("Failed to allocate address");
  }
  rtnl_addr_set_family(tunaddr, addr.family());
  auto localaddr = nl_addr_build(addr.family(),
    const_cast<unsigned char *>(addr.bytes()), addr.byteCount());
  if (!localaddr) {
    throw FbossError("Failed to build destination address for ",  addr);
  }
  SCOPE_EXIT { nl_addr_put(localaddr); };
  auto error = rtnl_addr_set_local(tunaddr, localaddr);
  nlCheckError(error, "Failed to set local address to ", addr);
  rtnl_addr_set_prefixlen(tunaddr, mask);
  rtnl_addr_set_ifindex(tunaddr, ifIndex);
  if (add) {
    error = rtnl_addr_add(sock_, tunaddr, NLM_F_EXCL);
  } else {
    error = rtnl_addr_delete(sock_, tunaddr, 0);
  }
  nlCheckError(error, "Failed to ", add ? "add" : "remove",
                " address ", addr, "/", static_cast<int>(mask),
                " to interface ", name, " @ index ", ifIndex);
  LOG(INFO) << (add ? "Added" : "Removed") << " address " << addr.str() << "/"
            << static_cast<int>(mask) << " on interface " << name
            << " @ index " << ifIndex;
}

void TunManager::addTunAddress(
    const std::string& name, RouterID rid, uint32_t ifIndex,
    folly::IPAddress addr, uint8_t mask) {
  addRemoveSourceRouteRule(rid, addr, true);
  SCOPE_FAIL {
    try {
      addRemoveSourceRouteRule(rid, addr, false);
    } catch (const std::exception& ex) {
    }
  };
  addRemoveTunAddress(name, ifIndex, addr, mask, true);
}

void TunManager::removeTunAddress(
    const std::string& name, RouterID rid, uint32_t ifIndex,
    folly::IPAddress addr, uint8_t mask) {
  addRemoveSourceRouteRule(rid, addr, false);
  SCOPE_FAIL {
    try {
      addRemoveSourceRouteRule(rid, addr, true);
    } catch (const std::exception& ex) {
    }
  };
  addRemoveTunAddress(name, ifIndex, addr, mask, false);
}

void TunManager::start() const {
  for (const auto& intf : intfs_) {
    intf.second->start();
  }
}

void TunManager::stop() const {
  for (const auto& intf : intfs_) {
    intf.second->stop();
  }
}

void TunManager::linkProcessor(struct nl_object *obj, void *data) {
  struct rtnl_link * link = reinterpret_cast<struct rtnl_link *>(obj);
  const auto name = rtnl_link_get_name(link);
  if (!name) {
    throw FbossError(" Device @ index ",
                       rtnl_link_get_ifindex(link), " does not have a name");
  }
  if (!TunIntf::isTunIntf(name)) {
    VLOG(3) << "Ignore interface " << name
                << " because it is not a tun interface";
    return;
  }
  static_cast<TunManager*>(data)->addIntf(TunIntf::getRidFromName(name),
          std::string(name), rtnl_link_get_ifindex(link));
}

void TunManager::addressProcessor(struct nl_object *obj, void *data) {
  struct rtnl_addr *addr = reinterpret_cast<struct rtnl_addr *>(obj);
  auto family = rtnl_addr_get_family(addr);
  if (family != AF_INET && family != AF_INET6) {
    VLOG(3) << "Skip address from device @ index "
              << rtnl_addr_get_ifindex(addr)
              << " because of its address family " << family;
    return;
  }
  auto localaddr = rtnl_addr_get_local(addr);
  if (!localaddr) {
   VLOG(3) << "Skip address from device @ index "
              << rtnl_addr_get_ifindex(addr)
              << " because of it does not have a local address ";
   return;
  }
  char buf[INET6_ADDRSTRLEN];
  nl_addr2str(localaddr, buf, sizeof(buf));
  if (!*buf) {
    VLOG(3) << "Device @ index " << rtnl_addr_get_ifindex(addr)
            << " does not have an address at family " << family;
  }
  auto ipaddr  = IPAddress::createNetwork(buf, -1, false).first;
  static_cast<TunManager *>(data)->addProbedAddr(
    rtnl_addr_get_ifindex(addr),
    ipaddr,
    nl_addr_get_prefixlen(localaddr)
  );
}

void TunManager::probe() {
  std::lock_guard<std::mutex> lock(mutex_);
  if (!probeDone_) {
    doProbe(lock);
  }
}

void TunManager::doProbe(std::lock_guard<std::mutex>& lock) {
  CHECK(!probeDone_);  // Callers must check for probeDone before calling
  stop();                       // stop all interfaces
  intfs_.clear();               // clear all interface info
  //get links
  struct nl_cache *cache;
  auto error = rtnl_link_alloc_cache(sock_, AF_UNSPEC, &cache);
  nlCheckError(error, "Cannot get links from Kernel");
  SCOPE_EXIT { nl_cache_free(cache); };
  nl_cache_foreach(cache, &TunManager::linkProcessor, this);
  //get addresses
  struct nl_cache *addressCache;
  error = rtnl_addr_alloc_cache(sock_, &addressCache);
  nlCheckError(error, "Cannot get addresses from Kernel");
  SCOPE_EXIT { nl_cache_free(addressCache); };
  nl_cache_foreach(addressCache, &TunManager::addressProcessor, this);
  // Bring up all interfaces. Interfaces could be already up.
  for (const auto& intf : intfs_) {
    bringupIntf(intf.second->getName(), intf.second->getIfIndex());
  }
  start();
  probeDone_ = true;
}

void TunManager::sync(std::shared_ptr<SwitchState> state) {
  // prepare the existing and new addresses
  typedef Interface::Addresses Addresses;
  typedef boost::container::flat_map<RouterID, Addresses> AddrMap;
  AddrMap newAddrs;
  auto map = state->getInterfaces();
  for (const auto& intf : map->getAllNodes()) {
    const auto& addrs = intf.second->getAddresses();
    newAddrs[intf.second->getRouterID()].insert(addrs.begin(), addrs.end());
  }
  // Hold mutex while changing interfaces
  std::lock_guard<std::mutex> lock(mutex_);
  if (!probeDone_) {
    doProbe(lock);
  }

  AddrMap oldAddrs;
  auto newMinMtu = getMinMtu(state);
  for (const auto& intf : intfs_) {
    const auto& addrs = intf.second->getAddresses();
    oldAddrs[intf.first].insert(addrs.begin(), addrs.end());
    if (intf.second->getMtu() != newMinMtu) {
      intf.second->setMtu(newMinMtu);
    }
  }

  auto applyAddrChanges =
    [&](const std::string& name, RouterID rid, int ifIndex,
        const Addresses& oldAddrs, const Addresses& newAddrs) {
    applyChanges(
        oldAddrs, newAddrs,
        [&](Addresses::const_iterator& oldIter,
            Addresses::const_iterator& newIter) {
          if (oldIter->second == newIter->second) {
            // addresses and masks are both same
            return;
          }
          removeTunAddress(name, rid, ifIndex, oldIter->first, oldIter->second);
          addTunAddress(name, rid, ifIndex, newIter->first, newIter->second);
        },
        [&](Addresses::const_iterator& newIter) {
          addTunAddress(name, rid, ifIndex, newIter->first, newIter->second);
        },
        [&](Addresses::const_iterator& oldIter) {
          removeTunAddress(name, rid, ifIndex, oldIter->first, oldIter->second);
        });
  };

  applyChanges(
      oldAddrs, newAddrs,
      [&](const AddrMap::const_iterator& oldIter,
          const AddrMap::const_iterator& newIter) {
        const auto& oldAddrs = oldIter->second;
        const auto& newAddrs = newIter->second;
        auto iter = intfs_.find(oldIter->first);
        CHECK(iter != intfs_.end());
        const auto& intf = iter->second;
        int ifIndex = intf->getIfIndex();
        auto rid = intf->getRouterId();
        const auto& name = intf->getName();
        applyAddrChanges(name, rid, ifIndex, oldAddrs, newAddrs);
        iter->second->setAddresses(newAddrs);
      },
      [&](const AddrMap::const_iterator& newIter) {
        addIntf(newIter->first, newIter->second);
      },
      [&](const AddrMap::const_iterator& oldIter) {
        removeIntf(oldIter->first);
      });

  start();
}

void TunManager::startProbe() {
  evb_->runInEventBaseThread([this]() {
      this->probe();
    });
}

void TunManager::startObservingUpdates() {
  sw_->registerStateObserver(this, "TunManager");
  observingState_ = true;
}

void TunManager::stateUpdated(const StateDelta& delta) {
  // TODO(aeckert): We currently compare the entire interface map instead
  // of using the iterator in this delta because some of the interfaces may get
  // get probed from hardware, before they are in the SwitchState. It would be
  // nicer if we did a little more work at startup to sync the state, perhaps
  // updating the SwitchState with the probed interfaces. This would allow us
  // to reuse the iterator in the delta for more readable code and also not
  // have to worry about waiting to listen to updates until the SwSwitch is in
  // the configured state. t4155406 should also help with that.

  auto state = delta.newState();
  evb_->runInEventBaseThread([this, state]() {
      this->sync(state);
    });
}

bool TunManager::sendPacketToHost(std::unique_ptr<RxPacket> pkt) {
  auto rid = pkt->getRouterID();
  std::lock_guard<std::mutex> lock(mutex_);
  auto iter = intfs_.find(rid);
  if (iter == intfs_.end()) {
    // the router ID has been deleted, make a log, and skip the pkt
    VLOG(4) << "Dropping a packet for unknown router " << rid;
    return false;
  }
  return iter->second->sendPacketToHost(std::move(pkt));
}


// TODO(aeckert): Find a way to reuse the iterator from NodeMapDelta here as
// this basically duplicates that code.
template<typename MAPNAME, typename CHANGEFN, typename ADDFN, typename REMOVEFN>
void TunManager::applyChanges(const MAPNAME& oldMap,
                              const MAPNAME& newMap,
                              CHANGEFN changeFn,
                              ADDFN addFn,
                              REMOVEFN removeFn) {
  auto oldIter = oldMap.begin();
  auto newIter = newMap.begin();
  while (oldIter != oldMap.end() && newIter != newMap.end()) {
    if (oldIter->first < newIter->first) {
      removeFn(oldIter);
      oldIter++;
    } else if (oldIter->first > newIter->first) {
      addFn(newIter);
      newIter++;
    } else {
      changeFn(oldIter, newIter);
      oldIter++;
      newIter++;
    }
  }
  for (; oldIter != oldMap.end(); oldIter++) {
    removeFn(oldIter);
  }
  for (; newIter != newMap.end(); newIter++) {
    addFn(newIter);
  }
}

}}
