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
#pragma once

#include "fboss/agent/types.h"
#include <folly/IPAddress.h>
#include "fboss/agent/state/RouteForwardInfo.h"
#include "fboss/agent/state/RouteTypes.h"
#include "fboss/agent/state/RouteTableMap.h"

#include <boost/container/flat_map.hpp>
#include <boost/container/flat_set.hpp>

namespace facebook { namespace fboss {

namespace cfg {
class SwitchConfig;
}

class InterfaceMap;
template<typename Addr> class RouteTableRib;

class RouteUpdater {
 public:
  /**
   * Constructor
   *
   * @param orig The existing routing tables
   * @param sync In sync mode or not.
   *             In non-sync mode, all route changes are on the top of the
   *             existing routing tables.
   *             If it is in sync mode, none of the routes from the existing
   *             routing tables will be automatically added into the new
   *             routing table. A special handling is performed in sync mode
   *             that if an existing route is re-added again with the same
   *             info and is also resolved with same forwarding info, the
   *             shared ptr of the existing route will be re-used in the new
   *             routing table. This helps the delta function later to figure
   *             out what routes are really changed.
   */
  explicit RouteUpdater(const std::shared_ptr<RouteTableMap>& orig,
                        bool sync = false);

  typedef RoutePrefixV4 PrefixV4;
  typedef RoutePrefixV6 PrefixV6;

  // method to add or update an interface route
  void addRoute(RouterID id, InterfaceID intf,
                const folly::IPAddress& intfAddr, uint8_t len);
  /**
   * method to add or update a route with a special action
   *
   * @action Can be either RouteForwardAction::DROP
   *         or RouteForwardAction::TO_CPU. For route with one or multiple
   *         nexthops, use addRoute() with nexthop parameter.
   */
  void addRoute(RouterID id, const folly::IPAddress& network, uint8_t mask,
                RouteForwardAction action);
  // methods to add or update a route with multiple nexthops
  void addRoute(RouterID id, const folly::IPAddress& network, uint8_t mask,
                const RouteNextHops& nhs);
  void addRoute(RouterID id, const folly::IPAddress& network, uint8_t mask,
                RouteNextHops&& nhs);
  // methods to delete a route
  void delRoute(RouterID id, const folly::IPAddress& network, uint8_t mask);

  std::shared_ptr<RouteTableMap> updateDone();

  // Add all interface routes (directly connected routes) and link local routes
  void addInterfaceAndLinkLocalRoutes(
      const std::shared_ptr<InterfaceMap>& intfs);

  // Helper functions to add or delete link local routes
  void addLinkLocalRoutes(RouterID id);
  void delLinkLocalRoutes(RouterID id);

  void updateStaticRoutes(const cfg::SwitchConfig& curCfg,
      const cfg::SwitchConfig& prevCfg);

 private:
  template<typename StaticRouteType>
  void staticRouteDelHelper(const std::vector<StaticRouteType>& oldRoutes,
      const boost::container::flat_map<RouterID,
      boost::container::flat_set<folly::CIDRNetwork>>& newRoutes);
  // Forbidden copy constructor and assignment operator
  RouteUpdater(RouteUpdater const &) = delete;
  RouteUpdater& operator=(RouteUpdater const &) = delete;

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
  bool sync_{false};

  // Helper functions to get/allocate the cloned RIB
  ClonedRib* createNewRib(RouterID id);
  ClonedRib::RibV4* getRibV4(RouterID id, bool createIfNotExist = true);
  ClonedRib::RibV6* getRibV6(RouterID id, bool createIfNotExist = true);
  template<typename RibT>
  auto makeClone(RibT* rib) -> decltype(rib->rib.get());

  template<typename RibT>
  void setRoutesWithNhopsForResolution(RibT* rib);
  // Helper functions to add or delete a route
  template<typename PrefixT, typename RibT, typename... Args>
  void addRoute(const PrefixT& prefix, RibT *rib, Args&&... args);
  template<typename PrefixT, typename RibT>
  void delRoute(const PrefixT& prefix, RibT *rib);

  // resolve all routes that are not resolved yet
  void resolve();
  template<typename RouteT, typename RtRibT>
  void resolve(RouteT* rt, RtRibT* rib, ClonedRib* clonedRib);
  template<typename RtRibT, typename AddrT>
  void getFwdInfoFromNhop(RtRibT* nRib, ClonedRib* ribCloned,
      const AddrT& nh, bool* hasToCpuNhops, bool* hasDropNhops,
      RouteForwardNexthops* fwd);
  // Functions to deduplicate routing tables during sync mode
  template<typename RibT>
  bool dedupRoutes(const RibT* origRib, RibT* newRib);
  std::shared_ptr<RouteTableMap> deduplicate(RouteTableMap::NodeContainer* map);
  std::shared_ptr<RouteTableMap> syncUpdateDone();
};

}}
