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

namespace facebook { namespace fboss {

using std::make_shared;

RouteUpdater::RouteUpdater(const std::shared_ptr<RouteTableMap>& orig,
                           bool sync)
    : orig_(orig), sync_(sync) {
  // In the sync mode, we don't clone from the original route table map.
  // Intead, we start from empty.
  if (sync_) {
    return;
  }
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

template<typename PrefixT, typename RibT, typename... Args>
void RouteUpdater::addRoute(const PrefixT& prefix, RibT *ribCloned,
                            Args&&... args) {
  typedef Route<typename PrefixT::AddressT> RouteT;
  auto rib = ribCloned->rib.get();
  auto old = rib->exactMatch(prefix);
  if (old && old->isSame(std::forward<Args>(args)...)) {
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
          RouteFields<typename PrefixT::AddressT>::COPY_ONLY_PREFIX);
      rib->updateRoute(newRoute);
    } else {
      newRoute = old;
    }
    newRoute->update(std::forward<Args>(args)...);
    VLOG(3) << "Updated route " << newRoute->str();
  } else {
    auto newRoute = make_shared<RouteT>(prefix, std::forward<Args>(args)...);
    rib->addRoute(newRoute);
    VLOG(3) << "Added route " << newRoute->str();
  }
  CHECK(ribCloned->cloned);
}

void RouteUpdater::addRoute(RouterID id, InterfaceID intf,
                            const folly::IPAddress& intfAddr, uint8_t len) {
  if (intfAddr.isV4()) {
    PrefixV4 ifSubnetPrefix{intfAddr.asV4().mask(len), len};
    if (ifSubnetPrefix.network.isLinkLocal()) {
      VLOG(3) << "Adding route for link local address "
              << folly::to<std::string>(ifSubnetPrefix);
      // Don't return here. We still need to add routes. Otherwise, packets to
      // hosts advertising routes via link local addresses are not routed
      // properly.
      //
      // Ideally, we would not add these routes (and return early here), but
      // keep track of link local addresses and which VLANs they are associated
      // with. See t7365038 for more details.
    }
    addRoute(ifSubnetPrefix, getRibV4(id), intf, intfAddr);
  } else {
    PrefixV6 ifSubnetPrefix{intfAddr.asV6().mask(len), len};
    if (ifSubnetPrefix.network.isLinkLocal()) {
      VLOG(3) << "Ignoring route for link local address "
              << folly::to<std::string>(ifSubnetPrefix);
      return;
    }
    addRoute(ifSubnetPrefix, getRibV6(id), intf, intfAddr);
  }
}

void RouteUpdater::addRoute(
    RouterID id, const folly::IPAddress& network, uint8_t mask,
    RouteForwardAction action) {
  if (network.isV4()) {
    PrefixV4 prefix{network.asV4().mask(mask), mask};
    return addRoute(prefix, getRibV4(id), action);
  } else {
    PrefixV6 prefix{network.asV6().mask(mask), mask};
    return addRoute(prefix, getRibV6(id), action);
  }
}

void RouteUpdater::addRoute(RouterID id, const folly::IPAddress& network,
                            uint8_t mask, const RouteNextHops& nhs) {
  if (network.isV4()) {
    PrefixV4 prefix{network.asV4().mask(mask), mask};
    return addRoute(prefix, getRibV4(id), nhs);
  } else {
    PrefixV6 prefix{network.asV6().mask(mask), mask};
    if (prefix.network.isLinkLocal()) {
      throw FbossError("Unexpected v6 routable route for link local address ",
                       prefix);
    }
    return addRoute(prefix, getRibV6(id), nhs);
  }
}

void RouteUpdater::addRoute(RouterID id, const folly::IPAddress& network,
                            uint8_t mask, RouteNextHops&& nhs) {
  if (network.isV4()) {
    PrefixV4 prefix{network.asV4().mask(mask), mask};
    return addRoute(prefix, getRibV4(id), std::move(nhs));
  } else {
    PrefixV6 prefix{network.asV6().mask(mask), mask};
    if (prefix.network.isLinkLocal()) {
      throw FbossError("Unexpected v6 routable route for link local address ",
                       prefix);
    }
    return addRoute(prefix, getRibV6(id), std::move(nhs));
  }
}

void RouteUpdater::addLinkLocalRoutes(RouterID id) {
  // only one v6 link local route
  const auto linkLocal = folly::IPAddress("fe80::");
  const uint8_t mask = 64;
  addRoute(id, linkLocal, mask, RouteForwardAction::TO_CPU);
}

void RouteUpdater::delLinkLocalRoutes(RouterID id) {
  // only one v6 link local route
  const auto linkLocal = folly::IPAddress("fe80::");
  const uint8_t mask = 64;
  delRoute(id, linkLocal, mask);
}

template<typename PrefixT, typename RibT>
void RouteUpdater::delRoute(const PrefixT& prefix, RibT *ribCloned) {
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
  rib->removeRoute(old);
  VLOG(3) << "Deleted route " << prefix.str();
  CHECK(ribCloned->cloned);
}
void RouteUpdater::delRoute(RouterID id, const folly::IPAddress& network,
                            uint8_t mask) {
  if (network.isV4()) {
    PrefixV4 prefix{network.asV4().mask(mask), mask};
    return delRoute(prefix, getRibV4(id, false));
  } else {
    PrefixV6 prefix{network.asV6().mask(mask), mask};
    return delRoute(prefix, getRibV6(id, false));
  }
}

template<typename RouteT, typename RtRibT>
void RouteUpdater::resolve(RouteT* route, RtRibT* rib, ClonedRib* ribCloned) {
  CHECK(!route->isConnected());
  RouteForwardNexthops fwd;
  // first, make sure the route was not published yet, if it is, clone one
  if (route->isPublished()) {
    auto newRoute = route->clone(RouteT::Fields::COPY_ONLY_PREFIX);
    // copy the nexthop
    newRoute->update(route->nexthops());
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
  route->setFlagsProcessing();
  // loop through all nexthops to find out the forward info
  for (const auto& nh : route->nexthops()) {
    if (nh.isV4()) {
      auto nRib = ribCloned->v4.rib.get();
      auto rt = nRib->longestMatch(nh.asV4());
      if (rt == nullptr) {
        VLOG (3) <<" Could not find route for nhop :  "<< nh.asV4();
        // try next nexthop
        continue;
      }
      if (rt->needResolve()) {
        resolve(rt.get(), nRib, ribCloned);
      }
      if (rt->isResolved()) {
        // if the route used to resolve the nexthop is directly connected
        const auto nhops = rt->getForwardInfo().getNexthops();
        if (rt->isConnected()) {
          const auto& rtNh = *nhops.begin();
          fwd.emplace(rtNh.intf, nh);
        } else {
          fwd.insert(nhops.begin(), nhops.end());
        }
      }
    } else {
      auto nRib = ribCloned->v6.rib.get();
      auto rt = nRib->longestMatch(nh.asV6());
      if (rt == nullptr) {
        // try next nexthop
        continue;
      }
      if (rt->needResolve()) {
        resolve(rt.get(), nRib, ribCloned);
      }
      if (rt->isResolved()) {
        // if the route used to resolve the nexthop is directly connected
        const auto nhops = rt->getForwardInfo().getNexthops();
        if (rt->isConnected()) {
          const auto& rtNh = *nhops.begin();
          fwd.emplace(rtNh.intf, nh);
        } else {
          fwd.insert(nhops.begin(), nhops.end());
        }
      }
    }
  }
  if (fwd.empty()) {
    route->setUnresolvable();
    VLOG(3) << "Cannot resolve route " << route->str();
  } else {
    route->setResolved(std::move(fwd));
    VLOG(3) << "Resolved route " << route->str();
  }
}


template<typename RibT>
void RouteUpdater::setRoutesWithNhopsForResolution(RibT* rib) {
  for (auto& rt : rib->getAllNodes()) {
    auto route = rt.second.get();
    if (route->isWithNexthops()) {
      if (route->isPublished()) {
        auto newRoute = route->clone(RibT::RouteType::Fields::COPY_ONLY_PREFIX);
        newRoute->update(route->nexthops());
        rib->updateRoute(newRoute);
        route = newRoute.get();
      }
      route->clearFlags();
    }
  }
}

namespace {
template<typename RibT>
bool allRouteFlagsCleared(const RibT* rib) {
  for (const auto& rt : rib->getAllNodes()) {
    const auto route = rt.second.get();
    if (route->isWithNexthops() && !route->flagsCleared()) {
      return false;
    }
  }
  return true;
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
      if (!sync_) {
        // While synching FIB all routes are new and
        // already have their flags not set, so no need
        // to clear flags
        setRoutesWithNhopsForResolution(rib);
      } else {
        DCHECK(allRouteFlagsCleared(rib));
      }
      for (auto& rt : rib->getAllNodes()) {
        if (rt.second->needResolve()) {
          resolve(rt.second.get(), rib, &ribCloned.second);
        }
      }
    }
    if (ribCloned.second.v6.cloned) {
      auto rib = ribCloned.second.v6.rib.get();
      if (!sync_) {
        // While synching FIB all routes are new and
        // already have their flags not set, so no need
        // to clear flags
        setRoutesWithNhopsForResolution(rib);
      } else {
        DCHECK(allRouteFlagsCleared(rib));
      }
      for (auto& rt : rib->getAllNodes()) {
        if (rt.second->needResolve()) {
          resolve(rt.second.get(), rib, &ribCloned.second);
        }
      }
    }
  }
}

std::shared_ptr<RouteTableMap> RouteUpdater::updateDone() {
  // resolve all routes
  resolve();
  if (sync_) {
    return syncUpdateDone();
  }
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
      addRoute(routerId, intf->getID(), addr.first, addr.second);
    }
  }
  for (auto id : routers) {
    addLinkLocalRoutes(id);
  }
}

template<typename RibT>
bool RouteUpdater::dedupRoutes(const RibT* oldRib, RibT* newRib) {
  bool isSame = true;
  if (oldRib == newRib) {
    return isSame;
  }
  const auto& oldRoutes = oldRib->getAllNodes();
  auto& newRoutes = newRib->writableNodes();
  auto oldIter = oldRoutes.begin();
  auto newIter = newRoutes.begin();
  while (oldIter != oldRoutes.end() && newIter != newRoutes.end()) {
    const auto& oldPrefix = oldIter->first;
    const auto& newPrefix = newIter->first;
    if (oldPrefix < newPrefix) {
      isSame = false;
      oldIter++;
      continue;
    }
    if (oldPrefix > newPrefix) {
      isSame = false;
      newIter++;
      continue;
    }
    const auto& oldRt = oldIter->second;
    auto& newRt = newIter->second;
    if (oldRt->isSame(newRt.get())) {
      // both routes are complete same, instead of using the new route,
      // we re-use the old route.
      newIter->second = oldRt;
    } else {
      isSame = false;
      newRt->inheritGeneration(*oldRt);
    }
    oldIter++;
    newIter++;
  }
  if (oldIter != oldRoutes.end() || newIter != newRoutes.end()) {
    isSame = false;
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

std::shared_ptr<RouteTableMap> RouteUpdater::syncUpdateDone() {
  // First, create the RouteTableMap based on clonedRibs_
  RouteTableMap::NodeContainer map;
  for (const auto& ribPair : clonedRibs_) {
    auto id = ribPair.first;
    const auto& rib = ribPair.second;
    CHECK(rib.v4.cloned && rib.v6.cloned);
    if (rib.v4.rib->empty() && rib.v6.rib->empty()) {
      continue;
    }
    auto newRt = make_shared<RouteTable>(ribPair.first);
    newRt->setRib(rib.v4.rib);
    newRt->setRib(rib.v6.rib);
    auto ret = map.emplace(id, std::move(newRt));
    CHECK(ret.second);
  }
  // Then, consolidate the original route tables with the new one
  return deduplicate(&map);
}

}}
