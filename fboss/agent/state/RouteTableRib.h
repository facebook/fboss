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
#pragma once

#include "fboss/agent/FbossError.h"
#include "fboss/agent/state/NodeMap.h"
#include "fboss/agent/state/RouteTypes.h"
#include "fboss/agent/types.h"
#include "fboss/lib/RadixTree.h"

namespace facebook::fboss {

template <typename AddrT>
class Route;
class SwitchState;

template <typename AddrT>
class RouteTableRib;

template <typename AddrT>
using RouteTableRibNodeMapTraits =
    NodeMapTraits<RoutePrefix<AddrT>, Route<AddrT>>;

template <typename AddrT>
class RouteTableRibNodeMap : public NodeMapT<
                                 RouteTableRibNodeMap<AddrT>,
                                 RouteTableRibNodeMapTraits<AddrT>> {
 public:
  RouteTableRibNodeMap() {}
  ~RouteTableRibNodeMap() override {}

  using Base =
      NodeMapT<RouteTableRibNodeMap<AddrT>, RouteTableRibNodeMapTraits<AddrT>>;
  using Prefix = RoutePrefix<AddrT>;
  using RouteType = typename Base::Node;

  bool empty() const {
    return size() == 0;
  }

  uint64_t size() const {
    return Base::size();
  }

  void addRoute(const std::shared_ptr<Route<AddrT>>& rt);

  void updateRoute(const std::shared_ptr<Route<AddrT>>& rt);

  void removeRoute(const std::shared_ptr<Route<AddrT>>& rt);

  std::shared_ptr<Route<AddrT>> getRouteIf(const Prefix& prefix) const {
    return Base::getNodeIf(prefix);
  }

 private:
  // Inherit the constructors required for clone()
  using Base::Base;
  friend class CloneAllocator;
};

template <typename AddrT>
class RouteTableRib : public NodeBase {
 public:
  using RoutesNodeMap = RouteTableRibNodeMap<AddrT>;
  RouteTableRib() : nodeMap_(std::make_shared<RoutesNodeMap>()) {}
  RouteTableRib(NodeID id, uint32_t generation)
      : NodeBase(id, generation), nodeMap_(std::make_shared<RoutesNodeMap>()) {}
  ~RouteTableRib() override {}

  using Prefix = RoutePrefix<AddrT>;
  using RouteType = Route<AddrT>;
  using RoutesRadixTree =
      facebook::network::RadixTree<AddrT, std::shared_ptr<Route<AddrT>>>;

  bool empty() const {
    return nodeMap_->empty();
  }

  uint64_t size() const {
    return nodeMap_->size();
  }

  const std::shared_ptr<RoutesNodeMap> routes() const {
    return nodeMap_;
  }
  std::shared_ptr<RoutesNodeMap> writableRoutes() {
    CHECK(!isPublished());
    return nodeMap_;
  }

  void publish() override {
    // We should expect radixTree_ and nodeMap_ in sync before we publish rib
    CHECK_EQ(size(), radixTree_.size());
    nodeMap_->publish();
    NodeBase::publish();
  }

  RouteTableRib* modify(RouterID id, std::shared_ptr<SwitchState>* state);

  std::shared_ptr<RouteTableRib> clone() const {
    // In this clone(), we make sure the root RouteTableRib version increased by
    // 1. And then we use the default NodeMap clone() to clone the childNode
    // `nodeMap_`, so we don't have to clone every route in `nodeMap_`.
    // Besides, we don't clone the RadixTree. So that, we can just add/update/
    // remove routes on nodeMap_ and then call
    // `cloneToRadixTreeWithForwardClear()` to build the whole radixTree_
    // after all the changes.
    auto routeTableRib =
        std::make_shared<RouteTableRib>(getNodeID(), getGeneration() + 1);
    // Note: this is the default NodeMap clone(), only the nodeMap pointer is
    // cloned, while all the routes are still the old route pointer.
    routeTableRib->nodeMap_ = nodeMap_->clone();
    return routeTableRib;
  }

  /*
   * Serialize to folly::dynamic
   */
  folly::dynamic toFollyDynamic() const;

  /*
   * Deserialize from folly::dynamic
   */
  static std::shared_ptr<RouteTableRib> fromFollyDynamic(
      const folly::dynamic& json);
  /*
   * Serialize to json string
   */
  static std::shared_ptr<RouteTableRib> fromJson(
      const folly::fbstring& jsonStr) {
    return fromFollyDynamic(folly::parseJson(jsonStr));
  }

  /*
   * The following functions modify the static state.
   * These should only be called on unpublished objects which are only visible
   * to a single thread.
   * To add/update/remove a route, we only do it on nodeMap_
   */
  void addRoute(const std::shared_ptr<Route<AddrT>>& route);
  void updateRoute(const std::shared_ptr<Route<AddrT>>& route);
  void removeRoute(const std::shared_ptr<Route<AddrT>>& route);
  std::shared_ptr<Route<AddrT>> exactMatch(const Prefix& prefix) const {
    return nodeMap_->getRouteIf(prefix);
  }

  void cloneToRadixTreeWithForwardClear() {
    // We should expect this function is called only before we publish the rib
    CHECK(!isPublished());
    radixTree_.clear();
    for (const auto& node : nodeMap_->getAllNodes()) {
      auto route = node.second;
      if (route->isPublished()) {
        route = route->clone(RouteType::Fields::COPY_PREFIX_AND_NEXTHOPS);
      }
      route->clearForward();
      radixTree_.insert(node.first.network, node.first.mask, route);
    }
    CHECK_EQ(size(), radixTree_.size());
  }

  // STRONGLY RECOMMEND to use routes() which returns the NodeMap
  // routesRadixTree() should only be used in the unit test to check whether
  // noddeMap_ and radixTree_ in sync
  const RoutesRadixTree& routesRadixTree() const {
    return radixTree_;
  }
  RoutesRadixTree& writableRoutesRadixTree() {
    CHECK(!isPublished());
    return radixTree_;
  }

  std::shared_ptr<Route<AddrT>> longestMatch(const AddrT& nexthop) const {
    auto citr = radixTree_.longestMatch(nexthop, nexthop.bitCount());
    return citr != radixTree_.end() ? citr->value() : nullptr;
  }

  void addRouteInRadixTree(const std::shared_ptr<Route<AddrT>>& route) {
    auto inserted =
        radixTree_.insert(route->prefix().network, route->prefix().mask, route)
            .second;
    if (!inserted) {
      throw FbossError(
          "Add failed, prefix for: ",
          route->str(),
          " already exists in RadixTree");
    }
  }
  void updateRouteInRadixTree(const std::shared_ptr<Route<AddrT>>& route) {
    auto itr =
        radixTree_.exactMatch(route->prefix().network, route->prefix().mask);
    if (itr == radixTree_.end()) {
      throw FbossError(
          "Update failed, prefix for: ",
          route->str(),
          " not present in RadixTree");
    }
    itr->value() = route;
  }
  void removeRouteInRadixTree(const std::shared_ptr<Route<AddrT>>& route) {
    auto erased =
        radixTree_.erase(route->prefix().network, route->prefix().mask);
    if (!erased) {
      throw FbossError(
          "Remove failed, prefix for: ",
          route->str(),
          " not present in RadixTree");
    }
  }

 private:
  RoutesRadixTree radixTree_;
  std::shared_ptr<RoutesNodeMap> nodeMap_;
};

} // namespace facebook::fboss
