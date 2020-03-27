/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/test/FakeAgentConfigFactory.h"
#include <folly/CppAttributes.h>
#include <folly/Format.h>
#include <folly/logging/xlog.h>
#include "fboss/agent/gen-cpp2/platform_config_types.h"
#include "fboss/lib/phy/gen-cpp2/phy_types.h"

namespace facebook::fboss::utility {

std::map<int, std::vector<cfg::PortProfileID>> getPortProfileIds() {
  return {{0,
           {
               cfg::PortProfileID::PROFILE_100G_4_NRZ_CL91,
               cfg::PortProfileID::PROFILE_50G_2_NRZ_NOFEC,
               cfg::PortProfileID::PROFILE_40G_4_NRZ_NOFEC,
               cfg::PortProfileID::PROFILE_25G_1_NRZ_NOFEC,
               cfg::PortProfileID::PROFILE_10G_1_NRZ_NOFEC,
           }},
          {1,
           {
               cfg::PortProfileID::PROFILE_25G_1_NRZ_NOFEC,
               cfg::PortProfileID::PROFILE_10G_1_NRZ_NOFEC,
           }},
          {2,
           {
               cfg::PortProfileID::PROFILE_50G_2_NRZ_NOFEC,
               cfg::PortProfileID::PROFILE_25G_1_NRZ_NOFEC,
               cfg::PortProfileID::PROFILE_10G_1_NRZ_NOFEC,
           }},
          {3,
           {
               cfg::PortProfileID::PROFILE_25G_1_NRZ_NOFEC,
               cfg::PortProfileID::PROFILE_10G_1_NRZ_NOFEC,
           }}};
}

std::unordered_map<cfg::PortProfileID, phy::PortProfileConfig>
getSupportedProfiles() {
  static std::unordered_map<cfg::PortProfileID, phy::PortProfileConfig>
      speedProfileMap;
  phy::PortProfileConfig portProfileConfig;
  portProfileConfig.speed = cfg::PortSpeed::XG;
  portProfileConfig.iphy.numLanes = 1;
  speedProfileMap.emplace(
      cfg::PortProfileID::PROFILE_10G_1_NRZ_NOFEC, portProfileConfig);
  portProfileConfig.speed = cfg::PortSpeed::TWENTYFIVEG;
  portProfileConfig.iphy.numLanes = 1;
  speedProfileMap.emplace(
      cfg::PortProfileID::PROFILE_25G_1_NRZ_NOFEC, portProfileConfig);
  portProfileConfig.speed = cfg::PortSpeed::FORTYG;
  portProfileConfig.iphy.numLanes = 4;
  speedProfileMap.emplace(
      cfg::PortProfileID::PROFILE_40G_4_NRZ_NOFEC, portProfileConfig);
  portProfileConfig.speed = cfg::PortSpeed::FIFTYG;
  portProfileConfig.iphy.numLanes = 2;
  speedProfileMap.emplace(
      cfg::PortProfileID::PROFILE_50G_2_NRZ_NOFEC, portProfileConfig);
  portProfileConfig.speed = cfg::PortSpeed::HUNDREDG;
  portProfileConfig.iphy.numLanes = 4;
  speedProfileMap.emplace(
      cfg::PortProfileID::PROFILE_100G_4_NRZ_CL91, portProfileConfig);
  return speedProfileMap;
}

cfg::PlatformPortConfig getPlatformPortConfig(
    uint16_t portId,
    cfg::PortProfileID profileId) {
  cfg::PlatformPortConfig platformPortConfig;
  auto supportedProfiles = getSupportedProfiles();
  auto speedProfileIter = supportedProfiles.find(profileId);
  platformPortConfig.subsumedPorts_ref() = {};
  for (int i = 1; i < speedProfileIter->second.iphy.numLanes; i++) {
    platformPortConfig.subsumedPorts_ref()->push_back(portId + i);
  }
  for (auto i = 0; i < speedProfileIter->second.iphy.numLanes; i++) {
    phy::PinConfig pinConfig;
    pinConfig.id.chip = chipName;
    pinConfig.id.lane = portId + i;
    platformPortConfig.pins.iphy_ref()->push_back(pinConfig);
  }
  return platformPortConfig;
}

std::vector<cfg::PlatformPortEntry> getPlatformPortEntries(
    uint16_t controllingPort) {
  std::vector<cfg::PlatformPortEntry> platformPortEntries;
  for (auto portProfiles : getPortProfileIds()) {
    PortID portId = PortID(controllingPort + portProfiles.first);
    cfg::PlatformPortEntry platformPortEntry;
    platformPortEntry.mapping.id = portId;
    platformPortEntry.mapping.name = "port" + std::to_string(portId);
    platformPortEntry.mapping.controllingPort = controllingPort;
    for (auto profileId : portProfiles.second) {
      auto supportedProfile = getPlatformPortConfig(portId, profileId);
      platformPortEntry.supportedProfiles.emplace(profileId, supportedProfile);
    }
    platformPortEntries.push_back(platformPortEntry);
  }
  return platformPortEntries;
}

cfg::AgentConfig getFakeAgentConfig() {
  cfg::AgentConfig config;
  config.platform.platformPorts_ref() = {};
  for (uint16_t portId = 0; portId < kMaxPorts; portId += kMaxLanes) {
    auto platformPortEntries = getPlatformPortEntries(portId);
    for (int lane = 0; lane < kMaxLanes; lane++) {
      config.platform.platformPorts_ref()->emplace(
          portId + lane, platformPortEntries[lane]);
    }
  }
  config.platform.supportedProfiles_ref() = {};
  for (auto supportedProfile : getSupportedProfiles()) {
    config.platform.supportedProfiles_ref()->emplace(supportedProfile);
  }
  phy::DataPlanePhyChip chip;
  chip.name = chipName;
  chip.type = phy::DataPlanePhyChipType::IPHY;
  chip.physicalID = 0;
  config.platform.chips_ref() = {};
  config.platform.chips_ref()->push_back(chip);

  config.platform.platformSettings.insert(std::make_pair(
      cfg::PlatformAttributes::CONNECTION_HANDLE, "test connection handle"));
  return config;
}

} // namespace facebook::fboss::utility
