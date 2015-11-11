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

#include <folly/dynamic.h>
#include <folly/IPAddress.h>
#include "fboss/agent/types.h"
#include "fboss/agent/state/RouteTypes.h"

#include <boost/container/flat_set.hpp>

namespace facebook { namespace fboss {

/**
 * RouteForwardInfo defines how to forward packets based on a route
 *
 * A RouteForwardInfo object first has an action field. The action field
 * determines if the packets shall be dropped, punt to CPU, or forward to
 * a nexthop or multiple nexthops (ECMP). If the action is to forward to
 * some nexthops, there are a set of nexthops specified in this class which
 * have the interface ID and immediate nexthop information.
 */
class RouteForwardInfo {
 public:
  typedef RouteForwardAction Action;

  /**
   * The nexthops in the forwarding info
   *
   * It includes a set of nexthops on where the route shall forward the packet.
   * In the case if action_ is not Action::NEXTHOPS, the set shall be empty.
   */
  struct Nexthop;
  typedef boost::container::flat_set<Nexthop> Nexthops;

  explicit RouteForwardInfo(Action action = Action::DROP)
      : action_(action) {
  }
  ~RouteForwardInfo() {
  }

  Action getAction() const {
    return action_;
  }

  const Nexthops& getNexthops() const {
    return nexthops_;
  }

  std::string str() const;

  /*
   * Serialize to folly::dynamic
   */
  folly::dynamic toFollyDynamic() const;

  /*
   * Deserialize from folly::dynamic
   */
  static RouteForwardInfo fromFollyDynamic(const folly::dynamic& fwdInfoJson);

  bool operator==(const RouteForwardInfo& info) const {
    return action_ == info.action_ && nexthops_ == info.nexthops_;
  }

  // Methods to manipulate this object

  bool isDrop() const {
    return action_ == Action::DROP;
  }
  void setDrop() {
    nexthops_.clear();
    action_ = Action::DROP;
  }

  bool isToCPU() const {
    return action_ == Action::TO_CPU;
  }
  void setToCPU() {
    nexthops_.clear();
    action_ = Action::TO_CPU;
  }

  void setAction(Action action) {
    CHECK(action == Action::TO_CPU || action == Action::DROP);
    action_ = action;
  }

  // Set one nexthop, a simple version for non-ECMP case
  void setNexthops(InterfaceID intf, const folly::IPAddress& nhop) {
    nexthops_.clear();
    nexthops_.emplace(intf, nhop);
    action_ = Action::NEXTHOPS;
  }
  // Set one or multiple nexthops
  void setNexthops(const Nexthops& nexthops) {
    nexthops_ = nexthops;
    action_ = Action::NEXTHOPS;
  }
  void setNexthops(Nexthops&& nexthops) {
    nexthops_ = std::move(nexthops);
    action_ = Action::NEXTHOPS;
  }

  // Reset the forwarding info
  void reset() {
    nexthops_.clear();
    action_ = Action::DROP;
  }

 private:
  Nexthops nexthops_;
  Action action_;
};

typedef RouteForwardInfo::Nexthops RouteForwardNexthops;

void toAppend(const RouteForwardInfo& fwd, std::string *result);
std::ostream& operator<<(std::ostream& os, const RouteForwardInfo& fwd);

/**
 * RouteForwardInfo::Nexthop Class
 *
 * A RouteForwardInfo::Nexthop is a path to reach a given nexthop, which is set
 * after the route is resolved.
 * In the case for the directly connected route, the 'nexthop' is the
 * interface IP address on the network.
 */
struct RouteForwardInfo::Nexthop {
  InterfaceID intf;
  folly::IPAddress nexthop;
  Nexthop(InterfaceID intf, folly::IPAddress nh)
      : intf(intf), nexthop(nh) {}
  bool operator<(const Nexthop& nhop) const;
  bool operator==(const Nexthop& nhop) const;
  bool operator!=(const Nexthop& nhop) const {
    return !operator==(nhop);
  }
  /*
   * Serialize to folly::dynamic
   */
  folly::dynamic toFollyDynamic() const;

  /*
   * Deserialize from folly::dynamic
   */
  static Nexthop fromFollyDynamic(const folly::dynamic& nhopJson);

  std::string str() const;
};

void toAppend(const RouteForwardNexthops& fwd, std::string *result);
std::ostream& operator<<(std::ostream& os, const RouteForwardNexthops& fwd);

}}
