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

#include <string>
#include <vector>
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/if/gen-cpp2/ctrl_types.h"

namespace facebook::fboss::utility {

bool isDualStage(const std::map<int64_t, cfg::DsfNode>& dsfNodeMap);

std::vector<std::pair<int64_t, std::string>> deviceToQueryInputCapacity(
    const std::vector<int64_t>& fabricSwitchIDs,
    const std::map<int64_t, cfg::DsfNode>& dsfNodeMap);

// TODO(zecheng): Create the same util for port map
std::map<std::string, std::vector<std::string>> getNeighborFabricPortsToSelf(
    const std::map<int32_t, PortInfoThrift>& myPortInfo);

} // namespace facebook::fboss::utility
