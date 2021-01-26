/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/RouteUpdateWrapper.h"

#include "fboss/agent/AddressUtil.h"
#include "fboss/agent/if/gen-cpp2/ctrl_types.h"

namespace facebook::fboss {

void RouteUpdateWrapper::addRoute(
    RouterID vrf,
    const folly::IPAddress& network,
    uint8_t mask,
    ClientID clientId,
    RouteNextHopEntry nhop) {
  if (rib_) {
    // TODO
    UnicastRoute tempRoute;
    tempRoute.dest_ref()->ip_ref() = network::toBinaryAddress(network);
    tempRoute.dest_ref()->prefixLength_ref() = mask;
    tempRoute.nextHops_ref() = util::fromRouteNextHopSet(nhop.getNextHopSet());
    ribRoutesToAdd_[std::make_pair(vrf, clientId)].emplace_back(
        std::move(tempRoute));
  } else {
    routeUpdater_->addRoute(vrf, network, mask, clientId, nhop);
  }
}

void RouteUpdateWrapper::delRoute(
    RouterID vrf,
    const folly::IPAddress& network,
    uint8_t mask,
    ClientID clientId) {
  if (rib_) {
    IpPrefix pfx;
    pfx.ip_ref() = network::toBinaryAddress(network);
    pfx.prefixLength_ref() = mask;
    ribRoutesToDelete_[std::make_pair(vrf, clientId)].emplace_back(
        std::move(pfx));
  } else {
    routeUpdater_->delRoute(vrf, network, mask, clientId);
  }
}
} // namespace facebook::fboss
