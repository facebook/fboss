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

#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/if/gen-cpp2/FbossCtrl.h"
#include "fboss/agent/rib/NetworkToRouteMap.h"
#include "fboss/agent/types.h"

#include <folly/Synchronized.h>

#include <functional>
#include <memory>
#include <thread>
#include <vector>

namespace facebook::fboss::rib {

class RoutingInformationBase {
 public:
  using FibUpdateFunction = std::function<void(
      RouterID vrf,
      const IPv4NetworkToRouteMap& v4NetworkToRoute,
      const IPv6NetworkToRouteMap& v6NetworkToRoute,
      void* cookie)>;

  struct UpdateStatistics {
    std::size_t v4RoutesAdded{0};
    std::size_t v4RoutesDeleted{0};
    std::size_t v6RoutesAdded{0};
    std::size_t v6RoutesDeleted{0};
    std::chrono::microseconds duration{0};
  };

  /*
   * `update()` first acquires exclusive ownership of the RIB and executes the
   * following sequence of actions:
   * 1. Injects and removes routes in `toAdd` and `toDelete`, respectively.
   * 2. Triggers recursive (IP) resolution.
   * 3. Updates the FIB synchronously.
   *
   * If a UnicastRoute does not specify its admin distance, then we derive its
   * admin distance via its clientID.  This is accomplished by a mapping from
   * client IDs to admin distances provided in configuration. Unfortunately,
   * this mapping is exposed via SwSwitch, which we can't a dependency on here.
   * The adminDistanceFromClientID allows callsites to propogate admin distances
   * per client.
   */
  UpdateStatistics update(
      RouterID routerID,
      ClientID clientID,
      AdminDistance adminDistanceFromClientID,
      const std::vector<UnicastRoute>& toAdd,
      const std::vector<IpPrefix>& toDelete,
      bool resetClientsRoutes,
      folly::StringPiece updateType,
      FibUpdateFunction fibUpdateCallback,
      void* cookie);

  /*
   * VrfAndNetworkToInterfaceRoute is conceptually a mapping from the pair
   * (RouterID, folly::CIDRNetwork) to the pair (Interface(1),
   * folly::IPAddress). An example of an element in this map is: (RouterID(0),
   * 169.254.0.0/16) --> (Interface(1), 169.254.0.1) This specifies that the
   * network 169.254.0.0/16 in VRF 0 can be reached via Interface 1, which has
   * an address of 169.254.0.1. in that subnet. Note that the IP address in the
   * key has its mask applied to it while the IP address value doesn't.
   */
  using RouterIDAndNetworkToInterfaceRoutes = boost::container::flat_map<
      RouterID,
      boost::container::flat_map<
          folly::CIDRNetwork,
          std::pair<InterfaceID, folly::IPAddress>>>;

  void reconfigure(
      const RouterIDAndNetworkToInterfaceRoutes&
          configRouterIDToInterfaceRoutes,
      const std::vector<cfg::StaticRouteWithNextHops>& staticRoutesWithNextHops,
      const std::vector<cfg::StaticRouteNoNextHops>& staticRoutesToNull,
      const std::vector<cfg::StaticRouteNoNextHops>& staticRoutesToCpu,
      FibUpdateFunction fibUpdateCallback,
      void* cookie);

  folly::dynamic toFollyDynamic() const;
  static RoutingInformationBase fromFollyDynamic(const folly::dynamic& ribJson);

  void createVrf(RouterID rid);
  std::vector<RouterID> getVrfList() const;
  std::vector<RouteDetails> getRouteTableDetails(RouterID rid) const;

  bool operator==(const RoutingInformationBase& other) const;
  bool operator!=(const RoutingInformationBase& other) const {
    return !(*this == other);
  }

 private:
  struct RouteTable {
    IPv4NetworkToRouteMap v4NetworkToRoute;
    IPv6NetworkToRouteMap v6NetworkToRoute;

    UpdateStatistics lastUpdateStats_;

    bool operator==(const RouteTable& other) const {
      return v4NetworkToRoute == other.v4NetworkToRoute &&
          v6NetworkToRoute == other.v6NetworkToRoute;
    }
    bool operator!=(const RouteTable& other) const {
      return !(*this == other);
    }
  };

  /*
   * Currently, route updates to separate VRFs are made to be sequential. In the
   * event FBOSS has to operate in a routing architecture with numerous VRFs,
   * we can avoid a slow down by a factor of number of VRFs by parallelizing
   * route updates across VRFs. This can be accomplished simply by associating
   * the mutex implicit in folly::Synchronized with an individual RouteTable.
   */
  using RouterIDToRouteTable = boost::container::flat_map<RouterID, RouteTable>;
  using SynchronizedRouteTables = folly::Synchronized<RouterIDToRouteTable>;

  RouterIDToRouteTable constructRouteTables(
      const SynchronizedRouteTables::WLockedPtr& lockedRouteTables,
      const RouterIDAndNetworkToInterfaceRoutes&
          configRouterIDToInterfaceRoutes) const;

  SynchronizedRouteTables synchronizedRouteTables_;
};

} // namespace facebook::fboss::rib
