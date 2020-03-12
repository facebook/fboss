/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/sai/api/RouteApi.h"

#include "fboss/agent/Constants.h"

#include <boost/functional/hash.hpp>

#include <functional>

namespace {

constexpr folly::StringPiece kPrefixAddr = "prefixAddr";
constexpr folly::StringPiece kPrefixMask = "prefixMask";

} // namespace
namespace std {

size_t hash<facebook::fboss::SaiRouteTraits::RouteEntry>::operator()(
    const facebook::fboss::SaiRouteTraits::RouteEntry& r) const {
  size_t seed = 0;
  boost::hash_combine(seed, r.switchId());
  boost::hash_combine(seed, r.virtualRouterId());
  boost::hash_combine(seed, std::hash<folly::CIDRNetwork>()(r.destination()));
  return seed;
}
} // namespace std

namespace facebook::fboss {

std::string SaiRouteTraits::RouteEntry::toString() const {
  const auto cidr = destination();
  return folly::to<std::string>(
      "RouteEntry(switch: ",
      switchId(),
      ", vrf: ",
      virtualRouterId(),
      ", prefix: ",
      cidr.first,
      "/",
      cidr.second,
      ")");
}

folly::dynamic SaiRouteTraits::RouteEntry::toFollyDynamic() const {
  folly::dynamic json = folly::dynamic::object;
  json[kSwitchId] = switchId();
  json[kVrf] = virtualRouterId();
  auto cidr = destination();
  json[kPrefixAddr] = folly::to<std::string>(cidr.first);
  json[kPrefixMask] = folly::to<std::string>(cidr.second);
  return json;
}

SaiRouteTraits::RouteEntry SaiRouteTraits::RouteEntry::fromFollyDynamic(
    const folly::dynamic& json) {
  auto switchId = json[kSwitchId].asInt();
  auto vrf = json[kVrf].asInt();
  folly::CIDRNetwork prefix;
  prefix.first = folly::IPAddress(json[kPrefixAddr].asString());
  prefix.second = json[kPrefixMask].asInt();
  return RouteEntry(switchId, vrf, prefix);
}
} // namespace facebook::fboss
