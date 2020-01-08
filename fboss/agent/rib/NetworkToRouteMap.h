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

#include "fboss/agent/rib/Route.h"
#include "fboss/lib/RadixTree.h"

#include <folly/IPAddress.h>
#include <folly/dynamic.h>

#include <memory>

namespace facebook::fboss::rib {

template <typename AddressT>
class NetworkToRouteMap
    : public facebook::network::RadixTree<AddressT, Route<AddressT>> {
  static constexpr auto kRoutes = "routes";

 public:
  folly::dynamic toFollyDynamic() const {
    folly::dynamic routesJson = folly::dynamic::array;
    for (const auto& route : *this) {
      routesJson.push_back(route.value().toFollyDynamic());
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
      RoutePrefix<AddressT> prefix = route.prefix();
      networkToRouteMap.insert(prefix.network, prefix.mask, std::move(route));
    }

    return networkToRouteMap;
  }
};

using IPv4NetworkToRouteMap = NetworkToRouteMap<folly::IPAddressV4>;
using IPv6NetworkToRouteMap = NetworkToRouteMap<folly::IPAddressV6>;

} // namespace facebook::fboss::rib
