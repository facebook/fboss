/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
// Copyright 2004-present Facebook.  All rights reserved.
#include "RouteTable.h"

#include "fboss/agent/state/Route.h"
#include "fboss/agent/state/RouteTableRib.h"
#include "fboss/agent/state/NodeBase-defs.h"
#include "fboss/agent/FbossError.h"
#include "fboss/agent/state/SwitchState.h"

namespace {
constexpr auto kRouterId = "routerId";
constexpr auto kRibV4 = "ribV4";
constexpr auto kRibV6 = "ribV6";
}

namespace facebook { namespace fboss {

RouteTableFields::RouteTableFields(RouterID id)
    : id(id),
      ribV4(std::make_shared<RibTypeV4>()),
      ribV6(std::make_shared<RibTypeV6>()) {
}

folly::dynamic RouteTableFields::toFollyDynamic() const {
  folly::dynamic rtable = folly::dynamic::object;
  rtable[kRouterId] = static_cast<uint32_t>(id);
  rtable[kRibV4] = ribV4->toFollyDynamic();
  rtable[kRibV6] = ribV6->toFollyDynamic();
  return rtable;
}

RouteTable* RouteTable::modify(std::shared_ptr<SwitchState>* state) {
  if (!isPublished()) {
    return this;
    // We must never have a child that is published, but something up the chain
    // is published.
  }
  auto clonedRouteTableMap = (*state)->getRouteTables()->modify(state);

  auto clonedRT = this->clone();
  auto it = clonedRouteTableMap->writableNodes().find(getID());
  it->second = clonedRT;
  return clonedRT.get();
}

RouteTableFields
RouteTableFields::fromFollyDynamic(const folly::dynamic& rtableJson) {
  RouteTableFields rtable(RouterID(rtableJson[kRouterId].asInt()));
  rtable.ribV4 = RibTypeV4::fromFollyDynamic(rtableJson[kRibV4]);
  rtable.ribV6 = RibTypeV6::fromFollyDynamic(rtableJson[kRibV6]);
  return rtable;
}

RouteTable::RouteTable(RouterID id) : NodeBaseT(id) {
}

RouteTable::~RouteTable() {
}

folly::Optional<std::pair<folly::IPAddress, InterfaceID>>
RouteTable::resolveL3Unicast (
    const folly::IPAddress& dstAddr) const {
  const RouteForwardInfo::Nexthops* nhs{nullptr};
  bool routeIsConnected{false};
  if (dstAddr.isV4()) {
    const auto dstAddrV4 = dstAddr.asV4();
    auto rib4 = getRibV4();
    auto route = rib4->longestMatch(dstAddrV4);
    if (!route || !route->isResolved()) {
      return folly::none;
    }
    nhs = &route->getForwardInfo().getNexthops();
    routeIsConnected = route->isConnected();
  } else {
    const auto dstAddrV6 = dstAddr.asV6();
    auto rib6 = getRibV6();
    auto route = rib6->longestMatch(dstAddrV6);
    if (!route || !route->isResolved()) {
      return folly::none;
    }
    nhs = &route->getForwardInfo().getNexthops();
    routeIsConnected = route->isConnected();
  }

  // Find the next hop which could be V4 or V6
  // If there are multiple next hops (ECMP) we need to be consistent in
  // our selection to avoid out of order packets.  Just pick the first resolved
  // route.
  // We now ignore maybeIfID and use the interface associated with the
  // chosen next hop.
  //
  // TODO: handle ECMP hashing
  auto nh = nhs->begin();
  if (nh != nhs->end()) {
    auto target = routeIsConnected ? dstAddr : nh->nexthop;
    return std::make_pair(target, nh->intf);
  }
  return folly::none;
}

bool RouteTable::empty() const {
  return getRibV4()->empty() && getRibV6()->empty();
}

template class NodeBaseT<RouteTable, RouteTableFields>;

}}
