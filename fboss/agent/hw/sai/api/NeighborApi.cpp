/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/sai/api/NeighborApi.h"

#include "fboss/agent/Constants.h"

#include <boost/functional/hash.hpp>

#include <functional>

namespace std {
size_t hash<facebook::fboss::SaiNeighborTraits::NeighborEntry>::operator()(
    const facebook::fboss::SaiNeighborTraits::NeighborEntry& n) const {
  size_t seed = 0;
  boost::hash_combine(seed, n.switchId());
  boost::hash_combine(seed, n.routerInterfaceId());
  boost::hash_combine(seed, std::hash<folly::IPAddress>()(n.ip()));
  return seed;
}
} // namespace std

namespace facebook::fboss {

std::string SaiNeighborTraits::NeighborEntry::toString() const {
  return folly::to<std::string>(
      "NeighborEntry(switch:",
      switchId(),
      ", rif: ",
      routerInterfaceId(),
      ", ip: ",
      ip().str(),
      ")");
}

folly::dynamic SaiNeighborTraits::NeighborEntry::toFollyDynamic() const {
  folly::dynamic json = folly::dynamic::object;
  json[kSwitchId] = switchId();
  json[kIntf] = routerInterfaceId();
  json[kIp] = ip().str();
  return json;
}

SaiNeighborTraits::NeighborEntry
SaiNeighborTraits::NeighborEntry::fromFollyDynamic(const folly::dynamic& json) {
  sai_object_id_t switchId = json[kSwitchId].asInt();
  sai_object_id_t intf = json[kIntf].asInt();
  folly::IPAddress ip(json[kIp].asString());
  return NeighborEntry(switchId, intf, ip);
}
} // namespace facebook::fboss
