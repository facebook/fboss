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
#include "fboss/agent/state/ForwardingInformationBase.h"
#include "fboss/agent/state/ForwardingInformationBaseContainer.h"
#include "fboss/agent/state/RouteTable.h"
#include "fboss/agent/state/RouteTableMap.h"
#include "fboss/agent/state/RouteTableRib.h"
#include "fboss/agent/state/SwitchState.h"

namespace facebook::fboss {

namespace {
template <typename AddrT>
std::shared_ptr<Route<AddrT>> findRouteImpl(
    bool isStandaloneRib,
    RouterID rid,
    const folly::CIDRNetwork& prefix,
    const std::shared_ptr<SwitchState>& state,
    bool exactMatch) {
  if (!exactMatch) {
    CHECK_EQ(prefix.second, prefix.first.bitCount())
        << " Longest match must pass in a IPAddress";
  }
  auto findInFib = [&prefix, exactMatch](const auto& fibRoutes) {
    if constexpr (std::is_same_v<AddrT, folly::IPAddressV6>) {
      return exactMatch
          ? fibRoutes->exactMatch({prefix.first.asV6(), prefix.second})
          : fibRoutes->longestMatch(prefix.first.asV6());
    } else {
      return exactMatch
          ? fibRoutes->exactMatch({prefix.first.asV4(), prefix.second})
          : fibRoutes->longestMatch(prefix.first.asV4());
    }
  };
  if (isStandaloneRib) {
    auto& fib = state->getFibs()->getFibContainer(rid)->getFib<AddrT>();
    return findInFib(fib);

  } else {
    auto& rib =
        state->getRouteTables()->getRouteTable(rid)->template getRib<AddrT>();
    return findInFib(rib);
  }
  CHECK(false) << " Should never get here, route lookup failed";
  return nullptr;
}
} // namespace

template <typename AddrT>
std::shared_ptr<Route<AddrT>> findRoute(
    bool isStandaloneRib,
    RouterID rid,
    const folly::CIDRNetwork& prefix,
    const std::shared_ptr<SwitchState>& state) {
  return findRouteImpl<AddrT>(
      isStandaloneRib, rid, prefix, state, true /*exact match*/);
}

template <typename AddrT>
std::shared_ptr<Route<AddrT>> findLongestMatchRoute(
    bool isStandaloneRib,
    RouterID rid,
    const AddrT& addr,
    const std::shared_ptr<SwitchState>& state) {
  return findRouteImpl<AddrT>(
      isStandaloneRib,
      rid,
      {addr, addr.bitCount()},
      state,
      false /*longest match*/);
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

template std::shared_ptr<Route<folly::IPAddressV6>> findLongestMatchRoute(
    bool isStandaloneRib,
    RouterID rid,
    const folly::IPAddressV6& addr,
    const std::shared_ptr<SwitchState>& state);

template std::shared_ptr<Route<folly::IPAddressV4>> findLongestMatchRoute(
    bool isStandaloneRib,
    RouterID rid,
    const folly::IPAddressV4& addr,
    const std::shared_ptr<SwitchState>& state);

} // namespace facebook::fboss
