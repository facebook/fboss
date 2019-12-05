// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/ResolvedNexthopMonitor.h"

#include "fboss/agent/ResolvedNexthopProbeScheduler.h"
#include "fboss/agent/state/DeltaFunctions.h"
#include "fboss/agent/state/Interface.h"
#include "fboss/agent/state/StateDelta.h"
#include "fboss/agent/state/SwitchState.h"

using facebook::fboss::DeltaFunctions::forEachAdded;
using facebook::fboss::DeltaFunctions::forEachChanged;
using facebook::fboss::DeltaFunctions::forEachRemoved;

namespace facebook {
namespace fboss {

ResolvedNexthopMonitor::ResolvedNexthopMonitor(SwSwitch* sw)
    : AutoRegisterStateObserver(sw, "ResolvedNexthopMonitor"), sw_(sw) {}

void ResolvedNexthopMonitor::stateUpdated(const StateDelta& delta) {
  scheduleProbes_ = false;
  for (auto const& rtDelta : delta.getRouteTablesDelta()) {
    forEachChanged(
        rtDelta.getRoutesV4Delta(),
        &ResolvedNexthopMonitor::processChangedRouteNextHops<RouteV4>,
        &ResolvedNexthopMonitor::processAddedRouteNextHops<RouteV4>,
        &ResolvedNexthopMonitor::processRemovedRouteNextHops<RouteV4>,
        this);
    forEachChanged(
        rtDelta.getRoutesV6Delta(),
        &ResolvedNexthopMonitor::processChangedRouteNextHops<RouteV6>,
        &ResolvedNexthopMonitor::processAddedRouteNextHops<RouteV6>,
        &ResolvedNexthopMonitor::processRemovedRouteNextHops<RouteV6>,
        this);
  }

  for (const auto& fibDelta : delta.getFibsDelta()) {
    forEachChanged(
        fibDelta.getV4FibDelta(),
        &ResolvedNexthopMonitor::processChangedRouteNextHops<RouteV4>,
        &ResolvedNexthopMonitor::processAddedRouteNextHops<RouteV4>,
        &ResolvedNexthopMonitor::processRemovedRouteNextHops<RouteV4>,
        this);
    forEachChanged(
        fibDelta.getV6FibDelta(),
        &ResolvedNexthopMonitor::processChangedRouteNextHops<RouteV6>,
        &ResolvedNexthopMonitor::processAddedRouteNextHops<RouteV6>,
        &ResolvedNexthopMonitor::processRemovedRouteNextHops<RouteV6>,
        this);
  }
  scheduleProbes_ = !added_.empty() || !removed_.empty();
  sw_->getResolvedNexthopProbeScheduler()->processChangedResolvedNexthops(
      std::move(added_), std::move(removed_));
}
} // namespace fboss
} // namespace facebook
