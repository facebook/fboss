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
#include "fboss/agent/SwitchStats.h"
#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/TxPacket.h"
#include "fboss/agent/Utils.h"
#include "fboss/agent/NeighborUpdater.h"
#include "fboss/agent/RouteUpdateLogger.h"
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
#include "fboss/agent/state/StateUtils.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/state/Vlan.h"
#include "fboss/agent/state/VlanMap.h"
#include "fboss/agent/if/gen-cpp2/NeighborListenerClient.h"

#include <folly/io/Cursor.h>
#include <folly/io/IOBuf.h>
#include <folly/MoveWrapper.h>
#include <folly/Range.h>
#include <thrift/lib/cpp2/async/DuplexChannel.h>

using apache::thrift::ClientReceiveState;
using facebook::fb303::cpp2::fb_status;
using folly::fbstring;
using folly::IOBuf;
using std::make_unique;
using folly::StringPiece;
using folly::IPAddress;
using folly::IPAddressV4;
using folly::IPAddressV6;
using folly::MacAddress;
using folly::io::RWPrivateCursor;
using std::chrono::duration_cast;
using std::chrono::seconds;
using std::chrono::steady_clock;
using std::map;
using std::shared_ptr;
using std::string;
using std::vector;
using std::unique_ptr;
using apache::thrift::server::TConnectionContext;

using facebook::network::toBinaryAddress;
using facebook::network::toAddress;
using facebook::network::toIPAddress;

namespace facebook { namespace fboss {

namespace util {

/**
 * Utility function to convert `Nexthops` (resolved ones) to list<BinaryAddress>
 */
std::vector<network::thrift::BinaryAddress>
fromFwdNextHops(RouteNextHopSet const& nexthops) {
  std::vector<network::thrift::BinaryAddress> nhs;
  nhs.reserve(nexthops.size());
  for (auto const& nexthop : nexthops) {
    auto addr = network::toBinaryAddress(nexthop.addr());
    addr.__isset.ifName = true;
    addr.ifName = util::createTunIntfName(nexthop.intf());
    nhs.emplace_back(std::move(addr));
  }
  return nhs;
}

}

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
  sw->registerNeighborListener(
    [=](const std::vector<std::string>& added,
        const std::vector<std::string>& deleted) {
      for (auto& listener : listeners_.accessAllThreads()) {
        LOG(INFO) << "Sending notification to bgpD";
        auto listenerPtr = &listener;
        listener.eventBase->runInEventBaseThread(
            [=] {
              LOG(INFO) << "firing off notification";
              invokeNeighborListeners(listenerPtr, added, deleted); });
      }
  });
}

fb_status ThriftHandler::getStatus() {
  if (sw_->isFullyInitialized()) {
    return fb_status::ALIVE;
  } else if (sw_->isExiting()) {
    return fb_status::STOPPING;
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
  auto routes = std::make_unique<std::vector<UnicastRoute>>();
  routes->emplace_back(std::move(*route));
  addUnicastRoutes(client, std::move(routes));
}

void ThriftHandler::deleteUnicastRoute(
    int16_t client, std::unique_ptr<IpPrefix> prefix) {
  auto prefixes = std::make_unique<std::vector<IpPrefix>>();
  prefixes->emplace_back(std::move(*prefix));
  deleteUnicastRoutes(client, std::move(prefixes));
}

void ThriftHandler::addUnicastRoutes(
    int16_t client, std::unique_ptr<std::vector<UnicastRoute>> routes) {
  ensureConfigured("addUnicastRoutes");
  ensureFibSynced("addUnicastRoutes");
  RouteUpdateStats stats(sw_, "Add", routes->size());
  auto updateFn = [&](const shared_ptr<SwitchState>& state) {
    RouteUpdater updater(state->getRouteTables());
    RouterID routerId = RouterID(0); // TODO, default vrf for now
    auto clientIdToAdmin = sw_->clientIdToAdminDistance(client);
    for (const auto& route : *routes) {
      auto network = toIPAddress(route.dest.ip);
      auto mask = static_cast<uint8_t>(route.dest.prefixLength);
      auto adminDistance = route.__isset.adminDistance ? route.adminDistance :
        clientIdToAdmin;
      RouteNextHops nexthops = util::toRouteNextHops(route.nextHopAddrs);
      if (nexthops.size()) {
        updater.addRoute(routerId, network, mask, ClientID(client),
                         RouteNextHopEntry(std::move(nexthops), adminDistance));
      } else {
        updater.addRoute(routerId, network, mask, ClientID(client),
                         RouteNextHopEntry(RouteForwardAction::DROP,
                           adminDistance));
      }
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
      updater.delRoute(routerId, network, mask, ClientID(client));
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
    RouteUpdater updater(state->getRouteTables());
    RouterID routerId = RouterID(0); // TODO, default vrf for now
    auto clientIdToAdmin = sw_->clientIdToAdminDistance(client);
    updater.removeAllRoutesForClient(routerId, ClientID(client));
    for (const auto& route : *routes) {
      folly::IPAddress network = toIPAddress(route.dest.ip);
      uint8_t mask = static_cast<uint8_t>(route.dest.prefixLength);
      auto adminDistance = route.__isset.adminDistance ? route.adminDistance :
        clientIdToAdmin;
      RouteNextHops nexthops = util::toRouteNextHops(route.nextHopAddrs);
      updater.addRoute(routerId, network, mask, ClientID(client),
                       RouteNextHopEntry(std::move(nexthops),
                         adminDistance));
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

  if (!sw_->isFibSynced()) {
    sw_->fibSynced();
  }
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
  sw_->getNeighborUpdater()->getNdpCacheData(ndpTable);
}

void ThriftHandler::getArpTable(std::vector<ArpEntryThrift>& arpTable) {
  ensureConfigured();
  sw_->getNeighborUpdater()->getArpCacheData(arpTable);
}

void ThriftHandler::getL2Table(std::vector<L2EntryThrift>& l2Table) {
  ensureConfigured();
  sw_->getHw()->fetchL2Table(&l2Table);
  VLOG(6) << "L2 Table size:" << l2Table.size();
}

void ThriftHandler::getAggregatePortTable(
    std::vector<AggregatePortEntryThrift>& aggregatePortTable) {
  ensureConfigured();
  sw_->fetchAggregatePortTable(aggregatePortTable);
  VLOG(6) << "Aggregate Port Table size:" << aggregatePortTable.size();
}

void ThriftHandler::fillPortStats(PortInfoThrift& portInfo) {
  auto portId = portInfo.portId;
  auto statMap = fbData->getStatMap();

  auto getSumStat = [&] (StringPiece prefix, StringPiece name) {
    // Currently, the internal name of the port is "port<n>", even though
    // `the external name is "eth<a>/<b>/<c>"
    auto portName = folly::to<std::string>("port", portId);
    auto statName = folly::to<std::string>(portName, ".", prefix, name);
    auto statPtr = statMap->getLockedStatPtr(statName);
    auto numLevels = statPtr->numLevels();
    // Cumulative (ALLTIME) counters are at (numLevels - 1)
    return statPtr->sum(numLevels - 1);
  };

  auto fillPortCounters = [&] (PortCounters& ctr, StringPiece prefix) {
    ctr.bytes = getSumStat(prefix, "bytes");
    ctr.ucastPkts = getSumStat(prefix, "unicast_pkts");
    ctr.multicastPkts = getSumStat(prefix, "multicast_pkts");
    ctr.broadcastPkts = getSumStat(prefix, "broadcast_pkts");
    ctr.errors.errors = getSumStat(prefix, "errors");
    ctr.errors.discards = getSumStat(prefix, "discards");
  };

  fillPortCounters(portInfo.output, "out_");
  fillPortCounters(portInfo.input, "in_");
}

void ThriftHandler::getPortInfoHelper(
    PortInfoThrift& portInfo,
    const std::shared_ptr<Port> port) {
  portInfo.portId = port->getID();
  portInfo.name = port->getName();
  portInfo.description = port->getDescription();
  portInfo.speedMbps = (int) port->getWorkingSpeed();
  for (auto entry : port->getVlans()) {
    portInfo.vlans.push_back(entry.first);
  }

  portInfo.adminState =
      PortAdminState(port->getAdminState() == cfg::PortState::ENABLED);
  portInfo.operState =
      PortOperState(port->getOperState() == Port::OperState::UP);
  portInfo.fecEnabled = sw_->getHw()->getPortFECConfig(port->getID());

  auto pause = port->getPause();
  portInfo.txPause = pause.tx;
  portInfo.rxPause = pause.rx;

  fillPortStats(portInfo);
}

void ThriftHandler::getPortInfo(PortInfoThrift &portInfo, int32_t portId) {
  ensureConfigured();

  const auto port =
      sw_->getState()->getPorts()->getPortIf(PortID(portId));
  if (!port) {
    throw FbossError("no such port ", portId);
  }

  getPortInfoHelper(portInfo, port);
}

void ThriftHandler::getAllPortInfo(map<int32_t, PortInfoThrift>& portInfoMap) {
  ensureConfigured();

  // NOTE: important to take pointer to switch state before iterating over
  // list of ports
  std::shared_ptr<SwitchState> swState = sw_->getState();
  for (const auto& port : *(swState->getPorts())) {
    auto portId = port->getID();
    auto& portInfo = portInfoMap[portId];
    getPortInfoHelper(portInfo, port);
  }
}

void ThriftHandler::getPortStats(PortInfoThrift& portInfo, int32_t portId) {
  getPortInfo(portInfo, portId);
}

void ThriftHandler::getAllPortStats(map<int32_t, PortInfoThrift>& portInfoMap) {
  getAllPortInfo(portInfoMap);
}

void ThriftHandler::getRunningConfig(std::string& configStr) {
  ensureConfigured();
  configStr = sw_->getConfigStr();
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

void ThriftHandler::setPortState(int32_t portNum, bool enable) {
  ensureConfigured();
  PortID portId = PortID(portNum);
  const auto port = sw_->getState()->getPorts()->getPortIf(portId);
  if (!port) {
    throw FbossError("no such port ", portNum);
  }

  cfg::PortState newPortState =
      enable ? cfg::PortState::ENABLED : cfg::PortState::DISABLED;

  if (port->getAdminState() == newPortState) {
    VLOG(2) << "setPortState: port already in state "
            << (enable ? "ENABLED" : "DISABLED");
    return;
  }

  auto updateFn = [=](const shared_ptr<SwitchState>& state) {
    shared_ptr<SwitchState> newState{state};
    auto newPort = port->modify(&newState);
    newPort->setAdminState(newPortState);
    return newState;
  };
  sw_->updateStateBlocking("set port state", updateFn);
}

void ThriftHandler::getRouteTable(std::vector<UnicastRoute>& routes) {
  ensureConfigured();
  for (const auto& routeTable : (*sw_->getAppliedState()->getRouteTables())) {
    for (const auto& ipv4 : *(routeTable->getRibV4()->routes())) {
      UnicastRoute tempRoute;
      if (!ipv4->isResolved()) {
        LOG(INFO) << "Skipping unresolved route: " << ipv4->toFollyDynamic();
        continue;
      }
      auto fwdInfo = ipv4->getForwardInfo();
      tempRoute.dest.ip = toBinaryAddress(ipv4->prefix().network);
      tempRoute.dest.prefixLength = ipv4->prefix().mask;
      tempRoute.nextHopAddrs = util::fromFwdNextHops(fwdInfo.getNextHopSet());
      routes.emplace_back(std::move(tempRoute));
    }
    for (const auto& ipv6 : *(routeTable->getRibV6()->routes())) {
      UnicastRoute tempRoute;
      if (!ipv6->isResolved()) {
        LOG(INFO) << "Skipping unresolved route: " << ipv6->toFollyDynamic();
        continue;
      }
      auto fwdInfo = ipv6->getForwardInfo();
      tempRoute.dest.ip = toBinaryAddress(ipv6->prefix().network);
      tempRoute.dest.prefixLength = ipv6->prefix().mask;
      tempRoute.nextHopAddrs = util::fromFwdNextHops(fwdInfo.getNextHopSet());
      routes.emplace_back(std::move(tempRoute));
    }
  }
}

void ThriftHandler::getRouteTableByClient(
    std::vector<UnicastRoute>& routes, int16_t client) {
  ensureConfigured();
  for (const auto& routeTable : (*sw_->getState()->getRouteTables())) {
    for (const auto& ipv4 : *(routeTable->getRibV4()->routes())) {
      auto entry = ipv4->getEntryForClient(ClientID(client));
      if (not entry) {
        continue;
      }

      UnicastRoute tempRoute;
      tempRoute.dest.ip = toBinaryAddress(ipv4->prefix().network);
      tempRoute.dest.prefixLength = ipv4->prefix().mask;
      tempRoute.nextHopAddrs = util::fromRouteNextHops(
          entry->getNextHopSet());
      routes.emplace_back(std::move(tempRoute));
    }

    for (const auto& ipv6 : *(routeTable->getRibV6()->routes())) {
      auto entry = ipv6->getEntryForClient(ClientID(client));
      if (not entry) {
        continue;
      }

      UnicastRoute tempRoute;
      tempRoute.dest.ip = toBinaryAddress(ipv6->prefix().network);
      tempRoute.dest.prefixLength = ipv6->prefix().mask;
      tempRoute.nextHopAddrs = util::fromRouteNextHops(
          entry->getNextHopSet());
      routes.emplace_back(std::move(tempRoute));
    }
  }
}

void ThriftHandler::getRouteTableDetails(std::vector<RouteDetails>& routes) {
  ensureConfigured();
  for (const auto& routeTable : (*sw_->getState()->getRouteTables())) {
    for (const auto& ipv4 : *(routeTable->getRibV4()->routes())) {
      RouteDetails rd = ipv4->toRouteDetails();
      routes.emplace_back(std::move(rd));
    }
    for (const auto& ipv6 : *(routeTable->getRibV6()->routes())) {
      RouteDetails rd = ipv6->toRouteDetails();
      routes.emplace_back(std::move(rd));
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
    if (!match || !match->isResolved()) {
      route.dest.ip = toBinaryAddress(IPAddressV4("0.0.0.0"));
      route.dest.prefixLength = 0;
      return;
    }
    const auto fwdInfo = match->getForwardInfo();
    route.dest.ip = toBinaryAddress(match->prefix().network);
    route.dest.prefixLength = match->prefix().mask;
    route.nextHopAddrs = util::fromFwdNextHops(fwdInfo.getNextHopSet());
  } else {
    auto ripV6Rib = routeTable->getRibV6();
    auto match = ripV6Rib->longestMatch(ipAddr.asV6());
    if (!match || !match->isResolved()) {
      route.dest.ip = toBinaryAddress(IPAddressV6("::0"));
      route.dest.prefixLength = 0;
      return;
    }
    const auto fwdInfo = match->getForwardInfo();
    route.dest.ip = toBinaryAddress(match->prefix().network);
    route.dest.prefixLength = match->prefix().mask;
    route.nextHopAddrs = util::fromFwdNextHops(fwdInfo.getNextHopSet());
  }
}

void ThriftHandler::getIpRouteDetails(
  RouteDetails& route, std::unique_ptr<Address> addr, int32_t vrfId) {
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
    if (match && match->isResolved()) {
      route = match->toRouteDetails();
    }
  } else {
    auto ripV6Rib = routeTable->getRibV6();
    auto match = ripV6Rib->longestMatch(ipAddr.asV6());
    if (match && match->isResolved()) {
      route = match->toRouteDetails();
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
  auto lldpMgr = sw_->getLldpMgr();
  if (lldpMgr == nullptr) {
    throw std::runtime_error("lldpMgr is not configured");
  }

  auto* db = lldpMgr->getDB();
  // Do an immediate check for expired neighbors
  db->pruneExpiredNeighbors();
  auto neighbors = db->getNeighbors();
  results.reserve(neighbors.size());
  auto now = steady_clock::now();
  for (const auto& entry : db->getNeighbors()) {
    results.push_back(thriftLinkNeighbor(entry, now));
  }
}

void ThriftHandler::invokeNeighborListeners(ThreadLocalListener* listener,
                                             std::vector<std::string> added,
                                             std::vector<std::string> removed) {
  // Collect the iterators to avoid erasing and potentially reordering
  // the iterators in the list.
  for (const auto& ctx : brokenClients_) {
    listener->clients.erase(ctx);
  }
  brokenClients_.clear();
  for (auto& client : listener->clients) {
    auto clientDone = [&](ClientReceiveState&& state) {
      try {
        NeighborListenerClientAsyncClient::recv_neighborsChanged(state);
      } catch (const std::exception& ex) {
        LOG(ERROR) << "Exception in neighbor listener: " << ex.what();
        brokenClients_.push_back(client.first);
      }
    };
    client.second->neighborsChanged(clientDone, added, removed);
  }
}

void ThriftHandler::async_eb_registerForNeighborChanged(
    ThriftCallback<void> cb) {
  auto ctx = cb->getConnectionContext()->getConnectionContext();
  auto client = ctx->getDuplexClient<NeighborListenerClientAsyncClient>();
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
  auto capture = make_unique<PktCapture>(
      info->name, info->maxPackets, info->direction);
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

void ThriftHandler::startLoggingRouteUpdates(
    std::unique_ptr<RouteUpdateLoggingInfo> info) {
  auto* routeUpdateLogger = sw_->getRouteUpdateLogger();
  folly::IPAddress addr = toIPAddress(info->prefix.ip);
  uint8_t mask = static_cast<uint8_t>(info->prefix.prefixLength);
  RouteUpdateLoggingInstance loggingInstance{
      RoutePrefix<folly::IPAddress>{addr, mask}, info->identifier, info->exact};
  routeUpdateLogger->startLoggingForPrefix(loggingInstance);
}

void ThriftHandler::stopLoggingRouteUpdates(
    std::unique_ptr<IpPrefix> prefix,
    std::unique_ptr<std::string> identifier) {
  auto* routeUpdateLogger = sw_->getRouteUpdateLogger();
  folly::IPAddress addr = toIPAddress(prefix->ip);
  uint8_t mask = static_cast<uint8_t>(prefix->prefixLength);
  routeUpdateLogger->stopLoggingForPrefix(addr, mask, *identifier);
}

void ThriftHandler::stopLoggingAnyRouteUpdates(
    std::unique_ptr<std::string> identifier) {
  auto* routeUpdateLogger = sw_->getRouteUpdateLogger();
  routeUpdateLogger->stopLoggingForIdentifier(*identifier);
}

void ThriftHandler::getRouteUpdateLoggingTrackedPrefixes(
    std::vector<RouteUpdateLoggingInfo>& infos) {
  auto* routeUpdateLogger = sw_->getRouteUpdateLogger();
  for (const auto& tracked : routeUpdateLogger->getTrackedPrefixes()) {
    RouteUpdateLoggingInfo info;
    IpPrefix prefix;
    prefix.ip = toBinaryAddress(tracked.prefix.network);
    prefix.prefixLength = tracked.prefix.mask;
    info.prefix = prefix;
    info.identifier = tracked.identifier;
    info.exact = tracked.exact;
    infos.push_back(info);
  }
}

void ThriftHandler::beginPacketDump(int32_t port) {
  // Client construction is serialized via SwSwitch event base
  sw_->constructPushClient(port);
}

void ThriftHandler::killDistributionProcess(){
  sw_->killDistributionProcess();
}

void ThriftHandler::sendPkt(int32_t port, int32_t vlan,
                            unique_ptr<fbstring> data) {
  ensureConfigured("sendPkt");
  auto buf = IOBuf::copyBuffer(reinterpret_cast<const uint8_t*>(data->data()),
                               data->size());
  auto pkt = make_unique<MockRxPacket>(std::move(buf));
  pkt->setSrcPort(PortID(port));
  pkt->setSrcVlan(VlanID(vlan));
  sw_->packetReceived(std::move(pkt));
}

void ThriftHandler::sendPktHex(int32_t port, int32_t vlan,
                               unique_ptr<fbstring> hex) {
  ensureConfigured("sendPktHex");
  auto pkt = MockRxPacket::fromHex(StringPiece(*hex));
  pkt->setSrcPort(PortID(port));
  pkt->setSrcVlan(VlanID(vlan));
  sw_->packetReceived(std::move(pkt));
}

void ThriftHandler::txPkt(int32_t port, unique_ptr<fbstring> data) {
  ensureConfigured("txPkt");

  unique_ptr<TxPacket> pkt = sw_->allocatePacket(data->size());
  RWPrivateCursor cursor(pkt->buf());
  cursor.push(StringPiece(*data));

  sw_->sendPacketOutOfPort(std::move(pkt), PortID(port));
}

void ThriftHandler::txPktL2(unique_ptr<fbstring> data) {
  ensureConfigured("txPktL2");

  unique_ptr<TxPacket> pkt = sw_->allocatePacket(data->size());
  RWPrivateCursor cursor(pkt->buf());
  cursor.push(StringPiece(*data));

  sw_->sendPacketSwitched(std::move(pkt));
}

void ThriftHandler::txPktL3(unique_ptr<fbstring> payload) {
  ensureConfigured("txPktL3");

  unique_ptr<TxPacket> pkt = sw_->allocateL3TxPacket(payload->size());
  RWPrivateCursor cursor(pkt->buf());
  cursor.push(StringPiece(*payload));

  sw_->sendL3Packet(std::move(pkt));
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
  return sw_->getNeighborUpdater()->flushEntry(vlanID, parsedIP);
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

void ThriftHandler::getSfpDomInfo(
    map<int32_t, SfpDom>& /*domInfos*/,
    unique_ptr<vector<int32_t>> /*ports*/) {
  throw FbossError(folly::to<std::string>("This call is no longer supported, ",
      "use getTransceiverInfo instead"));
}

void ThriftHandler::getTransceiverInfo(
    map<int32_t, TransceiverInfo>& /*info*/,
    unique_ptr<vector<int32_t>> /*ids*/) {
  throw FbossError(folly::to<std::string>("This call is no longer supported, ",
      "please call the QsfpService instead"));
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
  if (sw_->isFullyConfigured()) {
    return;
  }

  if (!function.empty()) {
    VLOG(1) << "failing thrift prior to switch configuration: " << function;
  }
  throw FbossError("switch is still initializing or is exiting and is not "
                   "fully configured yet");
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
  auto numCounters = sw_->getHighresSamplers(samplers.get(), req->counterSet);

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

void ThriftHandler::reloadConfig() {
  return sw_->applyConfig("reload config initiated by thrift call");
}

}} // facebook::fboss
