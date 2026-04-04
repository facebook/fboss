// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/rib/RibMySidUpdater.h"

#include "fboss/agent/rib/NextHopIDManager.h"
#include "fboss/agent/state/MySid.h"
#include "fboss/agent/state/Route.h"

namespace facebook::fboss {

RibMySidUpdater::RibMySidUpdater(
    IPv4NetworkToRouteMap* v4Routes,
    IPv6NetworkToRouteMap* v6Routes,
    NextHopIDManager* nextHopIDManager,
    MySidTable* mySidTable)
    : v4Routes_{v4Routes},
      v6Routes_{v6Routes},
      nextHopIDManager_{nextHopIDManager},
      mySidTable_{mySidTable} {}

void RibMySidUpdater::resolve() {
  for (auto& [prefix, mySid] : *mySidTable_) {
    resolveOneMySid(mySid);
  }
}

void RibMySidUpdater::resolve(
    const std::set<folly::CIDRNetwork>& mySidsToResolve) {
  for (const auto& cidr : mySidsToResolve) {
    const folly::CIDRNetworkV6 cidrV6{cidr.first.asV6(), cidr.second};
    auto it = mySidTable_->find(cidrV6);
    if (it == mySidTable_->end()) {
      continue;
    }
    resolveOneMySid(it->second);
  }
}

void RibMySidUpdater::resolveOneMySid(std::shared_ptr<MySid>& mySid) {
  auto unresolvedId = mySid->getUnresolveNextHopsId();
  if (!unresolvedId.has_value()) {
    return;
  }
  updateResolvedNextHopSetId(
      mySid, resolveNextHopSet(nextHopIDManager_->getNextHops(*unresolvedId)));
}

RouteNextHopSet RibMySidUpdater::resolveNextHopSet(
    const RouteNextHopSet& unresolvedNhops) const {
  RouteNextHopSet resolved;
  for (const auto& nh : unresolvedNhops) {
    auto nhResolved = resolveNhop(nh);
    resolved.insert(nhResolved.begin(), nhResolved.end());
  }
  return resolved;
}

RouteNextHopSet RibMySidUpdater::resolveNhop(const NextHop& nh) const {
  if (nh.intfID().has_value()) {
    return {nh};
  }

  const auto& addr = nh.addr();
  RouteNextHopSet resolved;

  auto collectResolved = [&resolved](auto* routeMap, const auto& nhAddr) {
    if (!routeMap) {
      return;
    }
    auto it = routeMap->longestMatch(nhAddr, nhAddr.bitCount());
    if (it == routeMap->end()) {
      return;
    }
    const auto& route = it->value();
    if (!route || !route->isResolved()) {
      return;
    }
    const auto& fwdNhops = route->getForwardInfo().normalizedNextHops();
    resolved.insert(fwdNhops.begin(), fwdNhops.end());
  };

  if (addr.isV4()) {
    collectResolved(v4Routes_, addr.asV4());
  } else {
    collectResolved(v6Routes_, addr.asV6());
  }
  return resolved;
}

void RibMySidUpdater::updateResolvedNextHopSetId(
    std::shared_ptr<MySid>& mySidPtr,
    const RouteNextHopSet& resolvedNhops) {
  const auto oldId = mySidPtr->getResolvedNextHopsId();

  std::optional<NextHopSetID> newId;
  if (!resolvedNhops.empty()) {
    if (oldId.has_value()) {
      newId = nextHopIDManager_->updateRouteNextHopSetID(*oldId, resolvedNhops)
                  .allocation.nextHopIdSetIter->second.id;
    } else {
      newId = nextHopIDManager_->getOrAllocRouteNextHopSetID(resolvedNhops)
                  .nextHopIdSetIter->second.id;
    }
  } else if (oldId.has_value()) {
    nextHopIDManager_->decrOrDeallocRouteNextHopSetID(*oldId);
  }

  if (newId == oldId) {
    return;
  }

  mySidPtr = mySidPtr->clone();
  mySidPtr->setResolvedNextHopsId(newId);
}

} // namespace facebook::fboss
