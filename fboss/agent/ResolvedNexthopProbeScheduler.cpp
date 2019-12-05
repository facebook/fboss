// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/ResolvedNexthopProbeScheduler.h"

#include "fboss/agent/ResolvedNexthopProbe.h"
#include "fboss/agent/SwSwitch.h"

namespace facebook {
namespace fboss {

ResolvedNexthopProbeScheduler::ResolvedNexthopProbeScheduler(SwSwitch* sw)
    : sw_(sw) {}

ResolvedNexthopProbeScheduler::~ResolvedNexthopProbeScheduler() {
  for (auto entry : resolvedNextHop2Probes_) {
    entry.second->stop();
  }
}

void ResolvedNexthopProbeScheduler::processChangedResolvedNexthops(
    std::vector<ResolvedNextHop> added,
    std::vector<ResolvedNextHop> removed) {
  for (auto nexthop : added) {
    auto itr = resolvedNextHop2UseCount_.find(nexthop);
    if (itr == resolvedNextHop2UseCount_.end()) {
      // add probe
      resolvedNextHop2UseCount_.emplace(nexthop, 1);
      resolvedNextHop2Probes_.emplace(
          nexthop,
          std::make_shared<ResolvedNextHopProbe>(
              sw_->getBackgroundEvb(), nexthop));
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
      resolvedNextHop2Probes_[nexthop]->stop();
      resolvedNextHop2Probes_.erase(nexthop);
    } else {
      itr->second--;
    }
  }
}

} // namespace fboss
} // namespace facebook
