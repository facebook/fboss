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

#include "fboss/agent/types.h"
#include "fboss/agent/state/NodeMap.h"
#include "fboss/agent/state/RouteTypes.h"

namespace facebook { namespace fboss {

template<typename AddrT>
class Route;

// FIXME: temporary solution before RadixTree
// use NodeMap to store all routes.
template<typename AddrT> using RouteTableRibTraits
  = NodeMapTraits<RoutePrefix<AddrT>, Route<AddrT>>;

template<typename AddrT>
class RouteTableRib
    : public NodeMapT<RouteTableRib<AddrT>, RouteTableRibTraits<AddrT>> {
 public:
  RouteTableRib();
  virtual ~RouteTableRib();

  typedef NodeMapT<RouteTableRib<AddrT>, RouteTableRibTraits<AddrT>> Base;
  typedef RoutePrefix<AddrT> Prefix;
  typedef typename Base::Node RouteType;

  bool empty() const {
    return Base::getAllNodes().size() == 0;
  }

  uint64_t size() const {
    return this->getAllNodes().size();
  }

  std::shared_ptr<Route<AddrT>> exactMatch(const Prefix& prefix) const {
    return Base::getNodeIf(prefix);
  }
  std::shared_ptr<Route<AddrT>> longestMatch(const AddrT& nexthop) const;

  /*
   * The following functions modify the static state.
   * These should only be called on unpublished objects which are only visible
   * to a single thread.
   */
  void addRoute(const std::shared_ptr<Route<AddrT>>& rt) {
    Base::addNode(rt);
  }
  void updateRoute(const std::shared_ptr<Route<AddrT>>& rt) {
    /*
     * NOTE: This function is called in the loop of this rib. It shall not
     * invalidate any existing iterators.
     */
    Base::updateNode(rt);
  }
  void removeRoute(const std::shared_ptr<Route<AddrT>>& rt) {
    Base::removeNode(rt);
  }

 private:
  // Inherit the constructors required for clone()
  using Base::NodeMapT;
  friend class CloneAllocator;
};

}}
