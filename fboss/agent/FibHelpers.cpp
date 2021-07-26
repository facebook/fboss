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

#include "fboss/agent/rib/RoutingInformationBase.h"
#include "fboss/agent/state/ForwardingInformationBase.h"
#include "fboss/agent/state/ForwardingInformationBaseContainer.h"
#include "fboss/agent/state/RouteTable.h"

#include "fboss/agent/state/RouteTableRib.h"
#include "fboss/agent/state/SwitchState.h"

namespace facebook::fboss {

namespace {

template <typename AddrT>
std::shared_ptr<Route<AddrT>> findInFib(
    const folly::CIDRNetwork& prefix,
    const std::shared_ptr<RouteTableRib<AddrT>>& fibRoutes,
    bool exactMatch) {
  if constexpr (std::is_same_v<AddrT, folly::IPAddressV6>) {
    return exactMatch
        ? fibRoutes->exactMatch({prefix.first.asV6(), prefix.second})
        : fibRoutes->longestMatch(prefix.first.asV6());
  } else {
    return exactMatch
        ? fibRoutes->exactMatch({prefix.first.asV4(), prefix.second})
        : fibRoutes->longestMatch(prefix.first.asV4());
  }
}

template <typename AddrT>
std::shared_ptr<Route<AddrT>> findInFib(
    const folly::CIDRNetwork& prefix,
    const std::shared_ptr<ForwardingInformationBase<AddrT>>& fibRoutes) {
  if constexpr (std::is_same_v<AddrT, folly::IPAddressV6>) {
    return fibRoutes->exactMatch({prefix.first.asV6(), prefix.second});
  } else {
    return fibRoutes->exactMatch({prefix.first.asV4(), prefix.second});
  }
}

template <typename AddrT>
std::shared_ptr<Route<AddrT>> findRouteInSwitchState(
    bool isStandaloneRib,
    RouterID rid,
    const folly::CIDRNetwork& prefix,
    const std::shared_ptr<SwitchState>& state,
    bool exactMatch) {
  if (!exactMatch) {
    CHECK_EQ(prefix.second, prefix.first.bitCount())
        << " Longest match must pass in a IPAddress";
  }
  if (isStandaloneRib) {
    CHECK(exactMatch)
        << "Switch state api should only be called for exact match lookups";
    auto& fib = state->getFibs()->getFibContainer(rid)->getFib<AddrT>();
    return findInFib(prefix, fib);
  } else {
    CHECK(false) << " Legacy RIB no longer supported";
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
  return findRouteInSwitchState<AddrT>(
      isStandaloneRib, rid, prefix, state, true);
}

template <typename AddrT>
std::shared_ptr<Route<AddrT>> findLongestMatchRoute(
    const RoutingInformationBase* rib,
    RouterID rid,
    const AddrT& addr,
    const std::shared_ptr<SwitchState>& state) {
  if (rib) {
    return rib->longestMatch(addr, rid);
  }
  return findRouteInSwitchState<AddrT>(
      rib ? true : false,
      rid,
      {addr, addr.bitCount()},
      state,
      false /*longest match*/);
}

std::pair<uint64_t, uint64_t> getRouteCount(
    bool isStandaloneRib,
    const std::shared_ptr<SwitchState>& state) {
  uint64_t v6Count{0}, v4Count{0};
  if (isStandaloneRib) {
    std::tie(v4Count, v6Count) = state->getFibs()->getRouteCount();
  } else {
    CHECK(false) << " Legacy RIB no longer supported";
  }
  return std::make_pair(v4Count, v6Count);
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

template std::shared_ptr<Route<folly::IPAddressV4>> findLongestMatchRoute(
    const RoutingInformationBase* rib,
    RouterID rid,
    const folly::IPAddressV4& addr,
    const std::shared_ptr<SwitchState>& state);

template std::shared_ptr<Route<folly::IPAddressV6>> findLongestMatchRoute(
    const RoutingInformationBase* rib,
    RouterID rid,
    const folly::IPAddressV6& addr,
    const std::shared_ptr<SwitchState>& state);

} // namespace facebook::fboss
