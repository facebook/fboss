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

void SwSwitchRouteUpdateWrapper::program() {
  if (sw_->isStandaloneRibEnabled()) {
    programStandAloneRib();
  } else {
    programLegacyRib();
  }
  ribRoutesToAddDel_.clear();
}

void SwSwitchRouteUpdateWrapper::programStandAloneRib() {
  for (auto [ridClientId, addDelRoutes] : ribRoutesToAddDel_) {
    auto adminDistance =
        sw_->clientIdToAdminDistance(static_cast<int>(ridClientId.second));
    // TODO handle route update failures
    auto stats = sw_->getRib()->update(
        ridClientId.first,
        ridClientId.second,
        adminDistance,
        addDelRoutes.toAdd,
        addDelRoutes.toDel,
        false,
        "RIB update",
        &swSwitchFibUpdate,
        static_cast<void*>(sw_));
    sw_->stats()->addRoutesV4(stats.v4RoutesAdded);
    sw_->stats()->addRoutesV6(stats.v6RoutesAdded);
    sw_->stats()->delRoutesV4(stats.v4RoutesDeleted);
    sw_->stats()->delRoutesV6(stats.v6RoutesDeleted);
  }
}

void SwSwitchRouteUpdateWrapper::programLegacyRib() {
  for (auto [ridClientId, addDelRoutes] : ribRoutesToAddDel_) {
      if (ridClientId.first != RouterID(0)) {
        throw FbossError("Multi-VRF only supported with Stand-Alone RIB");
      }
      auto adminDistance =
          sw_->clientIdToAdminDistance(static_cast<int>(ridClientId.second));
      auto& toAdd = addDelRoutes.toAdd;
      auto& toDel = addDelRoutes.toDel;
      RouterID routerId = ridClientId.first;
      ClientID clientId = ridClientId.second;
      auto updateFn = [&](const std::shared_ptr<SwitchState>& state) {
        RouteUpdater updater(state->getRouteTables());
        for (auto& route : toAdd) {
          std::vector<NextHopThrift> nhts;
          folly::IPAddress network =
              network::toIPAddress(*route.dest_ref()->ip_ref());
          uint8_t mask =
              static_cast<uint8_t>(*route.dest_ref()->prefixLength_ref());
          if (route.nextHops_ref()->empty() &&
              !route.nextHopAddrs_ref()->empty()) {
            nhts = thriftNextHopsFromAddresses(*route.nextHopAddrs_ref());
          } else {
            nhts = *route.nextHops_ref();
          }
          RouteNextHopSet nexthops = util::toRouteNextHopSet(nhts);
          if (nexthops.size()) {
            updater.addRoute(
                routerId,
                network,
                mask,
                clientId,
                RouteNextHopEntry(std::move(nexthops), adminDistance));
          } else {
            XLOG(DBG3) << "Blackhole route:" << network << "/"
                       << static_cast<int>(mask);
            updater.addRoute(
                routerId,
                network,
                mask,
                clientId,
                RouteNextHopEntry(RouteForwardAction::DROP, adminDistance));
          }
          if (network.isV4()) {
            sw_->stats()->addRouteV4();
          } else {
            sw_->stats()->addRouteV6();
          }
      }
      // Del routes
      for (auto& prefix : toDel) {
        auto network = network::toIPAddress(*prefix.ip_ref());
        auto mask = static_cast<uint8_t>(*prefix.prefixLength_ref());
        if (network.isV4()) {
          sw_->stats()->delRouteV4();
        } else {
          sw_->stats()->delRouteV6();
        }
        updater.delRoute(routerId, network, mask, clientId);
      }

      auto newRt = updater.updateDone();
      if (!newRt) {
        return std::shared_ptr<SwitchState>();
      }
      auto newState = state->clone();
      newState->resetRouteTables(std::move(newRt));
      return newState;
    };
    sw_->updateStateBlocking("Add/Del routes", updateFn);
  }
}

} // namespace facebook::fboss
