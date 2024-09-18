/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/TunManager.h"

extern "C" {
#include <linux/if.h>
#include <netlink/route/addr.h>
#include <netlink/route/link.h>
#include <netlink/route/route.h>
#include <netlink/route/rule.h>
#include <sys/ioctl.h>
}

#include <folly/MapUtil.h>
#include <folly/lang/CString.h>
#include <folly/logging/xlog.h>
#include "fboss/agent/NlError.h"
#include "fboss/agent/RxPacket.h"
#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/SysError.h"
#include "fboss/agent/TunIntf.h"
#include "fboss/agent/state/Interface.h"
#include "fboss/agent/state/InterfaceMap.h"
#include "fboss/agent/state/Port.h"
#include "fboss/agent/state/SwitchState.h"

#include <boost/container/flat_set.hpp>

namespace {
const int kDefaultMtu = 1500;
}

namespace facebook::fboss {

using folly::IPAddress;

TunManager::TunManager(SwSwitch* sw, FbossEventBase* evb) : sw_(sw), evb_(evb) {
  DCHECK(sw) << "NULL pointer to SwSwitch.";
  DCHECK(evb) << "NULL pointer to EventBase";

  sock_ = nl_socket_alloc();
  if (!sock_) {
    throw FbossError("failed to allocate libnl socket");
  }
  auto error = nl_connect(sock_, NETLINK_ROUTE);
  nlCheckError(error, "failed to connect netlink socket to NETLINK_ROUTE");
}

void TunManager::stopProcessing() {
  if (observingState_) {
    sw_->unregisterStateObserver(this);
  }
  std::lock_guard<std::mutex> lock(mutex_);
  stop();
  nl_close(sock_);
  nl_socket_free(sock_);
}

TunManager::~TunManager() {}

void TunManager::startObservingUpdates() {
  sw_->registerStateObserver(this, "TunManager");
  observingState_ = true;
}

void TunManager::stateUpdated(const StateDelta& delta) {
  // TODO(aeckert): t15067879 We currently compare the entire
  // interface map instead of using the iterator in this delta because
  // some of the interfaces may get get probed from hardware before
  // they are in the SwitchState. It would be nicer if we did a little
  // more work at startup to sync the state, perhaps updating the
  // SwitchState with the probed interfaces. This would allow us to
  // reuse the iterator in the delta for more readable code and also
  // not have to worry about waiting to listen to updates until the
  // SwSwitch is in the configured state. t4155406 should also help
  // with that.

  auto state = delta.newState();
  evb_->runInFbossEventBaseThread([this, state]() { this->sync(state); });
}

bool TunManager::sendPacketToHost(
    InterfaceID dstIfID,
    std::unique_ptr<RxPacket> pkt) {
  std::lock_guard<std::mutex> lock(mutex_);
  auto iter = intfs_.find(dstIfID);
  if (iter == intfs_.end()) {
    // the Interface ID has been deleted, make a log, and skip the pkt
    XLOG(DBG4) << "Dropping a packet for unknown interface " << dstIfID;
    return false;
  }
  return iter->second->sendPacketToHost(std::move(pkt));
}

void TunManager::addExistingIntf(const std::string& ifName, int ifIndex) {
  InterfaceID ifID = utility::getIDFromTunIntfName(ifName);
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

void TunManager::addNewIntf(
    InterfaceID ifID,
    bool isUp,
    const Interface::Addresses& addrs) {
  auto ret = intfs_.emplace(ifID, nullptr);
  if (!ret.second) {
    throw FbossError("Duplicate interface for interface ", ifID);
  }

  SCOPE_FAIL {
    intfs_.erase(ret.first);
  };
  auto intf = std::make_unique<TunIntf>(
      sw_, evb_, ifID, isUp, addrs, getInterfaceMtu(ifID));

  SCOPE_FAIL {
    intf->setDelete();
  };
  const auto ifName = intf->getName();
  auto ifIndex = intf->getIfIndex();

  // bring up the interface so that we can add the default route in next step
  setIntfStatus(ifName, ifIndex, isUp);

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

  // Remove all source routing rules attached to this interface. Addresses
  // and routes are automatically removed once interface is deteled.
  auto& intf = iter->second;
  for (auto const& addr : intf->getAddresses()) {
    addRemoveSourceRouteRule(ifID, addr.first, false);
  }

  // Remove the route table and associated rule
  removeRouteTable(ifID, intf->getIfIndex());
  intf->setDelete();
  intfs_.erase(iter);
}

void TunManager::addProbedAddr(
    int ifIndex,
    const IPAddress& addr,
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
  XLOG(DBG3) << "Cannot find interface @ index " << ifIndex
             << " for probed address " << addr.str() << "/"
             << static_cast<int>(mask);
}

void TunManager::setIntfStatus(
    const std::string& ifName,
    int ifIndex,
    bool status) {
  /**
   * NOTE: Why use `ioctl` instead of `netlink` ?
   *
   * netlink's `rtnl_link_change` API was in-effective. Internally netlink
   * tries to pass a message with RTM_NEWLINK which some old kernel couldn't
   * process. After messing my head around with netlink for few hours I decided
   * to use `ioctl` which was much easy and straight-forward operation.
   */

  // Prepare socket
  auto sockFd = socket(PF_INET, SOCK_DGRAM, 0);
  sysCheckError(sockFd, "Failed to open socket");
  SCOPE_EXIT {
    close(sockFd);
  };

  // Prepare request
  struct ifreq ifr;
  memset(&ifr, 0, sizeof(ifr));
  folly::strlcpy(ifr.ifr_name, ifName.c_str(), IFNAMSIZ);

  // Get existing flags
  int error = ioctl(sockFd, SIOCGIFFLAGS, static_cast<void*>(&ifr));
  sysCheckError(error, "Failed to get existing interface flags of ", ifName);

  // Mutate flags
  if (status) {
    ifr.ifr_flags |= IFF_UP;
  } else {
    ifr.ifr_flags &= ~IFF_UP;
  }

  // Set flags
  error = ioctl(sockFd, SIOCSIFFLAGS, static_cast<void*>(&ifr));
  sysCheckError(error, "Failed to set interface flags on ", ifName);
  XLOG(DBG2) << "Brought " << (status ? "up" : "down") << " interface "
             << ifName << " @ index " << ifIndex;
}

int TunManager::getTableId(InterfaceID ifID) const {
  int tableId;
  switch (sw_->getSwitchInfoTable().l3SwitchType()) {
    case cfg::SwitchType::NPU:
      tableId = getTableIdForNpu(ifID);
      break;
    case cfg::SwitchType::VOQ:
      tableId = getTableIdForVoq(ifID);
      break;
    case cfg::SwitchType::PHY:
    case cfg::SwitchType::FABRIC:
      CHECK(false);
      throw FbossError("No system port range in SwitchSettings for VOQ switch");
  }

  // Sanity checks. Generated ID must be in range [1-253]
  CHECK_GE(tableId, 1);
  CHECK_LE(tableId, 253);

  return tableId;
}

int TunManager::getTableIdForNpu(InterfaceID ifID) const {
  // Kernel only supports up to 256 tables. The last few are used by kernel
  // as main, default, and local. IDs 0, 254 and 255 are not available. So we
  // use range 1-253 for our usecase.

  // Hacky. Need better solution but works for now. Our InterfaceID are
  // Type-1: 2000, 2001, 2002, 2003 ...
  // Type-2: 4000, 4001, 4002, 4003, 4004, ...
  // Type-3: 10, 11, 12, 13 (Virtual Interfaces)
  int tableId = static_cast<int>(ifID);
  if (ifID >= InterfaceID(4000)) { // 4000, 4001, 4002, 4003 ...
    tableId = ifID - 4000 + 201; // 201, 202, 203, ...
  } else if (ifID >= InterfaceID(3000)) { // 3000, 3001, ...
    // <RGSW, FUJI> in OLR has 96 upinks, hence we cannot use 4000
    tableId = ifID - 3000 + 101; // 101, 102, 103, ....
  } else if (ifID >= InterfaceID(2000)) { // 2000, 2001, ...
    tableId = ifID - 2000 + 1; // 1, 2, 3, ....
  } else { // 10, 11, 12, 13 ...
    tableId = 250 - (ifID - 10); // 250, 249, 248, ...
  }

  return tableId;
}

int TunManager::getTableIdForVoq(InterfaceID ifID) const {
  // Kernel only supports up to 256 tables. The last few are used by kernel
  // as main, default, and local. IDs 0, 254 and 255 are not available. So we
  // use range 1-253 for our usecase.
  //
  // VOQ systems use port based RIFs.
  // Port based RIF IDs are assigned starting minimum system port range of the
  // first switch. Thus, map ifID to 1-253 with ifID - firstSwitchSysPortMin
  // In practice, [firstSwitchSysPortMin, lastSwitchSysPortUsed] range is <<
  // 253. Moreover, getTableID asserts that the computed ID is <= 253

  auto firstSwitchSysPortRange = getFirstSwitchSystemPortIdRange(
      utility::getFirstNodeIf(sw_->getState()->getSwitchSettings())
          ->getSwitchIdToSwitchInfo());
  return ifID - *firstSwitchSysPortRange.minimum();
}

int TunManager::getInterfaceMtu(InterfaceID ifID) const {
  auto interface = sw_->getState()->getInterfaces()->getNodeIf(ifID);
  return interface ? interface->getMtu() : kDefaultMtu;
}

void TunManager::addRemoveRouteTable(InterfaceID ifID, int ifIndex, bool add) {
  // We just store default routes (one for IPv4 and one for IPv6) in each route
  // table.
  const folly::IPAddress addrs[] = {
      IPAddress{"0.0.0.0"}, // v4 default
      IPAddress{"::0"}, // v6 default
  };

  for (const auto& addr : addrs) {
    auto route = rtnl_route_alloc();
    SCOPE_EXIT {
      rtnl_route_put(route);
    };

    auto error = rtnl_route_set_family(route, addr.family());
    nlCheckError(error, "Failed to set family to", addr.family());

    rtnl_route_set_table(route, getTableId(ifID));
    rtnl_route_set_scope(route, RT_SCOPE_UNIVERSE);
    rtnl_route_set_protocol(route, RTPROT_FBOSS);

    error = rtnl_route_set_type(route, RTN_UNICAST);
    nlCheckError(error, "Failed to set type to ", RTN_UNICAST);

    auto destaddr = nl_addr_build(
        addr.family(),
        const_cast<unsigned char*>(addr.bytes()),
        addr.byteCount());
    if (!destaddr) {
      throw FbossError("Failed to build destination address for ", addr);
    }
    SCOPE_EXIT {
      nl_addr_put(destaddr);
    };

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
      XLOG(WARNING) << "Failed to " << (add ? "add" : "remove")
                    << " default route " << addr << " @index " << ifIndex
                    << ". ErrorCode: " << error;
    }
    XLOG(DBG2) << (add ? "Added" : "Removed") << " default route " << addr
               << " @ index " << ifIndex << " in table " << getTableId(ifID)
               << " for interface " << ifID;
  }
}

void TunManager::addRemoveSourceRouteRule(
    InterfaceID ifID,
    const folly::IPAddress& addr,
    bool add) {
  // We should not add source routing rule for link-local addresses because
  // they can be re-used across interfaces.
  if (addr.isLinkLocal()) {
    XLOG(DBG2) << "Ignoring source routing rule for link-local address "
               << addr;
    return;
  }

  auto rule = rtnl_rule_alloc();
  SCOPE_EXIT {
    rtnl_rule_put(rule);
  };

  rtnl_rule_set_family(rule, addr.family());
  rtnl_rule_set_table(rule, getTableId(ifID));
  rtnl_rule_set_action(rule, FR_ACT_TO_TBL);

  auto sourceaddr = nl_addr_build(
      addr.family(),
      const_cast<unsigned char*>(addr.bytes()),
      addr.byteCount());
  if (!sourceaddr) {
    throw FbossError("Failed to build destination address for ", addr);
  }
  SCOPE_EXIT {
    nl_addr_put(sourceaddr);
  };

  nl_addr_set_prefixlen(sourceaddr, addr.bitCount());
  auto error = rtnl_rule_set_src(rule, sourceaddr);
  nlCheckError(error, "Failed to set destination route to ", addr);

  if (add) {
    error = rtnl_rule_add(sock_, rule, NLM_F_REPLACE);
  } else {
    error = rtnl_rule_delete(sock_, rule, 0);
  }
  // There are scenarios where the rule has already been deleted and we try to
  // delete it again. In that case, we can ignore the error.
  if (!add && (error == -NLE_OBJ_NOTFOUND)) {
    XLOG(WARNING) << "Rule not existing for address " << addr
                  << " to lookup table " << getTableId(ifID)
                  << " for interface " << ifID;
  } else {
    nlCheckError(
        error,
        "Failed to ",
        add ? "add" : "remove",
        " rule for address ",
        addr,
        " to lookup table ",
        getTableId(ifID),
        " for interface ",
        ifID);
    XLOG(DBG2) << (add ? "Added" : "Removed") << " rule for address " << addr
               << " to lookup table " << getTableId(ifID) << " for interface "
               << ifID;
  }
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
  SCOPE_EXIT {
    rtnl_addr_put(tunaddr);
  };

  rtnl_addr_set_family(tunaddr, addr.family());
  auto localaddr = nl_addr_build(
      addr.family(),
      const_cast<unsigned char*>(addr.bytes()),
      addr.byteCount());
  if (!localaddr) {
    throw FbossError("Failed to build destination address for ", addr);
  }
  SCOPE_EXIT {
    nl_addr_put(localaddr);
  };

  auto error = rtnl_addr_set_local(tunaddr, localaddr);
  nlCheckError(error, "Failed to set local address to ", addr);

  rtnl_addr_set_prefixlen(tunaddr, mask);
  rtnl_addr_set_ifindex(tunaddr, ifIndex);

  if (add) {
    /**
     * When you bring down interface some routes are purged but some still stay
     * there (I tested from command line and v6 routes were gone but v4 were
     * there). To be on safe side, when we bring up interface we always add
     * addresses and routes for that interface with REPLACE flag overriding
     * existing ones if any.
     */
    error = rtnl_addr_add(sock_, tunaddr, NLM_F_REPLACE);
  } else {
    error = rtnl_addr_delete(sock_, tunaddr, 0);
  }
  nlCheckError(
      error,
      "Failed to ",
      add ? "add" : "remove",
      " address ",
      addr,
      "/",
      static_cast<int>(mask),
      " to interface ",
      ifName,
      " @ index ",
      ifIndex);
  XLOG(DBG2) << (add ? "Added" : "Removed") << " address " << addr.str() << "/"
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
    } catch (const std::exception&) {
      XLOG(ERR) << "Failed to removed partially added source rule on "
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
    } catch (const std::exception&) {
      XLOG(ERR) << "Failed to add partially added source rule on "
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

void TunManager::linkProcessor(struct nl_object* obj, void* data) {
  struct rtnl_link* link = reinterpret_cast<struct rtnl_link*>(obj);

  // Get name of an interface
  const auto name = rtnl_link_get_name(link);
  if (!name) {
    throw FbossError(
        "Device @ index ",
        rtnl_link_get_ifindex(link),
        " does not have a name");
  }

  // Only add interface if it is a Tun interface
  if (!utility::isTunIntfName(name)) {
    XLOG(DBG3) << "Ignore interface " << name
               << " because it is not a tun interface";
    return;
  }

  static_cast<TunManager*>(data)->addExistingIntf(
      std::string(name), rtnl_link_get_ifindex(link));
}

void TunManager::addressProcessor(struct nl_object* obj, void* data) {
  struct rtnl_addr* addr = reinterpret_cast<struct rtnl_addr*>(obj);

  // Validate family
  auto family = rtnl_addr_get_family(addr);
  if (family != AF_INET && family != AF_INET6) {
    XLOG(DBG3) << "Skip address from device @ index "
               << rtnl_addr_get_ifindex(addr)
               << " because of its address family " << family;
    return;
  }

  // Validate address
  auto localaddr = rtnl_addr_get_local(addr);
  if (!localaddr) {
    XLOG(DBG3) << "Skip address from device @ index "
               << rtnl_addr_get_ifindex(addr)
               << " because of it does not have a local address ";
    return;
  }

  // Convert rtnl_addr to string representation
  char buf[INET6_ADDRSTRLEN];
  nl_addr2str(localaddr, buf, sizeof(buf));
  if (!*buf) {
    XLOG(DBG3) << "Device @ index " << rtnl_addr_get_ifindex(addr)
               << " does not have an address at family " << family;
  }

  auto ipaddr = IPAddress::createNetwork(buf, -1, false).first;
  static_cast<TunManager*>(data)->addProbedAddr(
      rtnl_addr_get_ifindex(addr), ipaddr, nl_addr_get_prefixlen(localaddr));
}

void TunManager::probe() {
  std::lock_guard<std::mutex> lock(mutex_);
  doProbe(lock);
}

void TunManager::doProbe(std::lock_guard<std::mutex>& /* lock */) {
  const auto startTs = std::chrono::steady_clock::now();
  SCOPE_EXIT {
    const auto endTs = std::chrono::steady_clock::now();
    auto elapsedMs =
        std::chrono::duration_cast<std::chrono::milliseconds>(endTs - startTs);
    XLOG(DBG2) << "Probing of linux state took " << elapsedMs.count() << "ms.";
  };

  CHECK(!probeDone_); // Callers must check for probeDone before calling
  stop(); // stop all interfaces
  intfs_.clear(); // clear all interface info

  // get links
  struct nl_cache* cache;
  auto error = rtnl_link_alloc_cache(sock_, AF_UNSPEC, &cache);
  nlCheckError(error, "Cannot get links from Kernel");
  SCOPE_EXIT {
    nl_cache_free(cache);
  };
  nl_cache_foreach(cache, &TunManager::linkProcessor, this);

  // get addresses
  struct nl_cache* addressCache;
  error = rtnl_addr_alloc_cache(sock_, &addressCache);
  nlCheckError(error, "Cannot get addresses from Kernel");
  SCOPE_EXIT {
    nl_cache_free(addressCache);
  };
  nl_cache_foreach(addressCache, &TunManager::addressProcessor, this);

  start();
  probeDone_ = true;
}

boost::container::flat_map<InterfaceID, bool> TunManager::getInterfaceStatus(
    std::shared_ptr<SwitchState> state) {
  boost::container::flat_map<InterfaceID, bool> statusMap;

  // Declare all virtual or state_sync disabled interfaces as up
  for (const auto& [_, intfMap] : std::as_const(*state->getInterfaces())) {
    for (auto iter : std::as_const(*intfMap)) {
      const auto& intf = iter.second;
      if (intf->isVirtual() || intf->isStateSyncDisabled()) {
        statusMap.emplace(intf->getID(), true);
      }
    }
  }

  // Derive interface status from all ports
  auto portMaps = state->getPorts();
  auto vlanMap = state->getVlans();
  for (const auto& portMap : std::as_const(*portMaps)) {
    for (const auto& port : std::as_const(*portMap.second)) {
      bool isPortUp = port.second->isPortUp();
      for (const auto& vlanIDToInfo : port.second->getVlans()) {
        auto vlan = vlanMap->getNodeIf(vlanIDToInfo.first);
        if (!vlan) {
          XLOG(ERR) << "Vlan " << vlanIDToInfo.first << " not found in state.";
          continue;
        }

        auto intfID = vlan->getInterfaceID();
        statusMap[intfID] |= isPortUp; // NOTE: We are applying `OR` operator
      } // for vlanIDToInfo
    } // for portIDToObj
  }

  return statusMap;
}

void TunManager::forceInitialSync() {
  evb_->runInFbossEventBaseThread([this]() {
    if (numSyncs_ == 0) {
      // no syncs occurred yet. Force initial sync. The initial sync is done
      // with applied state, and subsequent sync's will also be done with the
      // applied states.
      sync(sw_->getState());
    }
  });
}

void TunManager::sync(std::shared_ptr<SwitchState> state) {
  CHECK(evb_->isInEventBaseThread());
  using Addresses = Interface::Addresses;
  using ConstAddressesIter = Addresses::const_iterator;
  using IntfInfo = std::pair<bool /* status */, Addresses>;
  using IntfToAddrsMap = boost::container::flat_map<InterfaceID, IntfInfo>;
  using ConstIntfToAddrsMapIter = IntfToAddrsMap::const_iterator;

  // Get interface status.
  auto intfStatusMap = getInterfaceStatus(state);

  // prepare new addresses
  IntfToAddrsMap newIntfToInfo;
  for (const auto& [_, intfMap] : std::as_const(*state->getInterfaces())) {
    for (auto iter : std::as_const(*intfMap)) {
      const auto& intf = iter.second;
      auto addrs = intf->getAddressesCopy();

      // Ideally all interfaces should be present in intfStatusMap as either
      // interface will be virtual or will have at least one port. Keeping
      // default status of interface to be DOWN in case if interface is not
      // virtual and is not associated with any physical port
      const auto status =
          folly::get_default(intfStatusMap, intf->getID(), false);

      newIntfToInfo[intf->getID()] = {status, addrs};
    }
  }

  // Hold mutex while changing interfaces
  std::lock_guard<std::mutex> lock(mutex_);
  if (!probeDone_) {
    doProbe(lock);
  }

  // prepare old addresses
  IntfToAddrsMap oldIntfToInfo;
  for (const auto& intf : intfs_) {
    const auto& addrs = intf.second->getAddresses();
    bool status = intf.second->getStatus();
    oldIntfToInfo[intf.first] = {status, addrs};

    // Change MTU if it has altered
    auto interface = state->getInterfaces()->getNodeIf(intf.first);
    if (interface && interface->getMtu() != intf.second->getMtu()) {
      intf.second->setMtu(interface->getMtu());
    }
  }

  // Callback function for updating addresses for a particular interface
  auto applyInterfaceAddrChanges = [this](
                                       InterfaceID ifID,
                                       const std::string& ifName,
                                       int ifIndex,
                                       const Addresses& oldAddrs,
                                       const Addresses& newAddrs) {
    applyChanges(
        oldAddrs,
        newAddrs,
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
      oldIntfToInfo,
      newIntfToInfo,
      [&](ConstIntfToAddrsMapIter& oldIter, ConstIntfToAddrsMapIter& newIter) {
        auto oldStatus = oldIter->second.first;
        auto newStatus = newIter->second.first;
        const auto& oldAddrs = oldIter->second.second;
        const auto& newAddrs = newIter->second.second;

        // Interface must exists
        const auto& intf = intfs_.at(oldIter->first);
        auto ifID = intf->getInterfaceID();
        int ifIndex = intf->getIfIndex();
        const auto& ifName = intf->getName();

        // mutate intf status and addresses
        intf->setStatus(newStatus);
        intf->setAddresses(newAddrs);

        // Update interface status
        if (oldStatus ^ newStatus) { // old and new status is different
          setIntfStatus(ifName, ifIndex, newStatus);
        }

        // We need to add route-table and tun-addresses if interface is brought
        // up recently.
        // NOTE: Do not add source routing rules because kernel doesn't handle
        // NLM_F_REPLACE flags and they just keep piling up :/
        if (!oldStatus and newStatus) {
          addRouteTable(ifID, ifIndex);
          for (const auto& addr : newAddrs) {
            addRemoveTunAddress(ifName, ifIndex, addr.first, addr.second, true);
          }
        }

        // Update interface addresses only if interface is up currently
        // We would like to process address change whenever current state of
        // interface is UP. We should not try to add addresses when interface is
        // down as it can throw exception for v6 address.
        if (newStatus) {
          applyInterfaceAddrChanges(ifID, ifName, ifIndex, oldAddrs, newAddrs);
        }
      },
      [&](ConstIntfToAddrsMapIter& newIter) {
        auto& statusAddr = newIter->second;
        addNewIntf(newIter->first, statusAddr.first, statusAddr.second);
      },
      [&](ConstIntfToAddrsMapIter& oldIter) { removeIntf(oldIter->first); });

  start();

  // track number of times sync is called
  ++numSyncs_;
}

// TODO(aeckert): Find a way to reuse the iterator from NodeMapDelta here as
// this basically duplicates that code.
template <
    typename MAPNAME,
    typename CHANGEFN,
    typename ADDFN,
    typename REMOVEFN>
void TunManager::applyChanges(
    const MAPNAME& oldMap,
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

} // namespace facebook::fboss
