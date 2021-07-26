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

#include "fboss/agent/state/SwitchState.h"

namespace facebook::fboss {

namespace {

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
    RouterID rid,
    const folly::CIDRNetwork& prefix,
    const std::shared_ptr<SwitchState>& state,
    bool exactMatch) {
  if (!exactMatch) {
    CHECK_EQ(prefix.second, prefix.first.bitCount())
        << " Longest match must pass in a IPAddress";
  }
  CHECK(exactMatch)
      << "Switch state api should only be called for exact match lookups";
  auto& fib = state->getFibs()->getFibContainer(rid)->getFib<AddrT>();
  return findInFib(prefix, fib);
}
} // namespace

template <typename AddrT>
std::shared_ptr<Route<AddrT>> findRoute(
    RouterID rid,
    const folly::CIDRNetwork& prefix,
    const std::shared_ptr<SwitchState>& state) {
  return findRouteInSwitchState<AddrT>(rid, prefix, state, true);
}

template <typename AddrT>
std::shared_ptr<Route<AddrT>> findLongestMatchRoute(
    const RoutingInformationBase* rib,
    RouterID rid,
    const AddrT& addr,
    const std::shared_ptr<SwitchState>& state) {
  return rib->longestMatch(addr, rid);
}

std::pair<uint64_t, uint64_t> getRouteCount(
    const std::shared_ptr<SwitchState>& state) {
  uint64_t v6Count{0}, v4Count{0};
  std::tie(v4Count, v6Count) = state->getFibs()->getRouteCount();
  return std::make_pair(v4Count, v6Count);
}

// Explicit instantiations
template std::shared_ptr<Route<folly::IPAddressV6>> findRoute(
    RouterID rid,
    const folly::CIDRNetwork& prefix,
    const std::shared_ptr<SwitchState>& state);

template std::shared_ptr<Route<folly::IPAddressV4>> findRoute(
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
