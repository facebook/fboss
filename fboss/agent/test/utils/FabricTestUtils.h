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

#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/test/TestEnsembleIf.h"
#include "fboss/agent/types.h"

namespace facebook::fboss::utility {
void checkFabricReachability(TestEnsembleIf* ensemble, SwitchID switchId);
void checkFabricReachabilityStats(TestEnsembleIf* ensemble, SwitchID switchId);
void populatePortExpectedNeighbors(
    const std::vector<PortID>& ports,
    cfg::SwitchConfig& cfg);
void checkPortFabricReachability(
    TestEnsembleIf* ensemble,
    SwitchID switchId,
    PortID portId);
} // namespace facebook::fboss::utility
