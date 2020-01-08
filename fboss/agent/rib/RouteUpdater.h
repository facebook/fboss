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

#include "fboss/agent/rib/NetworkToRouteMap.h"
#include "fboss/agent/rib/Route.h"
#include "fboss/agent/rib/RouteNextHopEntry.h"
#include "fboss/agent/rib/RouteNextHopsMulti.h"
#include "fboss/agent/rib/RouteTypes.h"

#include <folly/IPAddress.h>

namespace facebook::fboss::rib {

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
  RouteUpdater(
      IPv4NetworkToRouteMap* v4Routes,
      IPv6NetworkToRouteMap* v6Routes);

  void addRoute(
      const folly::IPAddress& network,
      uint8_t mask,
      ClientID clientID,
      RouteNextHopEntry entry);
  void addLinkLocalRoutes();
  void addInterfaceRoute(
      const folly::IPAddress& network,
      uint8_t mask,
      const folly::IPAddress& address,
      InterfaceID interface);

  // TODO(samank): make del vs remove consistent
  void
  delRoute(const folly::IPAddress& network, uint8_t mask, ClientID clientID);
  void delLinkLocalRoutes();
  void removeAllRoutesForClient(ClientID clientID);

  void updateDone();

 private:
  IPv4NetworkToRouteMap* v4Routes_{nullptr};
  IPv6NetworkToRouteMap* v6Routes_{nullptr};

  // TODO(samank): rename in original file
  template <typename AddressT>
  using Prefix = RoutePrefix<AddressT>;

  // TODO(samank): make these static
  template <typename AddressT>
  void addRouteImpl(
      const Prefix<AddressT>& prefix,
      NetworkToRouteMap<AddressT>* routes,
      ClientID clientID,
      RouteNextHopEntry entry);
  template <typename AddressT>
  void delRouteImpl(
      const Prefix<AddressT>& prefix,
      NetworkToRouteMap<AddressT>* routes,
      ClientID clientID);
  template <typename AddressT>
  void removeAllRoutesFromClientImpl(
      NetworkToRouteMap<AddressT>* routes,
      ClientID clientID);
  template <typename AddressT>
  void updateDoneImpl(NetworkToRouteMap<AddressT>* routes);

  template <typename AddressT>
  void resolve(NetworkToRouteMap<AddressT>* routes);
  template <typename AddressT>
  void resolveOne(Route<AddressT>* route);

  template <typename AddressT>
  void getFwdInfoFromNhop(
      NetworkToRouteMap<AddressT>* routes,
      const AddressT& nh,
      const std::optional<LabelForwardingAction>& labelAction,
      bool* hasToCpu,
      bool* hasDrop,
      RouteNextHopSet& fwd);
};

} // namespace facebook::fboss::rib
