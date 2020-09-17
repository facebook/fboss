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

  *entry.mapping_ref()->id_ref() = 4;
  *entry.mapping_ref()->name_ref() = "eth2/2/1";
  *entry.mapping_ref()->controllingPort_ref() = 3;

  int xphySysLaneStart = 6;
  int xphyLineLaneStart = 12;
  for (int i = 0; i < 2; i++) {
    phy::PinConnection pin;
    phy::PinID iphy;
    *iphy.chip_ref() = kDefaultIphyChipName;
    *iphy.lane_ref() = kDefaultAsicLaneStart + i;
    *pin.a_ref() = iphy;

    phy::Pin xphyPin;
    phy::PinJunction xphy;
    phy::PinID xphySys;
    *xphySys.chip_ref() = kDefaultXphyChipName;
    *xphySys.lane_ref() = xphySysLaneStart + i;
    *xphy.system_ref() = xphySys;

    for (int j = 0; j < 2; j++) {
      phy::PinConnection linePin;
      phy::PinID xphyLine;
      *xphyLine.chip_ref() = kDefaultXphyChipName;
      *xphyLine.lane_ref() = xphyLineLaneStart + i * 2 + j;
      *linePin.a_ref() = xphyLine;

      phy::Pin tcvrPin;
      phy::PinID tcvr;
      *tcvr.chip_ref() = kDefaultTcvrChipName;
      *tcvr.lane_ref() = i * 2 + j;
      tcvrPin.set_end(tcvr);
      linePin.z_ref() = tcvrPin;

      xphy.line_ref()->push_back(linePin);
    }
    xphyPin.set_junction(xphy);
    pin.z_ref() = xphyPin;

    entry.mapping_ref()->pins_ref()->push_back(pin);
  }

  // prepare PortPinConfig for profiles
  cfg::PlatformPortConfig portCfg;
  std::vector<phy::PinConfig> xphySys;
  std::vector<phy::PinConfig> xphyLine;
  std::vector<phy::PinConfig> transceiver;
  for (int i = 0; i < 2; i++) {
    phy::PinConfig iphyCfg;
    *iphyCfg.id_ref()->chip_ref() = kDefaultIphyChipName;
    *iphyCfg.id_ref()->lane_ref() = kDefaultAsicLaneStart + i;
    portCfg.pins_ref()->iphy_ref()->push_back(iphyCfg);

    phy::PinConfig xphySysCfg;
    *xphySysCfg.id_ref()->chip_ref() = kDefaultXphyChipName;
    *xphySysCfg.id_ref()->lane_ref() = xphySysLaneStart + i;
    xphySys.push_back(xphySysCfg);

    for (int j = 0; j < 2; j++) {
      phy::PinConfig xphyLineCfg;
      *xphyLineCfg.id_ref()->chip_ref() = kDefaultXphyChipName;
      *xphyLineCfg.id_ref()->lane_ref() = xphyLineLaneStart + i * 2 + j;
      xphyLine.push_back(xphyLineCfg);

      phy::PinConfig tcvrCfg;
      *tcvrCfg.id_ref()->chip_ref() = kDefaultTcvrChipName;
      *tcvrCfg.id_ref()->lane_ref() = i * 2 + j;
      transceiver.push_back(tcvrCfg);
    }
  }
  portCfg.pins_ref()->transceiver_ref() = transceiver;
  portCfg.pins_ref()->xphySys_ref() = xphySys;
  portCfg.pins_ref()->xphyLine_ref() = xphyLine;
  entry.supportedProfiles_ref()[cfg::PortProfileID::PROFILE_100G_4_NRZ_RS528] =
      portCfg;

  return entry;
}

cfg::PlatformPortEntry getPlatformPortEntryWithoutXPHY() {
  cfg::PlatformPortEntry entry;

  *entry.mapping_ref()->id_ref() = 4;
  *entry.mapping_ref()->name_ref() = "eth2/2/1";
  *entry.mapping_ref()->controllingPort_ref() = 3;

  for (int i = 0; i < 4; i++) {
    phy::PinConnection pin;
    phy::PinID iphy;
    *iphy.chip_ref() = kDefaultIphyChipName;
    *iphy.lane_ref() = kDefaultAsicLaneStart + i;
    *pin.a_ref() = iphy;

    phy::Pin tcvrPin;
    phy::PinID tcvr;
    *tcvr.chip_ref() = kDefaultTcvrChipName;
    *tcvr.lane_ref() = i;
    tcvrPin.set_end(tcvr);
    pin.z_ref() = tcvrPin;

    entry.mapping_ref()->pins_ref()->push_back(pin);
  }

  // prepare PortPinConfig for profiles
  cfg::PlatformPortConfig portCfg;
  std::vector<phy::PinConfig> transceiver;
  for (int i = 0; i < 4; i++) {
    phy::PinConfig iphyCfg;
    *iphyCfg.id_ref()->chip_ref() = kDefaultIphyChipName;
    *iphyCfg.id_ref()->lane_ref() = kDefaultAsicLaneStart + i;
    portCfg.pins_ref()->iphy_ref()->push_back(iphyCfg);

    phy::PinConfig tcvrCfg;
    *tcvrCfg.id_ref()->chip_ref() = kDefaultTcvrChipName;
    *tcvrCfg.id_ref()->lane_ref() = i;
    transceiver.push_back(tcvrCfg);
  }
  portCfg.pins_ref()->transceiver_ref() = transceiver;
  entry.supportedProfiles_ref()[cfg::PortProfileID::PROFILE_100G_4_NRZ_RS528] =
      portCfg;

  return entry;
}

cfg::PlatformPortEntry getPlatformPortEntryWithoutTransceiver() {
  cfg::PlatformPortEntry entry;

  entry.mapping_ref()->id_ref() = 4;
  entry.mapping_ref()->name_ref() = "eth2/2/1";
  entry.mapping_ref()->controllingPort_ref() = 3;

  for (int i = 0; i < 4; i++) {
    phy::PinConnection pin;
    phy::PinID iphy;
    iphy.chip_ref() = kDefaultIphyChipName;
    iphy.lane_ref() = kDefaultAsicLaneStart + i;
    pin.a_ref() = iphy;

    entry.mapping_ref()->pins_ref()->push_back(pin);
  }

  // prepare PortPinConfig for profiles
  cfg::PlatformPortConfig portCfg;
  for (int i = 0; i < 4; i++) {
    phy::PinConfig iphyCfg;
    iphyCfg.id_ref()->chip_ref() = kDefaultIphyChipName;
    iphyCfg.id_ref()->lane_ref() = kDefaultAsicLaneStart + i;
    portCfg.pins_ref()->iphy_ref()->push_back(iphyCfg);
  }
  entry.supportedProfiles_ref()
      [cfg::PortProfileID::PROFILE_100G_4_NRZ_NOFEC_COPPER] = portCfg;

  return entry;
}

std::map<std::string, phy::DataPlanePhyChip> getPlatformChips() {
  std::map<std::string, phy::DataPlanePhyChip> chips;
  phy::DataPlanePhyChip iphy;
  *iphy.name_ref() = kDefaultIphyChipName;
  *iphy.type_ref() = phy::DataPlanePhyChipType::IPHY;
  *iphy.physicalID_ref() = 0;
  chips[*iphy.name_ref()] = iphy;

  phy::DataPlanePhyChip xphy;
  *xphy.name_ref() = kDefaultXphyChipName;
  *xphy.type_ref() = phy::DataPlanePhyChipType::XPHY;
  *xphy.physicalID_ref() = 8;
  chips[*xphy.name_ref()] = xphy;

  phy::DataPlanePhyChip tcvr;
  *tcvr.name_ref() = kDefaultTcvrChipName;
  *tcvr.type_ref() = phy::DataPlanePhyChipType::TRANSCEIVER;
  *tcvr.physicalID_ref() = 2;
  chips[*tcvr.name_ref()] = tcvr;

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
    EXPECT_EQ(*transeiverLanes[i].chip_ref(), kDefaultTcvrChipName);
    EXPECT_EQ(*transeiverLanes[i].lane_ref(), i);
  }
}

TEST(PlatformConfigUtilsTests, GetTransceiverLanesFromProfilesWithoutXPHY) {
  auto transeiverLanes = utility::getTransceiverLanes(
      getPlatformPortEntryWithoutXPHY(),
      getPlatformChips(),
      cfg::PortProfileID::PROFILE_100G_4_NRZ_RS528);
  EXPECT_EQ(transeiverLanes.size(), 4);
  for (int i = 0; i < 4; i++) {
    EXPECT_EQ(*transeiverLanes[i].chip_ref(), kDefaultTcvrChipName);
    EXPECT_EQ(*transeiverLanes[i].lane_ref(), i);
  }
}

TEST(PlatformConfigUtilsTests, GetTransceiverLanesFromMappingWithXPHY) {
  auto transeiverLanes = utility::getTransceiverLanes(
      getPlatformPortEntryWithXPHY(), getPlatformChips());
  EXPECT_EQ(transeiverLanes.size(), 4);
  for (int i = 0; i < 4; i++) {
    EXPECT_EQ(*transeiverLanes[i].chip_ref(), kDefaultTcvrChipName);
    EXPECT_EQ(*transeiverLanes[i].lane_ref(), i);
  }
}

TEST(PlatformConfigUtilsTests, GetTransceiverLanesFromMappingWithoutXPHY) {
  auto transeiverLanes = utility::getTransceiverLanes(
      getPlatformPortEntryWithoutXPHY(), getPlatformChips());
  EXPECT_EQ(transeiverLanes.size(), 4);
  for (int i = 0; i < 4; i++) {
    EXPECT_EQ(*transeiverLanes[i].chip_ref(), kDefaultTcvrChipName);
    EXPECT_EQ(*transeiverLanes[i].lane_ref(), i);
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
      *portEntry.mapping_ref()->id_ref() = controllingPort + i;
      *portEntry.mapping_ref()->controllingPort_ref() = controllingPort;
      platformPorts[*portEntry.mapping_ref()->id_ref()] = portEntry;
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
    EXPECT_EQ(*iphyLanes[i].chip_ref(), kDefaultIphyChipName);
    EXPECT_EQ(*iphyLanes[i].lane_ref(), kDefaultAsicLaneStart + i);
  }
}

TEST(PlatformConfigUtilsTests, GetOrderedIphyLanesFromProfilesWithoutXPHY) {
  auto iphyLanes = utility::getOrderedIphyLanes(
      getPlatformPortEntryWithoutXPHY(),
      getPlatformChips(),
      cfg::PortProfileID::PROFILE_100G_4_NRZ_RS528);
  EXPECT_EQ(iphyLanes.size(), 4);
  for (int i = 0; i < 4; i++) {
    EXPECT_EQ(*iphyLanes[i].chip_ref(), kDefaultIphyChipName);
    EXPECT_EQ(*iphyLanes[i].lane_ref(), kDefaultAsicLaneStart + i);
  }
}

TEST(PlatformConfigUtilsTests, GetOrderedIphyLanesFromMappingWithXPHY) {
  auto iphyLanes = utility::getOrderedIphyLanes(
      getPlatformPortEntryWithXPHY(), getPlatformChips());
  EXPECT_EQ(iphyLanes.size(), 2);
  for (int i = 0; i < 2; i++) {
    EXPECT_EQ(*iphyLanes[i].chip_ref(), kDefaultIphyChipName);
    EXPECT_EQ(*iphyLanes[i].lane_ref(), kDefaultAsicLaneStart + i);
  }
}

TEST(PlatformConfigUtilsTests, GetOrderedIphyLanesFromMappingWithoutXPHY) {
  auto iphyLanes = utility::getOrderedIphyLanes(
      getPlatformPortEntryWithoutXPHY(), getPlatformChips());
  EXPECT_EQ(iphyLanes.size(), 4);
  for (int i = 0; i < 4; i++) {
    EXPECT_EQ(*iphyLanes[i].chip_ref(), kDefaultIphyChipName);
    EXPECT_EQ(*iphyLanes[i].lane_ref(), kDefaultAsicLaneStart + i);
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
      *portEntry.mapping_ref()->id_ref() = controllingPort + i;
      *portEntry.mapping_ref()->controllingPort_ref() = controllingPort;
      platformPorts[*portEntry.mapping_ref()->id_ref()] = portEntry;
    }
  }

  int controllingPortID = 5;
  const auto& subsidiaryPorts = utility::getPlatformPortsByControllingPort(
      platformPorts, PortID(controllingPortID));
  EXPECT_EQ(subsidiaryPorts.size(), 4);
  for (int i = 0; i < 4; i++) {
    EXPECT_EQ(
        *subsidiaryPorts[i].mapping_ref()->id_ref(), controllingPortID + i);
    EXPECT_EQ(
        *subsidiaryPorts[i].mapping_ref()->controllingPort_ref(),
        controllingPortID);
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
  EXPECT_EQ(
      *portChips.begin()->second.type_ref(), phy::DataPlanePhyChipType::XPHY);
  EXPECT_EQ(*portChips.begin()->second.physicalID_ref(), 8);
}

TEST(PlatformConfigUtilsTests, GetTransceiverId) {
  auto transceiverId = utility::getTransceiverId(
      getPlatformPortEntryWithXPHY(), getPlatformChips());
  EXPECT_TRUE(transceiverId);
  EXPECT_EQ(transceiverId.value(), 2);

  transceiverId = utility::getTransceiverId(
      getPlatformPortEntryWithoutTransceiver(), getPlatformChips());
  EXPECT_FALSE(transceiverId);
}

} // namespace facebook::fboss::utility
