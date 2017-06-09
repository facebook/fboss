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
#include "fboss/agent/types.h"
#include "fboss/agent/state/NodeMap.h"
#include "fboss/agent/state/RouteTypes.h"
#include "fboss/lib/RadixTree.h"

namespace facebook { namespace fboss {

template<typename AddrT>
class Route;
class SwitchState;

template <typename AddrT>
class RouteTableRib;

template<typename AddrT> using RouteTableRibNodeMapTraits
  = NodeMapTraits<RoutePrefix<AddrT>, Route<AddrT>>;

template<typename AddrT>
class RouteTableRibNodeMap
    : public NodeMapT<RouteTableRibNodeMap<AddrT>,
    RouteTableRibNodeMapTraits<AddrT>> {
 public:
  explicit RouteTableRibNodeMap() {}
  ~RouteTableRibNodeMap() override {}

  using Base =  NodeMapT<RouteTableRibNodeMap<AddrT>,
        RouteTableRibNodeMapTraits<AddrT>>;
  using Prefix = RoutePrefix<AddrT>;
  using RouteType = typename Base::Node;

  bool empty() const {
    return Base::getAllNodes().size() == 0;
  }

  uint64_t size() const {
    return this->getAllNodes().size();
  }

  void addRoutes(const RouteTableRib<AddrT>& rib) {
    for(const auto& routeItr: rib.routes()) {
      addRoute(routeItr->value());
    }
  }
 private:
  void addRoute(const std::shared_ptr<Route<AddrT>>& rt) {
    Base::addNode(rt);
  }
  // Inherit the constructors required for clone()
  using Base::Base;
  friend class CloneAllocator;
};

template<typename AddrT>
class RouteTableRib : public NodeBase {
 public:
  RouteTableRib() {}
  RouteTableRib(NodeID id, uint32_t generation):
    NodeBase(id, generation) {}
  ~RouteTableRib() override {}

  using Prefix =  RoutePrefix<AddrT>;
  using RouteType = Route<AddrT>;
  using Routes = facebook::network::RadixTree<AddrT,
        std::shared_ptr<Route<AddrT>>>;

  bool empty() const {
    return size() == 0;
  }

  uint64_t size() const {
    return rib_.size();
  }

  const Routes& routes() const { return rib_; }
  Routes& writableRoutes() {
    CHECK(!isPublished());
    return rib_;
  }

  void publish() override {
    NodeBase::publish();
    for (auto routeIter: rib_) {
      routeIter->value()->publish();
    }
  }
  std::shared_ptr<Route<AddrT>> exactMatch(const Prefix& prefix) const {
    auto citr = rib_.exactMatch(prefix.network, prefix.mask);
    return citr != rib_.end() ? citr->value() : nullptr;
  }
  std::shared_ptr<Route<AddrT>> longestMatch(const AddrT& nexthop) const {
    auto citr = rib_.longestMatch(nexthop, nexthop.bitCount());
    return citr != rib_.end() ? citr->value() : nullptr;
  }

  RouteTableRib* modify(RouterID id, std::shared_ptr<SwitchState>* state);

  std::shared_ptr<RouteTableRib> clone() const {
    auto routeTableRib = std::make_shared<RouteTableRib>(getNodeID(),
        getGeneration() + 1);
    for (auto& routeItr: rib_) {
      /* Explicitly insert cloned routes rather than calling
       * rib_.clone(), which will just use the shared_ptr copy
       * constructor
       */
      routeTableRib->rib_.insert(routeItr->ipAddress(),
          routeItr->masklen(), routeItr->value()->clone());
    }
    return routeTableRib;
  }
  /*
   * Serialize to folly::dynamic
   */
  folly::dynamic toFollyDynamic() const;

   /*
    * Deserialize from folly::dynamic
    */
   static std::shared_ptr<RouteTableRib>
     fromFollyDynamic(const folly::dynamic& json);
  /*
   * Serialize to json string
   */
  static std::shared_ptr<RouteTableRib>
    fromJson(const folly::fbstring& jsonStr) {
       return fromFollyDynamic(folly::parseJson(jsonStr));
  }

  /*
   * The following functions modify the static state.
   * These should only be called on unpublished objects which are only visible
   * to a single thread.
   */
  void addRoute(const std::shared_ptr<Route<AddrT>>& rt) {
    auto inserted = rib_.insert(rt->prefix().network,
        rt->prefix().mask, rt).second;
    if (!inserted) {
      throw FbossError("Prefix for: ", rt->str(), " already exists");
    }
  }
  void updateRoute(const std::shared_ptr<Route<AddrT>>& rt) {
    auto itr = rib_.exactMatch(rt->prefix().network, rt->prefix().mask);
    if (itr == rib_.end()) {
      throw FbossError("Update failed, prefix for: ", rt->str(),
          " not present");
    }
    itr->value() = rt;
  }
  void removeRoute(const std::shared_ptr<Route<AddrT>>& rt) {
    auto erased = rib_.erase(rt->prefix().network, rt->prefix().mask);
    if (!erased) {
      throw FbossError("Remove failed, prefix for: ", rt->str(),
          " not present");
    }
  }


 private:
  Routes rib_;
};

}}
