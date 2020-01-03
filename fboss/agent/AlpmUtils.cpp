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

#include <utility>

#include <folly/IPAddressV4.h>
#include <folly/IPAddressV6.h>
#include <optional>

#include "fboss/agent/Utils.h"
#include "fboss/agent/if/gen-cpp2/ctrl_types.h"
#include "fboss/agent/state/RouteUpdater.h"
#include "fboss/agent/state/SwitchState.h"

using facebook::fboss::getMinimumAlpmState;
using facebook::fboss::SwitchState;

namespace {

std::pair<uint64_t, uint64_t> numMinV4AndV6AlpmRoutes() {
  uint64_t v4Routes{0}, v6Routes{0};
  getMinimumAlpmState()->getRouteTables()->getRouteCount(&v4Routes, &v6Routes);
  return std::make_pair(v4Routes, v6Routes);
}
} // namespace

namespace facebook::fboss {

std::shared_ptr<SwitchState> setupAlpmState(
    std::shared_ptr<SwitchState> curState) {
  // In ALPM mode we need to make sure that the first route added is
  // the default route and that the route table always contains a default
  // route
  RouteUpdater updater(curState->getRouteTables());
  updater.addRoute(
      RouterID(0),
      folly::IPAddressV4("0.0.0.0"),
      0,
      ClientID::STATIC_INTERNAL,
      RouteNextHopEntry(
          RouteForwardAction::DROP, AdminDistance::MAX_ADMIN_DISTANCE));
  updater.addRoute(
      RouterID(0),
      folly::IPAddressV6("::"),
      0,
      ClientID::STATIC_INTERNAL,
      RouteNextHopEntry(
          RouteForwardAction::DROP, AdminDistance::MAX_ADMIN_DISTANCE));
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

std::shared_ptr<SwitchState> getMinimumAlpmState() {
  return setupAlpmState(std::make_shared<SwitchState>());
}

uint64_t numMinAlpmRoutes() {
  auto v4AndV6 = numMinV4AndV6AlpmRoutes();
  return v4AndV6.first + v4AndV6.second;
}

uint64_t numMinAlpmV4Routes() {
  return numMinV4AndV6AlpmRoutes().first;
}

uint64_t numMinAlpmV6Routes() {
  return numMinV4AndV6AlpmRoutes().second;
}

} // namespace facebook::fboss
