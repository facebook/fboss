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
#include <linux/if.h>
}

#include "fboss/agent/RxPacket.h"
#include "fboss/agent/NlError.h"
#include "fboss/agent/TunIntf.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/state/InterfaceMap.h"
#include "fboss/agent/state/Interface.h"
#include <folly/io/async/EventBase.h>

#include <boost/container/flat_set.hpp>

namespace {
  const int kDefaultMtu = 1500;
}

namespace facebook { namespace fboss {

using folly::IPAddress;
using folly::EventBase;

TunManager::TunManager(SwSwitch *sw, EventBase *evb) : sw_(sw), evb_(evb) {
  DCHECK(sw) << "NULL pointer to SwSwitch.";
  DCHECK(evb) << "NULL pointer to EventBase";

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
  auto ifID = InterfaceID(pkt->getSrcVlan());
  std::lock_guard<std::mutex> lock(mutex_);
  auto iter = intfs_.find(ifID);
  if (iter == intfs_.end()) {
    // the Interface ID has been deleted, make a log, and skip the pkt
    VLOG(4) << "Dropping a packet for unknown unknwon " << ifID;
    return false;
  }
  return iter->second->sendPacketToHost(std::move(pkt));
}

void TunManager::addIntf(const std::string& ifName, int ifIndex) {
  InterfaceID ifID = TunIntf::getIDFromTunIntfName(ifName);
  auto ret = intfs_.emplace(ifID, nullptr);
  if (!ret.second) {
    throw FbossError("Duplicate interface ", ifName);
  }

  SCOPE_FAIL {
    intfs_.erase(ret.first);
  };
  ret.first->second.reset(
      new TunIntf(sw_, evb_, ifID, ifIndex, getInterfaceMtu(ifID)));
}

void TunManager::addIntf(InterfaceID ifID, const Interface::Addresses& addrs) {
  auto ret = intfs_.emplace(ifID, nullptr);
  if (!ret.second) {
    throw FbossError("Duplicate interface for interface ", ifID);
  }

  SCOPE_FAIL {
    intfs_.erase(ret.first);
  };
  auto intf = std::make_unique<TunIntf>(
      sw_, evb_, ifID, addrs, getInterfaceMtu(ifID));

  SCOPE_FAIL {
    intf->setDelete();
  };
  const auto ifName = intf->getName();
  auto ifIndex = intf->getIfIndex();

  // bring up the interface so that we can add the default route next step
  bringUpIntf(ifName, ifIndex);

  // create a new route table for this InterfaceID
  addRouteTable(ifID, ifIndex);

  // add all addresses
  for (const auto& addr : addrs) {
    addTunAddress(ifID, ifName, ifIndex, addr.first, addr.second);
  }

  // Store it in local map on success
  ret.first->second = std::move(intf);
}

void TunManager::removeIntf(InterfaceID ifID) {
  auto iter = intfs_.find(ifID);
  if (iter == intfs_.end()) {
    throw FbossError("Cannot find interface ", ifID, " for deleting.");
  }

  // Remove all addresses attached on this interface
  auto& intf = iter->second;
  for (auto const& addr : intf->getAddresses()) {
    removeTunAddress(
        ifID,
        intf->getName(),
        intf->getIfIndex(),
        addr.first  /* ip */,
        addr.second /* mask */);
  }

  // Remove the route table and associated rule
  removeRouteTable(ifID, intf->getIfIndex());
  intf->setDelete();
  intfs_.erase(iter);
}

void TunManager::addProbedAddr(
    int ifIndex, const IPAddress& addr, uint8_t mask) {
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

void TunManager::bringUpIntf(const std::string& ifName, int ifIndex) {
  // TODO: We need to change the interface status based on real HW
  // interface status (up/down). Make them up all the time for now.
  auto link = rtnl_link_alloc();
  if (!link) {
    throw FbossError("Failed to allocate link to bring up");
  }
  SCOPE_EXIT { rtnl_link_put(link); };

  rtnl_link_set_ifindex(link, ifIndex);
  rtnl_link_set_family(link, AF_UNSPEC);
  rtnl_link_set_flags(link, IFF_UP);

  auto error = rtnl_link_change(sock_, link, link, 0);
  nlCheckError(
      error, "Failed to bring up interface ", ifName, " @ index ", ifIndex);

  LOG(INFO) << "Brought up interface " << ifName << " @ index " << ifIndex;
}

int TunManager::getTableId(InterfaceID ifID) const {
  // Kernel only supports up to 256 tables. The last few are used by kernel
  // as main, default, and local. IDs 0, 254 and 255 are not available. So we
  // use range 1-253 for our usecase.

  // Special case to handle old front0 interfaces.
  // XXX: Delete this case after 6 months (atleast one full rollout). 08-22-2016
  if (ifID == InterfaceID(0)) {
    return 100;   // This was the old-ID used for creating front0
  }

  // Hacky. Need better solution but works for now. Our InterfaceID are
  // Type-1: 2000, 2001, 2002, 2003 ...
  // Type-2: 4000, 4001, 4002, 4003, 4004, ...
  int tableId = static_cast<int>(ifID);
  if (ifID >= InterfaceID(4000)) {   // 4000, 4001, 4002, 4003 ...
    tableId = ifID - 4000 + 201;  // 201, 202, 203, ... [201-253]
  } else {  // 2000, 2001, ...
    tableId = ifID - 2000 + 1;    // 1, 2, 3, .... [1-200]
  }

  // Sanity checks. Generated ID must be in range [1-253]
  CHECK_GE(tableId, 1);
  CHECK_LE(tableId, 253);

  return tableId;
}

int TunManager::getInterfaceMtu(InterfaceID ifID) const {
  auto interface = sw_->getState()->getInterfaces()->getInterfaceIf(ifID);
  return interface ? interface->getMtu() : kDefaultMtu;
}

void TunManager::addRemoveRouteTable(InterfaceID ifID, int ifIndex, bool add) {
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

    rtnl_route_set_table(route, getTableId(ifID));
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

    rtnl_route_nh_set_ifindex(nexthop, ifIndex);
    rtnl_route_add_nexthop(route, nexthop);

    if (add) {
      error = rtnl_route_add(sock_, route, NLM_F_REPLACE);
    } else {
      error = rtnl_route_delete(sock_, route, 0);
    }
    /**
     * Disable: Because of some weird reason this CHECK fails while deleting
     * v4 default route. However route actually gets wiped off from Linux
     * routing table.
    nlCheckError(error, "Failed to ", add ? "add" : "remove",
                  " default route ", addr, " @ index ", ifIndex,
                  " in table ", getTableId(ifID), " for interface ", ifID,
                  ". ErrorCode: ", error);
      */
    if (error < 0) {
      LOG(WARNING) << "Failed to " << (add ? "add" : "remove")
                   << " default route " << addr << " @index " << ifIndex
                   << ". ErrorCode: " << error;
    }
    LOG(INFO) << (add ? "Added" : "Removed") << " default route " << addr
              << " @ index " << ifIndex << " in table " << getTableId(ifID)
              << " for interface " << ifID;
  }
}

void TunManager::addRemoveSourceRouteRule(
    InterfaceID ifID, const folly::IPAddress& addr, bool add) {
  auto rule = rtnl_rule_alloc();
  SCOPE_EXIT { rtnl_rule_put(rule); };

  rtnl_rule_set_family(rule, addr.family());
  rtnl_rule_set_table(rule, getTableId(ifID));
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
                " to lookup table ", getTableId(ifID),
                " for interface ", ifID);
  LOG(INFO) << (add ? "Added" : "Removed") << " rule for address " << addr
            << " to lookup table " << getTableId(ifID)
            << " for interface " << ifID;
}

void TunManager::addRemoveTunAddress(
    const std::string& ifName,
    uint32_t ifIndex,
    const folly::IPAddress& addr,
    uint8_t mask,
    bool add) {
  auto tunaddr = rtnl_addr_alloc();
  if (!tunaddr) {
    throw FbossError("Failed to allocate address");
  }
  SCOPE_EXIT { rtnl_addr_put(tunaddr); };

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
                " to interface ", ifName, " @ index ", ifIndex);
  LOG(INFO) << (add ? "Added" : "Removed") << " address " << addr.str() << "/"
            << static_cast<int>(mask) << " on interface " << ifName
            << " @ index " << ifIndex;
}

void TunManager::addTunAddress(
    InterfaceID ifID,
    const std::string& ifName,
    uint32_t ifIndex,
    folly::IPAddress addr,
    uint8_t mask) {
  addRemoveSourceRouteRule(ifID, addr, true);
  SCOPE_FAIL {
    try {
      addRemoveSourceRouteRule(ifID, addr, false);
    } catch (const std::exception& ex) {
      LOG(ERROR) << "Failed to removed partially added source rule on "
                 << "interface " << ifName;
    }
  };
  addRemoveTunAddress(ifName, ifIndex, addr, mask, true);
}

void TunManager::removeTunAddress(
    InterfaceID ifID,
    const std::string& ifName,
    uint32_t ifIndex,
    folly::IPAddress addr,
    uint8_t mask) {
  addRemoveSourceRouteRule(ifID, addr, false);
  SCOPE_FAIL {
    try {
      addRemoveSourceRouteRule(ifID, addr, true);
    } catch (const std::exception& ex) {
      LOG(ERROR) << "Failed to add partially added source rule on "
                 << "interface " << ifName;
    }
  };
  addRemoveTunAddress(ifName, ifIndex, addr, mask, false);
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
    throw FbossError("Device @ index ",
                     rtnl_link_get_ifindex(link), " does not have a name");
  }

  // Only add interface if it is a Tun interface
  if (!TunIntf::isTunIntfName(name)) {
    VLOG(3) << "Ignore interface " << name
            << " because it is not a tun interface";
    return;
  }

  static_cast<TunManager*>(data)->addIntf(
      std::string(name),
      rtnl_link_get_ifindex(link));
}

void TunManager::addressProcessor(struct nl_object *obj, void *data) {
  struct rtnl_addr *addr = reinterpret_cast<struct rtnl_addr *>(obj);

  // Validate family
  auto family = rtnl_addr_get_family(addr);
  if (family != AF_INET && family != AF_INET6) {
    VLOG(3) << "Skip address from device @ index "
            << rtnl_addr_get_ifindex(addr)
            << " because of its address family " << family;
    return;
  }

  // Validate address
  auto localaddr = rtnl_addr_get_local(addr);
  if (!localaddr) {
   VLOG(3) << "Skip address from device @ index "
           << rtnl_addr_get_ifindex(addr)
           << " because of it does not have a local address ";
   return;
  }

  // Convert rtnl_addr to string representation
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

void TunManager::doProbe(std::lock_guard<std::mutex>& /* lock */) {
  CHECK(!probeDone_);  // Callers must check for probeDone before calling
  stop();              // stop all interfaces
  intfs_.clear();      // clear all interface info

  // get links
  struct nl_cache *cache;
  auto error = rtnl_link_alloc_cache(sock_, AF_UNSPEC, &cache);
  nlCheckError(error, "Cannot get links from Kernel");
  SCOPE_EXIT { nl_cache_free(cache); };
  nl_cache_foreach(cache, &TunManager::linkProcessor, this);

  // get addresses
  struct nl_cache *addressCache;
  error = rtnl_addr_alloc_cache(sock_, &addressCache);
  nlCheckError(error, "Cannot get addresses from Kernel");
  SCOPE_EXIT { nl_cache_free(addressCache); };
  nl_cache_foreach(addressCache, &TunManager::addressProcessor, this);

  // Bring up all interfaces. Interfaces could be already up.
  for (const auto& intf : intfs_) {
    bringUpIntf(intf.second->getName(), intf.second->getIfIndex());
  }

  start();
  probeDone_ = true;
}

void TunManager::sync(std::shared_ptr<SwitchState> state) {
  using Addresses = Interface::Addresses;
  using ConstAddressesIter = Addresses::const_iterator;
  using IntfToAddrsMap = boost::container::flat_map<InterfaceID, Addresses>;
  using ConstIntfToAddrsMapIter = IntfToAddrsMap::const_iterator;

  // prepare new addresses
  IntfToAddrsMap newIntfToAddrs;
  auto intfMap = state->getInterfaces();
  for (const auto& intf : intfMap->getAllNodes()) {
    const auto& addrs = intf.second->getAddresses();
    newIntfToAddrs[intf.first].insert(addrs.begin(), addrs.end());
  }

  // Hold mutex while changing interfaces
  std::lock_guard<std::mutex> lock(mutex_);
  if (!probeDone_) {
    doProbe(lock);
  }

  // prepare old addresses
  IntfToAddrsMap oldIntfToAddrs;
  for (const auto& intf : intfs_) {
    const auto& addrs = intf.second->getAddresses();
    oldIntfToAddrs[intf.first].insert(addrs.begin(), addrs.end());

    // Change MTU if it has altered
    auto interface = intfMap->getInterfaceIf(intf.first);
    if (interface && interface->getMtu() != intf.second->getMtu()) {
      intf.second->setMtu(interface->getMtu());
    }
  }

  // Callback function for updating addresses for a particular interface
  auto applyInterfaceAddrChanges =
    [this](InterfaceID ifID, const std::string& ifName, int ifIndex,
           const Addresses& oldAddrs, const Addresses& newAddrs) {
    applyChanges(
        oldAddrs, newAddrs,
        [&](ConstAddressesIter& oldIter, ConstAddressesIter& newIter) {
          if (oldIter->second == newIter->second) {
            // addresses and masks are both same
            return;
          }
          removeTunAddress(
              ifID, ifName, ifIndex, oldIter->first, oldIter->second);
          addTunAddress(ifID, ifName, ifIndex, newIter->first, newIter->second);
        },
        [&](ConstAddressesIter& newIter) {
          addTunAddress(ifID, ifName, ifIndex, newIter->first, newIter->second);
        },
        [&](ConstAddressesIter& oldIter) {
          removeTunAddress(
              ifID, ifName, ifIndex, oldIter->first, oldIter->second);
        });
  };

  // Apply changes for all interfaces
  applyChanges(
      oldIntfToAddrs, newIntfToAddrs,
      [&](ConstIntfToAddrsMapIter& oldIter, ConstIntfToAddrsMapIter& newIter) {
        const auto& oldAddrs = oldIter->second;
        const auto& newAddrs = newIter->second;
        auto iter = intfs_.find(oldIter->first);
        CHECK(iter != intfs_.end());
        const auto& intf = iter->second;
        auto ifID = intf->getInterfaceID();
        int ifIndex = intf->getIfIndex();
        const auto& ifName = intf->getName();
        applyInterfaceAddrChanges(ifID, ifName, ifIndex, oldAddrs, newAddrs);
        iter->second->setAddresses(newAddrs);
      },
      [&](ConstIntfToAddrsMapIter& newIter) {
        addIntf(newIter->first, newIter->second);
      },
      [&](ConstIntfToAddrsMapIter& oldIter) {
        removeIntf(oldIter->first);
      });

  start();
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

}}  // namespace facebook::fboss
