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
#include "fboss/agent/SwitchStats.h"
#include "fboss/agent/Utils.h"
#include "fboss/agent/rib/ForwardingInformationBaseUpdater.h"
#include "fboss/agent/state/RouteUpdater.h"
#include "fboss/agent/state/SwitchState.h"

#include <memory>

namespace facebook::fboss {

std::shared_ptr<SwitchState> swSwitchFibUpdate(
    facebook::fboss::RouterID vrf,
    const facebook::fboss::IPv4NetworkToRouteMap& v4NetworkToRoute,
    const facebook::fboss::IPv6NetworkToRouteMap& v6NetworkToRoute,
    void* cookie) {
  facebook::fboss::ForwardingInformationBaseUpdater fibUpdater(
      vrf, v4NetworkToRoute, v6NetworkToRoute);

  auto sw = static_cast<facebook::fboss::SwSwitch*>(cookie);
  sw->updateStateWithHwFailureProtection("", std::move(fibUpdater));
  return sw->getState();
}

SwSwitchRouteUpdateWrapper::SwSwitchRouteUpdateWrapper(
    SwSwitch* sw,
    RoutingInformationBase* rib)
    : RouteUpdateWrapper(
          rib,
          rib ? swSwitchFibUpdate : std::optional<FibUpdateFunction>(),
          rib ? sw : nullptr),
      sw_(sw) {}

void SwSwitchRouteUpdateWrapper::updateStats(
    const RoutingInformationBase::UpdateStatistics& stats) {
  sw_->stats()->addRoutesV4(stats.v4RoutesAdded);
  sw_->stats()->addRoutesV6(stats.v6RoutesAdded);
  sw_->stats()->delRoutesV4(stats.v4RoutesDeleted);
  sw_->stats()->delRoutesV6(stats.v6RoutesDeleted);
}

void SwSwitchRouteUpdateWrapper::programLegacyRib(
    const SyncFibFor& syncFibFor) {
  auto updateFn = [this, &syncFibFor](const std::shared_ptr<SwitchState>& in) {
    auto [newState, stats] = programLegacyRibHelper(in, syncFibFor);
    updateStats(stats);
    return newState;
  };
  sw_->updateStateWithHwFailureProtection("Add/Del routes", updateFn);
}

AdminDistance SwSwitchRouteUpdateWrapper::clientIdToAdminDistance(
    ClientID clientID) const {
  return sw_->clientIdToAdminDistance(static_cast<int>(clientID));
}

void SwSwitchRouteUpdateWrapper::programClassIDLegacyRib(
    RouterID rid,
    const std::vector<folly::CIDRNetwork>& prefixes,
    std::optional<cfg::AclLookupClass> classId,
    bool async) {
  auto updateFn =
      [this, rid, prefixes, classId](const std::shared_ptr<SwitchState>& in) {
        return updateClassIdLegacyRibHelper(in, rid, prefixes, classId);
      };
  if (async) {
    sw_->updateState("Update classId routes, async", updateFn);
  } else {
    sw_->updateStateBlocking("Update classId routes", updateFn);
  }
}
} // namespace facebook::fboss
