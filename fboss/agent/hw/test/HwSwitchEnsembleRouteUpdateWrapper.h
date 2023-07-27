/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#pragma once

#include "fboss/agent/HwSwitchRouteUpdateWrapper.h"
#include "fboss/agent/rib/RoutingInformationBase.h"
#include "fboss/agent/test/RouteDistributionGenerator.h"

namespace facebook::fboss {

class HwSwitchEnsemble;

void hwSwitchFibUpdate(
    facebook::fboss::RouterID vrf,
    const facebook::fboss::IPv4NetworkToRouteMap& v4NetworkToRoute,
    const facebook::fboss::IPv6NetworkToRouteMap& v6NetworkToRoute,
    const facebook::fboss::LabelToRouteMap& labelToRoute,
    void* cookie);

class HwSwitchEnsembleRouteUpdateWrapper : public HwSwitchRouteUpdateWrapper {
 public:
  HwSwitchEnsembleRouteUpdateWrapper(
      HwSwitchEnsemble* hwEnsemble,
      RoutingInformationBase* rib);
  HwSwitchEnsembleRouteUpdateWrapper(HwSwitchEnsembleRouteUpdateWrapper&&) =
      default;
  HwSwitchEnsembleRouteUpdateWrapper& operator=(
      HwSwitchEnsembleRouteUpdateWrapper&&) = default;

  void programRoutes(
      RouterID rid,
      ClientID client,
      const utility::RouteDistributionGenerator::ThriftRouteChunks&
          routeChunks) {
    programRoutesImpl(rid, client, routeChunks, true /* add*/);
  }

  void unprogramRoutes(
      RouterID rid,
      ClientID client,
      const utility::RouteDistributionGenerator::ThriftRouteChunks&
          routeChunks) {
    programRoutesImpl(rid, client, routeChunks, false /* del*/);
  }

 private:
  void programRoutesImpl(
      RouterID rid,
      ClientID client,
      const utility::RouteDistributionGenerator::ThriftRouteChunks& routeChunks,
      bool add);

  HwSwitchEnsemble* hwEnsemble_;
};
} // namespace facebook::fboss
