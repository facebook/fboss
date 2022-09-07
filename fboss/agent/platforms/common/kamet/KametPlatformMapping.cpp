/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/platforms/common/kamet/KametPlatformMapping.h"

namespace facebook::fboss {
namespace {
constexpr auto kJsonPlatformMappingStr = R"(
{
}
)";
cfg::PlatformMapping buildMapping() {
  cfg::PlatformMapping platformMapping;
  for (auto i = 0; i < 192; ++i) {
    // 4 lanes of each Blackhawk core are routed to one of the
    // 2 Beas chips on Kamet platform
    if (i % 4 == 0) {
      phy::DataPlanePhyChip chip;
      // TODO make BC core id map front panel mapping. So BC0 becomes
      // first front panel port
      chip.name() = folly::to<std::string>("BC", i / 4);
      chip.type() = phy::DataPlanePhyChipType::IPHY;
      chip.physicalID() = i / 4;
      platformMapping.chips()->push_back(chip);
    }
    cfg::PlatformPortEntry portEntry;
    portEntry.mapping()->id() = i;
    // FIXME - fab port id does not match front panel port
    portEntry.mapping()->name() = folly::to<std::string>("fab1/", i + 1, "/1");
    portEntry.mapping()->controllingPort() = i;
    phy::PinConnection pinConnect;
    phy::PinID asicSerdesPinId;
    asicSerdesPinId.chip() = *platformMapping.chips()->back().name();
    asicSerdesPinId.lane() = i % 4;
    pinConnect.a() = asicSerdesPinId;
    // FIXME - get z end and polarity swap info
    portEntry.mapping()->pins()->push_back(pinConnect);
    portEntry.mapping()->portType() = cfg::PortType::FABRIC_PORT;
    cfg::PlatformPortConfig portConfig;
    portConfig.subsumedPorts() = std::vector<int32_t>({i});
    phy::PinConfig pinConfig;
    pinConfig.id() = asicSerdesPinId;
    phy::PortPinConfig portPinConfig;
    portPinConfig.iphy()->push_back(pinConfig);
    portConfig.pins() = portPinConfig;
    portEntry.supportedProfiles()->insert(
        {cfg::PortProfileID::PROFILE_53POINT125G_1_PAM4_RS545_COPPER,
         portConfig});
    platformMapping.ports()->emplace(i, std::move(portEntry));
  }
  // Fill in supported profiles
  cfg::PlatformPortConfigFactor configFactor;
  configFactor.profileID() =
      cfg::PortProfileID::PROFILE_53POINT125G_1_PAM4_RS545_COPPER;
  phy::ProfileSideConfig profileSideConfig;
  profileSideConfig.numLanes() = 1;
  profileSideConfig.modulation() = phy::IpModulation::PAM4;
  // TODO - set to RS545 fec
  profileSideConfig.fec() = phy::FecMode::NONE;
  profileSideConfig.medium() = TransmitterTechnology::COPPER;

  phy::PortProfileConfig profileConfig;
  profileConfig.speed() = cfg::PortSpeed::FIFTYTHREEPOINTONETWOFIVEG;
  profileConfig.iphy() = profileSideConfig;

  cfg::PlatformPortProfileConfigEntry portProfileConfigEntry;
  portProfileConfigEntry.factor() = configFactor;
  portProfileConfigEntry.profile() = profileConfig;
  platformMapping.platformSupportedProfiles()->push_back(
      portProfileConfigEntry);

  return platformMapping;
}
} // namespace

KametPlatformMapping::KametPlatformMapping()
    : PlatformMapping(buildMapping()) {}

} // namespace facebook::fboss
