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

#include <boost/container/flat_map.hpp>
#include <folly/IPAddressV4.h>
#include <folly/IPAddressV6.h>

#include "fboss/agent/state/RouteNextHop.h"
#include "fboss/agent/types.h"


namespace facebook { namespace fboss {

class StateDelta;
/*
 * Simple class that maintains a map of route next hops to the
 * number of routes pointing to that next hop. The next hops
 * mapped here are post route resolution, so the next hops
 * are in directly attached subnets.
 * This map is then used in NeighborUpdater to pro actively
 * ARP/NDP for any next hops which don't have ARP/NDP resolved
 * for them.
 */
class NexthopToRouteCount {
 public:
   explicit NexthopToRouteCount() {}
   void stateChanged(const StateDelta& delta);
   // Using int rather than uint to check against bugs where we
   // get -ve reference counts
   using RouterID2NhopRefCounts = boost::container::flat_map<RouterID,
         boost::container::flat_map<NextHop, int64_t>>;

  using const_iterator = RouterID2NhopRefCounts::const_iterator;
  const_iterator begin() const { return rid2nhopRefCounts_.begin(); }
  const_iterator end() const { return rid2nhopRefCounts_.end(); }

 private:
    // Forbidden copy constructor and assignment operator
    NexthopToRouteCount(NexthopToRouteCount const &) = delete;
    NexthopToRouteCount& operator=(NexthopToRouteCount const &) = delete;
    // Process route changes
    template<typename RouteT>
    void processChangedRoute(const RouterID id,
        const std::shared_ptr<RouteT>& oldRoute,
        const std::shared_ptr<RouteT>& newRoute);
    template<typename RouteT>
    void processAddedRoute(const RouterID id,
        const std::shared_ptr<RouteT>& newRoute);
    template<typename RouteT>
    void processRemovedRoute(const RouterID id,
        const std::shared_ptr<RouteT>& newRoute);

    void incNexthopReference(RouterID rid, const NextHop& nhop);
    void decNexthopReference(RouterID rid, const NextHop& nhop);

    RouterID2NhopRefCounts rid2nhopRefCounts_;
};
}}
