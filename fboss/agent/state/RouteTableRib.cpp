/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "RouteTableRib.h"

#include "fboss/agent/state/NodeMap-defs.h"
#include "fboss/agent/state/Route.h"

namespace facebook { namespace fboss {

template<typename AddrT>
RouteTableRib<AddrT>::RouteTableRib() {
}

template<typename AddrT>
RouteTableRib<AddrT>::~RouteTableRib() {
}

template<typename AddrT>
std::shared_ptr<Route<AddrT>> RouteTableRib<AddrT>::longestMatch(
    const AddrT& nexthop) const {
  std::shared_ptr<Route<AddrT>> bestMatch;
  int bestMatchMask = -1;
  for (const auto& rt : Base::getAllNodes()) {
    const auto& prefix = rt.first;
    if (prefix.mask > bestMatchMask
        && nexthop.inSubnet(prefix.network, prefix.mask)) {
      bestMatch = rt.second;
      bestMatchMask = prefix.mask;
    }
  }
  return bestMatch;
}

FBOSS_INSTANTIATE_NODE_MAP(RouteTableRib<folly::IPAddressV4>,
                           RouteTableRibTraits<folly::IPAddressV4>);
FBOSS_INSTANTIATE_NODE_MAP(RouteTableRib<folly::IPAddressV6>,
                           RouteTableRibTraits<folly::IPAddressV6>);
template class RouteTableRib<folly::IPAddressV4>;
template class RouteTableRib<folly::IPAddressV6>;

}} // facebook::fboss
