/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "AlpmUtils.h"

#include <folly/IPAddressV4.h>
#include <folly/IPAddressV6.h>

#include "fboss/agent/Utils.h"
#include "fboss/agent/if/gen-cpp2/ctrl_types.h"
#include "fboss/agent/state/RouteUpdater.h"
#include "fboss/agent/state/SwitchState.h"

namespace facebook {
namespace fboss {

std::shared_ptr<SwitchState> setupAlpmState(
    std::shared_ptr<SwitchState> curState) {

  // In ALPM mode we need to make sure that the first route added is
  // the default route and that the route table always contains a default
  // route
  RouteUpdater updater(curState->getRouteTables());
  updater.addRoute(RouterID(0), folly::IPAddressV4("0.0.0.0"), 0,
      StdClientIds2ClientID(StdClientIds::STATIC_INTERNAL),
      RouteNextHopEntry(RouteForwardAction::DROP,
        AdminDistance::MAX_ADMIN_DISTANCE));
  updater.addRoute(RouterID(0), folly::IPAddressV6("::"), 0,
      StdClientIds2ClientID(StdClientIds::STATIC_INTERNAL),
      RouteNextHopEntry(RouteForwardAction::DROP,
        AdminDistance::MAX_ADMIN_DISTANCE));
  auto newRt = updater.updateDone();
  if (newRt) {
    // If null default routes from above got added,
    // reflect them in a new switch state
    auto newState = curState->clone();
    newState->resetRouteTables(std::move(newRt));
    newState->publish();
    return newState;
  }
  return nullptr;
}

} // namespace fboss
} // namespace facebook
