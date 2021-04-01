/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#pragma once

#include "fboss/agent/state/Route.h"
#include "fboss/lib/RadixTree.h"

#include <folly/IPAddress.h>
#include <folly/dynamic.h>

#include <memory>

namespace facebook::fboss {

template <typename AddressT>
class NetworkToRouteMap
    : public facebook::network::
          RadixTree<AddressT, std::shared_ptr<Route<AddressT>>> {
  static constexpr auto kRoutes = "routes";

 public:
  using Base =
      facebook::network::RadixTree<AddressT, std::shared_ptr<Route<AddressT>>>;
  using Base::Base;
  /* implicit */ NetworkToRouteMap(Base&& radixTree)
      : Base(std::move(radixTree)) {}
  using RouteT = Route<AddressT>;
  using FilterFn = std::function<bool(const std::shared_ptr<RouteT>)>;

  folly::dynamic toFollyDynamic() const {
    return toFollyDynamic([](const std::shared_ptr<RouteT>&) { return true; });
  }
  folly::dynamic toFollyDynamic(const FilterFn& fn) const {
    folly::dynamic routesJson = folly::dynamic::array;
    for (const auto& routeNode : *this) {
      if (fn(routeNode.value())) {
        routesJson.push_back(routeNode.value()->toFollyDynamic());
      }
    }
    folly::dynamic routesObject = folly::dynamic::object;
    routesObject[kRoutes] = std::move(routesJson);
    return routesObject;
  }

  static NetworkToRouteMap<AddressT> fromFollyDynamic(
      const folly::dynamic& routes) {
    NetworkToRouteMap<AddressT> networkToRouteMap;

    auto routesJson = routes[kRoutes];
    for (const auto& routeJson : routesJson) {
      auto route = Route<AddressT>::fromFollyDynamic(routeJson);
      RoutePrefix<AddressT> prefix = route->prefix();
      networkToRouteMap.insert(prefix.network, prefix.mask, std::move(route));
    }

    return networkToRouteMap;
  }
  template <typename Fn>
  void forAll(const Fn& fn) {
    std::for_each(this->begin(), this->end(), fn);
  }
  void cloneAll() {
    forAll([](auto& ritr) { ritr.value() = ritr.value()->clone(); });
  }
  void publishAll() {
    forAll([](auto& ritr) { ritr.value()->publish(); });
  }
};

using IPv4NetworkToRouteMap = NetworkToRouteMap<folly::IPAddressV4>;
using IPv6NetworkToRouteMap = NetworkToRouteMap<folly::IPAddressV6>;

} // namespace facebook::fboss
