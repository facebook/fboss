/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/rib/RoutingInformationBase.h"
#include "fboss/agent/rib/RouteNextHopEntry.h"
#include "fboss/agent/rib/RouteUpdater.h"

#include <utility>

#include "fboss/agent/AddressUtil.h"

namespace facebook {
namespace fboss {
namespace rib {

void RoutingInformationBase::update(
    RouterID routerID,
    ClientID clientID,
    AdminDistance adminDistanceFromClientID,
    const std::vector<UnicastRoute>& toAdd,
    const std::vector<IpPrefix>& toDelete,
    bool resetClientsRoutes) {
  auto lockedRouteTables = synchronizedRouteTables_.wlock();

  auto it = lockedRouteTables->find(routerID);
  if (it == lockedRouteTables->end()) {
    // We can be more strict about admitting VRFs by taking the set of valid
    // VRFs on construction, and checking routerID belongs to that set here.
    bool inserted = false;
    std::tie(it, inserted) = lockedRouteTables->emplace(routerID, RouteTable());
    CHECK(inserted);
  }

  RouteUpdater updater(
      &(it->second.v4NetworkToRoute),
      &(it->second.v6NetworkToRoute));

  if (resetClientsRoutes) {
    updater.removeAllRoutesForClient(clientID);
  }

  for (const auto& route : toAdd) {
    auto network = facebook::network::toIPAddress(route.dest.ip);
    auto mask = static_cast<uint8_t>(route.dest.prefixLength);

    updater.addRoute(
        network,
        mask,
        clientID,
        RouteNextHopEntry::from(route, adminDistanceFromClientID));
  }

  for (const auto& prefix : toDelete) {
    auto network = facebook::network::toIPAddress(prefix.ip);
    auto mask = static_cast<uint8_t>(prefix.prefixLength);

    updater.delRoute(network, mask, clientID);
  }

  updater.updateDone();
}

} // namespace rib
} // namespace fboss
} // namespace facebook
