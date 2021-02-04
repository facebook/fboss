/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/test/HwSwitchEnsembleRouteUpdateWrapper.h"

#include "fboss/agent/Utils.h"
#include "fboss/agent/if/gen-cpp2/ctrl_types.h"
#include "fboss/agent/rib/ForwardingInformationBaseUpdater.h"
#include "fboss/agent/state/RouteUpdater.h"
#include "fboss/agent/state/SwitchState.h"

#include "fboss/agent/hw/test/HwSwitchEnsemble.h"

#include <folly/logging/xlog.h>
#include <memory>

namespace facebook::fboss {

void hwSwitchEnsembleFibUpdate(
    facebook::fboss::RouterID vrf,
    const facebook::fboss::rib::IPv4NetworkToRouteMap& v4NetworkToRoute,
    const facebook::fboss::rib::IPv6NetworkToRouteMap& v6NetworkToRoute,
    void* cookie) {
  facebook::fboss::rib::ForwardingInformationBaseUpdater fibUpdater(
      vrf, v4NetworkToRoute, v6NetworkToRoute);

  auto hwEnsemble = static_cast<facebook::fboss::HwSwitchEnsemble*>(cookie);
  hwEnsemble->applyNewState(fibUpdater(hwEnsemble->getProgrammedState()));
}

HwSwitchEnsembleRouteUpdateWrapper::HwSwitchEnsembleRouteUpdateWrapper(
    HwSwitchEnsemble* hwEnsemble)
    : RouteUpdateWrapper(hwEnsemble->isStandaloneRibEnabled()),
      hwEnsemble_(hwEnsemble) {}

void HwSwitchEnsembleRouteUpdateWrapper::programStandAloneRib() {
  for (auto [ridClientId, addDelRoutes] : ribRoutesToAddDel_) {
    // TODO handle route update failures
    hwEnsemble_->getRib()->update(
        ridClientId.first,
        ridClientId.second,
        AdminDistance::MAX_ADMIN_DISTANCE,
        addDelRoutes.toAdd,
        addDelRoutes.toDel,
        false,
        "RIB update",
        &hwSwitchEnsembleFibUpdate,
        static_cast<void*>(hwEnsemble_));
  }
}

void HwSwitchEnsembleRouteUpdateWrapper::programLegacyRib() {
  for (auto [ridClientId, addDelRoutes] : ribRoutesToAddDel_) {
    if (ridClientId.first != RouterID(0)) {
      throw FbossError("Multi-VRF only supported with Stand-Alone RIB");
    }
    auto& toAdd = addDelRoutes.toAdd;
    auto& toDel = addDelRoutes.toDel;
    RouterID routerId = ridClientId.first;
    ClientID clientId = ridClientId.second;
    auto state = hwEnsemble_->getProgrammedState();
    RouteUpdater updater(state->getRouteTables());
    for (auto& route : toAdd) {
      std::vector<NextHopThrift> nhts;
      folly::IPAddress network =
          network::toIPAddress(*route.dest_ref()->ip_ref());
      uint8_t mask =
          static_cast<uint8_t>(*route.dest_ref()->prefixLength_ref());
      if (route.nextHops_ref()->empty() && !route.nextHopAddrs_ref()->empty()) {
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
            RouteNextHopEntry(
                std::move(nexthops), AdminDistance::MAX_ADMIN_DISTANCE));
      } else {
        XLOG(DBG3) << "Blackhole route:" << network << "/"
                   << static_cast<int>(mask);
        updater.addRoute(
            routerId,
            network,
            mask,
            clientId,
            RouteNextHopEntry(
                RouteForwardAction::DROP, AdminDistance::MAX_ADMIN_DISTANCE));
      }
    }
    // Del routes
    for (auto& prefix : toDel) {
      auto network = network::toIPAddress(*prefix.ip_ref());
      auto mask = static_cast<uint8_t>(*prefix.prefixLength_ref());
      updater.delRoute(routerId, network, mask, clientId);
    }

    auto newRt = updater.updateDone();
    if (!newRt) {
      return;
    }
    auto newState = state->clone();
    newState->resetRouteTables(std::move(newRt));
    hwEnsemble_->applyNewState(newState);
  }
}

} // namespace facebook::fboss
