/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "RouteTypes.h"
#include "fboss/agent/AddressUtil.h"

namespace {
constexpr auto kAddress = "address";
constexpr auto kMask = "mask";
constexpr auto kDrop = "Drop";
constexpr auto kToCpu = "ToCPU";
constexpr auto kNexthops = "Nexthops";
} // namespace

namespace facebook::fboss {

std::string forwardActionStr(RouteForwardAction action) {
  switch (action) {
    case DROP:
      return kDrop;
    case TO_CPU:
      return kToCpu;
    case NEXTHOPS:
      return kNexthops;
  }
  CHECK(0);
}

RouteForwardAction str2ForwardAction(const std::string& action) {
  if (action == kDrop) {
    return DROP;
  } else if (action == kToCpu) {
    return TO_CPU;
  } else {
    CHECK(action == kNexthops);
    return NEXTHOPS;
  }
}

//
// RoutePrefix<> Class
//

template <typename AddrT>
bool RoutePrefix<AddrT>::operator<(const RoutePrefix& p2) const {
  if (mask < p2.mask) {
    return true;
  } else if (mask > p2.mask) {
    return false;
  }
  return network < p2.network;
}

template <typename AddrT>
bool RoutePrefix<AddrT>::operator>(const RoutePrefix& p2) const {
  if (mask > p2.mask) {
    return true;
  } else if (mask < p2.mask) {
    return false;
  }
  return network > p2.network;
}

template <typename AddrT>
folly::dynamic RoutePrefix<AddrT>::toFollyDynamic() const {
  folly::dynamic pfx = folly::dynamic::object;
  pfx[kAddress] = network.str();
  pfx[kMask] = mask;
  return pfx;
}

template <typename AddrT>
RoutePrefix<AddrT> RoutePrefix<AddrT>::fromFollyDynamic(
    const folly::dynamic& pfxJson) {
  RoutePrefix pfx;
  pfx.network = AddrT(pfxJson[kAddress].stringPiece());
  pfx.mask = pfxJson[kMask].asInt();
  return pfx;
}

void toAppend(const RoutePrefixV4& prefix, std::string* result) {
  result->append(prefix.str());
}

void toAppend(const RoutePrefixV6& prefix, std::string* result) {
  result->append(prefix.str());
}

template class RoutePrefix<folly::IPAddressV4>;
template class RoutePrefix<folly::IPAddressV6>;

} // namespace facebook::fboss
