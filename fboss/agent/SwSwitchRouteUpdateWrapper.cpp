/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/SwSwitchRouteUpdateWrapper.h"

#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/SwitchIdScopeResolver.h"
#include "fboss/agent/SwitchStats.h"
#include "fboss/agent/rib/NextHopIDManager.h"
#include "fboss/agent/rib/RibToSwitchStateUpdater.h"
#include "fboss/agent/rib/RouteUpdater.h"

#include "fboss/agent/state/SwitchState.h"

#include <memory>

namespace facebook::fboss {

namespace {

// Creates a RibToSwitchStateFunction that captures the useRollback flag to
// determine which SwSwitch update method to call:
// - useRollback=true:  calls updateStateWithForcedRollback (for testing)
// - useRollback=false: calls updateStateWithHwFailureProtection (production)
RibToSwitchStateFunction createRibToSwitchStateFunction(
    const std::optional<StateDeltaApplication>& deltaApplicationBehavior) {
  return [deltaApplicationBehavior](
             const facebook::fboss::SwitchIdScopeResolver* resolver,
             facebook::fboss::RouterID vrf,
             const facebook::fboss::IPv4NetworkToRouteMap& v4NetworkToRoute,
             const facebook::fboss::IPv6NetworkToRouteMap& v6NetworkToRoute,
             const facebook::fboss::LabelToRouteMap& labelToRoute,
             const NextHopIDManager* nextHopIDManager,
             const MySidTable& mySidTable,
             void* cookie) -> StateDelta {
    // Since RIB to switch state updater will be accessed in swSwitch's update
    // event base protect with a lock. This is even though we know the update is
    // synchronous and we will call getLastDelta only after the update is done.
    // Its cleaner to protect variables accessed from multiple threads.
    folly::Synchronized<facebook::fboss::RibToSwitchStateUpdater>
        ribToSwitchStateUpdater(
            std::in_place,
            resolver,
            vrf,
            v4NetworkToRoute,
            v6NetworkToRoute,
            labelToRoute,
            nextHopIDManager,
            mySidTable);

    auto wRibToSwitchStateUpdater = ribToSwitchStateUpdater.wlock();
    auto sw = static_cast<facebook::fboss::SwSwitch*>(cookie);

    sw->updateStateWithHwFailureProtection(
        "update fib",
        [&wRibToSwitchStateUpdater](const std::shared_ptr<SwitchState>& in) {
          return (*wRibToSwitchStateUpdater)(in);
        },
        deltaApplicationBehavior);

    auto lastDelta = wRibToSwitchStateUpdater->getLastDelta();
    // Fib update could get cancelled - e.g. when SwSwitch already started its
    // exit. In that case last delta will be empty. Account for this case
    auto oldState =
        lastDelta ? lastDelta->oldState() : std::make_shared<SwitchState>();
    return StateDelta(oldState, sw->getState());
  };
}

} // namespace

SwSwitchRouteUpdateWrapper::SwSwitchRouteUpdateWrapper(
    SwSwitch* sw,
    RoutingInformationBase* rib,
    const std::optional<StateDeltaApplication>& deltaApplicationBehavior)
    : RouteUpdateWrapper(
          sw->getScopeResolver(),
          rib,
          rib ? createRibToSwitchStateFunction(deltaApplicationBehavior)
              : std::optional<RibToSwitchStateFunction>(),
          rib ? sw : nullptr),
      sw_(sw) {}

void SwSwitchRouteUpdateWrapper::updateStats(
    const RoutingInformationBase::UpdateStatistics& stats) {
  sw_->stats()->addRoutesV4(stats.v4RoutesAdded);
  sw_->stats()->addRoutesV6(stats.v6RoutesAdded);
  sw_->stats()->delRoutesV4(stats.v4RoutesDeleted);
  sw_->stats()->delRoutesV6(stats.v6RoutesDeleted);
  sw_->stats()->ribRouteProgrammingTimeUs(stats.duration);
}

AdminDistance SwSwitchRouteUpdateWrapper::clientIdToAdminDistance(
    ClientID clientID) const {
  return sw_->clientIdToAdminDistance(static_cast<int>(clientID));
}

} // namespace facebook::fboss
