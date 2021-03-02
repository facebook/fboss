/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/RouteUpdateWrapper.h"

#include "fboss/agent/AddressUtil.h"
#include "fboss/agent/rib/ForwardingInformationBaseUpdater.h"
#include "fboss/agent/rib/RoutingInformationBase.h"
#include "fboss/agent/state/NodeBase-defs.h"
#include "fboss/agent/state/RouteTable.h"
#include "fboss/agent/state/RouteTableRib.h"
#include "fboss/agent/state/RouteUpdater.h"
#include "fboss/agent/state/SwitchState.h"

#include <folly/logging/xlog.h>

namespace facebook::fboss {

void RouteUpdateWrapper::addRoute(
    RouterID vrf,
    const folly::IPAddress& network,
    uint8_t mask,
    ClientID clientId,
    RouteNextHopEntry nhop) {
  UnicastRoute tempRoute;
  tempRoute.dest_ref()->ip_ref() = network::toBinaryAddress(network);
  tempRoute.dest_ref()->prefixLength_ref() = mask;
  if (nhop.getAction() == RouteForwardAction::NEXTHOPS) {
    tempRoute.nextHops_ref() = util::fromRouteNextHopSet(nhop.getNextHopSet());
    tempRoute.action_ref() = RouteForwardAction::NEXTHOPS;
  } else {
    tempRoute.action_ref() = nhop.getAction() == RouteForwardAction::DROP
        ? RouteForwardAction::DROP
        : RouteForwardAction::TO_CPU;
  }
  addRoute(vrf, clientId, std::move(tempRoute));
}

void RouteUpdateWrapper::addRoute(
    RouterID vrf,
    ClientID clientId,
    const UnicastRoute& route) {
  ribRoutesToAddDel_[std::make_pair(vrf, clientId)].toAdd.emplace_back(route);
}

void RouteUpdateWrapper::delRoute(
    RouterID vrf,
    const folly::IPAddress& network,
    uint8_t mask,
    ClientID clientId) {
  IpPrefix pfx;
  pfx.ip_ref() = network::toBinaryAddress(network);
  pfx.prefixLength_ref() = mask;
  delRoute(vrf, pfx, clientId);
}

void RouteUpdateWrapper::delRoute(
    RouterID vrf,
    const IpPrefix& pfx,
    ClientID clientId) {
  ribRoutesToAddDel_[std::make_pair(vrf, clientId)].toDel.emplace_back(
      std::move(pfx));
}

void RouteUpdateWrapper::program(
    const std::unordered_set<RouterIDAndClient>& syncFibFor) {
  syncFibFor_ = syncFibFor;
  if (isStandaloneRibEnabled_) {
    programStandAloneRib();
  } else {
    programLegacyRib();
  }
  ribRoutesToAddDel_.clear();
  syncFibFor_.clear();
}

void RouteUpdateWrapper::programMinAlpmState() {
  if (isStandaloneRibEnabled_) {
    getRib()->ensureVrf(RouterID(0));
  }
  addRoute(
      RouterID(0),
      folly::IPAddressV4("0.0.0.0"),
      0,
      ClientID::STATIC_INTERNAL,
      RouteNextHopEntry(
          RouteForwardAction::DROP, AdminDistance::MAX_ADMIN_DISTANCE));
  addRoute(
      RouterID(0),
      folly::IPAddressV6("::"),
      0,
      ClientID::STATIC_INTERNAL,
      RouteNextHopEntry(
          RouteForwardAction::DROP, AdminDistance::MAX_ADMIN_DISTANCE));
  program();
}

std::pair<std::shared_ptr<SwitchState>, RouteUpdateWrapper::UpdateStatistics>
RouteUpdateWrapper::programLegacyRibHelper(
    const std::shared_ptr<SwitchState>& in) const {
  UpdateStatistics stats;
  auto state = in->clone();
  RouteUpdater updater(state->getRouteTables());
  for (auto [ridClientId, addDelRoutes] : ribRoutesToAddDel_) {
    if (ridClientId.first != RouterID(0)) {
      throw FbossError("Multi-VRF only supported with Stand-Alone RIB");
    }
    auto adminDistance = clientIdToAdminDistance(ridClientId.second);
    auto& toAdd = addDelRoutes.toAdd;
    auto& toDel = addDelRoutes.toDel;
    RouterID routerId = ridClientId.first;
    ClientID clientId = ridClientId.second;
    if (syncFibFor_.find(ridClientId) != syncFibFor_.end()) {
      updater.removeAllRoutesForClient(routerId, clientId);
    }
    for (auto& route : toAdd) {
      std::vector<NextHopThrift> nhts;
      folly::IPAddress network =
          network::toIPAddress(*route.dest_ref()->ip_ref());
      uint8_t mask =
          static_cast<uint8_t>(*route.dest_ref()->prefixLength_ref());
      if (route.nextHops_ref()->empty() && !route.nextHopAddrs_ref()->empty()) {
        nhts = thriftNextHopsFromAddresses(*route.nextHopAddrs_ref());
      } else {
        nhts = *route.nextHops_ref();
      }
      RouteNextHopSet nexthops = util::toRouteNextHopSet(nhts);
      if (nexthops.size()) {
        updater.addRoute(
            routerId,
            network,
            mask,
            clientId,
            RouteNextHopEntry(std::move(nexthops), adminDistance));
      } else {
        XLOG(DBG3) << "Blackhole route:" << network << "/"
                   << static_cast<int>(mask);
        updater.addRoute(
            routerId,
            network,
            mask,
            clientId,
            RouteNextHopEntry(
                route.action_ref() ? *route.action_ref()
                                   : RouteForwardAction::DROP,
                adminDistance));
      }
      if (network.isV4()) {
        ++stats.v4RoutesAdded;
      } else {
        ++stats.v6RoutesAdded;
      }
    }
    // Del routes
    for (auto& prefix : toDel) {
      auto network = network::toIPAddress(*prefix.ip_ref());
      auto mask = static_cast<uint8_t>(*prefix.prefixLength_ref());
      if (network.isV4()) {
        ++stats.v4RoutesDeleted;
      } else {
        ++stats.v6RoutesDeleted;
      }
      updater.delRoute(routerId, network, mask, clientId);
    }
  }

  auto newRt = updater.updateDone();
  if (!newRt) {
    return {std::shared_ptr<SwitchState>(), stats};
  }
  newRt->publish();
  state->resetRouteTables(std::move(newRt));
  printStats(stats);
  return {state, stats};
}

void RouteUpdateWrapper::printStats(const UpdateStatistics& stats) const {
  XLOG(DBG0) << " Routes added: " << stats.v4RoutesAdded + stats.v6RoutesAdded
             << " Routes deleted: "
             << stats.v4RoutesDeleted + stats.v6RoutesDeleted << " Duration "
             << stats.duration.count() << " us ";
}

void RouteUpdateWrapper::programStandAloneRib() {
  for (auto [ridClientId, addDelRoutes] : ribRoutesToAddDel_) {
    auto stats = getRib()->update(
        ridClientId.first,
        ridClientId.second,
        clientIdToAdminDistance(ridClientId.second),
        addDelRoutes.toAdd,
        addDelRoutes.toDel,
        syncFibFor_.find(ridClientId) != syncFibFor_.end(),
        "RIB update",
        *fibUpdateFn_,
        fibUpdateCookie_);
    printStats(stats);
    updateStats(stats);
  }
}

void RouteUpdateWrapper::programClassID(
    RouterID rid,
    const std::vector<folly::CIDRNetwork>& prefixes,
    std::optional<cfg::AclLookupClass> classId,
    bool async) {
  if (isStandaloneRibEnabled_) {
    programClassIDStandAloneRib(rid, prefixes, classId, async);
  } else {
    programClassIDLegacyRib(rid, prefixes, classId, async);
  }
}

void RouteUpdateWrapper::programClassIDStandAloneRib(
    RouterID rid,
    const std::vector<folly::CIDRNetwork>& prefixes,
    std::optional<cfg::AclLookupClass> classId,
    bool async) {
  if (async) {
    getRib()->setClassIDAsync(
        rid, prefixes, *fibUpdateFn_, classId, fibUpdateCookie_);
  } else {
    getRib()->setClassID(
        rid, prefixes, *fibUpdateFn_, classId, fibUpdateCookie_);
  }
}

std::shared_ptr<SwitchState> RouteUpdateWrapper::updateClassIdLegacyRibHelper(
    const std::shared_ptr<SwitchState>& in,
    RouterID rid,
    const std::vector<folly::CIDRNetwork>& prefixes,
    std::optional<cfg::AclLookupClass> classId) {
  auto newState{in};

  auto updateClassId = [classId](auto rib, auto& prefix) {
    auto route = rib->routes()->getRouteIf(prefix);
    if (route) {
      route = route->clone();
      route->updateClassID(classId);
      rib->updateRoute(route);
    }
  };
  for (const auto& cidr : prefixes) {
    auto routeTables = newState->getRouteTables();
    auto routeTable = routeTables->getRouteTable(rid);

    if (cidr.first.isV6()) {
      RoutePrefix<folly::IPAddressV6> routePrefixV6{
          cidr.first.asV6(), cidr.second};
      auto rib = routeTable->getRibV6()->modify(rid, &newState);
      updateClassId(rib, routePrefixV6);
    } else {
      RoutePrefix<folly::IPAddressV4> routePrefixV4{
          cidr.first.asV4(), cidr.second};
      auto rib = routeTable->getRibV4()->modify(rid, &newState);
      updateClassId(rib, routePrefixV4);
    }
  }
  return newState;
}
} // namespace facebook::fboss
