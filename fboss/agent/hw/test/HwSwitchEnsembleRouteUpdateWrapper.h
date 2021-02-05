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

#include "fboss/agent/RouteUpdateWrapper.h"
#include "fboss/agent/rib/RoutingInformationBase.h"

namespace facebook::fboss {

class HwSwitchEnsemble;

void hwSwitchFibUpdate(
    facebook::fboss::RouterID vrf,
    const facebook::fboss::rib::IPv4NetworkToRouteMap& v4NetworkToRoute,
    const facebook::fboss::rib::IPv6NetworkToRouteMap& v6NetworkToRoute,
    void* cookie);

class HwSwitchEnsembleRouteUpdateWrapper : public RouteUpdateWrapper {
 public:
  explicit HwSwitchEnsembleRouteUpdateWrapper(HwSwitchEnsemble* hwEnsemble);

 private:
  void updateStats(
      const rib::RoutingInformationBase::UpdateStatistics& /*stats*/) override {
  }
  AdminDistance clientIdToAdminDistance(ClientID /*clientID*/) const override {
    return AdminDistance::MAX_ADMIN_DISTANCE;
  }
  void programLegacyRib() override;
  void programStandAloneRib() override;
  HwSwitchEnsemble* hwEnsemble_;
};
} // namespace facebook::fboss
