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

#include "fboss/agent/state/RouteNextHopBase.h"
#include "fboss/agent/state/RouteTypes.h"

namespace facebook { namespace fboss {

class RouteNextHopEntry {
 public:
  using Action = RouteForwardAction;
  using NextHopSet = boost::container::flat_set<RouteNextHopBase>;

  RouteNextHopEntry() {}

  explicit RouteNextHopEntry(Action action) : action_(action) {
    CHECK_NE(action_, Action::NEXTHOPS);
  }

  explicit RouteNextHopEntry(NextHopSet nhopSet)
      : action_(Action::NEXTHOPS), nhopSet_(std::move(nhopSet)) {
  }

  explicit RouteNextHopEntry(RouteNextHopBase nhop)
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

 private:
  Action action_{Action::DROP};
  NextHopSet nhopSet_;
};

/**
 * Comparision operators
 */
bool operator==(const RouteNextHopEntry& a, const RouteNextHopEntry& b);
bool operator< (const RouteNextHopEntry& a, const RouteNextHopEntry& b);

void toAppend(const RouteNextHopEntry::NextHopSet& nhops, std::string *result);
std::ostream& operator<<(
    std::ostream& os, const RouteNextHopEntry::NextHopSet& nhops);

}}
