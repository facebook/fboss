// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/hw/switch_asics/HwAsic.h"

namespace facebook::fboss::utility {

int getDsfNodeCount(const HwAsic* asic);

// Returns config with remote DSF node added. If numRemoteNodes is not
// specified, it will check the asic type and use max DSF node count
// (128 for J2 and 256 for J3).
std::optional<std::map<int64_t, cfg::DsfNode>> addRemoteIntfNodeCfg(
    const std::map<int64_t, cfg::DsfNode>& curDsfNodes,
    std::optional<int> numRemoteNodes = std::nullopt);

std::pair<int, cfg::DsfNode> getRemoteFabricNodeCfg(
    const std::map<int64_t, cfg::DsfNode>& curDsfNodes,
    int fabricLevel,
    int clusterId,
    cfg::AsicType asicType,
    PlatformType platformType);
} // namespace facebook::fboss::utility
