// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/rib/RibMySidUpdater.h"

#include "fboss/agent/rib/NextHopIDManager.h"
#include "fboss/agent/rib/RoutingInformationBase.h"
#include "fboss/agent/state/MySid.h"
#include "fboss/agent/state/Route.h"

namespace facebook::fboss {

RibMySidUpdater::RibMySidUpdater(
    const VrfRouteTables& routeTables,
    NextHopIDManager* nextHopIDManager,
    MySidTable* mySidTable,
    uint32_t ecmpWidth)
    : routeTables_{routeTables},
      nextHopIDManager_{nextHopIDManager},
      mySidTable_{mySidTable},
      ecmpWidth_{ecmpWidth} {}

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
  if (unresolvedId.has_value()) {
    updateResolvedNextHopSetId(
        mySid,
        resolveNextHopSet(nextHopIDManager_->getNextHops(*unresolvedId)),
        NextHopRole::PRIMARY);
  }
  auto backupUnresolvedId = mySid->getBackupUnresolveNextHopsId();
  if (backupUnresolvedId.has_value()) {
    updateResolvedNextHopSetId(
        mySid,
        resolveNextHopSet(nextHopIDManager_->getNextHops(*backupUnresolvedId)),
        NextHopRole::BACKUP);
  }
  mySid->publish();
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

  auto collectResolved = [this, &resolved, &nh](
                             auto* routeMap, const auto& nhAddr) {
    if (!routeMap) {
      return false;
    }
    auto it = routeMap->longestMatch(nhAddr, nhAddr.bitCount());
    if (it == routeMap->end()) {
      return false;
    }
    const auto& route = it->value();
    if (!route || !route->isResolved()) {
      return false;
    }

    const auto fwdNhops = getNormalizedNextHopsFromRib(
        nextHopIDManager_, route->getForwardInfo(), ecmpWidth_);
    if (route->isConnected()) {
      resolved.insert(ResolvedNextHop(
          nh.addr(),
          fwdNhops.begin()->intf(),
          fwdNhops.begin()->weight(),
          nh.labelForwardingAction(),
          nh.disableTTLDecrement(),
          nh.topologyInfo(),
          nh.adjustedWeight(),
          nh.srv6SegmentList(),
          nh.tunnelType(),
          nh.tunnelId(),
          nh.cost(),
          nh.role()));
    } else {
      std::vector<ResolvedNextHop> nhops;
      nhops.reserve(fwdNhops.size());
      for (const auto& fwdNh : fwdNhops) {
        nhops.emplace_back(
            fwdNh.addr(),
            fwdNh.intf(),
            fwdNh.weight(),
            nh.labelForwardingAction(),
            nh.disableTTLDecrement(),
            nh.topologyInfo(),
            nh.adjustedWeight(),
            nh.srv6SegmentList(),
            nh.tunnelType(),
            nh.tunnelId(),
            nh.cost(),
            nh.role());
      }
      resolved.insert(nhops.begin(), nhops.end());
    }
    return true;
  };

  for (const auto& [v4Routes, v6Routes] : routeTables_) {
    bool found = false;
    if (addr.isV4()) {
      found = collectResolved(v4Routes, addr.asV4());
    } else {
      found = collectResolved(v6Routes, addr.asV6());
    }
    if (found) {
      break;
    }
  }
  return resolved;
}

void RibMySidUpdater::updateResolvedNextHopSetId(
    std::shared_ptr<MySid>& mySidPtr,
    const RouteNextHopSet& resolvedNhops,
    NextHopRole role) {
  const auto oldId = role == NextHopRole::BACKUP
      ? mySidPtr->getBackupResolvedNextHopsId()
      : mySidPtr->getResolvedNextHopsId();

  std::optional<NextHopSetID> newId;
  if (!resolvedNhops.empty()) {
    newId = nextHopIDManager_->getOrAllocRouteNextHopSetID(resolvedNhops)
                .nextHopIdSetIter->second.id;
  }
  if (oldId.has_value()) {
    nextHopIDManager_->decrOrDeallocRouteNextHopSetID(*oldId);
  }

  if (newId == oldId) {
    return;
  }

  mySidPtr = mySidPtr->clone();
  if (role == NextHopRole::BACKUP) {
    mySidPtr->setBackupResolvedNextHopsId(newId);
  } else {
    mySidPtr->setResolvedNextHopsId(newId);
  }
}

} // namespace facebook::fboss
