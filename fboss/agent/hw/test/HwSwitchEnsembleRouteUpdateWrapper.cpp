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
    : RouteUpdateWrapper(
          hwEnsemble->isStandaloneRibEnabled(),
          hwEnsemble->isStandaloneRibEnabled()
              ? hwSwitchEnsembleFibUpdate
              : std::optional<FibUpdateFunction>(),
          hwEnsemble->isStandaloneRibEnabled() ? hwEnsemble : nullptr),
      hwEnsemble_(hwEnsemble) {}

rib::RoutingInformationBase* HwSwitchEnsembleRouteUpdateWrapper::getRib() {
  return hwEnsemble_->getRib();
}

void HwSwitchEnsembleRouteUpdateWrapper::programLegacyRib() {
  auto [newState, stats] =
      programLegacyRibHelper(hwEnsemble_->getProgrammedState());
  updateStats(stats);
  hwEnsemble_->applyNewState(newState);
}

AdminDistance HwSwitchEnsembleRouteUpdateWrapper::clientIdToAdminDistance(
    ClientID clientId) const {
  static const std::map<ClientID, AdminDistance> kClient2Admin = {
      {ClientID::BGPD, AdminDistance::EBGP},
      {ClientID::OPENR, AdminDistance::OPENR},
      {ClientID::STATIC_ROUTE, AdminDistance::STATIC_ROUTE},
  };
  auto itr = kClient2Admin.find(clientId);
  return itr == kClient2Admin.end() ? AdminDistance::MAX_ADMIN_DISTANCE
                                    : itr->second;
}
} // namespace facebook::fboss
