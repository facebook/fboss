/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/state/RouteTypes.h"
#include "fboss/agent/AddressUtil.h"

namespace {
constexpr auto kAddress = "address";
constexpr auto kMask = "mask";
constexpr auto kDrop = "Drop";
constexpr auto kToCpu = "ToCPU";
constexpr auto kNexthops = "Nexthops";
constexpr auto kLabel = "label";
} // namespace

namespace facebook::fboss {

std::string forwardActionStr(RouteForwardAction action) {
  switch (action) {
    case RouteForwardAction::DROP:
      return kDrop;
    case RouteForwardAction::TO_CPU:
      return kToCpu;
    case RouteForwardAction::NEXTHOPS:
      return kNexthops;
  }
  CHECK(0);
}

RouteForwardAction str2ForwardAction(const std::string& action) {
  if (action == kDrop) {
    return RouteForwardAction::DROP;
  } else if (action == kToCpu) {
    return RouteForwardAction::TO_CPU;
  } else {
    CHECK(action == kNexthops);
    return RouteForwardAction::NEXTHOPS;
  }
}

//
// RoutePrefix<> Class
//

template <typename AddrT>
bool RoutePrefix<AddrT>::operator<(const RoutePrefix& p2) const {
  if (mask() < p2.mask()) {
    return true;
  } else if (mask() > p2.mask()) {
    return false;
  }
  return network() < p2.network();
}

template <typename AddrT>
bool RoutePrefix<AddrT>::operator>(const RoutePrefix& p2) const {
  if (mask() > p2.mask()) {
    return true;
  } else if (mask() < p2.mask()) {
    return false;
  }
  return network() > p2.network();
}

template <typename AddrT>
RoutePrefix<AddrT> RoutePrefix<AddrT>::fromString(std::string str) {
  std::vector<std::string> vec;

  folly::split('/', str, vec);
  CHECK_EQ(2, vec.size());
  auto prefix = RoutePrefix{AddrT(vec.at(0)), folly::to<uint8_t>(vec.at(1))};
  return prefix;
}

void toAppend(const RoutePrefixV4& prefix, std::string* result) {
  result->append(prefix.str());
}

void toAppend(const RoutePrefixV6& prefix, std::string* result) {
  result->append(prefix.str());
}

void toAppend(const RouteKeyMpls& route, std::string* result) {
  result->append(fmt::format("{}", route.label()));
}

void toAppend(const RouteForwardAction& action, std::string* result) {
  result->append(forwardActionStr(action));
}

std::ostream& operator<<(std::ostream& os, const RouteForwardAction& action) {
  os << forwardActionStr(action);
  return os;
}

template <typename AddrT>
state::RoutePrefix RoutePrefix<AddrT>::toThrift() const {
  return this->data();
}

template <typename AddrT>
RoutePrefix<AddrT> RoutePrefix<AddrT>::fromThrift(
    const state::RoutePrefix& thriftPrefix) {
  return RoutePrefix<AddrT>(thriftPrefix);
}

template struct RoutePrefix<folly::IPAddress>;
template struct RoutePrefix<folly::IPAddressV4>;
template struct RoutePrefix<folly::IPAddressV6>;

} // namespace facebook::fboss
