/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/FibHelpers.h"
#include "fboss/agent/state/RouteTable.h"
#include "fboss/agent/state/RouteTableMap.h"
#include "fboss/agent/state/RouteTableRib.h"
#include "fboss/agent/state/SwitchState.h"

namespace facebook::fboss {

template <typename AddrT>
std::shared_ptr<Route<AddrT>> findRoute(
    bool isStandaloneRib,
    RouterID rid,
    const folly::CIDRNetwork& prefix,
    const std::shared_ptr<SwitchState>& state) {
  if (isStandaloneRib) {
    return nullptr;
  } else {
    auto& routes = state->getRouteTables()
                       ->getRouteTable(rid)
                       ->template getRib<AddrT>()
                       ->routes();
    if constexpr (std::is_same_v<AddrT, folly::IPAddressV6>) {
      return routes->getRouteIf(
          RoutePrefix<folly::IPAddressV6>{prefix.first.asV6(), prefix.second});
    } else {
      return routes->getRouteIf(
          RoutePrefix<folly::IPAddressV4>{prefix.first.asV4(), prefix.second});
    }
  }
  CHECK(false) << " Should never get here, route lookup failed";
  return nullptr;
}

template std::shared_ptr<Route<folly::IPAddressV6>> findRoute(
    bool isStandaloneRib,
    RouterID rid,
    const folly::CIDRNetwork& prefix,
    const std::shared_ptr<SwitchState>& state);

template std::shared_ptr<Route<folly::IPAddressV4>> findRoute(
    bool isStandaloneRib,
    RouterID rid,
    const folly::CIDRNetwork& prefix,
    const std::shared_ptr<SwitchState>& state);
} // namespace facebook::fboss
