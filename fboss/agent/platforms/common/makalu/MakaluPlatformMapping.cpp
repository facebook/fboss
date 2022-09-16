/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/platforms/common/makalu/MakaluPlatformMapping.h"

namespace facebook::fboss {
namespace {
struct NifInfo {
  int lane;
  int coreId;
  int corePortIndex;
};

cfg::PlatformMapping buildMapping() {
  cfg::PlatformMapping platformMapping;
  // Fabric port mapping. For fabric ports only 40 out of the 48
  // backplane ports are routed to front panel. However the ASIC
  // returns all ports so we need to create lane mappings for them
  for (auto lane = 0, port = 256; lane < 192; ++lane, ++port) {
    // 4 lanes of each Blackhawk core are routed to one of the
    // 2 Indus chips on Makalu platform
    if (lane % 4 == 0) {
      phy::DataPlanePhyChip chip;
      // TODO - Make BC core id map front panel mapping. So BC0 becomes
      // first front panel port
      chip.name() = folly::to<std::string>("BC", lane / 4);
      chip.type() = phy::DataPlanePhyChipType::IPHY;
      chip.physicalID() = lane / 4;
      platformMapping.chips()->push_back(chip);
    }
    cfg::PlatformPortEntry portEntry;
    portEntry.mapping()->id() = port;
    // FIXME - fab port id does not match front panel port
    portEntry.mapping()->name() = folly::to<std::string>("fab1/", port, "/1");
    portEntry.mapping()->controllingPort() = port;
    phy::PinConnection pinConnect;
    phy::PinID asicSerdesPinId;
    asicSerdesPinId.chip() = *platformMapping.chips()->back().name();
    asicSerdesPinId.lane() = lane % 4;
    pinConnect.a() = asicSerdesPinId;
    // FIXME - get z end and polarity swap info
    portEntry.mapping()->pins()->push_back(pinConnect);
    portEntry.mapping()->portType() = cfg::PortType::FABRIC_PORT;
    cfg::PlatformPortConfig portConfig;
    portConfig.subsumedPorts() = std::vector<int32_t>({port});
    phy::PinConfig pinConfig;
    pinConfig.id() = asicSerdesPinId;
    phy::PortPinConfig portPinConfig;
    portPinConfig.iphy()->push_back(pinConfig);
    portConfig.pins() = portPinConfig;
    portEntry.supportedProfiles()->insert(
        {cfg::PortProfileID::PROFILE_53POINT125G_1_PAM4_RS545_COPPER,
         portConfig});
    platformMapping.ports()->emplace(port, std::move(portEntry));
  }
  auto nifBcCoreId = 49;
  std::map<int, NifInfo> nif2NifInfo = {
      {1, {80, 1, 1}},
      {17, {88, 1, 17}},
      {25, {96, 1, 25}},
      {33, {104, 1, 33}},
      {41, {112, 1, 41}},
      {49, {120, 1, 49}},
      {57, {128, 1, 57}},
      {65, {136, 1, 65}},
      {73, {64, 0, 73}},
      {81, {56, 0, 81}},
      {89, {48, 0, 89}},
      {97, {40, 0, 97}},
      {105, {32, 0, 105}},
      {113, {24, 0, 113}},
      {121, {16, 0, 121}},
      {129, {8, 0, 129}}};
  for (auto& [port, nifInfo] : nif2NifInfo) {
    auto [lane, core, corePortIndex] = nifInfo;
    phy::DataPlanePhyChip chip;
    // TODO - Make BC core id map front panel mapping. So BC0 becomes
    // first front panel port
    chip.name() = folly::to<std::string>("BC", nifBcCoreId++);
    chip.type() = phy::DataPlanePhyChipType::IPHY;
    chip.physicalID() = lane / 4;
    platformMapping.chips()->push_back(chip);
    cfg::PlatformPortEntry portEntry;
    portEntry.mapping()->id() = port;
    // FIXME - eth port id does not match front panel port
    portEntry.mapping()->name() = folly::to<std::string>("eth1/", port, "/1");
    portEntry.mapping()->controllingPort() = port;
    portEntry.mapping()->attachedCoreId() = core;
    portEntry.mapping()->attachedCorePortIndex() = corePortIndex;
    phy::PortPinConfig portPinConfig;
    for (auto pinIdx = 0; pinIdx < 4; ++pinIdx) {
      phy::PinConnection pinConnect;
      phy::PinID asicSerdesPinId;
      asicSerdesPinId.chip() = *platformMapping.chips()->back().name();
      asicSerdesPinId.lane() = pinIdx;
      pinConnect.a() = asicSerdesPinId;
      // FIXME - get z end and polarity swap info
      portEntry.mapping()->pins()->push_back(pinConnect);
      // Populate phy.PortPinConfig
      phy::PinConfig pinConfig;
      pinConfig.id() = asicSerdesPinId;
      portPinConfig.iphy()->push_back(pinConfig);
    }
    portEntry.mapping()->portType() = cfg::PortType::INTERFACE_PORT;
    cfg::PlatformPortConfig portConfig;
    portConfig.subsumedPorts() =
        std::vector<int32_t>({port + 1, port + 2, port + 3});
    portConfig.pins() = portPinConfig;
    portEntry.supportedProfiles()->insert(
        {cfg::PortProfileID::PROFILE_100G_4_NRZ_RS528_OPTICAL, portConfig});
    portEntry.supportedProfiles()->insert(
        {cfg::PortProfileID::PROFILE_100G_4_NRZ_RS528_COPPER, portConfig});
    platformMapping.ports()->emplace(port, std::move(portEntry));
  }
  // Fill in supported profiles
  {
    // Fabric port profile
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
  }
  {
    // NIF port profile
    cfg::PlatformPortConfigFactor configFactor;
    configFactor.profileID() =
        cfg::PortProfileID::PROFILE_100G_4_NRZ_RS528_OPTICAL;
    phy::ProfileSideConfig profileSideConfig;
    profileSideConfig.numLanes() = 4;
    profileSideConfig.modulation() = phy::IpModulation::PAM4;
    // TODO - set to RS528 fec
    profileSideConfig.fec() = phy::FecMode::NONE;
    profileSideConfig.medium() = TransmitterTechnology::OPTICAL;

    phy::PortProfileConfig profileConfig;
    profileConfig.speed() = cfg::PortSpeed::HUNDREDG;
    profileConfig.iphy() = profileSideConfig;

    {
      cfg::PlatformPortProfileConfigEntry portProfileConfigEntry;
      portProfileConfigEntry.factor() = configFactor;
      portProfileConfigEntry.profile() = profileConfig;
      platformMapping.platformSupportedProfiles()->push_back(
          portProfileConfigEntry);
    }
    // 100G copper
    configFactor.profileID() =
        cfg::PortProfileID::PROFILE_100G_4_NRZ_RS528_COPPER;
    profileSideConfig.medium() = TransmitterTechnology::COPPER;
    profileConfig.iphy() = profileSideConfig;
    {
      cfg::PlatformPortProfileConfigEntry portProfileConfigEntry;
      portProfileConfigEntry.factor() = configFactor;
      portProfileConfigEntry.profile() = profileConfig;
      platformMapping.platformSupportedProfiles()->push_back(
          portProfileConfigEntry);
    }
  }
  return platformMapping;
}
} // namespace

MakaluPlatformMapping::MakaluPlatformMapping()
    : PlatformMapping(buildMapping()) {}

} // namespace facebook::fboss
