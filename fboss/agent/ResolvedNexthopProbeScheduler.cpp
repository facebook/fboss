// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/ResolvedNexthopProbeScheduler.h"

#include "fboss/agent/ResolvedNexthopProbe.h"
#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/state/Interface.h"
#include "fboss/agent/state/SwitchState.h"

namespace facebook::fboss {

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
              sw_, sw_->getBackgroundEvb(), nexthop));
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

void ResolvedNexthopProbeScheduler::schedule() {
  auto state = sw_->getState();
  for (const auto& entry : resolvedNextHop2UseCount_) {
    auto intf =
        state->getInterfaces()->getInterface(entry.first.intfID().value());
    auto vlanId = sw_->getVlanIDHelper(intf->getVlanIDIf());
    auto vlan = state->getVlans()->getVlan(vlanId);
    auto startProbe = entry.first.addr().isV4()
        ? shouldProbe(entry.first.addr().asV4(), vlan.get())
        : shouldProbe(entry.first.addr().asV6(), vlan.get());
    if (startProbe) {
      resolvedNextHop2Probes_[entry.first]->start();
    } else {
      resolvedNextHop2Probes_[entry.first]->stop();
    }
  }
}

} // namespace facebook::fboss
