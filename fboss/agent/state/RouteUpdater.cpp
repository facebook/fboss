/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
// Copyright 2004-present Facebook.  All rights reserved.
#include "RouteUpdater.h"

#include "fboss/agent/state/Interface.h"
#include "fboss/agent/state/InterfaceMap.h"
#include "fboss/agent/state/NodeBase.h"
#include "fboss/agent/state/NodeBase-defs.h"
#include "fboss/agent/state/Route.h"
#include "fboss/agent/state/RouteTable.h"
#include "fboss/agent/state/RouteTableMap.h"
#include "fboss/agent/state/RouteTableRib.h"
#include "fboss/agent/FbossError.h"
#include "fboss/agent/if/gen-cpp2/ctrl_types.h"

using folly::IPAddress;
using folly::IPAddressV4;
using folly::IPAddressV6;
using boost::container::flat_map;
using boost::container::flat_set;
using folly::CIDRNetwork;

namespace facebook { namespace fboss {

static const
RouteUpdater::PrefixV6 kIPv6LinkLocalPrefix{folly::IPAddressV6("fe80::"), 64};
static const auto kInterfaceRouteClientId =
  StdClientIds2ClientID(StdClientIds::INTERFACE_ROUTE);

using std::make_shared;

RouteUpdater::RouteUpdater(const std::shared_ptr<RouteTableMap>& orig)
    : orig_(orig) {
  for (const auto& rt : orig->getAllNodes()) {
    auto& rib = clonedRibs_[rt.first];
    rib.v4.rib = rt.second->getRibV4();
    rib.v6.rib = rt.second->getRibV6();
  }
}

RouteUpdater::ClonedRib* RouteUpdater::createNewRib(RouterID id) {
  ClonedRib newRib;
  newRib.v4.rib = make_shared<RouteTableRibV4>();
  newRib.v4.cloned = true;
  newRib.v6.rib = make_shared<RouteTableRibV6>();
  newRib.v6.cloned = true;
  auto ret = clonedRibs_.emplace(id, newRib);
  if (!ret.second) {
    throw FbossError("Duplicated cloned RIB for vrf ", id);
  }
  return &ret.first->second;
}

RouteUpdater::ClonedRib::RibV4* RouteUpdater::getRibV4(
    RouterID id, bool createIfNotExist) {
  auto iter = clonedRibs_.find(id);
  if (iter != clonedRibs_.end()) {
    return &iter->second.v4;
  } else if (!createIfNotExist) {
    return nullptr;
  }
  return &createNewRib(id)->v4;
}

RouteUpdater::ClonedRib::RibV6* RouteUpdater::getRibV6(
    RouterID id, bool createIfNotExist) {
  auto iter = clonedRibs_.find(id);
  if (iter != clonedRibs_.end()) {
    return &iter->second.v6;
  } else if (!createIfNotExist) {
    return nullptr;
  }
  return &createNewRib(id)->v6;
}

template<typename RibT>
auto RouteUpdater::makeClone(RibT* rib) -> decltype(rib->rib.get()) {
  if (rib->cloned) {
    return rib->rib.get();
  }
  auto newRtRib = rib->rib->clone();
  rib->rib.swap(newRtRib);
  rib->cloned = true;
  return rib->rib.get();
}

template<typename PrefixT, typename RibT>
void RouteUpdater::addRouteImpl(const PrefixT& prefix, RibT *ribCloned,
                                ClientID clientId, RouteNextHopEntry entry) {
  typedef Route<typename PrefixT::AddressT> RouteT;
  auto rib = ribCloned->rib.get();
  auto old = rib->exactMatch(prefix);
  if (old && old->has(clientId, entry)) {
      return;
  }
  rib = makeClone(ribCloned);
  if (old) {
    std::shared_ptr<RouteT> newRoute;
    // If the node is not published yet, we assume this thread has exclusive
    // access to the node. Therefore, we can do the modification in-place
    // directly.
    if (old->isPublished()) {
      newRoute = old->clone(
          RouteFields<typename PrefixT::AddressT>::COPY_PREFIX_AND_NEXTHOPS);
      rib->updateRoute(newRoute);
    } else {
      newRoute = old;
    }
    newRoute->update(clientId, std::move(entry));
    VLOG(3) << "Updated route " << newRoute->str();
  } else {
    auto newRoute = make_shared<RouteT>(prefix, clientId, std::move(entry));
    rib->addRoute(newRoute);
    VLOG(3) << "Added route " << newRoute->str();
  }
  CHECK(ribCloned->cloned);
}

void RouteUpdater::addRoute(
    RouterID id, const folly::IPAddress& network, uint8_t mask,
    ClientID clientId, RouteNextHopEntry entry) {
  if (network.isV4()) {
    PrefixV4 prefix{network.asV4().mask(mask), mask};
    addRouteImpl(prefix, getRibV4(id), clientId, std::move(entry));
  } else {
    PrefixV6 prefix{network.asV6().mask(mask), mask};
    if (prefix.network.isLinkLocal()) {
      VLOG(2) << "Ignoring v6 link-local interface route: " << prefix.str();
      return;
    }
    addRouteImpl(prefix, getRibV6(id), clientId, std::move(entry));
  }
}

void RouteUpdater::addLinkLocalRoutes(RouterID id) {
  // NOTE: v4 link-local route is not added because currently fboss handles
  // v4 link-locals as normal routes.

  // Add v6 link-local route
  addRouteImpl(
      kIPv6LinkLocalPrefix,
      getRibV6(id),
      StdClientIds2ClientID(StdClientIds::LINKLOCAL_ROUTE),
      RouteNextHopEntry(RouteForwardAction::TO_CPU));
}

void RouteUpdater::delLinkLocalRoutes(RouterID id) {
  // Delete v6 link-local route
  delRouteImpl(
      kIPv6LinkLocalPrefix,
      getRibV6(id),
      StdClientIds2ClientID(StdClientIds::LINKLOCAL_ROUTE));
}

template<typename PrefixT, typename RibT>
void RouteUpdater::delRouteImpl(
    const PrefixT& prefix, RibT *ribCloned, ClientID clientId) {
  if (!ribCloned) {
    VLOG(3) << "Failed to delete non-existing route " << prefix.str();
    return;
  }
  auto rib = ribCloned->rib.get();
  auto old = rib->exactMatch(prefix);
  if (!old) {
    VLOG(3) << "Failed to delete non-existing route " << prefix.str();
    return;
  }
  rib = makeClone(ribCloned);
  // Re-get the route from the cloned RIB
  old = rib->exactMatch(prefix);
  old->delEntryForClient(clientId);
  // TODO Do I need to publish the change??
  VLOG(3) << "Deleted nexthops for client " << clientId <<
             " from route " << prefix.str();
  if (old->hasNoEntry()) {
    rib->removeRoute(old);
    VLOG(3) << "...and then deleted route " << prefix.str();
  }
  CHECK(ribCloned->cloned);
}

void RouteUpdater::delRoute(
    RouterID id, const folly::IPAddress& network,
    uint8_t mask, ClientID clientId) {
  if (network.isV4()) {
    PrefixV4 prefix{network.asV4().mask(mask), mask};
    delRouteImpl(prefix, getRibV4(id, false), clientId);
  } else {
    PrefixV6 prefix{network.asV6().mask(mask), mask};
    delRouteImpl(prefix, getRibV6(id, false), clientId);
  }
}

template<typename AddrT, typename RibT>
void RouteUpdater::removeAllRoutesForClientImpl(RibT *ribCloned,
                                                ClientID clientId) {
  auto rib = makeClone(ribCloned);

  std::vector<std::shared_ptr<Route<AddrT>>> routesToDelete;

  for (auto& rt : rib->routes()) {
    auto route = rt.value().get();
    route->delEntryForClient(clientId);
    if (route->hasNoEntry()) {
      // The nexthops we removed was the only one.  Delete the route.
      routesToDelete.push_back(rt.value());
    }
  }

  // Now, delete whatever routes went from 1 nexthoplist to 0.
  for (std::shared_ptr<Route<AddrT>>& rt : routesToDelete) {
    rib->removeRoute(rt);
  }
}

void RouteUpdater::removeAllRoutesForClient(RouterID rid, ClientID clientId) {
  removeAllRoutesForClientImpl<IPAddressV4>(getRibV4(rid), clientId);
  removeAllRoutesForClientImpl<IPAddressV6>(getRibV6(rid), clientId);
}

template<typename RtRibT, typename AddrT>
void RouteUpdater::getFwdInfoFromNhop(
    RtRibT* nRib, ClonedRib* ribCloned, const AddrT& nh,
    bool *hasToCpu, bool *hasDrop, RouteNextHopSet* fwd) {
  auto rt = nRib->longestMatch(nh);
  if (rt == nullptr) {
    VLOG (3) <<" Could not find route for nhop :  "<< nh;
    // Un resolvable next hop
    return;
  }
  if (rt->needResolve()) {
    resolveOne(rt.get(), nRib, ribCloned);
  }
  if (rt->isResolved()) {
    const auto& fwdInfo = rt->getForwardInfo();
    if (fwdInfo.isDrop()) {
      *hasDrop = true;
    } else if (fwdInfo.isToCPU()) {
      *hasToCpu = true;
    } else {
      const auto& nhops = fwdInfo.getNextHopSet();
      // if the route used to resolve the nexthop is directly connected
      if (rt->isConnected()) {
        const auto& rtNh = *nhops.begin();
        fwd->emplace(RouteNextHop::createForward(nh, rtNh.intf()));
      } else {
        fwd->insert(nhops.begin(), nhops.end());
      }
    }
  }
}

template<typename RouteT, typename RtRibT>
void RouteUpdater::resolveOne(
    RouteT* route, RtRibT* rib, ClonedRib* ribCloned) {
  // first, make sure the route was not published yet, if it is, clone one
  if (route->isPublished()) {
    auto newRoute = route->clone(RouteT::Fields::COPY_PREFIX_AND_NEXTHOPS);
    // insert the cloned route back to the RIB
    // Note: resolve() is called in a loop over 'rib'. But we are modifying
    // the rib here. Fortunately, updateRoute() here does not actually
    // invalidate the existing iterator, it just replace the value pointed by
    // an existing iterator.
    // It might be nice to change resolve() to accept a RouteTableRib::iterator
    // as a first argument. This way it could just modify iter.second, instead
    // of having to call updateRoute(). This way it would be very clear that
    // this can't invalidate the iterator. (It would also avoid having to
    // re-lookup the node in updateRoute().)
    // The current API for the temporary RouteTableRib solution does not return
    // any iterator. Once we switch to the RadixTree() solution, we should make
    // this change.
    rib->updateRoute(newRoute);
    route = newRoute.get();
    CHECK(!route->isPublished());
  }
  // mark this route is in processing. This processing bit shall be cleared
  // in setUnresolvable() or setResolved()
  route->setProcessing();

  RouteNextHopSet fwd;
  bool hasToCpu{false};
  bool hasDrop{false};

  auto bestPair = route->getBestEntry();
  const auto clientId = bestPair.first;
  const auto bestEntry = bestPair.second;
  const auto action = bestEntry->getAction();
  if (action == RouteForwardAction::DROP) {
    hasDrop = true;
  } else if (action == RouteForwardAction::TO_CPU) {
    hasToCpu = true;
  } else {
    // loop through all nexthops to find out the forward info
    for (const auto& nh : bestEntry->getNextHopSet()) {
      const auto& addr = nh.addr();
      // There are two reasons why InterfaceID is specified in the next hop.
      // 1) The nexthop was generated for interface route.
      //    In this case, the clientId is INTERFACE_ROUTE
      // 2) The nexthop was for v6 link-local address.
      // In both cases, this nexthop is resolved.
      if (nh.intfID().hasValue()) {
        // It is either an interface route or v6 link-local
        CHECK(clientId == kInterfaceRouteClientId
              or (addr.isV6() and addr.isLinkLocal()));
        fwd.emplace(RouteNextHop::createForward(addr, nh.intfID().value()));
        continue;
      }

      // nexthops are a set of IPs
      if (addr.isV4()) {
        auto nRib = ribCloned->v4.rib.get();
        getFwdInfoFromNhop(nRib, ribCloned, nh.addr().asV4(), &hasToCpu,
                           &hasDrop, &fwd);
      } else {
        auto nRib = ribCloned->v6.rib.get();
        getFwdInfoFromNhop(nRib, ribCloned, nh.addr().asV6(), &hasToCpu,
                           &hasDrop, &fwd);
      }
    }
  }

  if (!fwd.empty()) {
    route->setResolved(RouteNextHopEntry(std::move(fwd)));
    if (clientId == kInterfaceRouteClientId) {
      route->setConnected();
    }
  } else if (hasToCpu) {
    route->setResolved(RouteNextHopEntry(RouteForwardAction::TO_CPU));
  } else if (hasDrop) {
    route->setResolved(RouteNextHopEntry(RouteForwardAction::DROP));
  } else {
    route->setUnresolvable();
  }

  VLOG(3) << (route->isResolved() ? "Resolved" : "Cannot resolve")
          << " route " << route->str();
}

template<typename RibT>
void RouteUpdater::cloneRoutesForResolution(RibT* rib) {
  for (auto& rt : rib->routes()) {
    auto route = rt.value().get();
    if (route->isPublished()) {
      auto newRoute =
        route->clone(RibT::RouteType::Fields::COPY_PREFIX_AND_NEXTHOPS);
      rib->updateRoute(newRoute);
      route = newRoute.get();
    }
    route->clearForward();
  }
}

void RouteUpdater::resolve() {
  // Ideally, just need to resolve the routes that is changed or impacted by
  // the changed routes.
  // However, Without copy-on-write RadixTree, it is O(n) to find out the
  // routes that are changed. In this case, we just simply loop through all
  // routes and resolve those that are not resolved yet.
  for (auto& ribCloned : clonedRibs_) {
    if (ribCloned.second.v4.cloned) {
      auto rib = ribCloned.second.v4.rib.get();
      cloneRoutesForResolution(rib);
      for (auto& rt : rib->routes()) {
        if (rt.value()->needResolve()) {
          resolveOne(rt.value().get(), rib, &ribCloned.second);
        }
      }
    }
    if (ribCloned.second.v6.cloned) {
      auto rib = ribCloned.second.v6.rib.get();
      cloneRoutesForResolution(rib);
      for (auto& rt : rib->routes()) {
        if (rt.value()->needResolve()) {
          resolveOne(rt.value().get(), rib, &ribCloned.second);
        }
      }
    }
  }
}

std::shared_ptr<RouteTableMap> RouteUpdater::updateDone() {
  // resolve all routes
  resolve();
  RouteTableMap::NodeContainer map;
  bool changed = false;
  for (const auto& ribPair : clonedRibs_) {
    auto id = ribPair.first;
    const auto& rib = ribPair.second;
    changed |= rib.v4.cloned || rib.v6.cloned;
    if (rib.v4.rib->empty() && rib.v6.rib->empty()) {
      continue;
    }

    auto origRt = orig_->getRouteTableIf(ribPair.first);
    if (!rib.v4.cloned && !rib.v6.cloned) {
      CHECK(origRt != nullptr);
      auto iter = map.emplace(id, std::move(origRt));
      CHECK(iter.second);
      continue;
    }
    auto newRt = make_shared<RouteTable>(ribPair.first);
    if (rib.v4.cloned) {
      newRt->setRib(rib.v4.rib);
    } else {
      if (origRt != nullptr) {
        newRt->setRib(origRt->getRibV4());
      }
    }
    if (rib.v6.cloned) {
      newRt->setRib(rib.v6.rib);
    } else {
      if (origRt != nullptr) {
        newRt->setRib(origRt->getRibV6());
      }
    }
    auto iter = map.emplace(id, std::move(newRt));
    CHECK(iter.second);
  }
  if (!changed || !deduplicate(&map)) {
    return nullptr;
  }
  return orig_->clone(map);
}

void RouteUpdater::addInterfaceAndLinkLocalRoutes(
    const std::shared_ptr<InterfaceMap>& intfs) {
  boost::container::flat_set<RouterID> routers;
  for (auto const& item: intfs->getAllNodes()) {
    const auto& intf = item.second;
    auto routerId = intf->getRouterID();
    routers.insert(routerId);
    for (auto const& addr : intf->getAddresses()) {
      auto nhop = RouteNextHop::createInterfaceNextHop(
          addr.first, intf->getID());
      addRoute(routerId, addr.first, addr.second,
               kInterfaceRouteClientId,
               RouteNextHopEntry(std::move(nhop),
                 AdminDistance::DIRECTLY_CONNECTED));
    }
  }
  for (auto id : routers) {
    addLinkLocalRoutes(id);
  }
}

template<typename StaticRouteType>
void RouteUpdater::staticRouteDelHelper(
    const std::vector<StaticRouteType>& oldRoutes,
    const flat_map<RouterID, flat_set<CIDRNetwork>>& newRoutes) {
  for (const auto& oldRoute : oldRoutes) {
    RouterID rid(oldRoute.routerID);
    auto network = IPAddress::createNetwork(oldRoute.prefix);
    auto itr = newRoutes.find(rid);
    if (itr == newRoutes.end() || itr->second.find(network) ==
        itr->second.end()) {
      delRoute(rid, network.first, network.second,
               StdClientIds2ClientID(StdClientIds::STATIC_ROUTE));
      VLOG(1) << "Unconfigured static route : " << network.first
        << "/" << (int)network.second;
    }
  }
}

void RouteUpdater::updateStaticRoutes(const cfg::SwitchConfig& curCfg,
    const cfg::SwitchConfig& prevCfg) {
  flat_map<RouterID, flat_set<CIDRNetwork>> newCfgVrf2StaticPfxs;
  auto processStaticRoutesNoNhops = [&](
      const std::vector<cfg::StaticRouteNoNextHops>& routes,
      RouteForwardAction action) {
    for (const auto& route : routes) {
      RouterID rid(route.routerID) ;
      auto network = IPAddress::createNetwork(route.prefix);
      if (newCfgVrf2StaticPfxs[rid].find(network)
          != newCfgVrf2StaticPfxs[rid].end()) {
        throw FbossError("Prefix : ",  network.first, "/",
            (int)network.second, " in multiple static routes");
      }
      addRoute(rid, network.first, network.second,
               StdClientIds2ClientID(StdClientIds::STATIC_ROUTE),
               RouteNextHopEntry(action));
      // Note down prefix for comparing with old static routes
      newCfgVrf2StaticPfxs[rid].emplace(network);
    }
  };


  if (curCfg.__isset.staticRoutesToNull) {
    processStaticRoutesNoNhops(curCfg.staticRoutesToNull, DROP);
  }
  if (curCfg.__isset.staticRoutesToCPU) {
    processStaticRoutesNoNhops(curCfg.staticRoutesToCPU, TO_CPU);
  }

  if (curCfg.__isset.staticRoutesWithNhops) {
    for (const auto& route : curCfg.staticRoutesWithNhops) {
      RouterID rid(route.routerID) ;
      auto network = IPAddress::createNetwork(route.prefix);
      RouteNextHops nhops;
      for (auto& nhopStr : route.nexthops) {
        nhops.emplace(
            RouteNextHop::createNextHop(folly::IPAddress(nhopStr)));
      }
      addRoute(rid, network.first, network.second,
               StdClientIds2ClientID(StdClientIds::STATIC_ROUTE),
               RouteNextHopEntry(std::move(nhops)));
      // Note down prefix for comparing with old static routes
      newCfgVrf2StaticPfxs[rid].emplace(network);
    }
  }
  // Now blow away any static routes that are not in the config
  // Ideally after this we should replay the routes from lower
  // precedence route announcers (e.g. BGP) for deleted prefixes.
  // The static route may have overridden a existing protocol
  // route and in that case rather than blowing away the route we
  // should just replace it - t8910011
  if (prevCfg.__isset.staticRoutesWithNhops) {
    staticRouteDelHelper(prevCfg.staticRoutesWithNhops, newCfgVrf2StaticPfxs);
  }
  if (prevCfg.__isset.staticRoutesToCPU) {
    staticRouteDelHelper(prevCfg.staticRoutesToCPU, newCfgVrf2StaticPfxs);
  }
  if (prevCfg.__isset.staticRoutesToNull) {
    staticRouteDelHelper(prevCfg.staticRoutesToNull, newCfgVrf2StaticPfxs);
  }
}

template<typename RibT>
bool RouteUpdater::dedupRoutes(const RibT* oldRib, RibT* newRib) {
  bool isSame = true;
  if (oldRib == newRib) {
    return isSame;
  }
  const auto& oldRoutes = oldRib->routes();
  auto& newRoutes = newRib->writableRoutes();
  // Copy routes from old route table if they are
  // same. For matching prefixes, which don't have
  // same attributes inherit the generation number
  for (auto oldIter : oldRoutes) {
    const auto& oldRt = oldIter->value();
    auto newIter = newRoutes.exactMatch(oldIter->ipAddress(),
        oldIter->masklen());
    if (newIter == newRoutes.end()) {
      isSame = false;
      continue;
    }
    auto& newRt = newIter->value();
    if (oldRt->isSame(newRt.get())) {
      // both routes are completely same, instead of using the new route,
      // we re-use the old route.
      newIter->value() = oldRt;
    } else {
      isSame = false;
      newRt->inheritGeneration(*oldRt);
    }
  }
  if (newRoutes.size() != oldRoutes.size()) {
    isSame = false;
  } else {
    // If sizes are same we would have already caught any
    // difference while looking up all routes from oldRoutes
    // in newRoutes, so do nothing
  }
  return isSame;
}

std::shared_ptr<RouteTableMap> RouteUpdater::deduplicate(
    RouteTableMap::NodeContainer* newTables) {
  bool allSame = true;
  const auto* oldTables = &orig_->getAllNodes();
  auto oldIter = oldTables->begin();
  auto newIter = newTables->begin();
  while (oldIter != oldTables->end() && newIter != newTables->end()) {
    const auto oldVrf = oldIter->first;
    const auto newVrf = newIter->first;
    if (oldVrf < newVrf) {
      allSame = false;
      oldIter++;
      continue;
    }
    if (oldVrf > newVrf) {
      allSame = false;
      newIter++;
      continue;
    }
    bool isSame = true;
    const auto& oldTable = oldIter->second;
    auto& newTable = newIter->second;
    if (oldTable != newTable) {
      // Handle V4 RIB
      const auto& oldV4 = oldTable->getRibV4();
      auto& newV4 = newTable->writableRibV4();
      if (dedupRoutes(oldV4.get(), newV4.get())) {
        newTable->setRib(oldV4);
      } else {
        isSame = false;
        newV4->inheritGeneration(*oldV4);
      }
      // Handle get V6 RIB
      const auto& oldV6 = oldTable->getRibV6();
      auto& newV6 = newTable->writableRibV6();
      if (dedupRoutes(oldV6.get(), newV6.get())) {
        newTable->setRib(oldV6);
      } else {
        isSame = false;
        newV6->inheritGeneration(*oldV6);
      }
    }
    // if both v4 RIB and v6 rib from the new RouteTable are as same as the
    // old one, we will just reuse the old RouteTable
    if (isSame) {
      newIter->second = oldTable;
    } else {
      allSame = false;
      newTable->inheritGeneration(*oldTable);
    }
    oldIter++;
    newIter++;
  }
  if (oldIter != oldTables->end() || newIter != newTables->end()) {
    allSame = false;
  }
  if (allSame) {
    return nullptr;
  }
  return orig_->clone(*newTables);
}

}}
