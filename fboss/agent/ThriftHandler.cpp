/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/ThriftHandler.h"

#include <folly/IPAddressV4.h>
#include <folly/IPAddressV6.h>
#include "fboss/agent/AddressUtil.h"
#include "fboss/agent/ArpHandler.h"
#include "fboss/agent/IPv6Handler.h"
#include "fboss/agent/SfpModule.h"
#include "fboss/agent/SwitchStats.h"
#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/capture/PktCapture.h"
#include "fboss/agent/capture/PktCaptureManager.h"
#include "fboss/agent/hw/mock/MockRxPacket.h"
#include "fboss/agent/state/ArpEntry.h"
#include "fboss/agent/state/ArpTable.h"
#include "fboss/agent/state/Interface.h"
#include "fboss/agent/state/InterfaceMap.h"
#include "fboss/agent/state/NdpTable.h"
#include "fboss/agent/state/NdpEntry.h"
#include "fboss/agent/state/Port.h"
#include "fboss/agent/state/Route.h"
#include "fboss/agent/state/RouteTable.h"
#include "fboss/agent/state/RouteTableRib.h"
#include "fboss/agent/state/RouteUpdater.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/state/Vlan.h"
#include "fboss/agent/state/VlanMap.h"

#include <folly/io/IOBuf.h>

using facebook::fb303::cpp2::fb_status;
using std::unique_ptr;
using folly::IOBuf;
using folly::make_unique;
using folly::StringPiece;
using folly::IPAddress;
using folly::IPAddressV4;
using folly::IPAddressV6;
using folly::MacAddress;
using std::string;
using std::map;
using std::shared_ptr;
using std::vector;
using namespace apache::thrift::transport;

using facebook::network::toBinaryAddress;
using facebook::network::toAddress;
using facebook::network::toIPAddress;

namespace facebook { namespace fboss {

class RouteUpdateStats {
 public:
  RouteUpdateStats(SwSwitch *sw, const std::string& func, uint32_t routes)
      : sw_(sw),
        func_(func),
        routes_(routes),
        start_(std::chrono::steady_clock::now()) {
  }
  ~RouteUpdateStats() {
    auto end = std::chrono::steady_clock::now();
    auto duration =
      std::chrono::duration_cast<std::chrono::microseconds>(end - start_);
    sw_->stats()->routeUpdate(duration, routes_);
    VLOG(0) << func_ << " " << routes_ << " routes took "
            << duration.count() << "us";
  }
 private:
  SwSwitch* sw_;
  const std::string func_;
  uint32_t routes_;
  std::chrono::time_point<std::chrono::steady_clock> start_;
};

ThriftHandler::ThriftHandler(SwSwitch* sw)
  : FacebookBase2("FBOSS"),
    sw_(sw) {
}

fb_status ThriftHandler::getStatus() {
  if (sw_->isFullyInitialized()) {
    return fb_status::ALIVE;
  } else {
    return fb_status::STARTING;
  }
}

void ThriftHandler::async_tm_getStatus(ThriftCallback<fb_status> callback) {
  callback->result(getStatus());
}

void ThriftHandler::flushCountersNow() {
  // Currently SwSwitch only contains thread local stats.
  //
  // Depending on how we design the HW-specific stats interface,
  // we may also need to make a separate call to force immediate collection of
  // hardware stats.
  sw_->publishStats();
}

void ThriftHandler::addUnicastRoute(
    int16_t client, std::unique_ptr<UnicastRoute> route) {
  ensureConfigured("addUnicastRoute");
  ensureFibSynced("addUnicastRoute");
  RouteUpdateStats stats(sw_, "Add", 1);
  RouterID routerId = RouterID(0); // TODO, default vrf for now
  folly::IPAddress network = toIPAddress(route->dest.ip);
  uint8_t mask = static_cast<uint8_t>(route->dest.prefixLength);
  RouteNextHops nexthops;
  nexthops.reserve(route->nextHopAddrs.size());
  for (const auto& nh : route->nextHopAddrs) {
    nexthops.emplace(toIPAddress(nh));
  }
  if (network.isV4()) {
    sw_->stats()->addRouteV4();
  } else {
    sw_->stats()->addRouteV6();
  }

  // Perform the update
  auto updateFn = [=](const shared_ptr<SwitchState>& state) {
    RouteUpdater updater(state->getRouteTables());
    updater.addRoute(routerId, network, mask, std::move(nexthops));
    auto newRt = updater.updateDone();
    if (!newRt) {
      return shared_ptr<SwitchState>();
    }
    auto newState = state->clone();
    newState->resetRouteTables(std::move(newRt));
    return newState;
  };
  sw_->updateStateBlocking("add unicast route", updateFn);
}

void ThriftHandler::deleteUnicastRoute(
    int16_t client, std::unique_ptr<IpPrefix> prefix) {
  ensureConfigured("deleteUnicastRoute");
  ensureFibSynced("deleteUnicastRoute");
  RouteUpdateStats stats(sw_, "Delete", 1);
  RouterID routerId = RouterID(0); // TODO, default vrf for now
  folly::IPAddress network =  toIPAddress(prefix->ip);
  uint8_t mask = static_cast<uint8_t>(prefix->prefixLength);
  if (network.isV4()) {
    sw_->stats()->delRouteV4();
  } else {
    sw_->stats()->delRouteV6();
  }

  // Perform the update
  auto updateFn = [=](const shared_ptr<SwitchState>& state) {
    RouteUpdater updater(state->getRouteTables());
    updater.delRoute(routerId, network, mask);
    auto newRt = updater.updateDone();
    if (!newRt) {
      return shared_ptr<SwitchState>();
    }
    auto newState = state->clone();
    newState->resetRouteTables(std::move(newRt));
    return newState;
  };
  sw_->updateStateBlocking("delete unicast route", updateFn);
}

void ThriftHandler::addUnicastRoutes(
    int16_t client, std::unique_ptr<std::vector<UnicastRoute>> routes) {
  ensureConfigured("addUnicastRoutes");
  ensureFibSynced("addUnicastRoutes");
  RouteUpdateStats stats(sw_, "Add", routes->size());
  auto updateFn = [&](const shared_ptr<SwitchState>& state) {
    RouteUpdater updater(state->getRouteTables());
    RouterID routerId = RouterID(0); // TODO, default vrf for now
    for (const auto& route : *routes) {
      auto network = toIPAddress(route.dest.ip);
      auto mask = static_cast<uint8_t>(route.dest.prefixLength);
      RouteNextHops nexthops;
      nexthops.reserve(route.nextHopAddrs.size());
      for (const auto& nh : route.nextHopAddrs) {
        nexthops.emplace(toIPAddress(nh));
      }
      updater.addRoute(routerId, network, mask, std::move(nexthops));
      if (network.isV4()) {
        sw_->stats()->addRouteV4();
      } else {
        sw_->stats()->addRouteV6();
      }
    }
    auto newRt = updater.updateDone();
    if (!newRt) {
      return shared_ptr<SwitchState>();
    }
    auto newState = state->clone();
    newState->resetRouteTables(std::move(newRt));
    return newState;
  };
  sw_->updateStateBlocking("add unicast route", updateFn);
}

void ThriftHandler::deleteUnicastRoutes(
    int16_t client, std::unique_ptr<std::vector<IpPrefix>> prefixes) {
  ensureConfigured("deleteUnicastRoutes");
  ensureFibSynced("deleteUnicastRoutes");
  RouteUpdateStats stats(sw_, "Delete", prefixes->size());
  // Perform the update
  auto updateFn = [&](const shared_ptr<SwitchState>& state) {
    RouteUpdater updater(state->getRouteTables());
    RouterID routerId = RouterID(0); // TODO, default vrf for now
    for (const auto& prefix : *prefixes) {
      auto network = toIPAddress(prefix.ip);
      auto mask = static_cast<uint8_t>(prefix.prefixLength);
      if (network.isV4()) {
        sw_->stats()->delRouteV4();
      } else {
        sw_->stats()->delRouteV6();
      }
      updater.delRoute(routerId, network, mask);
    }
    auto newRt = updater.updateDone();
    if (!newRt) {
      return shared_ptr<SwitchState>();
    }
    auto newState = state->clone();
    newState->resetRouteTables(std::move(newRt));
    return newState;
  };
  sw_->updateStateBlocking("delete unicast route", updateFn);
}

void ThriftHandler::syncFib(
    int16_t client, std::unique_ptr<std::vector<UnicastRoute>> routes) {
  ensureConfigured("syncFib");
  RouteUpdateStats stats(sw_, "Sync", routes->size());

  // Note that we capture routes by reference here, since it is a unique_ptr.
  // This is safe since we use updateStateBlocking(), so routes will still
  // be valid in our scope when updateFn() is called.
  // We could use folly::MoveWrapper if we did need to capture routes by value.
  auto updateFn = [&](const shared_ptr<SwitchState>& state) {
    // create an update object starting from empty
    RouteUpdater updater(state->getRouteTables(), true);
    // add all interface routes
    updater.addInterfaceAndLinkLocalRoutes(state->getInterfaces());
    RouterID routerId = RouterID(0); // TODO, default vrf for now
    for (auto const& route : *routes) {
      folly::IPAddress network = toIPAddress(route.dest.ip);
      uint8_t mask = static_cast<uint8_t>(route.dest.prefixLength);
      RouteNextHops nexthops;
      nexthops.reserve(route.nextHopAddrs.size());
      for (const auto& nh : route.nextHopAddrs) {
        nexthops.emplace(toIPAddress(nh));
      }
      updater.addRoute(routerId, network, mask, std::move(nexthops));
      if (network.isV4()) {
        sw_->stats()->addRouteV4();
      } else {
        sw_->stats()->addRouteV6();
      }
    }
    auto newRt = updater.updateDone();
    if (!newRt) {
      return shared_ptr<SwitchState>();
    }
    auto newState = state->clone();
    newState->resetRouteTables(std::move(newRt));
    return newState;
  };
  sw_->updateStateBlocking("sync fib", updateFn);

  sw_->clearWarmBootCache();
  sw_->fibSynced();
}

void ThriftHandler::getAllInterfaces(
    std::map<int32_t, InterfaceDetail>& interfaces) {
  ensureConfigured();
  for (const auto& intf : (*sw_->getState()->getInterfaces())) {
    auto& interfaceDetail = interfaces[intf->getID()];

    interfaceDetail.interfaceName = intf->getName();
    interfaceDetail.interfaceId = intf->getID();
    interfaceDetail.vlanId = intf->getVlanID();
    interfaceDetail.routerId = intf->getRouterID();
    interfaceDetail.mac = intf->getMac().toString();
    interfaceDetail.address.reserve(intf->getAddresses().size());
    for (const auto& addrAndMask: intf->getAddresses()) {
      IpPrefix temp;
      temp.ip = toBinaryAddress(addrAndMask.first);
      temp.prefixLength = addrAndMask.second;
      interfaceDetail.address.push_back(temp);
    }
  }
}

void ThriftHandler::getInterfaceList(std::vector<std::string>& interfaceList) {
  ensureConfigured();
  for (const auto& intf : (*sw_->getState()->getInterfaces())) {
    interfaceList.push_back(intf->getName());
  }
}

void ThriftHandler::getInterfaceDetail(InterfaceDetail& interfaceDetail,
                                                        int32_t interfaceId) {
  ensureConfigured();
  const auto& intf = sw_->getState()->getInterfaces()->getInterfaceIf(
                                                      InterfaceID(interfaceId));
  if (!intf) {
    throw FbossError("no such interface ", interfaceId);
  }

  interfaceDetail.interfaceName = intf->getName();
  interfaceDetail.interfaceId = intf->getID();
  interfaceDetail.vlanId = intf->getVlanID();
  interfaceDetail.routerId = intf->getRouterID();
  interfaceDetail.mac = intf->getMac().toString();
  interfaceDetail.address.clear();
  interfaceDetail.address.reserve(intf->getAddresses().size());
  for (const auto& addrAndMask: intf->getAddresses()) {
    IpPrefix temp;
    temp.ip = toBinaryAddress(addrAndMask.first);
    temp.prefixLength = int(addrAndMask.second);
    interfaceDetail.address.push_back(temp);
  }
}

void ThriftHandler::getNdpTable(std::vector<NdpEntryThrift>& ndpTable) {
  ensureConfigured();
  shared_ptr<SwitchState> state = sw_->getState();
  for (const auto& vlan : *state->getVlans()) {
    const auto& ndpTab = vlan->getNdpTable();
    for (const auto& entry : ndpTab->getAllNodes()) {
      NdpEntryThrift temp;
      temp.ip = toBinaryAddress(entry.second->getIP());
      temp.mac = entry.second->getMac().toString();
      temp.port = entry.second->getPort();
      temp.vlanName = vlan->getName();
      temp.vlanID = vlan->getID();
      ndpTable.push_back(temp);
    }
  }
}

void ThriftHandler::getArpTable(std::vector<ArpEntryThrift>& arpTable) {
  ensureConfigured();
  shared_ptr<SwitchState> state = sw_->getState();
  for (const auto& vlan : *state->getVlans()) {
    const auto& arpTab = vlan->getArpTable();
    for (const auto& entry : arpTab->getAllNodes()) {
      ArpEntryThrift temp;
      temp.ip = toBinaryAddress(entry.second->getIP());
      temp.mac = folly::to<string>(entry.second->getMac());
      temp.port = entry.second->getPort();
      temp.vlanName = vlan->getName();
      temp.vlanID = vlan->getID();
      arpTable.push_back(temp);
    }
  }
}

void ThriftHandler::getPortStatus(map<int32_t, PortStatus>& statusMap,
                                  unique_ptr<vector<int32_t>> ports) {
  ensureConfigured();
  if (ports->empty()) {
    statusMap = sw_->getPortStatus();
  } else {
    for (auto port : *ports) {
      statusMap[port] = sw_->getPortStatus(PortID(port));
    }
  }
}

void ThriftHandler::getRouteTable(std::vector<UnicastRoute>& route) {
  ensureConfigured();
  for (const auto& routeTable : (*sw_->getState()->getRouteTables())) {
    for (const auto& ipv4Rib : routeTable->getRibV4()->getAllNodes()) {
      UnicastRoute tempRoute;
      auto ipv4 = ipv4Rib.second.get();
      auto fwdInfo = ipv4->getForwardInfo();
      tempRoute.dest.ip = toBinaryAddress(
                                                      ipv4->prefix().network);
      tempRoute.dest.prefixLength = ipv4->prefix().mask;
      for (const auto& hop : fwdInfo.getNexthops()) {
        tempRoute.nextHopAddrs.push_back(
                    toBinaryAddress(hop.nexthop));
      }
      route.push_back(tempRoute);
    }
    for (const auto& ipv6Rib : routeTable->getRibV6()->getAllNodes()) {
      UnicastRoute tempRoute;
      auto ipv6 = ipv6Rib.second.get();
      auto fwdInfo = ipv6->getForwardInfo();
      tempRoute.dest.ip = toBinaryAddress(
                                                     ipv6->prefix().network);
      tempRoute.dest.prefixLength = ipv6->prefix().mask;
      for (const auto& hop : fwdInfo.getNexthops()) {
        tempRoute.nextHopAddrs.push_back(
                    toBinaryAddress(hop.nexthop));
      }
      route.push_back(tempRoute);
    }
  }
}

void ThriftHandler::getIpRoute(UnicastRoute& route,
                                std::unique_ptr<Address> addr, int32_t vrfId) {
  ensureConfigured();
  folly::IPAddress ipAddr = toIPAddress(*addr);
  auto routeTable = sw_->getState()->getRouteTables()->getRouteTableIf(
                                                              RouterID(vrfId));
  if (!routeTable) {
    throw FbossError("No Such VRF ", vrfId);
  }

  if (ipAddr.isV4()) {
    auto ripV4Rib = routeTable->getRibV4();
    auto match = ripV4Rib->longestMatch(ipAddr.asV4());
    if (!match) {
      route.dest.ip = toBinaryAddress(IPAddressV4("0.0.0.0"));
      route.dest.prefixLength = 0;
      return;
    }
    const auto fwdInfo = match->getForwardInfo();
    route.dest.ip = toBinaryAddress(match->prefix().network);
    route.dest.prefixLength = match->prefix().mask;
    for (auto iter = fwdInfo.getNexthops().begin();
              iter != fwdInfo.getNexthops().end(); ++iter) {
      route.nextHopAddrs.push_back(toBinaryAddress(iter->nexthop));
    }
  } else {
    auto ripV6Rib = routeTable->getRibV6();
    auto match = ripV6Rib->longestMatch(ipAddr.asV6());
    if (!match) {
      route.dest.ip = toBinaryAddress(IPAddressV6("::0"));
      route.dest.prefixLength = 0;
      return;
    }
    const auto fwdInfo = match->getForwardInfo();
    route.dest.ip = toBinaryAddress(match->prefix().network);
    route.dest.prefixLength = match->prefix().mask;
    for (auto iter = fwdInfo.getNexthops().begin();
              iter != fwdInfo.getNexthops().end(); ++iter) {
      route.nextHopAddrs.push_back(toBinaryAddress(iter->nexthop));
    }
  }
}

void ThriftHandler::startPktCapture(unique_ptr<CaptureInfo> info) {
  auto* mgr = sw_->getCaptureMgr();
  auto capture = make_unique<PktCapture>(info->name, info->maxPackets);
  mgr->startCapture(std::move(capture));
}

void ThriftHandler::stopPktCapture(unique_ptr<std::string> name) {
  auto* mgr = sw_->getCaptureMgr();
  mgr->forgetCapture(*name);
}

void ThriftHandler::stopAllPktCaptures() {
  auto* mgr = sw_->getCaptureMgr();
  mgr->forgetAllCaptures();
}

void ThriftHandler::sendPkt(int32_t port, int32_t vlan,
                         unique_ptr<folly::fbstring> data) {
  ensureConfigured("sendPkt");
  auto buf = IOBuf::copyBuffer(reinterpret_cast<const uint8_t*>(data->data()),
                               data->size());
  auto pkt = make_unique<MockRxPacket>(std::move(buf));
  pkt->setSrcPort(PortID(port));
  pkt->setSrcVlan(VlanID(vlan));
  sw_->packetReceived(std::move(pkt));
}

void ThriftHandler::sendPktHex(int32_t port, int32_t vlan,
                            std::unique_ptr<std::string> hex) {
  ensureConfigured("sendPktHex");
  auto pkt = MockRxPacket::fromHex(StringPiece(*hex));
  pkt->setSrcPort(PortID(port));
  pkt->setSrcVlan(VlanID(vlan));
  sw_->packetReceived(std::move(pkt));
}

Vlan* ThriftHandler::getVlan(int32_t vlanId) {
  ensureConfigured();
  return sw_->getState()->getVlans()->getVlan(VlanID(vlanId)).get();
}

Vlan* ThriftHandler::getVlan(const std::string& vlanName) {
  ensureConfigured();
  return sw_->getState()->getVlans()->getVlanSlow(vlanName).get();
}

int32_t ThriftHandler::flushNeighborEntry(unique_ptr<BinaryAddress> ip,
                                          int32_t vlan) {
  ensureConfigured("flushNeighborEntry");

  auto parsedIP = toIPAddress(*ip);
  VlanID vlanID(vlan);
  if (parsedIP.isV4()) {
    return sw_->getArpHandler()->flushArpEntryBlocking(parsedIP.asV4(),
                                                       vlanID);
  }
  return sw_->getIPv6Handler()->flushNdpEntryBlocking(parsedIP.asV6(), vlanID);
}

void ThriftHandler::getVlanAddresses(Addresses& addrs, int32_t vlan) {
  getVlanAddresses(getVlan(vlan), addrs, toAddress);
}

void ThriftHandler::getVlanAddressesByName(Addresses& addrs,
    unique_ptr<string> vlan) {
  getVlanAddresses(getVlan(*vlan), addrs, toAddress);
}

void ThriftHandler::getVlanBinaryAddresses(BinaryAddresses& addrs,
    int32_t vlan) {
  getVlanAddresses(getVlan(vlan), addrs, toBinaryAddress);
}

void ThriftHandler::getVlanBinaryAddressesByName(BinaryAddresses& addrs,
    const std::unique_ptr<std::string> vlan) {
  getVlanAddresses(getVlan(*vlan), addrs, toBinaryAddress);
}

void ThriftHandler::getSfpDomInfo(map<int32_t, SfpDom>& domInfos,
                                  unique_ptr<vector<int32_t>> ports) {
  ensureConfigured();
  if (ports->empty()) {
    domInfos = sw_->getSfpDoms();
  } else {
    for (auto port : *ports) {
      domInfos[port] = sw_->getSfpDom(PortID(port));
    }
  }
}

template<typename ADDR_TYPE, typename ADDR_CONVERTER>
void ThriftHandler::getVlanAddresses(const Vlan* vlan,
    std::vector<ADDR_TYPE>& addrs, ADDR_CONVERTER& converter) {
  ensureConfigured();
  CHECK(vlan);
  for (auto intf : (*sw_->getState()->getInterfaces())) {
    if (intf->getVlanID() == vlan->getID()) {
      for (const auto& addrAndMask : intf->getAddresses()) {
          addrs.push_back(converter(addrAndMask.first));
      }
    }
  }
}

BootType ThriftHandler::getBootType() {
  return sw_->getBootType();
}

void ThriftHandler::ensureConfigured(StringPiece function) {
  if (sw_->isConfigured()) {
    return;
  }

  if (!function.empty()) {
    VLOG(1) << "failing thrift prior to switch configuration: " << function;
  }
  throw FbossError("switch is still initializing and is not fully "
                   "configured yet");
}

void ThriftHandler::ensureFibSynced(StringPiece function) {
  if (sw_->isFibSynced()) {
    return;
  }

  if (!function.empty()) {
    VLOG(1) << "failing thrift prior to FIB Sync: " << function;
  }
  throw FbossError("switch is still initializing, FIB not synced yet");
}
}} // facebook::fboss
