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
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/types.h"
#include "fboss/lib/phy/ExternalPhy.h"
#include "fboss/lib/phy/gen-cpp2/phy_types.h"

namespace facebook::fboss {
using TcvrToPortMap = std::unordered_map<TransceiverID, std::vector<PortID>>;
using PortToTcvrMap = std::unordered_map<PortID, std::vector<TransceiverID>>;

namespace utility {

std::vector<phy::PinID> getTransceiverLanes(
    const cfg::PlatformPortEntry& port,
    const std::map<std::string, phy::DataPlanePhyChip>& chipsMap,
    std::optional<cfg::PortProfileID> profileID = std::nullopt);

std::map<int32_t, phy::LaneConfig> getIphyLaneConfigs(
    const std::vector<phy::PinConfig>& iphyPinConfigs);

std::vector<phy::PinID> getOrderedIphyLanes(
    const cfg::PlatformPortEntry& port,
    const std::map<std::string, phy::DataPlanePhyChip>& chipsMap,
    std::optional<cfg::PortProfileID> profileID = std::nullopt);

// Get subsidiary PortID list based on controlling port
std::map<PortID, std::vector<PortID>> getSubsidiaryPortIDs(
    const std::map<int32_t, cfg::PlatformPortEntry>& platformPorts);

std::vector<cfg::PlatformPortEntry> getPlatformPortsByControllingPort(
    const std::map<int32_t, cfg::PlatformPortEntry>& platformPorts,
    PortID controllingPort);

std::vector<cfg::PlatformPortEntry> getPlatformPortsByChip(
    const std::map<int32_t, cfg::PlatformPortEntry>& platformPorts,
    const phy::DataPlanePhyChip& chip);

std::vector<phy::PinConfig> getPinsFromPortPinConfig(
    const phy::PortPinConfig& pinConfig,
    phy::DataPlanePhyChipType chipType);

std::map<std::string, phy::DataPlanePhyChip> getDataPlanePhyChips(
    const cfg::PlatformPortEntry& port,
    const std::map<std::string, phy::DataPlanePhyChip>& chipsMap,
    std::optional<phy::DataPlanePhyChipType> chipType = std::nullopt,
    const std::optional<cfg::PortProfileID> profileID = std::nullopt);

std::map<int32_t, phy::PolaritySwap> getXphyLinePolaritySwapMap(
    const cfg::PlatformPortEntry& platformPortEntry,
    cfg::PortProfileID profileID,
    const std::map<std::string, phy::DataPlanePhyChip>& chipsMap,
    const phy::PortProfileConfig& portProfileConfig);

const std::vector<TransceiverID> getTransceiverIds(
    const cfg::PlatformPortEntry& port,
    const std::map<std::string, phy::DataPlanePhyChip>& chipsMap,
    const std::optional<cfg::PortProfileID> profileID = std::nullopt);

std::vector<TransceiverID> getTransceiverIds(
    const std::map<std::string, phy::DataPlanePhyChip>& chipsMap);

std::vector<uint32_t> getHwPortLanes(
    cfg::PlatformPortEntry& platformPort,
    cfg::PortProfileID profileID,
    const std::map<std::string, phy::DataPlanePhyChip>& dataPlanePhyChips,
    std::function<uint32_t(uint32_t, uint32_t)>&& getPhysicalLaneId);

/*
 * Get a mapping of TransceiverID to a list of PortIDs that transfer data
 * through this transceiver.
 */
TcvrToPortMap getTcvrToPortMap(
    const std::map<int32_t, cfg::PlatformPortEntry>& platformPorts,
    const std::map<std::string, phy::DataPlanePhyChip>& chipsMap);

/*
 * Get a mapping of PortID to a list of TransceiverIDs that transfer data
 * for this port.
 *
 * Right now, platform mapping does not support multiple transceivers per port,
 * so this mapping will always be 1:1.
 */
PortToTcvrMap getPortToTcvrMap(
    const std::map<int32_t, cfg::PlatformPortEntry>& platformPorts,
    const std::map<std::string, phy::DataPlanePhyChip>& chipsMap);

} // namespace utility
} // namespace facebook::fboss
