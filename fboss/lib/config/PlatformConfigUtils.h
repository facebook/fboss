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

#include "fboss/agent/gen-cpp2/platform_config_types.h"
#include "fboss/agent/types.h"
#include "fboss/lib/phy/ExternalPhy.h"
#include "fboss/lib/phy/gen-cpp2/phy_types.h"

namespace facebook::fboss::utility {

std::vector<phy::PinID> getTransceiverLanes(
    const cfg::PlatformPortEntry& port,
    const std::vector<phy::DataPlanePhyChip>& chips,
    std::optional<cfg::PortProfileID> profileID = std::nullopt);

std::map<int32_t, phy::LaneConfig> getIphyLaneConfigs(
    const std::vector<phy::PinConnection>& pinConnections,
    const std::vector<phy::PinConfig>& iphyPinConfigs);

std::vector<phy::PinID> getOrderedIphyLanes(
    const cfg::PlatformPortEntry& port,
    const std::vector<phy::DataPlanePhyChip>& chips,
    std::optional<cfg::PortProfileID> profileID = std::nullopt);

// Get subsidiary PortID list based on controlling port
std::map<PortID, std::vector<PortID>> getSubsidiaryPortIDs(
    const facebook::fboss::cfg::PlatformConfig& platformCfg);

std::vector<cfg::PlatformPortEntry> getPlatformPortsByControllingPort(
    const std::map<int32_t, cfg::PlatformPortEntry>& platformPorts,
    PortID controllingPort);

std::optional<phy::DataPlanePhyChip> getXphyChip(
    const cfg::PlatformPortEntry& port,
    const std::vector<phy::DataPlanePhyChip>& chips);

std::map<int32_t, phy::PolaritySwap> getXphyLinePolaritySwapMap(
    const std::vector<phy::PinConnection>& pinConnections,
    const std::vector<phy::DataPlanePhyChip>& chips);

} // namespace facebook::fboss::utility
