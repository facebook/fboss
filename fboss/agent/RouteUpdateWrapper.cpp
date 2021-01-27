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

namespace facebook::fboss {

void RouteUpdateWrapper::addRoute(
    RouterID vrf,
    const folly::IPAddress& network,
    uint8_t mask,
    ClientID clientId,
    RouteNextHopEntry nhop) {
  UnicastRoute tempRoute;
  tempRoute.dest_ref()->ip_ref() = network::toBinaryAddress(network);
  tempRoute.dest_ref()->prefixLength_ref() = mask;
  tempRoute.nextHops_ref() = util::fromRouteNextHopSet(nhop.getNextHopSet());
  ribRoutesToAddDel_[std::make_pair(vrf, clientId)].toAdd.emplace_back(
      std::move(tempRoute));
}

void RouteUpdateWrapper::delRoute(
    RouterID vrf,
    const folly::IPAddress& network,
    uint8_t mask,
    ClientID clientId) {
  IpPrefix pfx;
  pfx.ip_ref() = network::toBinaryAddress(network);
  pfx.prefixLength_ref() = mask;
  ribRoutesToAddDel_[std::make_pair(vrf, clientId)].toDel.emplace_back(
      std::move(pfx));
}
} // namespace facebook::fboss
