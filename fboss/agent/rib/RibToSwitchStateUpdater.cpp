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

#include "fboss/agent/AgentFeatures.h"
#include "fboss/agent/state/SwitchState.h"

namespace facebook::fboss {

RibToSwitchStateUpdater::RibToSwitchStateUpdater(
    const SwitchIdScopeResolver* resolver,
    RouterID vrf,
    const IPv4NetworkToRouteMap& v4NetworkToRoute,
    const IPv6NetworkToRouteMap& v6NetworkToRoute,
    const LabelToRouteMap& labelToRoute,
    const NextHopIDManager* nextHopIDManager,
    const MySidTable& mySidTable,
    int actions)
    : actions_(actions),
      fibUpdater_(
          resolver,
          vrf,
          v4NetworkToRoute,
          v6NetworkToRoute,
          labelToRoute),
      mySidUpdater_(resolver, mySidTable),
      nhopStateUpdater_(nextHopIDManager) {}

std::shared_ptr<SwitchState> RibToSwitchStateUpdater::operator()(
    const std::shared_ptr<SwitchState>& state) {
  auto nextState = state;
  if (actions_ & UPDATE_FIB) {
    nextState = fibUpdater_(nextState);
  }
  if (actions_ & UPDATE_MYSID) {
    nextState = mySidUpdater_(nextState);
  }
  nextState = nhopStateUpdater_(nextState);
  if (!FLAGS_verify_fib_nexthop_id_consistency) {
    DCHECK(nhopStateUpdater_.verifyNextHopIdConsistency(nextState));
  } else {
    CHECK(nhopStateUpdater_.verifyNextHopIdConsistency(nextState));
  }
  lastDelta_ = StateDelta(state, nextState);
  return nextState;
}

} // namespace facebook::fboss
