/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#pragma once

#include <folly/IPAddress.h>
#include "fboss/agent/state/RouteNextHopEntry.h"
#include "fboss/agent/state/RouteNextHopsMulti.h"
#include "fboss/agent/state/RouteTableMap.h"
#include "fboss/agent/state/RouteTypes.h"
#include "fboss/agent/types.h"

#include <boost/container/flat_map.hpp>
#include <boost/container/flat_set.hpp>

namespace facebook::fboss {

namespace cfg {
class SwitchConfig;
}

class InterfaceMap;
template <typename Addr>
class RouteTableRib;

/**
 * Expected behavior of RouteUpdater::resolve():
 *
 * RouteUpdater::resolve() resolves the route table forwarding information
 * based on the RIB, by doing recursively route table lookup. At the end of
 * the process, every route will be either unresolved or resolved with a
 * ECMP group.
 *
 * There are clear expectation on resolving FIB for a route, when all
 * nexthops are resolved to actual IPs. However, if is not clearly
 * defined and documented expectation if ECMP group has mix of action(s)
 * (i.e. DROP, TO_CPU) and IP nexthops.
 *
 * The following is the current implentation of resolve():
 * 1. No weighted ECMP. Each entry in the ECMP group is unique and has equal
 *    weight.
 * 2. A ECMP group could have either DROP, TO_CPU, or a set of IP nexthops.
 * 3. If DROP and other types (i.e. TO_CPU and IP nexthops) are part of the
 *    results of route resolve process. The finally FIB will be DROP.
 * 4. If TO_CPU and IP nexthops are part of the results of resolving process,
 *    only IP nexthops will be in the final ECMP group.
 * 5. If and only if TO_CPU is the only nexthop (directly or indirectly) of
 *    a route, TO_CPU action will be only path in the resolved ECMP group.
 */
class RouteUpdater {
 public:
  /**
   * Constructor
   *
   * @param orig The existing routing tables
   * All route changes are on the top of the existing routing tables.
   */
  explicit RouteUpdater(const std::shared_ptr<RouteTableMap>& orig);

  typedef RoutePrefixV4 PrefixV4;
  typedef RoutePrefixV6 PrefixV6;

  void addRoute(
      RouterID id,
      const folly::IPAddress& network,
      uint8_t mask,
      ClientID clientId,
      RouteNextHopEntry entry);

  void delRoute(
      RouterID id,
      const folly::IPAddress& network,
      uint8_t mask,
      ClientID clientId);

  void removeAllRoutesForClient(RouterID rid, ClientID clientId);

  std::shared_ptr<RouteTableMap> updateDone();

  void addInterfaceAndLinkLocalRoutes(
      const std::shared_ptr<InterfaceMap>& intfs);

  void addLinkLocalRoutes(RouterID id);
  void delLinkLocalRoutes(RouterID id);

 private:
  template <typename StaticRouteType>
  // Forbidden copy constructor and assignment operator
  RouteUpdater(RouteUpdater const&) = delete;
  RouteUpdater& operator=(RouteUpdater const&) = delete;

  typedef RouteTableRib<folly::IPAddressV4> RouteTableRibV4;
  typedef RouteTableRib<folly::IPAddressV6> RouteTableRibV6;

  struct ClonedRib {
    struct RibV4 {
      std::shared_ptr<RouteTableRibV4> rib;
      bool cloned{false};
    } v4;
    struct RibV6 {
      std::shared_ptr<RouteTableRibV6> rib;
      bool cloned{false};
    } v6;
  };
  boost::container::flat_map<RouterID, ClonedRib> clonedRibs_;
  const std::shared_ptr<RouteTableMap>& orig_;

  // Helper functions to get/allocate the cloned RIB
  ClonedRib* createNewRib(RouterID id);
  ClonedRib::RibV4* getRibV4(RouterID id, bool createIfNotExist = true);
  ClonedRib::RibV6* getRibV6(RouterID id, bool createIfNotExist = true);
  template <typename RibT>
  auto makeClone(RibT* rib) -> decltype(rib->rib.get());

  template <typename PrefixT, typename RibT>
  void addRouteImpl(
      const PrefixT& prefix,
      RibT* rib,
      ClientID clientId,
      RouteNextHopEntry entry);
  template <typename PrefixT, typename RibT>
  void delRouteImpl(const PrefixT& prefix, RibT* ribCloned, ClientID clientId);
  template <typename AddrT, typename RibT>
  void removeAllRoutesForClientImpl(RibT* ribCloned, ClientID clientId);

  // resolve all routes that are not resolved yet
  void resolve();
  template <typename RouteT>
  void resolveOne(RouteT* route, ClonedRib* clonedRib);
  template <typename RtRibT, typename AddrT>
  void getFwdInfoFromNhop(
      RtRibT* nRib,
      ClonedRib* ribCloned,
      const AddrT& nh,
      const std::optional<LabelForwardingAction>& labelAction,
      bool* hasToCpu,
      bool* hasDrop,
      RouteNextHopSet& fwd);

  // Functions to deduplicate routing tables during sync mode
  template <typename RibT>
  bool dedupRoutes(const RibT* origRib, RibT* newRib);
  std::shared_ptr<RouteTableMap> deduplicate(RouteTableMap::NodeContainer* map);
};

} // namespace facebook::fboss
