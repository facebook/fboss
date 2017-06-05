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

#include <boost/container/flat_set.hpp>

#include <folly/dynamic.h>

#include "fboss/agent/state/RouteNextHop.h"
#include "fboss/agent/state/RouteTypes.h"

namespace facebook { namespace fboss {

/**
 * Multiple RouteNextHop for ECMP route.
 */
class RouteNextHopEntry {
 public:
  using Action = RouteForwardAction;
  using NextHopSet = boost::container::flat_set<RouteNextHop>;

  explicit RouteNextHopEntry(Action action = Action::DROP) : action_(action) {
    CHECK_NE(action_, Action::NEXTHOPS);
  }

  explicit RouteNextHopEntry(NextHopSet nhopSet);

  explicit RouteNextHopEntry(RouteNextHop nhop)
      : action_(Action::NEXTHOPS) {
    nhopSet_.emplace(std::move(nhop));
  }

  Action getAction() const {
    return action_;
  }

  const NextHopSet& getNextHopSet() const {
    return nhopSet_;
  }

  std::string str() const;

  /*
   * Serialize to folly::dynamic
   */
  folly::dynamic toFollyDynamic() const;

  /*
   * Deserialize from folly::dynamic
   */
  static RouteNextHopEntry fromFollyDynamic(const folly::dynamic& entryJson);

  // Methods to manipulate this object
  bool isDrop() const {
    return action_ == Action::DROP;
  }
  bool isToCPU() const {
    return action_ == Action::TO_CPU;
  }

  // Reset the NextHopSet
  void reset() {
    nhopSet_.clear();
    action_ = Action::DROP;
  }

 private:
  Action action_{Action::DROP};
  NextHopSet nhopSet_;
};

/**
 * Comparision operators
 */
bool operator==(const RouteNextHopEntry& a, const RouteNextHopEntry& b);
bool operator< (const RouteNextHopEntry& a, const RouteNextHopEntry& b);

void toAppend(const RouteNextHopEntry& entry, std::string *result);
std::ostream& operator<<(std::ostream& os, const RouteNextHopEntry& entry);

void toAppend(const RouteNextHopEntry::NextHopSet& nhops, std::string *result);
std::ostream& operator<<(
    std::ostream& os, const RouteNextHopEntry::NextHopSet& nhops);

// TODO: replace all RouteNextHops with RouteNextHopSet
using RouteNextHops = RouteNextHopEntry::NextHopSet;
using RouteNextHopSet = RouteNextHopEntry::NextHopSet;

namespace util {

/**
 * Convert thrift representation of nexthops to RouteNextHops.
 */
RouteNextHops
toRouteNextHops(std::vector<network::thrift::BinaryAddress> const& nhAddrs);

/**
 * Convert RouteNextHops to thrift representaion of nexthops
 */
std::vector<network::thrift::BinaryAddress>
fromRouteNextHops(RouteNextHops const& nhs);

}

}}
