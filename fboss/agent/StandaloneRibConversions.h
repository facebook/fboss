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

#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/rib/RoutingInformationBase.h"
#include "fboss/agent/state/RouteTableMap.h"

#include <memory>

namespace facebook::fboss {

rib::RoutingInformationBase switchStateToStandaloneRib(
    const std::shared_ptr<RouteTableMap>& swStateRib);

std::shared_ptr<RouteTableMap> standaloneToSwitchStateRib(
    const rib::RoutingInformationBase& standaloneRib);

void syncFibWithStandaloneRib(
    rib::RoutingInformationBase& standaloneRib,
    SwSwitch* swSwitch);

} // namespace facebook::fboss
