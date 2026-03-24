/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/rib/RibToSwitchStateUpdater.h"

namespace facebook::fboss {

RibToSwitchStateUpdater::RibToSwitchStateUpdater(
    const SwitchIdScopeResolver* resolver,
    RouterID vrf,
    const IPv4NetworkToRouteMap& v4NetworkToRoute,
    const IPv6NetworkToRouteMap& v6NetworkToRoute,
    const LabelToRouteMap& labelToRoute,
    const NextHopIDManager* nextHopIDManager,
    const MySidTable& mySidTable)
    : fibUpdater_(
          resolver,
          vrf,
          v4NetworkToRoute,
          v6NetworkToRoute,
          labelToRoute,
          nextHopIDManager),
      mySidUpdater_(resolver, mySidTable) {}

std::shared_ptr<SwitchState> RibToSwitchStateUpdater::operator()(
    const std::shared_ptr<SwitchState>& state) {
  auto nextState = fibUpdater_(state);
  nextState = mySidUpdater_(nextState);
  lastDelta_ = StateDelta(state, nextState);
  return nextState;
}

} // namespace facebook::fboss
