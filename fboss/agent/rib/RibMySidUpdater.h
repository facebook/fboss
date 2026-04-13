// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/agent/rib/NetworkToRouteMap.h"
#include "fboss/agent/rib/RouteUpdater.h"
#include "fboss/agent/state/RouteNextHop.h"

#include <memory>
#include <optional>
#include <set>
#include <vector>

namespace facebook::fboss {

class NextHopIDManager;
class MySid;

/**
 * RibMySidUpdater resolves the next hops for MySid entries in the MySidTable.
 *
 * For each MySid entry that has an unresolvedNextHopsId, it:
 *   1. Retrieves the unresolved RouteNextHopSet from NextHopIDManager.
 *   2. Resolves each member next hop against the v4/v6 route tables from all
 *      VRFs (which must already be resolved by RibRouteUpdater). The first
 *      VRF that contains a matching route wins.
 *   3. Allocates or updates the resolvedNextHopsId in NextHopIDManager.
 *   4. Updates mySid.resolvedNextHopsId if the resolved set changed.
 */
class RibMySidUpdater {
 public:
  // Each element is a (v4RouteMap, v6RouteMap) pair for one VRF. The updater
  // tries each VRF in order and uses the first matching route.
  using VrfRouteTables =
      std::vector<std::pair<IPv4NetworkToRouteMap*, IPv6NetworkToRouteMap*>>;

  RibMySidUpdater(
      const VrfRouteTables& routeTables,
      NextHopIDManager* nextHopIDManager,
      MySidTable* mySidTable);

  void resolve();
  void resolve(const std::set<folly::CIDRNetwork>& mySidsToResolve);

 private:
  void resolveOneMySid(std::shared_ptr<MySid>& mySid);
  RouteNextHopSet resolveNextHopSet(
      const RouteNextHopSet& unresolvedNhops) const;
  RouteNextHopSet resolveNhop(const NextHop& nh) const;
  void updateResolvedNextHopSetId(
      std::shared_ptr<MySid>& mySidPtr,
      const RouteNextHopSet& resolvedNhops);

  VrfRouteTables routeTables_;
  NextHopIDManager* nextHopIDManager_{nullptr};
  MySidTable* mySidTable_{nullptr};
};

} // namespace facebook::fboss
