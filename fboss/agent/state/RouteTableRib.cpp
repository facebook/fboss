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
#include "fboss/agent/state/RouteTable.h"
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
  // Generate the radix tree from the cloned nodeMap_. Remember, we haven't
  // clone every route yet. So we will still use the old route pointer.
  // Right now only revertNewRouteEntry() needs modify(). The expected result of
  // modify() is that we have a cloned RouteTableRib return if the current one
  // is published. To make sure the cloned RouteTableRib works, we need to
  // ensure radixTree_ and nodeMap_ in sync before we return a newly cloned rib.
  for (const auto& node : nodeMap_->getAllNodes()) {
    clonedRib->radixTree_.insert(
        node.first.network, node.first.mask, node.second);
  }
  CHECK_EQ(clonedRib->size(), clonedRib->radixTree_.size());

  auto clonedRibPtr = clonedRib.get();
  clonedRouteTable->setRib(clonedRib);

  return clonedRibPtr;
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
