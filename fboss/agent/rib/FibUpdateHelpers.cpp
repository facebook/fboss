/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/rib/FibUpdateHelpers.h"

#include "fboss/agent/SwitchIdScopeResolver.h"
#include "fboss/agent/rib/ForwardingInformationBaseUpdater.h"
#include "fboss/agent/rib/NetworkToRouteMap.h"
#include "fboss/agent/state/SwitchState.h"

namespace facebook::fboss {
std::shared_ptr<facebook::fboss::SwitchState> ribToSwitchStateUpdate(
    const SwitchIdScopeResolver* resolver,
    facebook::fboss::RouterID vrf,
    const IPv4NetworkToRouteMap& v4NetworkToRoute,
    const IPv6NetworkToRouteMap& v6NetworkToRoute,
    const LabelToRouteMap& labelToRoute,
    void* cookie) {
  ForwardingInformationBaseUpdater fibUpdater(
      resolver, vrf, v4NetworkToRoute, v6NetworkToRoute, labelToRoute);

  auto switchState =
      static_cast<std::shared_ptr<facebook::fboss::SwitchState>*>(cookie);
  *switchState = fibUpdater(*switchState);
  (*switchState)->publish();
  return *switchState;
}

std::shared_ptr<facebook::fboss::SwitchState> noopFibUpdate(
    const SwitchIdScopeResolver* resolver,
    facebook::fboss::RouterID /*vrf*/,
    const IPv4NetworkToRouteMap& /*v4NetworkToRoute*/,
    const IPv6NetworkToRouteMap& /*v6NetworkToRoute*/,
    const LabelToRouteMap& /*labelToRoute*/,
    void* /*cookie*/) {
  return nullptr;
}

} // namespace facebook::fboss
