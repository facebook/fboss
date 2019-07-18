/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/test/AgentConfigFactory.h"
#include <folly/CppAttributes.h>
#include <folly/Format.h>
#include <folly/logging/xlog.h>
#include "fboss/agent/gen-cpp2/platform_config_types.h"
#include "fboss/lib/phy/gen-cpp2/phy_types.h"

namespace facebook {
namespace fboss {
namespace utility {

cfg::PlatformPortSettings getPlatformPortSettingsConfig(
    uint16_t portId,
    uint16_t numOfLanes) {
  cfg::PlatformPortSettings platformPortSettings;
  phy::LaneSettings laneSettings;
  platformPortSettings.__isset.iphy = true;
  for (auto i = 0; i < numOfLanes; i++) {
    platformPortSettings.iphy_ref()->lanes.emplace(portId + i, laneSettings);
    if (i > 0) {
      platformPortSettings.subsumedPorts.push_back(portId + i);
    }
  }
  return platformPortSettings;
}

std::unordered_map<cfg::PortSpeed, uint16_t> getValidPortSettings() {
  // TODO(srikrishnagopu): This currently works with 4 lanes each is 25G serdes.
  // Use platform to determine the model and max lanes.
  std::unordered_map<cfg::PortSpeed, uint16_t> speedLaneMap;
  speedLaneMap.insert(std::make_pair(cfg::PortSpeed::XG, 1));
  speedLaneMap.insert(std::make_pair(cfg::PortSpeed::TWENTYFIVEG, 1));
  speedLaneMap.insert(std::make_pair(cfg::PortSpeed::FIFTYG, 2));
  speedLaneMap.insert(std::make_pair(cfg::PortSpeed::HUNDREDG, 4));
  return speedLaneMap;
}

cfg::PlatformPort getPlatformPortConfig(uint16_t portId) {
  cfg::PlatformPort platformPort;
  platformPort.id = portId;
  platformPort.name = "port" + std::to_string(portId);
  auto speedLaneMap = getValidPortSettings();
  for (auto speedLane : speedLaneMap) {
    auto portSettings = getPlatformPortSettingsConfig(portId, speedLane.second);
    platformPort.supportedSpeeds.emplace(speedLane.first, portSettings);
  }
  return platformPort;
}

cfg::AgentConfig getAgentConfig() {
  cfg::AgentConfig config;
  for (uint16_t portId = 0; portId < kMaxPorts; portId++) {
    auto platformPortConfig = getPlatformPortConfig(portId);
    config.__isset.platform = true;
    config.platform_ref()->ports.emplace(portId, platformPortConfig);
  }
  return config;
}

} // namespace utility
} // namespace fboss
} // namespace facebook
