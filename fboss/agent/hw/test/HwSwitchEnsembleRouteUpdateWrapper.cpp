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

std::shared_ptr<SwitchState> hwSwitchEnsembleFibUpdate(
    facebook::fboss::RouterID vrf,
    const facebook::fboss::IPv4NetworkToRouteMap& v4NetworkToRoute,
    const facebook::fboss::IPv6NetworkToRouteMap& v6NetworkToRoute,
    void* cookie) {
  facebook::fboss::ForwardingInformationBaseUpdater fibUpdater(
      vrf, v4NetworkToRoute, v6NetworkToRoute);

  auto hwEnsemble = static_cast<facebook::fboss::HwSwitchEnsemble*>(cookie);
  hwEnsemble->getHwSwitch()->transactionsSupported()
      ? hwEnsemble->applyNewStateTransaction(
            fibUpdater(hwEnsemble->getProgrammedState()))
      : hwEnsemble->applyNewState(fibUpdater(hwEnsemble->getProgrammedState()));
  return hwEnsemble->getProgrammedState();
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

RoutingInformationBase* HwSwitchEnsembleRouteUpdateWrapper::getRib() {
  return hwEnsemble_->getRib();
}

void HwSwitchEnsembleRouteUpdateWrapper::programLegacyRib(
    const SyncFibFor& syncFibFor) {
  auto [newState, stats] =
      programLegacyRibHelper(hwEnsemble_->getProgrammedState(), syncFibFor);
  updateStats(stats);
  hwEnsemble_->getHwSwitch()->transactionsSupported()
      ? hwEnsemble_->applyNewStateTransaction(newState)
      : hwEnsemble_->applyNewState(newState);
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

void HwSwitchEnsembleRouteUpdateWrapper::programClassIDLegacyRib(
    RouterID rid,
    const std::vector<folly::CIDRNetwork>& prefixes,
    std::optional<cfg::AclLookupClass> classId,
    bool async) {
  CHECK(!async) << "aync updates not available in Hw tests";
  hwEnsemble_->applyNewState(updateClassIdLegacyRibHelper(
      hwEnsemble_->getProgrammedState(), rid, prefixes, classId));
}

void HwSwitchEnsembleRouteUpdateWrapper::programRoutes(
    RouterID rid,
    ClientID client,
    AdminDistance admin,
    const utility::RouteDistributionGenerator::RouteChunks& routeChunks) {
  for (const auto& routeChunk : routeChunks) {
    for (const auto& route : routeChunk) {
      RouteNextHopSet nhops;
      for (const auto& ip : route.nhops) {
        nhops.emplace(UnresolvedNextHop(ip, ECMP_WEIGHT));
      }
      addRoute(
          rid,
          route.prefix.first,
          route.prefix.second,
          client,
          RouteNextHopEntry(nhops, admin));
    }
  }
  program();
}

} // namespace facebook::fboss
