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
#include "fboss/agent/state/SwitchState.h"

namespace {
constexpr auto kRoutes = "routes";
}

namespace facebook { namespace fboss {

FBOSS_INSTANTIATE_NODE_MAP(RouteTableRibNodeMap<folly::IPAddressV4>,
                           RouteTableRibNodeMapTraits<folly::IPAddressV4>);
FBOSS_INSTANTIATE_NODE_MAP(RouteTableRibNodeMap<folly::IPAddressV6>,
                           RouteTableRibNodeMapTraits<folly::IPAddressV6>);

template<typename AddrT>
folly::dynamic RouteTableRib<AddrT>::toFollyDynamic() const {
  folly::dynamic routesJson = folly::dynamic::array;
  for (const auto& route: rib_) {
    routesJson.push_back(route->value()->toFollyDynamic());
  }
  folly::dynamic routes = folly::dynamic::object;
  routes[kRoutes] = std::move(routesJson);
  return routes;
}

template<typename AddrT>
std::shared_ptr<RouteTableRib<AddrT>>
RouteTableRib<AddrT>::fromFollyDynamic(const folly::dynamic& routes) {
  auto rib = std::make_shared<RouteTableRib<AddrT>>();
  auto routesJson = routes[kRoutes];
  for (const auto& routeJson: routesJson) {
    rib->addRoute(Route<AddrT>::fromFollyDynamic(routeJson));
  }
  return rib;
}

template <typename AddrT>
RouteTableRib<AddrT>* RouteTableRib<AddrT>::modify(
    RouterID id,
    std::shared_ptr<SwitchState>* state) {
  if (!isPublished()) {
    return this;
  }

  std::shared_ptr<RouteTable> routeTable =
      (*state)->getRouteTables()->getRouteTable(id);
  RouteTable* clonedRouteTable = routeTable->modify(state);

  auto clonedRib = this->clone();
  auto clonedRibPtr = clonedRib.get();
  clonedRouteTable->setRib(clonedRib);

  return clonedRibPtr;
}

template class RouteTableRib<folly::IPAddressV4>;
template class RouteTableRib<folly::IPAddressV6>;

}} // facebook::fboss
