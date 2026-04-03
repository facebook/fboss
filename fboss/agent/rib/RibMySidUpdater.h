// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/agent/rib/NetworkToRouteMap.h"
#include "fboss/agent/rib/RouteUpdater.h"
#include "fboss/agent/state/RouteNextHop.h"

#include <memory>
#include <optional>

namespace facebook::fboss {

class NextHopIDManager;
class MySid;

/**
 * RibMySidUpdater resolves the next hops for MySid entries in the MySidTable.
 *
 * For each MySid entry that has an unresolvedNextHopsId, it:
 *   1. Retrieves the unresolved RouteNextHopSet from NextHopIDManager.
 *   2. Resolves each member next hop against the v4/v6 route tables
 *      (which must already be resolved by RibRouteUpdater).
 *   3. Allocates or updates the resolvedNextHopsId in NextHopIDManager.
 *   4. Updates mySid.resolvedNextHopsId if the resolved set changed.
 */
class RibMySidUpdater {
 public:
  RibMySidUpdater(
      IPv4NetworkToRouteMap* v4Routes,
      IPv6NetworkToRouteMap* v6Routes,
      NextHopIDManager* nextHopIDManager,
      MySidTable* mySidTable);

  void resolve();

 private:
  RouteNextHopSet resolveNextHopSet(
      const RouteNextHopSet& unresolvedNhops) const;
  RouteNextHopSet resolvedForNhop(const NextHop& nh) const;
  void updateResolvedNextHopSetId(
      std::shared_ptr<MySid>& mySidPtr,
      const RouteNextHopSet& resolvedNhops);

  IPv4NetworkToRouteMap* v4Routes_{nullptr};
  IPv6NetworkToRouteMap* v6Routes_{nullptr};
  NextHopIDManager* nextHopIDManager_{nullptr};
  MySidTable* mySidTable_{nullptr};
};

} // namespace facebook::fboss
