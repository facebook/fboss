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
#include "common/stats/ServiceData.h"
#include "fboss/agent/AddressUtil.h"
#include "fboss/agent/ArpHandler.h"
#include "fboss/agent/HighresCounterSubscriptionHandler.h"
#include "fboss/agent/IPv6Handler.h"
#include "fboss/agent/LldpManager.h"
#include "fboss/agent/SfpModule.h"
#include "fboss/agent/SwitchStats.h"
#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/Utils.h"
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
#include "fboss/agent/if/gen-cpp2/PortStatusListenerClient.h"

#include <folly/io/IOBuf.h>
#include <folly/MoveWrapper.h>
#include <thrift/lib/cpp2/async/DuplexChannel.h>

using apache::thrift::ClientReceiveState;
using facebook::fb303::cpp2::fb_status;
using folly::IOBuf;
using folly::make_unique;
using folly::StringPiece;
using folly::IPAddress;
using folly::IPAddressV4;
using folly::IPAddressV6;
using folly::MacAddress;
using std::chrono::duration_cast;
using std::chrono::seconds;
using std::chrono::steady_clock;
using std::map;
using std::shared_ptr;
using std::string;
using std::vector;
using std::unique_ptr;
using namespace apache::thrift::transport;
using apache::thrift::server::TConnectionContext;

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

ThriftHandler::ThriftHandler(SwSwitch* sw) : FacebookBase2("FBOSS"), sw_(sw) {
  sw->registerPortStatusListener([=](PortID id, PortStatus st) {
    for (auto& listener : listeners_.accessAllThreads()) {
      auto listenerPtr = &listener;
      listener.eventBase->runInEventBaseThread(
          [=] { invokePortStatusListeners(listenerPtr, id, st); });
    }
  });
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

void ThriftHandler::getProductInfo(ProductInfo& productInfo) {
  sw_->getProductInfo(productInfo);
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

void ThriftHandler::fillPortStatistics(PortStatThrift& stats) {
  auto portId = stats.portId;
  auto statMap = fbData->getStatMap();

  auto getSumStat = [&] (StringPiece prefix, StringPiece name) {
    auto statName = folly::to<std::string>("port", portId, ".", prefix, name);
    return statMap->getStatPtr(statName)->getSum(0);
  };

  auto fillPortCounters = [&] (PortCounters& ctr, StringPiece prefix) {
    ctr.bytes = getSumStat(prefix, "bytes");
    ctr.ucastPkts = getSumStat(prefix, "unicast_pkts");
    ctr.multicastPkts = getSumStat(prefix, "multicast_pkts");
    ctr.broadcastPkts = getSumStat(prefix, "broadcast_pkts");
    ctr.errors.errors = getSumStat(prefix, "errors");
    ctr.errors.discards = getSumStat(prefix, "discards");
  };

  const auto& status = sw_->getPortStatus(PortID(portId));
  stats.adminState = PortAdminState(status.enabled);
  stats.operState = PortOperState(status.up);

  fillPortCounters(stats.output, "out_");
  fillPortCounters(stats.input, "in_");
}

void ThriftHandler::getPortStats(PortStatThrift& portStats, int32_t portId) {
  ensureConfigured();
  const auto port = sw_->getState()->getPorts()->getPortIf(PortID(portId));

  if (!port) {
    throw FbossError("no such port ", portId);
  }
  portStats.portId = portId;
  portStats.speedMbps = int32_t(port->getSpeed());
  fillPortStatistics(portStats);
}

void
ThriftHandler::getAllPortStats(map<int32_t, PortStatThrift>& portStatsMap) {
  ensureConfigured();

  for (const auto& port : (*sw_->getState()->getPorts())) {
    auto id = port->getID();

    auto& stats = portStatsMap[id];
    stats.portId = id;
    stats.speedMbps = int32_t(port->getSpeed());

    fillPortStatistics(stats);
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
      tempRoute.dest.ip = toBinaryAddress(ipv4->prefix().network);
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

static LinkNeighborThrift thriftLinkNeighbor(const LinkNeighbor& n,
                                             steady_clock::time_point now) {
  LinkNeighborThrift tn;
  tn.localPort = n.getLocalPort();
  tn.localVlan = n.getLocalVlan();
  tn.srcMac = n.getMac().toString();
  tn.chassisIdType = static_cast<int32_t>(n.getChassisIdType());
  tn.chassisId = n.getChassisId();
  tn.printableChassisId = n.humanReadableChassisId();
  tn.portIdType = static_cast<int32_t>(n.getPortIdType());
  tn.portId = n.getPortId();
  tn.printablePortId = n.humanReadablePortId();
  tn.originalTTL = duration_cast<seconds>(n.getTTL()).count();
  tn.ttlSecondsLeft =
    duration_cast<seconds>(n.getExpirationTime() - now).count();
  if (!n.getSystemName().empty()) {
    tn.systemName = n.getSystemName();
    tn.__isset.systemName = true;
  }
  if (!n.getSystemDescription().empty()) {
    tn.systemDescription = n.getSystemDescription();
    tn.__isset.systemDescription = true;
  }
  if (!n.getPortDescription().empty()) {
    tn.portDescription = n.getPortDescription();
    tn.__isset.portDescription = true;
  }
  return tn;
}

void ThriftHandler::getLldpNeighbors(vector<LinkNeighborThrift>& results) {
  ensureConfigured();
  auto* db = sw_->getLldpMgr()->getDB();
  // Do an immediate check for expired neighbors
  db->pruneExpiredNeighbors();
  auto neighbors = db->getNeighbors();
  results.reserve(neighbors.size());
  auto now = steady_clock::now();
  for (const auto& entry : db->getNeighbors()) {
    results.push_back(thriftLinkNeighbor(entry, now));
  }
}

void ThriftHandler::invokePortStatusListeners(ThreadLocalListener* listener,
                                              PortID port,
                                              PortStatus status) {
  // Collect the iterators to avoid erasing and potentially reordering
  // the iterators in the list.
  std::vector<const TConnectionContext*> brokenClients;
  for (auto& client : listener->clients) {
    auto clientDone = [&](ClientReceiveState&& state) {
      try {
        PortStatusListenerClientAsyncClient::recv_portStatusChanged(state);
      } catch (const std::exception& ex) {
        LOG(ERROR) << "Exception in port listener: " << ex.what();
        brokenClients.push_back(client.first);
      }
    };
    client.second->portStatusChanged(clientDone, port, status);
  }
  for (const auto& ctx : brokenClients) {
    listener->clients.erase(ctx);
  }
}

void ThriftHandler::async_eb_registerForPortStatusChanged(
    ThriftCallback<void> cb) {
  auto ctx = cb->getConnectionContext()->getConnectionContext();
  auto client = ctx->getDuplexClient<PortStatusListenerClientAsyncClient>();
  auto info = listeners_.get();
  CHECK(cb->getEventBase()->isInEventBaseThread());
  if (!info) {
    info = new ThreadLocalListener(cb->getEventBase());
    listeners_.reset(info);
  }
  DCHECK_EQ(info->eventBase, cb->getEventBase());
  if (!info->eventBase) {
    info->eventBase = cb->getEventBase();
  }
  info->clients.emplace(ctx, client);
  cb->done();
}

void ThriftHandler::startPktCapture(unique_ptr<CaptureInfo> info) {
  ensureConfigured();
  auto* mgr = sw_->getCaptureMgr();
  auto capture = make_unique<PktCapture>(info->name, info->maxPackets);
  mgr->startCapture(std::move(capture));
}

void ThriftHandler::stopPktCapture(unique_ptr<std::string> name) {
  ensureConfigured();
  auto* mgr = sw_->getCaptureMgr();
  mgr->forgetCapture(*name);
}

void ThriftHandler::stopAllPktCaptures() {
  ensureConfigured();
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
                            std::unique_ptr<folly::fbstring> hex) {
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

// If this is a premature client disconnect from a duplex connection, we need to
// clean up state.  Failure to do so may allow the server's duplex clients to
// use the destroyed context => segfaults.
void ThriftHandler::connectionDestroyed(TConnectionContext* ctx) {
  // Port status notifications
  if (listeners_) {
    listeners_->clients.erase(ctx);
  }

  // If there is an ongoing high-resolution counter subscription, kill it. Don't
  // grab a write lock if there are no active calls
  if (!highresKillSwitches_.asConst()->empty()) {
    SYNCHRONIZED(highresKillSwitches_) {
      auto killSwitchIter = highresKillSwitches_.find(ctx);

      if (killSwitchIter != highresKillSwitches_.end()) {
        killSwitchIter->second->set();
        highresKillSwitches_.erase(killSwitchIter);
      }
    }
  }
}

void ThriftHandler::async_tm_subscribeToCounters(
    ThriftCallback<bool> callback, unique_ptr<CounterSubscribeRequest> req) {

  // Grab the requested samplers from the underlying switch implementation
  auto samplers = make_unique<HighresSamplerList>();
  auto numCounters = sw_->getHighresSamplers(samplers.get(), req->counters);

  if (numCounters > 0) {
    // Grab the connection context and create a kill switch
    auto ctx = callback->getConnectionContext()->getConnectionContext();
    auto killSwitch = std::make_shared<Signal>();
    SYNCHRONIZED(highresKillSwitches_) {
      highresKillSwitches_[ctx] = killSwitch;
    }

    // Create the sender
    auto client = ctx->getDuplexClient<FbossHighresClientAsyncClient>();
    auto eventBase = callback->getEventBase();
    auto sender = std::make_shared<SampleSender>(std::move(client), killSwitch,
                                                 eventBase, numCounters);

    // Create the sample producer and send it on its way
    auto producer = make_unique<SampleProducer>(
        std::move(samplers), std::move(sender), std::move(killSwitch),
        eventBase, *req.get(), numCounters);
    auto wrappedProducer = folly::makeMoveWrapper(std::move(producer));
    auto veryNice = req->veryNice;
    std::thread producerThread([wrappedProducer, veryNice]() mutable {
      if (veryNice) {
        incNiceValue(20);
      }
      (*wrappedProducer)->produce();
    });
    producerThread.detach();

    callback->result(true);
  } else {
    LOG(ERROR) << "None of the requested counters were valid.";
    callback->result(false);
  }
}

int32_t ThriftHandler::getIdleTimeout() {
  if (thriftIdleTimeout_ < 0) {
    throw FbossError("Idle timeout has not been set");
  }
  return thriftIdleTimeout_;
}
}} // facebook::fboss
