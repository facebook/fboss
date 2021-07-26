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

#include "fboss/agent/state/DeltaFunctions.h"
#include "fboss/agent/state/Route.h"

#include "fboss/agent/state/RouteTable.h"
#include "fboss/agent/state/RouteTableMap.h"
#include "fboss/agent/state/StateDelta.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/test/RouteDistributionGenerator.h"

#include <memory>
#include <string>

namespace facebook::fboss {

class Platform;

// Random test state with 2 ports and 2 vlans
cfg::SwitchConfig getTestConfig();
constexpr auto kExtraRoutes = 128 /*interface route*/ + 1 /*link local route*/;
uint64_t getRouteCount(const std::shared_ptr<SwitchState>& state);

uint64_t getRouteCount(
    const utility::RouteDistributionGenerator::RouteChunks& routeChunks);
uint64_t getRouteCount(
    const utility::RouteDistributionGenerator::ThriftRouteChunks& routeChunks);

void verifyRouteCount(
    const utility::RouteDistributionGenerator& routeDistributionGen,
    uint64_t alreadyExistingRoutes,
    uint64_t expectedNewRoutes);

void verifyChunking(
    const utility::RouteDistributionGenerator::RouteChunks& routeChunks,
    unsigned int totalRoutes,
    unsigned int chunkSize);

void verifyChunking(
    const utility::RouteDistributionGenerator& routeDistributionGen,
    unsigned int totalRoutes,
    unsigned int chunkSize);

} // namespace facebook::fboss
