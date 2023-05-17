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
class SwSwitch;
class SwitchIdScopeResolver;

std::shared_ptr<SwitchState> swSwitchFibUpdate(
    const facebook::fboss::SwitchIdScopeResolver* resolver,
    facebook::fboss::RouterID vrf,
    const facebook::fboss::IPv4NetworkToRouteMap& v4NetworkToRoute,
    const facebook::fboss::IPv6NetworkToRouteMap& v6NetworkToRoute,
    const facebook::fboss::LabelToRouteMap& labelToRoute,
    void* cookie);

class SwSwitchRouteUpdateWrapper : public RouteUpdateWrapper {
 public:
  explicit SwSwitchRouteUpdateWrapper(
      SwSwitch* sw,
      RoutingInformationBase* rib);

 private:
  AdminDistance clientIdToAdminDistance(ClientID clientID) const override;
  void updateStats(
      const RoutingInformationBase::UpdateStatistics& stats) override;
  SwSwitch* sw_;
};
} // namespace facebook::fboss
