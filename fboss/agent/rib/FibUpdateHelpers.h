/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/types.h"

#include "fboss/agent/rib/NetworkToRouteMap.h"

#include <memory>

namespace facebook::fboss {

class SwitchState;
class SwitchIdScopeResolver;

std::shared_ptr<SwitchState> ribToSwitchStateUpdate(
    const SwitchIdScopeResolver* resolver,
    RouterID vrf,
    const IPv4NetworkToRouteMap& v4NetworkToRoute,
    const IPv6NetworkToRouteMap& v6NetworkToRoute,
    const LabelToRouteMap& labelToRoute,
    void* cookie);

std::shared_ptr<SwitchState> noopFibUpdate(
    const SwitchIdScopeResolver* resolver,
    RouterID vrf,
    const IPv4NetworkToRouteMap& v4NetworkToRoute,
    const IPv6NetworkToRouteMap& v6NetworkToRoute,
    const LabelToRouteMap& labelToRoute,
    void* cookie);
} // namespace facebook::fboss
