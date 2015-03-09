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
#include <sys/ioctl.h>
#include <libnetlink.h>
#include <ll_map.h>
#include <linux/rtnetlink.h>
#include <linux/if.h>
#include <linux/if_tun.h>
#include <linux/fib_rules.h>
}

#include "fboss/agent/RxPacket.h"
#include "fboss/agent/SysError.h"
#include "fboss/agent/TunIntf.h"
#include "fboss/agent/state/InterfaceMap.h"
#include "fboss/agent/state/Interface.h"
#include "thrift/lib/cpp/async/TEventBase.h"

#include <boost/container/flat_set.hpp>

namespace facebook { namespace fboss {

using folly::IPAddress;
using apache::thrift::async::TEventBase;

TunManager::TunManager(SwSwitch *sw, TEventBase *evb) : sw_(sw), evb_(evb) {
  auto ret = rtnl_open(&rth_, 0);
  sysCheckError(ret, "Failed to open rtnl");
}

TunManager::~TunManager() {
  stop();
  rtnl_close(&rth_);
}

void TunManager::addIntf(RouterID rid, const std::string& name, int ifIdx) {
  auto ret = intfs_.emplace(rid, nullptr);
  if (!ret.second) {
    throw FbossError("Duplicate interface ", name, " in router ", rid);
  }
  SCOPE_FAIL {
    intfs_.erase(ret.first);
  };
  ret.first->second.reset(new TunIntf(sw_, evb_, name, rid, ifIdx));
}

void TunManager::addIntf(RouterID rid, const Interface::Addresses& addrs) {
  auto ret = intfs_.emplace(rid, nullptr);
  if (!ret.second) {
    throw FbossError("Duplicate interface for router ", rid);
  }
  SCOPE_FAIL {
    intfs_.erase(ret.first);
  };
  auto intf = folly::make_unique<TunIntf>(sw_, evb_, rid, addrs);
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
  struct {
    struct nlmsghdr n;
    struct ifinfomsg ifi;
    char buf[256];
  } req;
  memset(&req, 0, sizeof(req));
  req.n.nlmsg_len = NLMSG_LENGTH(sizeof(struct ifinfomsg));
  req.n.nlmsg_type = RTM_NEWLINK;
  req.n.nlmsg_flags = NLM_F_REQUEST;
  req.ifi.ifi_family = AF_UNSPEC;
  req.ifi.ifi_change |= IFF_UP;
  req.ifi.ifi_flags |= IFF_UP;
  req.ifi.ifi_index = ifIndex;
  auto ret = rtnl_talk(&rth_, &req.n, 0, 0, nullptr);
  sysCheckError(ret, "Failed to bring up interface ", name,
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
  struct {
    struct nlmsghdr n;
    struct rtmsg r;
    char buf[256];
  } req;
  const folly::IPAddress addrs[] = {
    IPAddress{"0.0.0.0"},       // v4 default
    IPAddress{"::0"},           // v6 default
  };

  for (const auto& addr : addrs) {
    memset(&req, 0, sizeof(req));
    req.n.nlmsg_len = NLMSG_LENGTH(sizeof(struct rtmsg));
    req.n.nlmsg_flags = NLM_F_REQUEST;
    if (add) {
      req.n.nlmsg_type = RTM_NEWROUTE;
      req.n.nlmsg_flags |= NLM_F_CREATE|NLM_F_REPLACE;
    } else {
      req.n.nlmsg_type = RTM_DELROUTE;
    }
    req.r.rtm_family = addr.family();
    req.r.rtm_table = getTableId(rid);
    req.r.rtm_scope = RT_SCOPE_NOWHERE;
    req.r.rtm_protocol = RTPROT_FBOSS;
    req.r.rtm_scope = RT_SCOPE_UNIVERSE;
    req.r.rtm_type = RTN_UNICAST;
    req.r.rtm_dst_len = 0;       // default route, /0
    addattr_l(&req.n, sizeof(req), RTA_DST, addr.bytes(), addr.byteCount());
    addattr32(&req.n, sizeof(req), RTA_OIF, ifIdx);
    auto ret = rtnl_talk(&rth_, &req.n, 0, 0, nullptr);
    sysCheckError(ret, "Failed to ", add ? "add" : "remove",
                  " default route ", addr, " @ index ", ifIdx,
                  " in table ", getTableId(rid), " for router ", rid);
    LOG(INFO) << (add ? "Added" : "Removed") << " default route " << addr
              << " @ index " << ifIdx << " in table " << getTableId(rid)
              << " for router " << rid;
  }
}

void TunManager::addRemoveSourceRouteRule(
    RouterID rid, folly::IPAddress addr, bool add) {
  struct {
    struct nlmsghdr n;
    struct rtmsg r;
    char buf[256];
  } req;

  memset(&req, 0, sizeof(req));
  req.n.nlmsg_len = NLMSG_LENGTH(sizeof(struct rtmsg));
  req.n.nlmsg_flags = NLM_F_REQUEST;
  if (add) {
    req.n.nlmsg_type = RTM_NEWRULE;
    req.n.nlmsg_flags |= NLM_F_CREATE|NLM_F_REPLACE;
  } else {
    req.n.nlmsg_type = RTM_DELRULE;
  }
  req.r.rtm_family = addr.family();
  req.r.rtm_protocol = RTPROT_FBOSS;
  req.r.rtm_scope = RT_SCOPE_UNIVERSE;
  req.r.rtm_type = RTN_UNICAST;
  req.r.rtm_flags = 0;
  // from addr
  addattr_l(&req.n, sizeof(req), FRA_SRC, addr.bytes(), addr.byteCount());
  req.r.rtm_src_len = addr.bitCount(); // match the exact address
  // table rid
  req.r.rtm_table = getTableId(rid);
  auto ret = rtnl_talk(&rth_, &req.n, 0, 0, nullptr);
  sysCheckError(ret, "Failed to ", add ? "add" : "remove",
                " rule for address ", addr,
                " to lookup table ", getTableId(rid),
                " for router ", rid);
  LOG(INFO) << (add ? "Added" : "Removed") << " rule for address " << addr
            << " to lookup table " << getTableId(rid) << " for router " << rid;
}

void TunManager::addRemoveTunAddress(
    const std::string& name, uint32_t ifIndex,
    folly::IPAddress addr, uint8_t mask, bool add) {
  struct {
    struct nlmsghdr n;
    struct ifaddrmsg ifa;
    char buf[256];
  } req;
  memset(&req, 0, sizeof(req));
  req.n.nlmsg_len = NLMSG_LENGTH(sizeof(struct ifaddrmsg));
  if (add) {
    req.n.nlmsg_flags = NLM_F_REQUEST|NLM_F_CREATE|NLM_F_EXCL;
    req.n.nlmsg_type = RTM_NEWADDR;
  } else {
    req.n.nlmsg_flags = NLM_F_REQUEST;
    req.n.nlmsg_type = RTM_DELADDR;
  }
  if (addr.isV4()) {
    req.ifa.ifa_family = AF_INET;
    addattr_l(&req.n, sizeof(req), IFA_LOCAL, addr.asV4().bytes(),
              folly::IPAddressV4::byteCount());
  } else {
    req.ifa.ifa_family = AF_INET6;
    addattr_l(&req.n, sizeof(req), IFA_LOCAL, addr.asV6().bytes(),
              folly::IPAddressV6::byteCount());
  }
  req.ifa.ifa_prefixlen = mask;
  req.ifa.ifa_index = ifIndex;
  auto ret = rtnl_talk(&rth_, &req.n, 0, 0, nullptr);
  sysCheckError(ret, "Failed to ", add ? "add" : "remove",
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

int TunManager::getLinkRespParser(const struct sockaddr_nl *who,
                                  struct nlmsghdr *n,
                                  void *arg) {
  // only cares about RTM_NEWLINK
  if (n->nlmsg_type != RTM_NEWLINK) {
    return 0;
  }

  struct ifinfomsg *ifi = static_cast<struct ifinfomsg *>(NLMSG_DATA(n));
  struct rtattr *tb[IFLA_MAX + 1];
  int len = n->nlmsg_len;
  len -= NLMSG_LENGTH(sizeof(*ifi));
  if (len < 0) {
    throw FbossError("Wrong length for RTM_GETLINK response ", len, " vs ",
                     NLMSG_LENGTH(sizeof(*ifi)));
  }
  parse_rtattr(tb, IFLA_MAX, IFLA_RTA(ifi), len);
  if (tb[IFLA_IFNAME] == nullptr) {
    throw FbossError("Device @ index ", static_cast<int>(ifi->ifi_index),
                     " does not have a nmae");
  }
  const auto name = rta_getattr_str(tb[IFLA_IFNAME]);
  // match the interface name against intfPrefix
  if (!TunIntf::isTunIntf(name)) {
    VLOG(3) << "Ignore interface " << name
            << " because it is not a tun interface";
    return 0;
  }
  // base on the name, get the router ID
  auto rid = TunIntf::getRidFromName(name);

  TunManager *mgr = static_cast<TunManager *>(arg);
  mgr->addIntf(rid, std::string(name), ifi->ifi_index);
  return 0;
}

int TunManager::getAddrRespParser(const struct sockaddr_nl *who,
                                  struct nlmsghdr *n,
                                  void *arg) {
  // only cares about RTM_NEWADDR
  if (n->nlmsg_type != RTM_NEWADDR) {
    return 0;
  }
  struct ifaddrmsg *ifa = static_cast<struct ifaddrmsg *>(NLMSG_DATA(n));
  struct rtattr *tb[IFA_MAX + 1];
  int len = n->nlmsg_len;
  len -= NLMSG_LENGTH(sizeof(*ifa));
  if (len < 0) {
    throw FbossError("Wrong length for RTM_GETADDR response ", len, " vs ",
                     NLMSG_LENGTH(sizeof(*ifa)));
  }
  // only care about v4 and v6 address
  if (ifa->ifa_family != AF_INET && ifa->ifa_family != AF_INET6) {
    VLOG(3) << "Skip address from device @ index "
            << static_cast<int>(ifa->ifa_index)
            << " because of its address family "
            << static_cast<int>(ifa->ifa_family);
    return 0;
  }

  parse_rtattr(tb, IFA_MAX, IFA_RTA(ifa), len);
  if (tb[IFA_ADDRESS] == nullptr) {
    VLOG(3) << "Device @ index " << static_cast<int>(ifa->ifa_index)
            << " does not have address at family "
            << static_cast<int>(ifa->ifa_family);
    return 0;
  }
  IPAddress addr;
  const void *data = RTA_DATA(tb[IFA_ADDRESS]);
  if (ifa->ifa_family == AF_INET) {
    addr = IPAddress(*static_cast<const in_addr *>(data));
  } else {
    addr = IPAddress(*static_cast<const in6_addr *>(data));
  }

  TunManager *mgr = static_cast<TunManager *>(arg);
  mgr->addProbedAddr(ifa->ifa_index, addr, ifa->ifa_prefixlen);
  return 0;
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

void TunManager::probe() {
  std::lock_guard<std::mutex> lock(mutex_);
  stop();                       // stop all interfaces
  intfs_.clear();               // clear all interface info
  auto ret = rtnl_wilddump_request(&rth_, AF_UNSPEC, RTM_GETLINK);
  sysCheckError(ret, "Cannot send RTM_GETLINK request");
  ret = rtnl_dump_filter(&rth_, getLinkRespParser, this);
  sysCheckError(ret, "Cannot process RTM_GETLINK response");
  ret = rtnl_wilddump_request(&rth_, AF_UNSPEC, RTM_GETADDR);
  sysCheckError(ret, "Cannot send RTM_GETADDR request");
  ret = rtnl_dump_filter(&rth_, getAddrRespParser, this);
  sysCheckError(ret, "Cannot process RTM_GETADDR response");
  // Bring up all interfaces. Interfaces could be already up.
  for (const auto& intf : intfs_) {
    bringupIntf(intf.second->getName(), intf.second->getIfIndex());
  }
  start();
}

void TunManager::sync(std::shared_ptr<InterfaceMap> map) {
  // prepare the existing and new addresses
  typedef Interface::Addresses Addresses;
  typedef boost::container::flat_map<RouterID, Addresses> AddrMap;
  AddrMap newAddrs;
  for (const auto& intf : map->getAllNodes()) {
    const auto& addrs = intf.second->getAddresses();
    newAddrs[intf.second->getRouterID()].insert(addrs.begin(), addrs.end());
  }
  AddrMap oldAddrs;
  for (const auto& intf : intfs_) {
    const auto& addrs = intf.second->getAddresses();
    oldAddrs[intf.first].insert(addrs.begin(), addrs.end());
  }
  // now, lock all interfaces and apply changes. Interfaces will be stopped
  // if there is some change to the interface
  std::lock_guard<std::mutex> lock(mutex_);

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

void TunManager::startSync(const std::shared_ptr<InterfaceMap>& map) {
  evb_->runInEventBaseThread([this, map]() {
      this->sync(map);
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
