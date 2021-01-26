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

namespace facebook::fboss {

void RouteUpdateWrapper::addRoute(
    RouterID vrf,
    const folly::IPAddress& network,
    uint8_t mask,
    ClientID clientId,
    RouteNextHopEntry nhop) {
  if (rib_) {
    // TODO

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
    // TODO

  } else {
    routeUpdater_->delRoute(vrf, network, mask, clientId);
  }
}
} // namespace facebook::fboss
