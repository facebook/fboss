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

namespace facebook::fboss {

template <typename AddrT>
void RouteTableRibNodeMap<AddrT>::addRoute(
    const std::shared_ptr<Route<AddrT>>& rt) {
  Base::addNode(rt);
}

template <typename AddrT>
void RouteTableRibNodeMap<AddrT>::updateRoute(
    const std::shared_ptr<Route<AddrT>>& rt) {
  Base::updateNode(rt);
}

template <typename AddrT>
void RouteTableRibNodeMap<AddrT>::removeRoute(
    const std::shared_ptr<Route<AddrT>>& rt) {
  Base::removeNode(rt);
}

FBOSS_INSTANTIATE_NODE_MAP(
    RouteTableRibNodeMap<folly::IPAddressV4>,
    RouteTableRibNodeMapTraits<folly::IPAddressV4>);
FBOSS_INSTANTIATE_NODE_MAP(
    RouteTableRibNodeMap<folly::IPAddressV6>,
    RouteTableRibNodeMapTraits<folly::IPAddressV6>);

template <typename AddrT>
folly::dynamic RouteTableRib<AddrT>::toFollyDynamic() const {
  folly::dynamic routesJson = folly::dynamic::array;
  for (const auto& route : *nodeMap_) {
    routesJson.push_back(route->toFollyDynamic());
  }
  folly::dynamic routes = folly::dynamic::object;
  routes[kRoutes] = std::move(routesJson);
  return routes;
}

template <typename AddrT>
std::shared_ptr<RouteTableRib<AddrT>> RouteTableRib<AddrT>::fromFollyDynamic(
    const folly::dynamic& routes) {
  auto rib = std::make_shared<RouteTableRib<AddrT>>();
  auto routesJson = routes[kRoutes];
  for (const auto& routeJson : routesJson) {
    auto route = Route<AddrT>::fromFollyDynamic(routeJson);
    rib->addRoute(route);
    rib->addRouteInRadixTree(route);
  }
  return rib;
}

template <typename AddrT>
void RouteTableRib<AddrT>::addRoute(
    const std::shared_ptr<Route<AddrT>>& route) {
  nodeMap_->addRoute(route);
}

template <typename AddrT>
void RouteTableRib<AddrT>::updateRoute(
    const std::shared_ptr<Route<AddrT>>& route) {
  nodeMap_->updateRoute(route);
}

template <typename AddrT>
void RouteTableRib<AddrT>::removeRoute(
    const std::shared_ptr<Route<AddrT>>& route) {
  nodeMap_->removeRoute(route);
}

template class RouteTableRib<folly::IPAddressV4>;
template class RouteTableRib<folly::IPAddressV6>;

} // namespace facebook::fboss
