/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "RouteNextHopEntry.h"

namespace facebook { namespace fboss {

std::string RouteNextHopEntry::str() const {
  std::string result;
  switch (action_) {
  case Action::DROP:
    result = "DROP";
    break;
  case Action::TO_CPU:
    result = "ToCPU";
    break;
  case Action::NEXTHOPS:
    toAppend(getNextHopSet(), &result);
    break;
  default:
    CHECK(0);
    break;
  }
  return result;
}

bool operator==(const RouteNextHopEntry& a, const RouteNextHopEntry& b) {
  return (a.getAction() == b.getAction()
          and a.getNextHopSet() == b.getNextHopSet());
}

bool operator< (const RouteNextHopEntry& a, const RouteNextHopEntry& b) {
  return ((a.getAction() == b.getAction())
          ? (a.getNextHopSet() < b.getNextHopSet())
          : a.getAction() < b.getAction());
}

void toAppend(const RouteNextHopEntry::NextHopSet& nhops, std::string *result) {
  for (const auto& nhop : nhops) {
    result->append(folly::to<std::string>(nhop.str(), " "));
  }
}

std::ostream& operator<<(std::ostream& os,
                         const RouteNextHopEntry::NextHopSet& nhops) {
  for (const auto& nhop : nhops) {
    os << nhop.str() << " ";
  }
  return os;
}

}}
