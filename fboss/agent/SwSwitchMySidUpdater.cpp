/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/SwSwitchMySidUpdater.h"

#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/SwitchIdScopeResolver.h"
#include "fboss/agent/rib/NetworkToRouteMap.h"
#include "fboss/agent/rib/RibToSwitchStateUpdater.h"
#include "fboss/agent/state/SwitchState.h"

namespace facebook::fboss {

RibMySidToSwitchStateFunction createRibMySidToSwitchStateFunction(
    const std::optional<StateDeltaApplication>& deltaApplicationBehavior) {
  return [deltaApplicationBehavior](
             const facebook::fboss::SwitchIdScopeResolver* resolver,
             const facebook::fboss::NextHopIDManager* nextHopIDManager,
             const MySidTable& mySidTable,
             void* cookie) -> StateDelta {
    IPv4NetworkToRouteMap emptyV4;
    IPv6NetworkToRouteMap emptyV6;
    LabelToRouteMap emptyLabel;
    facebook::fboss::RibToSwitchStateUpdater updater(
        resolver,
        RouterID(0), // don't care
        emptyV4,
        emptyV6,
        emptyLabel,
        nextHopIDManager,
        mySidTable,
        RibToSwitchStateUpdater::UPDATE_MYSID);
    auto sw = static_cast<facebook::fboss::SwSwitch*>(cookie);

    auto oldState = sw->getState();

    sw->updateStateWithHwFailureProtection(
        "update mySids",
        [&updater](const std::shared_ptr<SwitchState>& in) {
          return updater(in);
        },
        deltaApplicationBehavior);

    return StateDelta(oldState, sw->getState());
  };
}

} // namespace facebook::fboss
