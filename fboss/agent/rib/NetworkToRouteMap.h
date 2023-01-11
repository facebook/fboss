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
#include "fboss/agent/types.h"
#include "fboss/lib/RadixTree.h"

#include <folly/IPAddress.h>
#include <folly/dynamic.h>

#include <memory>
#include <type_traits>

namespace facebook::fboss {

template <typename AddressT>
class NetworkToRouteMap
    : public std::conditional_t<
          std::is_same_v<LabelID, AddressT>,
          std::unordered_map<LabelID, std::shared_ptr<Route<LabelID>>>,
          facebook::network::
              RadixTree<AddressT, std::shared_ptr<Route<AddressT>>>> {
  static constexpr auto kRoutes = "routes";

 public:
  using Base = std::conditional_t<
      std::is_same_v<LabelID, AddressT>,
      std::unordered_map<LabelID, std::shared_ptr<Route<LabelID>>>,
      facebook::network::RadixTree<AddressT, std::shared_ptr<Route<AddressT>>>>;
  using Base::Base;
  /* implicit */ NetworkToRouteMap(Base&& radixTree)
      : Base(std::move(radixTree)) {}
  using RouteT = Route<AddressT>;
  using FilterFn = std::function<bool(const std::shared_ptr<RouteT>)>;
  using Iterator = std::conditional_t<
      std::is_same_v<LabelID, AddressT>,
      std::unordered_map<LabelID, std::shared_ptr<Route<LabelID>>>::iterator,
      typename facebook::network::
          RadixTree<AddressT, std::shared_ptr<Route<AddressT>>>::Iterator>;

  folly::dynamic toFollyDynamic() const {
    return toFollyDynamic([](const std::shared_ptr<RouteT>&) { return true; });
  }
  folly::dynamic toFollyDynamic(const FilterFn& fn) const {
    folly::dynamic routesJson = folly::dynamic::array;
    for (const auto& routeNode : *this) {
      std::shared_ptr<Route<AddressT>> route;
      if constexpr (std::is_same_v<LabelID, AddressT>) {
        route = routeNode.second;
      } else {
        route = routeNode.value();
      }
      if (fn(route)) {
        routesJson.push_back(route->toFollyDynamic());
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
      auto prefix = route->prefix();
      networkToRouteMap.insert(prefix, std::move(route));
    }

    return networkToRouteMap;
  }

  std::pair<Iterator, bool> insert(
      typename Route<AddressT>::Prefix key,
      std::shared_ptr<Route<AddressT>> route) {
    if constexpr (std::is_same_v<LabelID, AddressT>) {
      return this->emplace(
          std::make_pair(LabelID(key.label()), std::move(route)));
    } else {
      return Base::insert(key.network(), key.mask(), std::move(route));
    }
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
using LabelToRouteMap = NetworkToRouteMap<LabelID>;

template <typename AddrT>
std::shared_ptr<Route<AddrT>>& value(
    typename NetworkToRouteMap<AddrT>::Iterator& iter) {
  if constexpr (std::is_same_v<LabelID, AddrT>) {
    return iter->second;
  } else {
    return iter->value();
  }
}

template <typename AddrT>
std::shared_ptr<Route<AddrT>>& value(
    facebook::network::RadixTreeNode<AddrT, std::shared_ptr<Route<AddrT>>>&
        iter) {
  return iter.value();
}

template <typename AddrT>
std::shared_ptr<Route<AddrT>>& value(
    std::pair<const AddrT, std::shared_ptr<Route<AddrT>>>& iter) {
  return iter.second;
}
} // namespace facebook::fboss
