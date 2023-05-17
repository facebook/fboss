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
  auto& fib = state->getMultiSwitchFibs()->getNode(rid)->getFib<AddrT>();
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
  std::tie(v4Count, v6Count) = state->getMultiSwitchFibs()->getRouteCount();
  return std::make_pair(v4Count, v6Count);
}

template <typename NeighborEntryT>
bool isNoHostRoute(const std::shared_ptr<NeighborEntryT>& entry) {
  /*
   * classID could be associated with a Host entry. However, there is no
   * Neighbor entry for Link Local addresses, thus don't assign classID for
   * Link Local addresses.
   * SAI implementations typically fail creation of SAI Neighbor Entry with
   * both classID and NoHostRoute (set for IPv6 link local only) are set.
   */
  if constexpr (std::is_same_v<NeighborEntryT, NdpEntry>) {
    if (entry->getIP().isLinkLocal()) {
      XLOG(DBG2) << "No classID for IPv6 linkLocal: " << entry->str();
      return true;
    }
  }
  return false;
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

template bool isNoHostRoute(const std::shared_ptr<MacEntry>& entry);
template bool isNoHostRoute(const std::shared_ptr<NdpEntry>& entry);
template bool isNoHostRoute(const std::shared_ptr<ArpEntry>& entry);

} // namespace facebook::fboss
