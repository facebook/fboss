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

void swSwitchFibUpdate(
    facebook::fboss::RouterID vrf,
    const facebook::fboss::rib::IPv4NetworkToRouteMap& v4NetworkToRoute,
    const facebook::fboss::rib::IPv6NetworkToRouteMap& v6NetworkToRoute,
    void* cookie) {
  facebook::fboss::rib::ForwardingInformationBaseUpdater fibUpdater(
      vrf, v4NetworkToRoute, v6NetworkToRoute);

  auto sw = static_cast<facebook::fboss::SwSwitch*>(cookie);
  sw->updateStateBlocking("", std::move(fibUpdater));
}

SwSwitchRouteUpdateWrapper::SwSwitchRouteUpdateWrapper(SwSwitch* sw)
    : RouteUpdateWrapper(sw->isStandaloneRibEnabled()), sw_(sw) {}

void SwSwitchRouteUpdateWrapper::updateStats(
    const rib::RoutingInformationBase::UpdateStatistics& stats) {
  sw_->stats()->addRoutesV4(stats.v4RoutesAdded);
  sw_->stats()->addRoutesV6(stats.v6RoutesAdded);
  sw_->stats()->delRoutesV4(stats.v4RoutesDeleted);
  sw_->stats()->delRoutesV6(stats.v6RoutesDeleted);
}

rib::RoutingInformationBase* SwSwitchRouteUpdateWrapper::getRib() {
  return sw_->getRib();
}

void SwSwitchRouteUpdateWrapper::programStandAloneRib() {
  for (auto [ridClientId, addDelRoutes] : ribRoutesToAddDel_) {
    // TODO handle route update failures
    auto stats = sw_->getRib()->update(
        ridClientId.first,
        ridClientId.second,
        clientIdToAdminDistance(ridClientId.second),
        addDelRoutes.toAdd,
        addDelRoutes.toDel,
        false,
        "RIB update",
        &swSwitchFibUpdate,
        static_cast<void*>(sw_));
    updateStats(stats);
  }
}

void SwSwitchRouteUpdateWrapper::programLegacyRib() {
  auto updateFn = [this](const std::shared_ptr<SwitchState>& in) {
    auto [newState, stats] = programLegacyRibHelper(in);
    updateStats(stats);
    return newState;
  };
  sw_->updateStateBlocking("Add/Del routes", updateFn);
}

AdminDistance SwSwitchRouteUpdateWrapper::clientIdToAdminDistance(
    ClientID clientID) const {
  return sw_->clientIdToAdminDistance(static_cast<int>(clientID));
}
} // namespace facebook::fboss
