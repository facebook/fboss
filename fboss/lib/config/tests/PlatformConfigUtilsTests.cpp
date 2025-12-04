/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include <gtest/gtest.h>

#include "fboss/agent/FbossError.h"
#include "fboss/lib/config/PlatformConfigUtils.h"

namespace {
using namespace facebook::fboss;
constexpr int kDefaultAsicLaneStart = 6;
constexpr auto kDefaultIphyChipName = "BC0";
constexpr auto kDefaultTcvrChipName = "eth2/2";
constexpr auto kDefaultXphyChipName = "XPHY8";

cfg::PlatformPortEntry getPlatformPortEntryWithXPHY() {
  cfg::PlatformPortEntry entry;

  *entry.mapping()->id() = 4;
  *entry.mapping()->name() = "eth2/2/1";
  *entry.mapping()->controllingPort() = 3;

  int xphySysLaneStart = 6;
  int xphyLineLaneStart = 12;
  for (int i = 0; i < 2; i++) {
    phy::PinConnection pin;
    phy::PinID iphy;
    *iphy.chip() = kDefaultIphyChipName;
    *iphy.lane() = kDefaultAsicLaneStart + i;
    *pin.a() = iphy;

    phy::Pin xphyPin;
    phy::PinJunction xphy;
    phy::PinID xphySys;
    *xphySys.chip() = kDefaultXphyChipName;
    *xphySys.lane() = xphySysLaneStart + i;
    *xphy.system() = xphySys;

    for (int j = 0; j < 2; j++) {
      phy::PinConnection linePin;
      phy::PinID xphyLine;
      *xphyLine.chip() = kDefaultXphyChipName;
      *xphyLine.lane() = xphyLineLaneStart + i * 2 + j;
      *linePin.a() = xphyLine;

      phy::Pin tcvrPin;
      phy::PinID tcvr;
      *tcvr.chip() = kDefaultTcvrChipName;
      *tcvr.lane() = i * 2 + j;
      tcvrPin.end() = tcvr;
      linePin.z() = tcvrPin;

      xphy.line()->push_back(linePin);
    }
    xphyPin.junction() = xphy;
    pin.z() = xphyPin;

    entry.mapping()->pins()->push_back(pin);
  }

  // prepare PortPinConfig for profiles
  cfg::PlatformPortConfig portCfg;
  std::vector<phy::PinConfig> xphySys;
  std::vector<phy::PinConfig> xphyLine;
  std::vector<phy::PinConfig> transceiver;
  for (int i = 0; i < 2; i++) {
    phy::PinConfig iphyCfg;
    *iphyCfg.id()->chip() = kDefaultIphyChipName;
    *iphyCfg.id()->lane() = kDefaultAsicLaneStart + i;
    portCfg.pins()->iphy()->push_back(iphyCfg);

    phy::PinConfig xphySysCfg;
    *xphySysCfg.id()->chip() = kDefaultXphyChipName;
    *xphySysCfg.id()->lane() = xphySysLaneStart + i;
    xphySys.push_back(xphySysCfg);

    for (int j = 0; j < 2; j++) {
      phy::PinConfig xphyLineCfg;
      *xphyLineCfg.id()->chip() = kDefaultXphyChipName;
      *xphyLineCfg.id()->lane() = xphyLineLaneStart + i * 2 + j;
      xphyLine.push_back(xphyLineCfg);

      phy::PinConfig tcvrCfg;
      *tcvrCfg.id()->chip() = kDefaultTcvrChipName;
      *tcvrCfg.id()->lane() = i * 2 + j;
      transceiver.push_back(tcvrCfg);
    }
  }
  portCfg.pins()->transceiver() = transceiver;
  portCfg.pins()->xphySys() = xphySys;
  portCfg.pins()->xphyLine() = xphyLine;
  entry.supportedProfiles()[cfg::PortProfileID::PROFILE_100G_4_NRZ_RS528] =
      portCfg;

  return entry;
}

cfg::PlatformPortEntry getPlatformPortEntryWithoutXPHY() {
  cfg::PlatformPortEntry entry;

  *entry.mapping()->id() = 4;
  *entry.mapping()->name() = "eth2/2/1";
  *entry.mapping()->controllingPort() = 3;

  for (int i = 0; i < 4; i++) {
    phy::PinConnection pin;
    phy::PinID iphy;
    *iphy.chip() = kDefaultIphyChipName;
    *iphy.lane() = kDefaultAsicLaneStart + i;
    *pin.a() = iphy;

    phy::Pin tcvrPin;
    phy::PinID tcvr;
    *tcvr.chip() = kDefaultTcvrChipName;
    *tcvr.lane() = i;
    tcvrPin.end() = tcvr;
    pin.z() = tcvrPin;

    entry.mapping()->pins()->push_back(pin);
  }

  // prepare PortPinConfig for profiles
  cfg::PlatformPortConfig portCfg;
  std::vector<phy::PinConfig> transceiver;
  for (int i = 0; i < 4; i++) {
    phy::PinConfig iphyCfg;
    *iphyCfg.id()->chip() = kDefaultIphyChipName;
    *iphyCfg.id()->lane() = kDefaultAsicLaneStart + i;
    portCfg.pins()->iphy()->push_back(iphyCfg);

    phy::PinConfig tcvrCfg;
    *tcvrCfg.id()->chip() = kDefaultTcvrChipName;
    *tcvrCfg.id()->lane() = i;
    transceiver.push_back(tcvrCfg);
  }
  portCfg.pins()->transceiver() = transceiver;
  entry.supportedProfiles()[cfg::PortProfileID::PROFILE_100G_4_NRZ_RS528] =
      portCfg;

  return entry;
}

cfg::PlatformPortEntry getPlatformPortEntryWithoutTransceiver() {
  cfg::PlatformPortEntry entry;

  entry.mapping()->id() = 4;
  entry.mapping()->name() = "eth2/2/1";
  entry.mapping()->controllingPort() = 3;

  for (int i = 0; i < 4; i++) {
    phy::PinConnection pin;
    phy::PinID iphy;
    iphy.chip() = kDefaultIphyChipName;
    iphy.lane() = kDefaultAsicLaneStart + i;
    pin.a() = iphy;

    entry.mapping()->pins()->push_back(pin);
  }

  // prepare PortPinConfig for profiles
  cfg::PlatformPortConfig portCfg;
  for (int i = 0; i < 4; i++) {
    phy::PinConfig iphyCfg;
    iphyCfg.id()->chip() = kDefaultIphyChipName;
    iphyCfg.id()->lane() = kDefaultAsicLaneStart + i;
    portCfg.pins()->iphy()->push_back(iphyCfg);
  }
  entry.supportedProfiles()
      [cfg::PortProfileID::PROFILE_100G_4_NRZ_NOFEC_COPPER] = portCfg;

  return entry;
}

cfg::PlatformPortEntry getPlatformPortEntryWithTwoTransceivers() {
  cfg::PlatformPortEntry entry;

  *entry.mapping()->id() = 35;
  *entry.mapping()->name() = "eth1/25/1";
  *entry.mapping()->controllingPort() = 35;

  // Create mapping pins with two transceivers
  for (int i = 0; i < 8; i++) {
    phy::PinConnection pin;
    phy::PinID iphy;
    *iphy.chip() = kDefaultIphyChipName;
    *iphy.lane() = kDefaultAsicLaneStart + i;
    *pin.a() = iphy;

    phy::Pin tcvrPin;
    phy::PinID tcvr;
    // First 4 lanes go to eth1/25, next 4 go to eth1/26
    if (i < 4) {
      *tcvr.chip() = "eth1/25";
      *tcvr.lane() = i;
    } else {
      *tcvr.chip() = "eth1/26";
      *tcvr.lane() = i - 4;
    }
    tcvrPin.end() = tcvr;
    pin.z() = tcvrPin;

    entry.mapping()->pins()->push_back(pin);
  }

  // prepare PortPinConfig for profiles with two transceivers
  cfg::PlatformPortConfig portCfg;
  std::vector<phy::PinConfig> transceiver;
  for (int i = 0; i < 8; i++) {
    phy::PinConfig iphyCfg;
    *iphyCfg.id()->chip() = kDefaultIphyChipName;
    *iphyCfg.id()->lane() = kDefaultAsicLaneStart + i;
    portCfg.pins()->iphy()->push_back(iphyCfg);

    phy::PinConfig tcvrCfg;
    // First 4 lanes go to eth1/25, next 4 go to eth1/26
    if (i < 4) {
      *tcvrCfg.id()->chip() = "eth1/25";
      *tcvrCfg.id()->lane() = i;
    } else {
      *tcvrCfg.id()->chip() = "eth1/26";
      *tcvrCfg.id()->lane() = i - 4;
    }
    transceiver.push_back(tcvrCfg);
  }
  portCfg.pins()->transceiver() = transceiver;
  entry.supportedProfiles()[cfg::PortProfileID::PROFILE_400G_8_PAM4_RS544X2N] =
      portCfg;

  // add PortPinConfig for profile only using one transceiver
  cfg::PlatformPortConfig portCfg2;
  std::vector<phy::PinConfig> tcvrPins;
  for (int i = 0; i < 4; i++) {
    phy::PinConfig iphyCfg;
    *iphyCfg.id()->chip() = kDefaultIphyChipName;
    *iphyCfg.id()->lane() = kDefaultAsicLaneStart + i;
    portCfg.pins()->iphy()->push_back(iphyCfg);

    phy::PinConfig tcvrCfg;
    // First 4 lanes go to eth1/25, next 4 go to eth1/26
    *tcvrCfg.id()->chip() = "eth1/25";
    *tcvrCfg.id()->lane() = i;

    tcvrPins.push_back(tcvrCfg);
  }
  portCfg.pins()->transceiver() = tcvrPins;
  entry.supportedProfiles()[cfg::PortProfileID::PROFILE_200G_4_PAM4_RS544X2N] =
      portCfg;

  return entry;
}

std::map<std::string, phy::DataPlanePhyChip> getPlatformChips() {
  std::map<std::string, phy::DataPlanePhyChip> chips;
  phy::DataPlanePhyChip iphy;
  *iphy.name() = kDefaultIphyChipName;
  *iphy.type() = phy::DataPlanePhyChipType::IPHY;
  *iphy.physicalID() = 0;
  chips[*iphy.name()] = iphy;

  phy::DataPlanePhyChip xphy;
  *xphy.name() = kDefaultXphyChipName;
  *xphy.type() = phy::DataPlanePhyChipType::XPHY;
  *xphy.physicalID() = 8;
  chips[*xphy.name()] = xphy;

  phy::DataPlanePhyChip tcvr;
  *tcvr.name() = kDefaultTcvrChipName;
  *tcvr.type() = phy::DataPlanePhyChipType::TRANSCEIVER;
  *tcvr.physicalID() = 2;
  chips[*tcvr.name()] = tcvr;

  // Add transceivers for multi-transceiver tests
  phy::DataPlanePhyChip tcvr25;
  *tcvr25.name() = "eth1/25";
  *tcvr25.type() = phy::DataPlanePhyChipType::TRANSCEIVER;
  *tcvr25.physicalID() = 25;
  chips[*tcvr25.name()] = tcvr25;

  phy::DataPlanePhyChip tcvr26;
  *tcvr26.name() = "eth1/26";
  *tcvr26.type() = phy::DataPlanePhyChipType::TRANSCEIVER;
  *tcvr26.physicalID() = 26;
  chips[*tcvr26.name()] = tcvr26;

  return chips;
}
} // namespace

namespace facebook::fboss::utility {

TEST(PlatformConfigUtilsTests, GetTransceiverLanesFromProfilesWithXPHY) {
  auto transeiverLanes = utility::getTransceiverLanes(
      getPlatformPortEntryWithXPHY(),
      getPlatformChips(),
      cfg::PortProfileID::PROFILE_100G_4_NRZ_RS528);
  EXPECT_EQ(transeiverLanes.size(), 4);
  for (int i = 0; i < 4; i++) {
    EXPECT_EQ(*transeiverLanes[i].chip(), kDefaultTcvrChipName);
    EXPECT_EQ(*transeiverLanes[i].lane(), i);
  }
}

TEST(PlatformConfigUtilsTests, GetTransceiverLanesFromProfilesWithoutXPHY) {
  auto transeiverLanes = utility::getTransceiverLanes(
      getPlatformPortEntryWithoutXPHY(),
      getPlatformChips(),
      cfg::PortProfileID::PROFILE_100G_4_NRZ_RS528);
  EXPECT_EQ(transeiverLanes.size(), 4);
  for (int i = 0; i < 4; i++) {
    EXPECT_EQ(*transeiverLanes[i].chip(), kDefaultTcvrChipName);
    EXPECT_EQ(*transeiverLanes[i].lane(), i);
  }
}

TEST(PlatformConfigUtilsTests, GetTransceiverLanesFromMappingWithXPHY) {
  auto transeiverLanes = utility::getTransceiverLanes(
      getPlatformPortEntryWithXPHY(), getPlatformChips());
  EXPECT_EQ(transeiverLanes.size(), 4);
  for (int i = 0; i < 4; i++) {
    EXPECT_EQ(*transeiverLanes[i].chip(), kDefaultTcvrChipName);
    EXPECT_EQ(*transeiverLanes[i].lane(), i);
  }
}

TEST(PlatformConfigUtilsTests, GetTransceiverLanesFromMappingWithoutXPHY) {
  auto transeiverLanes = utility::getTransceiverLanes(
      getPlatformPortEntryWithoutXPHY(), getPlatformChips());
  EXPECT_EQ(transeiverLanes.size(), 4);
  for (int i = 0; i < 4; i++) {
    EXPECT_EQ(*transeiverLanes[i].chip(), kDefaultTcvrChipName);
    EXPECT_EQ(*transeiverLanes[i].lane(), i);
  }
}

TEST(PlatformConfigUtilsTests, GetTransceiverLanesFromWrongProfiles) {
  EXPECT_THROW(
      utility::getTransceiverLanes(
          getPlatformPortEntryWithXPHY(),
          getPlatformChips(),
          cfg::PortProfileID::PROFILE_100G_4_NRZ_CL91),
      FbossError);
}

TEST(PlatformConfigUtilsTests, GetSubsidiaryPortIDs) {
  std::map<int32_t, cfg::PlatformPortEntry> platformPorts;
  for (auto controllingPort : {1, 5}) {
    for (int i = 0; i < 4; i++) {
      auto portEntry = getPlatformPortEntryWithXPHY();
      *portEntry.mapping()->id() = controllingPort + i;
      *portEntry.mapping()->controllingPort() = controllingPort;
      platformPorts[*portEntry.mapping()->id()] = portEntry;
    }
  }
  auto subsidiaryPorts = utility::getSubsidiaryPortIDs(platformPorts);
  EXPECT_EQ(subsidiaryPorts.size(), 2);
  for (auto controllingPort : {1, 5}) {
    auto portsItr = subsidiaryPorts.find(PortID(controllingPort));
    EXPECT_TRUE(portsItr != subsidiaryPorts.end());
    for (int i = 0; i < 4; i++) {
      EXPECT_EQ(portsItr->second[i], PortID(controllingPort + i));
    }
  }
}

TEST(PlatformConfigUtilsTests, GetSubsidiaryPortIDsWithEmptyConfig) {
  std::map<int32_t, cfg::PlatformPortEntry> platformPorts;
  auto subsidiaryPorts = utility::getSubsidiaryPortIDs(platformPorts);
  EXPECT_EQ(subsidiaryPorts.size(), 0);
}

TEST(PlatformConfigUtilsTests, GetOrderedIphyLanesFromProfilesWithXPHY) {
  auto iphyLanes = utility::getOrderedIphyLanes(
      getPlatformPortEntryWithXPHY(),
      getPlatformChips(),
      cfg::PortProfileID::PROFILE_100G_4_NRZ_RS528);
  EXPECT_EQ(iphyLanes.size(), 2);
  for (int i = 0; i < 2; i++) {
    EXPECT_EQ(*iphyLanes[i].chip(), kDefaultIphyChipName);
    EXPECT_EQ(*iphyLanes[i].lane(), kDefaultAsicLaneStart + i);
  }
}

TEST(PlatformConfigUtilsTests, GetOrderedIphyLanesFromProfilesWithoutXPHY) {
  auto iphyLanes = utility::getOrderedIphyLanes(
      getPlatformPortEntryWithoutXPHY(),
      getPlatformChips(),
      cfg::PortProfileID::PROFILE_100G_4_NRZ_RS528);
  EXPECT_EQ(iphyLanes.size(), 4);
  for (int i = 0; i < 4; i++) {
    EXPECT_EQ(*iphyLanes[i].chip(), kDefaultIphyChipName);
    EXPECT_EQ(*iphyLanes[i].lane(), kDefaultAsicLaneStart + i);
  }
}

TEST(PlatformConfigUtilsTests, GetOrderedIphyLanesFromMappingWithXPHY) {
  auto iphyLanes = utility::getOrderedIphyLanes(
      getPlatformPortEntryWithXPHY(), getPlatformChips());
  EXPECT_EQ(iphyLanes.size(), 2);
  for (int i = 0; i < 2; i++) {
    EXPECT_EQ(*iphyLanes[i].chip(), kDefaultIphyChipName);
    EXPECT_EQ(*iphyLanes[i].lane(), kDefaultAsicLaneStart + i);
  }
}

TEST(PlatformConfigUtilsTests, GetOrderedIphyLanesFromMappingWithoutXPHY) {
  auto iphyLanes = utility::getOrderedIphyLanes(
      getPlatformPortEntryWithoutXPHY(), getPlatformChips());
  EXPECT_EQ(iphyLanes.size(), 4);
  for (int i = 0; i < 4; i++) {
    EXPECT_EQ(*iphyLanes[i].chip(), kDefaultIphyChipName);
    EXPECT_EQ(*iphyLanes[i].lane(), kDefaultAsicLaneStart + i);
  }
}

TEST(PlatformConfigUtilsTests, GetOrderedIphyLanesFromWrongProfiles) {
  EXPECT_THROW(
      utility::getOrderedIphyLanes(
          getPlatformPortEntryWithXPHY(),
          getPlatformChips(),
          cfg::PortProfileID::PROFILE_100G_4_NRZ_CL91),
      FbossError);
}

TEST(PlatformConfigUtilsTests, GetPlatformPortsByControllingPort) {
  std::map<int32_t, cfg::PlatformPortEntry> platformPorts;
  for (auto controllingPort : {1, 5}) {
    for (int i = 0; i < 4; i++) {
      auto portEntry = getPlatformPortEntryWithXPHY();
      *portEntry.mapping()->id() = controllingPort + i;
      *portEntry.mapping()->controllingPort() = controllingPort;
      platformPorts[*portEntry.mapping()->id()] = portEntry;
    }
  }

  int controllingPortID = 5;
  const auto& subsidiaryPorts = utility::getPlatformPortsByControllingPort(
      platformPorts, PortID(controllingPortID));
  EXPECT_EQ(subsidiaryPorts.size(), 4);
  for (int i = 0; i < 4; i++) {
    EXPECT_EQ(*subsidiaryPorts[i].mapping()->id(), controllingPortID + i);
    EXPECT_EQ(
        *subsidiaryPorts[i].mapping()->controllingPort(), controllingPortID);
  }
}

TEST(
    PlatformConfigUtilsTests,
    GetPlatformPortsByControllingPortWithEmptyConfig) {
  std::map<int32_t, cfg::PlatformPortEntry> platformPorts;
  int controllingPortID = 5;
  const auto& subsidiaryPorts = utility::getPlatformPortsByControllingPort(
      platformPorts, PortID(controllingPortID));
  EXPECT_EQ(subsidiaryPorts.size(), 0);
}

TEST(PlatformConfigUtilsTests, GetDataPlanePhyChipsAll) {
  const auto& portChips = utility::getDataPlanePhyChips(
      getPlatformPortEntryWithXPHY(), getPlatformChips());
  EXPECT_EQ(portChips.size(), 3);
}

TEST(PlatformConfigUtilsTests, GetDataPlanePhyChipsXphy) {
  const auto& portChips = utility::getDataPlanePhyChips(
      getPlatformPortEntryWithXPHY(),
      getPlatformChips(),
      phy::DataPlanePhyChipType::XPHY);
  EXPECT_EQ(portChips.size(), 1);
  EXPECT_EQ(portChips.begin()->first, kDefaultXphyChipName);
  EXPECT_EQ(*portChips.begin()->second.type(), phy::DataPlanePhyChipType::XPHY);
  EXPECT_EQ(*portChips.begin()->second.physicalID(), 8);
}

TEST(PlatformConfigUtilsTests, GetDataPlanePhyChipsWithProfileId) {
  // Test getting chips for a specific profile
  const auto& portChips = utility::getDataPlanePhyChips(
      getPlatformPortEntryWithXPHY(),
      getPlatformChips(),
      std::nullopt,
      cfg::PortProfileID::PROFILE_100G_4_NRZ_RS528);
  EXPECT_EQ(portChips.size(), 3);

  // Verify we get all chip types: IPHY, XPHY, and TRANSCEIVER
  int iphyCount = 0, xphyCount = 0, tcvrCount = 0;
  for (const auto& chip : portChips) {
    if (*chip.second.type() == phy::DataPlanePhyChipType::IPHY) {
      iphyCount++;
    } else if (*chip.second.type() == phy::DataPlanePhyChipType::XPHY) {
      xphyCount++;
    } else if (*chip.second.type() == phy::DataPlanePhyChipType::TRANSCEIVER) {
      tcvrCount++;
    }
  }
  EXPECT_EQ(iphyCount, 1);
  EXPECT_EQ(xphyCount, 1);
  EXPECT_EQ(tcvrCount, 1);
}

TEST(PlatformConfigUtilsTests, GetDataPlanePhyChipsWithProfileIdIphyOnly) {
  // Test getting only IPHY chips for a specific profile
  const auto& portChips = utility::getDataPlanePhyChips(
      getPlatformPortEntryWithXPHY(),
      getPlatformChips(),
      phy::DataPlanePhyChipType::IPHY,
      cfg::PortProfileID::PROFILE_100G_4_NRZ_RS528);
  EXPECT_EQ(portChips.size(), 1);
  EXPECT_EQ(portChips.begin()->first, kDefaultIphyChipName);
  EXPECT_EQ(*portChips.begin()->second.type(), phy::DataPlanePhyChipType::IPHY);
}

TEST(PlatformConfigUtilsTests, GetDataPlanePhyChipsWithProfileIdXphyOnly) {
  // Test getting only XPHY chips for a specific profile
  const auto& portChips = utility::getDataPlanePhyChips(
      getPlatformPortEntryWithXPHY(),
      getPlatformChips(),
      phy::DataPlanePhyChipType::XPHY,
      cfg::PortProfileID::PROFILE_100G_4_NRZ_RS528);
  EXPECT_EQ(portChips.size(), 1);
  EXPECT_EQ(portChips.begin()->first, kDefaultXphyChipName);
  EXPECT_EQ(*portChips.begin()->second.type(), phy::DataPlanePhyChipType::XPHY);
  EXPECT_EQ(*portChips.begin()->second.physicalID(), 8);
}

TEST(
    PlatformConfigUtilsTests,
    GetDataPlanePhyChipsWithProfileIdTransceiverOnly) {
  // Test getting only TRANSCEIVER chips for a specific profile
  const auto& portChips = utility::getDataPlanePhyChips(
      getPlatformPortEntryWithXPHY(),
      getPlatformChips(),
      phy::DataPlanePhyChipType::TRANSCEIVER,
      cfg::PortProfileID::PROFILE_100G_4_NRZ_RS528);
  EXPECT_EQ(portChips.size(), 1);
  EXPECT_EQ(portChips.begin()->first, kDefaultTcvrChipName);
  EXPECT_EQ(
      *portChips.begin()->second.type(),
      phy::DataPlanePhyChipType::TRANSCEIVER);
  EXPECT_EQ(*portChips.begin()->second.physicalID(), 2);
}

TEST(PlatformConfigUtilsTests, GetDataPlanePhyChipsWithProfileIdNoXphyConfig) {
  // Test getting chips for a port without XPHY using profile
  const auto& portChips = utility::getDataPlanePhyChips(
      getPlatformPortEntryWithoutXPHY(),
      getPlatformChips(),
      std::nullopt,
      cfg::PortProfileID::PROFILE_100G_4_NRZ_RS528);
  EXPECT_EQ(portChips.size(), 2); // Only IPHY and TRANSCEIVER

  bool hasIphy = false, hasTcvr = false;
  for (const auto& chip : portChips) {
    if (*chip.second.type() == phy::DataPlanePhyChipType::IPHY) {
      hasIphy = true;
    } else if (*chip.second.type() == phy::DataPlanePhyChipType::TRANSCEIVER) {
      hasTcvr = true;
    }
  }
  EXPECT_TRUE(hasIphy);
  EXPECT_TRUE(hasTcvr);
}

TEST(PlatformConfigUtilsTests, GetDataPlanePhyChipsWithInvalidProfileId) {
  // Test that using an invalid profile ID falls back to mapping-based lookup
  const auto& portChips = utility::getDataPlanePhyChips(
      getPlatformPortEntryWithXPHY(),
      getPlatformChips(),
      std::nullopt,
      cfg::PortProfileID::PROFILE_100G_4_NRZ_CL91);
  // Should fall back to using the mapping pins (not profile-based)
  EXPECT_EQ(portChips.size(), 3);

  const auto& portChipsNullopt = utility::getDataPlanePhyChips(
      getPlatformPortEntryWithXPHY(),
      getPlatformChips(),
      std::nullopt,
      std::nullopt);
  // Should fall back to using the mapping pins (not profile-based)
  EXPECT_EQ(portChipsNullopt.size(), 3);
}

TEST(PlatformConfigUtilsTests, GetTransceiverIdsWithProfileId) {
  // Test getting transceiver IDs with a specific profile
  auto transceiverIds = utility::getTransceiverIds(
      getPlatformPortEntryWithXPHY(),
      getPlatformChips(),
      cfg::PortProfileID::PROFILE_100G_4_NRZ_RS528);
  EXPECT_FALSE(transceiverIds.empty());
  EXPECT_EQ(transceiverIds[0], TransceiverID{2});

  // Test with port without XPHY
  transceiverIds = utility::getTransceiverIds(
      getPlatformPortEntryWithoutXPHY(),
      getPlatformChips(),
      cfg::PortProfileID::PROFILE_100G_4_NRZ_RS528);
  EXPECT_FALSE(transceiverIds.empty());
  EXPECT_EQ(transceiverIds[0], TransceiverID{2});
}

TEST(PlatformConfigUtilsTests, GetTransceiverIdsFromPortAndChips) {
  auto transceiverIds = utility::getTransceiverIds(
      getPlatformPortEntryWithXPHY(), getPlatformChips());
  EXPECT_EQ(transceiverIds.size(), 1);
  EXPECT_EQ(transceiverIds[0], TransceiverID{2});

  transceiverIds = utility::getTransceiverIds(
      getPlatformPortEntryWithoutTransceiver(), getPlatformChips());
  EXPECT_EQ(transceiverIds.size(), 0);
}

TEST(PlatformConfigUtilsTests, GetTransceiverIdsFromChips) {
  const auto& portChips = utility::getDataPlanePhyChips(
      getPlatformPortEntryWithXPHY(), getPlatformChips());
  const auto& transceiverIds = utility::getTransceiverIds(portChips);

  EXPECT_EQ(transceiverIds.size(), 1);
  EXPECT_EQ(transceiverIds[0], TransceiverID{2});
}

TEST(PlatformConfigUtilsTests, GetPlatformPortsByChip) {
  std::map<int32_t, cfg::PlatformPortEntry> platformPorts;

  auto portWithXphy = getPlatformPortEntryWithXPHY();
  static const auto kPortWithXphyID = 1000;
  portWithXphy.mapping()->id() = kPortWithXphyID;
  platformPorts.emplace(kPortWithXphyID, portWithXphy);

  auto portWithoutXphy = getPlatformPortEntryWithoutXPHY();
  static const auto kPortWithoutXphyID = 1001;
  portWithoutXphy.mapping()->id() = kPortWithoutXphyID;
  platformPorts.emplace(kPortWithoutXphyID, portWithoutXphy);

  auto portWithoutXphyAndTcvr = getPlatformPortEntryWithoutTransceiver();
  static const auto kPortWithoutXphyAndTcvr = 1002;
  portWithoutXphyAndTcvr.mapping()->id() = kPortWithoutXphyAndTcvr;
  platformPorts.emplace(kPortWithoutXphyAndTcvr, portWithoutXphyAndTcvr);

  const auto& platformChips = getPlatformChips();

  const auto& portsWithIphy = utility::getPlatformPortsByChip(
      platformPorts, platformChips.find(kDefaultIphyChipName)->second);
  // All three ports should use the same iphy
  const std::set<int32_t> expectedPortsWithIphy = {
      kPortWithXphyID, kPortWithoutXphyID, kPortWithoutXphyAndTcvr};
  std::set<int32_t> actualPortsWithIphy;
  for (const auto& port : portsWithIphy) {
    actualPortsWithIphy.insert(
        folly::copy(port.mapping().value().id().value()));
  }
  EXPECT_EQ(actualPortsWithIphy, expectedPortsWithIphy);

  const auto& portsWithXphy = utility::getPlatformPortsByChip(
      platformPorts, platformChips.find(kDefaultXphyChipName)->second);
  // Only two ports should use the same xphy
  const std::set<int32_t> expectedPortsWithXphy = {kPortWithXphyID};
  std::set<int32_t> actualPortsWithXphy;
  for (const auto& port : portsWithXphy) {
    actualPortsWithXphy.insert(
        folly::copy(port.mapping().value().id().value()));
  }
  EXPECT_EQ(actualPortsWithXphy, expectedPortsWithXphy);

  const auto& portsWithTcvr = utility::getPlatformPortsByChip(
      platformPorts, platformChips.find(kDefaultTcvrChipName)->second);
  // Only two ports should use the same transceiver
  const std::set<int32_t> expectedPortsWithTcvr = {
      kPortWithXphyID, kPortWithoutXphyID};
  std::set<int32_t> actualPortsWithTcvr;
  for (const auto& port : portsWithTcvr) {
    actualPortsWithTcvr.insert(
        folly::copy(port.mapping().value().id().value()));
  }
  EXPECT_EQ(actualPortsWithTcvr, expectedPortsWithTcvr);

  // make a fake chip
  phy::DataPlanePhyChip fake;
  fake.name() = "fake";
  const auto& portsWithFakeChip =
      utility::getPlatformPortsByChip(platformPorts, fake);
  EXPECT_TRUE(portsWithFakeChip.empty());
}

cfg::PlatformPortEntry createSingleTcvrPortEntry(
    int32_t id,
    const std::string& chipName) {
  cfg::PlatformPortEntry entry;
  *entry.mapping()->id() = id;
  *entry.mapping()->name() = "eth" + std::to_string(id);
  *entry.mapping()->controllingPort() = id;
  for (int i = 0; i < 4; i++) {
    phy::PinConnection pin;
    phy::PinID iphy;
    *iphy.chip() = "phy";
    *iphy.lane() = i;
    *pin.a() = iphy;
    phy::Pin tcvrPin;
    phy::PinID tcvr;
    *tcvr.chip() = chipName;
    *tcvr.lane() = i;
    tcvrPin.end() = tcvr;
    pin.z() = tcvrPin;
    entry.mapping()->pins()->push_back(pin);
  }
  return entry;
}

cfg::PlatformPortEntry createMultiTcvrPortEntry(
    int32_t id,
    const std::string& chipName1,
    const std::string& chipName2) {
  cfg::PlatformPortEntry entry;
  *entry.mapping()->id() = id;
  *entry.mapping()->name() = "eth" + std::to_string(id);
  *entry.mapping()->controllingPort() = id;
  for (int i = 0; i < 2; i++) {
    phy::PinConnection pin;
    phy::PinID iphy;
    *iphy.chip() = "phy";
    *iphy.lane() = i;
    *pin.a() = iphy;
    phy::Pin tcvrPin;
    phy::PinID tcvr;
    *tcvr.chip() = chipName1;
    *tcvr.lane() = i;
    tcvrPin.end() = tcvr;
    pin.z() = tcvrPin;
    entry.mapping()->pins()->push_back(pin);
  }
  for (int i = 2; i < 4; i++) {
    phy::PinConnection pin;
    phy::PinID iphy;
    *iphy.chip() = "phy";
    *iphy.lane() = i;
    *pin.a() = iphy;
    phy::Pin tcvrPin;
    phy::PinID tcvr;
    *tcvr.chip() = chipName2;
    *tcvr.lane() = i - 2;
    tcvrPin.end() = tcvr;
    pin.z() = tcvrPin;
    entry.mapping()->pins()->push_back(pin);
  }
  return entry;
}
std::map<std::string, phy::DataPlanePhyChip> createChipsMap() {
  std::map<std::string, phy::DataPlanePhyChip> chips;
  for (int i = 0; i < 4; i++) {
    phy::DataPlanePhyChip chip;
    *chip.name() = "tcvr" + std::to_string(i);
    *chip.type() = phy::DataPlanePhyChipType::TRANSCEIVER;
    *chip.physicalID() = i;
    chips[*chip.name()] = chip;
  }
  return chips;
}
} // namespace facebook::fboss::utility
namespace facebook::fboss::utility {
TEST(PlatformConfigUtilsTests, GetTcvrToPortMap) {
  std::map<int32_t, cfg::PlatformPortEntry> platformPorts;
  platformPorts[1] = createSingleTcvrPortEntry(1, "tcvr0");
  platformPorts[2] = createSingleTcvrPortEntry(2, "tcvr0");
  platformPorts[3] = createSingleTcvrPortEntry(3, "tcvr1");
  platformPorts[4] = createMultiTcvrPortEntry(4, "tcvr2", "tcvr3");
  auto chipsMap = createChipsMap();
  auto tcvrToPortMap = getTcvrToPortMap(platformPorts, chipsMap);

  EXPECT_EQ(tcvrToPortMap.size(), 4);
  EXPECT_EQ(tcvrToPortMap[TransceiverID(0)].size(), 2);
  EXPECT_EQ(tcvrToPortMap[TransceiverID(0)][0], PortID(1));
  EXPECT_EQ(tcvrToPortMap[TransceiverID(0)][1], PortID(2));
  EXPECT_EQ(tcvrToPortMap[TransceiverID(1)].size(), 1);
  EXPECT_EQ(tcvrToPortMap[TransceiverID(1)][0], PortID(3));
  EXPECT_EQ(tcvrToPortMap[TransceiverID(2)].size(), 1);
  EXPECT_EQ(tcvrToPortMap[TransceiverID(2)][0], PortID(4));
  EXPECT_EQ(tcvrToPortMap[TransceiverID(3)].size(), 1);
  EXPECT_EQ(tcvrToPortMap[TransceiverID(3)][0], PortID(4));
}
TEST(PlatformConfigUtilsTests, GetTcvrToPortMapNoTransceiver) {
  std::map<int32_t, cfg::PlatformPortEntry> platformPorts;
  platformPorts[1] = createSingleTcvrPortEntry(1, "nonexistent");
  auto chipsMap = createChipsMap();
  auto tcvrToPortMap = getTcvrToPortMap(platformPorts, chipsMap);
  EXPECT_EQ(tcvrToPortMap.size(), 0);
}
TEST(PlatformConfigUtilsTests, GetPortToTcvrMap) {
  std::map<int32_t, cfg::PlatformPortEntry> platformPorts;
  platformPorts[1] = createSingleTcvrPortEntry(1, "tcvr0");
  platformPorts[2] = createSingleTcvrPortEntry(2, "tcvr0");
  platformPorts[3] = createSingleTcvrPortEntry(3, "tcvr1");
  platformPorts[4] = createMultiTcvrPortEntry(4, "tcvr2", "tcvr3");
  auto chipsMap = createChipsMap();
  auto portToTcvrMap = getPortToTcvrMap(platformPorts, chipsMap);

  EXPECT_EQ(portToTcvrMap.size(), 4);
  EXPECT_EQ(portToTcvrMap[PortID(1)].size(), 1);
  EXPECT_EQ(portToTcvrMap[PortID(1)][0], TransceiverID(0));
  EXPECT_EQ(portToTcvrMap[PortID(2)].size(), 1);
  EXPECT_EQ(portToTcvrMap[PortID(2)][0], TransceiverID(0));
  EXPECT_EQ(portToTcvrMap[PortID(3)].size(), 1);
  EXPECT_EQ(portToTcvrMap[PortID(3)][0], TransceiverID(1));
  EXPECT_EQ(portToTcvrMap[PortID(4)].size(), 2);
  EXPECT_EQ(portToTcvrMap[PortID(4)][0], TransceiverID(2));
  EXPECT_EQ(portToTcvrMap[PortID(4)][1], TransceiverID(3));
}
TEST(PlatformConfigUtilsTests, GetPortToTcvrMapNoTransceiver) {
  std::map<int32_t, cfg::PlatformPortEntry> platformPorts;
  platformPorts[1] = createSingleTcvrPortEntry(1, "nonexistent");
  auto chipsMap = createChipsMap();
  auto portToTcvrMap = getPortToTcvrMap(platformPorts, chipsMap);
  EXPECT_EQ(portToTcvrMap.size(), 0);
}

TEST(PlatformConfigUtilsTests, GetDataPlanePhyChipsWithTwoTransceivers) {
  // Test getting all chips for a port with two transceivers
  const auto& portChips = utility::getDataPlanePhyChips(
      getPlatformPortEntryWithTwoTransceivers(), getPlatformChips());

  // Should have 1 IPHY and 2 TRANSCEIVERs (no XPHY)
  EXPECT_EQ(portChips.size(), 3);

  int iphyCount = 0, tcvrCount = 0;
  std::set<std::string> tcvrNames;
  for (const auto& chip : portChips) {
    if (*chip.second.type() == phy::DataPlanePhyChipType::IPHY) {
      iphyCount++;
      EXPECT_EQ(*chip.second.name(), kDefaultIphyChipName);
    } else if (*chip.second.type() == phy::DataPlanePhyChipType::TRANSCEIVER) {
      tcvrCount++;
      tcvrNames.insert(*chip.second.name());
    }
  }
  EXPECT_EQ(iphyCount, 1);
  EXPECT_EQ(tcvrCount, 2);

  // Verify we have both transceivers
  EXPECT_TRUE(tcvrNames.count("eth1/25") > 0);
  EXPECT_TRUE(tcvrNames.count("eth1/26") > 0);
}

TEST(
    PlatformConfigUtilsTests,
    GetDataPlanePhyChipsWithTwoTransceiversProfileId) {
  // Test getting chips for a port with two transceivers using profile ID
  const auto& portChips = utility::getDataPlanePhyChips(
      getPlatformPortEntryWithTwoTransceivers(),
      getPlatformChips(),
      std::nullopt,
      cfg::PortProfileID::PROFILE_400G_8_PAM4_RS544X2N);

  // Should have 1 IPHY and 2 TRANSCEIVERs
  EXPECT_EQ(portChips.size(), 3);

  int iphyCount = 0, tcvrCount = 0;
  std::set<std::string> tcvrNames;
  for (const auto& chip : portChips) {
    if (*chip.second.type() == phy::DataPlanePhyChipType::IPHY) {
      iphyCount++;
    } else if (*chip.second.type() == phy::DataPlanePhyChipType::TRANSCEIVER) {
      tcvrCount++;
      tcvrNames.insert(*chip.second.name());
    }
  }
  EXPECT_EQ(iphyCount, 1);
  EXPECT_EQ(tcvrCount, 2);
  EXPECT_TRUE(tcvrNames.count("eth1/25") > 0);
  EXPECT_TRUE(tcvrNames.count("eth1/26") > 0);
}

TEST(
    PlatformConfigUtilsTests,
    GetDataPlanePhyChipsWithTwoTransceiversTransceiverOnly) {
  // Test getting only TRANSCEIVER chips for a port with two transceivers
  const auto& portChips = utility::getDataPlanePhyChips(
      getPlatformPortEntryWithTwoTransceivers(),
      getPlatformChips(),
      phy::DataPlanePhyChipType::TRANSCEIVER,
      cfg::PortProfileID::PROFILE_400G_8_PAM4_RS544X2N);

  // Should have 2 TRANSCEIVERs only
  EXPECT_EQ(portChips.size(), 2);

  std::set<std::string> tcvrNames;
  std::set<int32_t> tcvrPhysicalIds;
  for (const auto& chip : portChips) {
    EXPECT_EQ(*chip.second.type(), phy::DataPlanePhyChipType::TRANSCEIVER);
    tcvrNames.insert(*chip.second.name());
    tcvrPhysicalIds.insert(*chip.second.physicalID());
  }

  // Verify we have both transceivers with correct IDs
  EXPECT_TRUE(tcvrNames.count("eth1/25") > 0);
  EXPECT_TRUE(tcvrNames.count("eth1/26") > 0);
  EXPECT_TRUE(tcvrPhysicalIds.count(25) > 0);
  EXPECT_TRUE(tcvrPhysicalIds.count(26) > 0);
}

TEST(PlatformConfigUtilsTests, GetTransceiverIdsWithTwoTransceivers) {
  // Test getting transceiver IDs for a port with two transceivers
  auto transceiverIds = utility::getTransceiverIds(
      getPlatformPortEntryWithTwoTransceivers(),
      getPlatformChips(),
      cfg::PortProfileID::PROFILE_400G_8_PAM4_RS544X2N);

  // Should return both transceiver IDs
  EXPECT_EQ(transceiverIds.size(), 2);

  std::set<int32_t> tcvrIdSet;
  for (const auto& tcvrId : transceiverIds) {
    tcvrIdSet.insert(static_cast<int32_t>(tcvrId));
  }

  EXPECT_TRUE(tcvrIdSet.count(25) > 0);
  EXPECT_TRUE(tcvrIdSet.count(26) > 0);
}

TEST(PlatformConfigUtilsTests, GetTransceiverLanesWithTwoTransceivers) {
  // Test getting transceiver lanes for a port with two transceivers
  auto transeiverLanes = utility::getTransceiverLanes(
      getPlatformPortEntryWithTwoTransceivers(),
      getPlatformChips(),
      cfg::PortProfileID::PROFILE_400G_8_PAM4_RS544X2N);

  // Should have 8 lanes total (4 from each transceiver)
  EXPECT_EQ(transeiverLanes.size(), 8);

  // Count lanes per transceiver
  std::map<std::string, int> laneCountByTcvr;
  for (const auto& lane : transeiverLanes) {
    laneCountByTcvr[*lane.chip()]++;
  }

  EXPECT_EQ(laneCountByTcvr["eth1/25"], 4);
  EXPECT_EQ(laneCountByTcvr["eth1/26"], 4);
}

TEST(PlatformConfigUtilsTests, GetPinsFromPortPinConfigIphy) {
  phy::PortPinConfig pinConfig;
  std::vector<phy::PinConfig> iphyPins;

  for (int i = 0; i < 4; i++) {
    phy::PinConfig pin;
    *pin.id()->chip() = "iphy0";
    *pin.id()->lane() = i;
    iphyPins.push_back(pin);
  }
  pinConfig.iphy() = iphyPins;

  auto pins = utility::getPinsFromPortPinConfig(
      pinConfig, phy::DataPlanePhyChipType::IPHY);

  EXPECT_EQ(pins.size(), 4);
  for (int i = 0; i < 4; i++) {
    EXPECT_EQ(*pins[i].id()->chip(), "iphy0");
    EXPECT_EQ(*pins[i].id()->lane(), i);
  }
}

TEST(PlatformConfigUtilsTests, GetPinsFromPortPinConfigXphy) {
  phy::PortPinConfig pinConfig;

  std::vector<phy::PinConfig> xphySys;
  for (int i = 0; i < 2; i++) {
    phy::PinConfig pin;
    *pin.id()->chip() = "xphy0";
    *pin.id()->lane() = i;
    xphySys.push_back(pin);
  }
  pinConfig.xphySys() = xphySys;

  std::vector<phy::PinConfig> xphyLine;
  for (int i = 0; i < 4; i++) {
    phy::PinConfig pin;
    *pin.id()->chip() = "xphy0";
    *pin.id()->lane() = 10 + i;
    xphyLine.push_back(pin);
  }
  pinConfig.xphyLine() = xphyLine;

  auto pins = utility::getPinsFromPortPinConfig(
      pinConfig, phy::DataPlanePhyChipType::XPHY);

  EXPECT_EQ(pins.size(), 6);
  for (int i = 0; i < 2; i++) {
    EXPECT_EQ(*pins[i].id()->chip(), "xphy0");
    EXPECT_EQ(*pins[i].id()->lane(), i);
  }
  for (int i = 0; i < 4; i++) {
    EXPECT_EQ(*pins[i + 2].id()->chip(), "xphy0");
    EXPECT_EQ(*pins[i + 2].id()->lane(), 10 + i);
  }
}

TEST(PlatformConfigUtilsTests, GetPinsFromPortPinConfigXphySysOnly) {
  phy::PortPinConfig pinConfig;

  std::vector<phy::PinConfig> xphySys;
  for (int i = 0; i < 2; i++) {
    phy::PinConfig pin;
    *pin.id()->chip() = "xphy0";
    *pin.id()->lane() = i;
    xphySys.push_back(pin);
  }
  pinConfig.xphySys() = xphySys;

  auto pins = utility::getPinsFromPortPinConfig(
      pinConfig, phy::DataPlanePhyChipType::XPHY);

  EXPECT_EQ(pins.size(), 2);
  for (int i = 0; i < 2; i++) {
    EXPECT_EQ(*pins[i].id()->chip(), "xphy0");
    EXPECT_EQ(*pins[i].id()->lane(), i);
  }
}

TEST(PlatformConfigUtilsTests, GetPinsFromPortPinConfigXphyLineOnly) {
  phy::PortPinConfig pinConfig;

  std::vector<phy::PinConfig> xphyLine;
  for (int i = 0; i < 4; i++) {
    phy::PinConfig pin;
    *pin.id()->chip() = "xphy0";
    *pin.id()->lane() = 10 + i;
    xphyLine.push_back(pin);
  }
  pinConfig.xphyLine() = xphyLine;

  auto pins = utility::getPinsFromPortPinConfig(
      pinConfig, phy::DataPlanePhyChipType::XPHY);

  EXPECT_EQ(pins.size(), 4);
  for (int i = 0; i < 4; i++) {
    EXPECT_EQ(*pins[i].id()->chip(), "xphy0");
    EXPECT_EQ(*pins[i].id()->lane(), 10 + i);
  }
}

TEST(PlatformConfigUtilsTests, GetPinsFromPortPinConfigXphyEmpty) {
  phy::PortPinConfig pinConfig;

  auto pins = utility::getPinsFromPortPinConfig(
      pinConfig, phy::DataPlanePhyChipType::XPHY);

  EXPECT_EQ(pins.size(), 0);
}

TEST(PlatformConfigUtilsTests, GetPinsFromPortPinConfigTransceiver) {
  phy::PortPinConfig pinConfig;

  std::vector<phy::PinConfig> tcvrPins;
  for (int i = 0; i < 4; i++) {
    phy::PinConfig pin;
    *pin.id()->chip() = "eth1/1";
    *pin.id()->lane() = i;
    tcvrPins.push_back(pin);
  }
  pinConfig.transceiver() = tcvrPins;

  auto pins = utility::getPinsFromPortPinConfig(
      pinConfig, phy::DataPlanePhyChipType::TRANSCEIVER);

  EXPECT_EQ(pins.size(), 4);
  for (int i = 0; i < 4; i++) {
    EXPECT_EQ(*pins[i].id()->chip(), "eth1/1");
    EXPECT_EQ(*pins[i].id()->lane(), i);
  }
}

TEST(PlatformConfigUtilsTests, GetPinsFromPortPinConfigTransceiverEmpty) {
  phy::PortPinConfig pinConfig;

  auto pins = utility::getPinsFromPortPinConfig(
      pinConfig, phy::DataPlanePhyChipType::TRANSCEIVER);

  EXPECT_EQ(pins.size(), 0);
}

TEST(PlatformConfigUtilsTests, GetPinsFromPortPinConfigAllTypes) {
  phy::PortPinConfig pinConfig;

  std::vector<phy::PinConfig> iphyPins;
  for (int i = 0; i < 2; i++) {
    phy::PinConfig pin;
    *pin.id()->chip() = "iphy0";
    *pin.id()->lane() = i;
    iphyPins.push_back(pin);
  }
  pinConfig.iphy() = iphyPins;

  std::vector<phy::PinConfig> xphySys;
  for (int i = 0; i < 2; i++) {
    phy::PinConfig pin;
    *pin.id()->chip() = "xphy0";
    *pin.id()->lane() = i;
    xphySys.push_back(pin);
  }
  pinConfig.xphySys() = xphySys;

  std::vector<phy::PinConfig> xphyLine;
  for (int i = 0; i < 4; i++) {
    phy::PinConfig pin;
    *pin.id()->chip() = "xphy0";
    *pin.id()->lane() = 10 + i;
    xphyLine.push_back(pin);
  }
  pinConfig.xphyLine() = xphyLine;

  std::vector<phy::PinConfig> tcvrPins;
  for (int i = 0; i < 4; i++) {
    phy::PinConfig pin;
    *pin.id()->chip() = "eth1/1";
    *pin.id()->lane() = i;
    tcvrPins.push_back(pin);
  }
  pinConfig.transceiver() = tcvrPins;

  auto iphyResult = utility::getPinsFromPortPinConfig(
      pinConfig, phy::DataPlanePhyChipType::IPHY);
  EXPECT_EQ(iphyResult.size(), 2);

  auto xphyResult = utility::getPinsFromPortPinConfig(
      pinConfig, phy::DataPlanePhyChipType::XPHY);
  EXPECT_EQ(xphyResult.size(), 6);

  auto tcvrResult = utility::getPinsFromPortPinConfig(
      pinConfig, phy::DataPlanePhyChipType::TRANSCEIVER);
  EXPECT_EQ(tcvrResult.size(), 4);
}

cfg::PlatformPortEntry getPlatformPortEntryWithBackplane() {
  cfg::PlatformPortEntry entry;

  *entry.mapping()->id() = 50;
  *entry.mapping()->name() = "eth3/1/1";
  *entry.mapping()->controllingPort() = 50;

  constexpr auto kBackplaneChipName = "backplane0";

  // For static mapping pins, backplane chips are in the z field (end of chain)
  for (int i = 0; i < 4; i++) {
    phy::PinConnection pin;
    phy::PinID iphy;
    *iphy.chip() = kDefaultIphyChipName;
    *iphy.lane() = kDefaultAsicLaneStart + i;
    *pin.a() = iphy;

    phy::Pin backplanePin;
    phy::PinID backplane;
    *backplane.chip() = kBackplaneChipName;
    *backplane.lane() = i;
    backplanePin.end() = backplane;
    pin.z() = backplanePin;

    entry.mapping()->pins()->push_back(pin);
  }

  // For profile pins, backplane chips are stored in the transceiver field
  cfg::PlatformPortConfig portCfg;
  std::vector<phy::PinConfig> backplanePins;
  for (int i = 0; i < 4; i++) {
    phy::PinConfig iphyCfg;
    *iphyCfg.id()->chip() = kDefaultIphyChipName;
    *iphyCfg.id()->lane() = kDefaultAsicLaneStart + i;
    portCfg.pins()->iphy()->push_back(iphyCfg);

    phy::PinConfig backplaneCfg;
    *backplaneCfg.id()->chip() = kBackplaneChipName;
    *backplaneCfg.id()->lane() = i;
    backplanePins.push_back(backplaneCfg);
  }
  portCfg.pins()->transceiver() = backplanePins;
  entry.supportedProfiles()
      [cfg::PortProfileID::PROFILE_100G_4_NRZ_NOFEC_COPPER] = portCfg;

  return entry;
}

std::map<std::string, phy::DataPlanePhyChip> getPlatformChipsWithBackplane() {
  auto chips = getPlatformChips();

  phy::DataPlanePhyChip backplane;
  *backplane.name() = "backplane0";
  *backplane.type() = phy::DataPlanePhyChipType::BACKPLANE;
  *backplane.physicalID() = 100;
  chips[*backplane.name()] = backplane;

  return chips;
}

TEST(PlatformConfigUtilsTests, GetDataPlanePhyChipsWithBackplaneFromProfile) {
  // Test getting chips from profile, where backplane chips are in transceiver
  // field
  const auto& portChips = utility::getDataPlanePhyChips(
      getPlatformPortEntryWithBackplane(),
      getPlatformChipsWithBackplane(),
      std::nullopt,
      cfg::PortProfileID::PROFILE_100G_4_NRZ_NOFEC_COPPER);

  // Should have 1 IPHY and 1 BACKPLANE, but we don't consider backplane in this
  // function.
  EXPECT_EQ(portChips.size(), 1);

  int iphyCount = 0, backplaneCount = 0;
  for (const auto& chip : portChips) {
    if (*chip.second.type() == phy::DataPlanePhyChipType::IPHY) {
      iphyCount++;
      EXPECT_EQ(*chip.second.name(), kDefaultIphyChipName);
    } else if (*chip.second.type() == phy::DataPlanePhyChipType::BACKPLANE) {
      backplaneCount++;
      EXPECT_EQ(*chip.second.name(), "backplane0");
      EXPECT_EQ(*chip.second.physicalID(), 100);
    }
  }
  EXPECT_EQ(iphyCount, 1);
  EXPECT_EQ(backplaneCount, 0);
}

TEST(PlatformConfigUtilsTests, GetDataPlanePhyChipsWithBackplaneFromMapping) {
  // Test getting chips from static mapping, where backplane chips are in z
  // field
  const auto& portChips = utility::getDataPlanePhyChips(
      getPlatformPortEntryWithBackplane(), getPlatformChipsWithBackplane());

  // Should have 1 IPHY and 1 BACKPLANE, but we don't consider backplane in this
  // function.
  EXPECT_EQ(portChips.size(), 1);

  int iphyCount = 0, backplaneCount = 0;
  for (const auto& chip : portChips) {
    if (*chip.second.type() == phy::DataPlanePhyChipType::IPHY) {
      iphyCount++;
      EXPECT_EQ(*chip.second.name(), kDefaultIphyChipName);
    } else if (*chip.second.type() == phy::DataPlanePhyChipType::BACKPLANE) {
      backplaneCount++;
      EXPECT_EQ(*chip.second.name(), "backplane0");
      EXPECT_EQ(*chip.second.physicalID(), 100);
    }
  }
  EXPECT_EQ(iphyCount, 1);
  EXPECT_EQ(backplaneCount, 0);
}

TEST(
    PlatformConfigUtilsTests,
    GetDataPlanePhyChipsWithBackplaneOnlyFromProfile) {
  const auto& tcvrChips = utility::getDataPlanePhyChips(
      getPlatformPortEntryWithBackplane(),
      getPlatformChipsWithBackplane(),
      phy::DataPlanePhyChipType::TRANSCEIVER,
      cfg::PortProfileID::PROFILE_100G_4_NRZ_NOFEC_COPPER);
  EXPECT_EQ(tcvrChips.size(), 0);

  const auto& phyChips = utility::getDataPlanePhyChips(
      getPlatformPortEntryWithBackplane(),
      getPlatformChipsWithBackplane(),
      phy::DataPlanePhyChipType::IPHY,
      cfg::PortProfileID::PROFILE_100G_4_NRZ_NOFEC_COPPER);
  EXPECT_EQ(phyChips.size(), 1);
  EXPECT_EQ(phyChips.begin()->first, kDefaultIphyChipName);
  EXPECT_EQ(*phyChips.begin()->second.type(), phy::DataPlanePhyChipType::IPHY);
  EXPECT_EQ(*phyChips.begin()->second.physicalID(), 0);
}

TEST(
    PlatformConfigUtilsTests,
    GetDataPlanePhyChipsWithBackplaneOnlyFromMapping) {
  const auto& tcvrChips = utility::getDataPlanePhyChips(
      getPlatformPortEntryWithBackplane(),
      getPlatformChipsWithBackplane(),
      phy::DataPlanePhyChipType::TRANSCEIVER);
  EXPECT_EQ(tcvrChips.size(), 0);

  const auto& phyChips = utility::getDataPlanePhyChips(
      getPlatformPortEntryWithBackplane(),
      getPlatformChipsWithBackplane(),
      phy::DataPlanePhyChipType::IPHY);
  EXPECT_EQ(phyChips.size(), 1);
  EXPECT_EQ(phyChips.begin()->first, kDefaultIphyChipName);
  EXPECT_EQ(*phyChips.begin()->second.type(), phy::DataPlanePhyChipType::IPHY);
  EXPECT_EQ(*phyChips.begin()->second.physicalID(), 0);
}

} // namespace facebook::fboss::utility
