// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/ResolvedNexthopProbeScheduler.h"

#include "fboss/agent/SwSwitch.h"

namespace facebook {
namespace fboss {

ResolvedNexthopProbeScheduler::ResolvedNexthopProbeScheduler(SwSwitch* sw)
    : sw_(sw) {}

void ResolvedNexthopProbeScheduler::processChangedResolvedNexthops(
    std::vector<ResolvedNextHop> added,
    std::vector<ResolvedNextHop> removed) {
  for (auto nexthop : added) {
    auto itr = resolvedNextHop2UseCount_.find(nexthop);
    if (itr == resolvedNextHop2UseCount_.end()) {
      // add probe
      resolvedNextHop2UseCount_.emplace(nexthop, 1);
      continue;
    }
    itr->second++;
  }

  for (auto nexthop : removed) {
    auto itr = resolvedNextHop2UseCount_.find(nexthop);
    CHECK(itr != resolvedNextHop2UseCount_.end());
    if (itr->second == 1) {
      // remove probe
      resolvedNextHop2UseCount_.erase(itr);
    } else {
      itr->second--;
    }
  }
}

} // namespace fboss
} // namespace facebook
