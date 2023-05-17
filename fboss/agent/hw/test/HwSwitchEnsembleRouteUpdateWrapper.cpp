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

#include "fboss/agent/state/SwitchState.h"

#include "fboss/agent/hw/test/HwSwitchEnsemble.h"

#include <folly/logging/xlog.h>
#include <memory>

namespace facebook::fboss {

std::shared_ptr<SwitchState> hwSwitchEnsembleFibUpdate(
    const SwitchIdScopeResolver* resolver,
    facebook::fboss::RouterID vrf,
    const facebook::fboss::IPv4NetworkToRouteMap& v4NetworkToRoute,
    const facebook::fboss::IPv6NetworkToRouteMap& v6NetworkToRoute,
    const facebook::fboss::LabelToRouteMap& labelToRoute,
    void* cookie) {
  facebook::fboss::ForwardingInformationBaseUpdater fibUpdater(
      nullptr, vrf, v4NetworkToRoute, v6NetworkToRoute, labelToRoute);

  auto hwEnsemble = static_cast<facebook::fboss::HwSwitchEnsemble*>(cookie);
  hwEnsemble->getHwSwitch()->transactionsSupported()
      ? hwEnsemble->applyNewStateTransaction(
            fibUpdater(hwEnsemble->getProgrammedState()))
      : hwEnsemble->applyNewState(fibUpdater(hwEnsemble->getProgrammedState()));
  return hwEnsemble->getProgrammedState();
}

HwSwitchEnsembleRouteUpdateWrapper::HwSwitchEnsembleRouteUpdateWrapper(
    HwSwitchEnsemble* hwEnsemble,
    RoutingInformationBase* rib)
    : RouteUpdateWrapper(
          &(hwEnsemble->scopeResolver()),
          rib,
          rib ? hwSwitchEnsembleFibUpdate : std::optional<FibUpdateFunction>(),
          rib ? hwEnsemble : nullptr),
      hwEnsemble_(hwEnsemble) {}

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

void HwSwitchEnsembleRouteUpdateWrapper::programRoutesImpl(
    RouterID rid,
    ClientID client,
    const utility::RouteDistributionGenerator::ThriftRouteChunks& routeChunks,
    bool add) {
  for (const auto& routeChunk : routeChunks) {
    std::for_each(
        routeChunk.begin(),
        routeChunk.end(),
        [this, client, rid, add](const auto& route) {
          if (add) {
            addRoute(rid, client, route);
          } else {
            delRoute(rid, *route.dest(), client);
          }
        });
    program();
  }
}
} // namespace facebook::fboss
