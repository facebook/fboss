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

#include "fboss/agent/types.h"

#include <folly/Synchronized.h>
#include <functional>
#include <thread>
#include <vector>

#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/if/gen-cpp2/FbossCtrl.h"
#include "fboss/agent/rib/NetworkToRouteMap.h"

namespace facebook {
namespace fboss {
class SwitchState;
} // namespace fboss
} // namespace facebook

namespace facebook {
namespace fboss {
namespace rib {

class RoutingInformationBase {
 public:
  // If a UnicastRoute does not specify its admin distance, then we derive its
  // admin distance via its clientID.  This is accomplished by a mapping from
  // client IDs to admin distances provided in configuration. Unfortunately,
  // this mapping is exposed via SwSwitch, which we can't a dependency on here.
  // The adminDistanceFromClientID allows callsites to propogate admin distances
  // per client.
  void update(
      RouterID routerID,
      ClientID clientID,
      AdminDistance adminDistanceFromClientID,
      const std::vector<UnicastRoute>& toAdd,
      const std::vector<IpPrefix>& toDelete,
      bool resetClientsRoutes = false);

  // VrfAndNetworkToInterfaceRoute is conceptually a mapping from the pair
  // (RouterID, folly::CIDRNetwork) to the pair (Interface(1),
  // folly::IPAddress). An example of an element in this map is: (RouterID(0),
  // 169.254.0.0/16) --> (Interface(1), 169.254.0.1) This specifies that the
  // network 169.254.0.0/16 in VRF 0 can be reached via Interface 1, which has
  // an address of 169.254.0.1. in that subnet. Note that the IP address in the
  // key has its mask applied to it while the IP address value doesn't.
  using RouterIDAndNetworkToInterfaceRoutes = boost::container::flat_map<
      RouterID,
      boost::container::flat_map<
          folly::CIDRNetwork,
          std::pair<InterfaceID, folly::IPAddress>>>;

  void reconfigure(
      const std::shared_ptr<SwitchState>& nextState,
      const RouterIDAndNetworkToInterfaceRoutes&
          configRouterIDToInterfaceRoutes,
      const std::vector<cfg::StaticRouteWithNextHops>& staticRoutesWithNextHops,
      const std::vector<cfg::StaticRouteNoNextHops>& staticRoutesToNull,
      const std::vector<cfg::StaticRouteNoNextHops>& staticRoutesToCpu);

 private:
  struct RouteTable {
    RouteTable() = default;

    IPv4NetworkToRouteMap v4NetworkToRoute;
    IPv6NetworkToRouteMap v6NetworkToRoute;
  };

  // Currently, route updates to separate VRFs are made to be sequential. In the
  // event FBOSS has to operate in a routing architecture with numerous VRFs,
  // we can avoid a slow down by a factor of the the number of VRFs by
  // parallelizing route updates across VRFs. This can be accomplished simply by
  // associating the mutex implicit in folly::Synchronized with an individual
  // RouteTable.
  using RouterIDToRouteTable = boost::container::flat_map<RouterID, RouteTable>;
  using SynchronizedRouteTables = folly::Synchronized<RouterIDToRouteTable>;

  RouterIDToRouteTable constructRouteTables(
      const SynchronizedRouteTables::WLockedPtr& lockedRouteTables,
      const RouterIDAndNetworkToInterfaceRoutes&
          configRouterIDToInterfaceRoutes) const;

  SynchronizedRouteTables synchronizedRouteTables_;
};

} // namespace rib
} // namespace fboss
} // namespace facebook
