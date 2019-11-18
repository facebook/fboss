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
cfg::PlatformPortEntry getPlatformPortEntryWithXPHY() {
  cfg::PlatformPortEntry entry;

  entry.mapping.id = 4;
  entry.mapping.name = "eth2/2/1";
  entry.mapping.controllingPort = 3;

  int asicLaneStart = 6;
  int xphySysLaneStart = 6;
  int xphyLineLaneStart = 12;
  for (int i = 0; i < 2; i++) {
    phy::PinConnection pin;
    phy::PinID iphy;
    iphy.chip = "BC0";
    iphy.lane = asicLaneStart + i;
    pin.a = iphy;

    phy::Pin xphyPin;
    phy::PinJunction xphy;
    phy::PinID xphySys;
    xphySys.chip = "XPHY8";
    xphySys.lane = xphySysLaneStart + i;
    xphy.system = xphySys;

    for (int j = 0; j < 2; j++) {
      phy::PinConnection linePin;
      phy::PinID xphyLine;
      xphyLine.chip = "XPHY8";
      xphyLine.lane = xphyLineLaneStart + i * 2 + j;
      linePin.a = xphyLine;

      phy::Pin tcvrPin;
      phy::PinID tcvr;
      tcvr.chip = "eth2/2";
      tcvr.lane = i * 2 + j;
      tcvrPin.set_end(tcvr);
      linePin.set_z(tcvrPin);

      xphy.line.push_back(linePin);
    }
    xphyPin.set_junction(xphy);
    pin.set_z(xphyPin);

    entry.mapping.pins.push_back(pin);
  }

  // prepare PortPinConfig for profiles
  cfg::PlatformPortConfig portCfg;
  std::vector<phy::PinConfig> xphySys;
  std::vector<phy::PinConfig> xphyLine;
  std::vector<phy::PinConfig> transceiver;
  for (int i = 0; i < 2; i++) {
    phy::PinConfig iphyCfg;
    iphyCfg.id.chip = "BC0";
    iphyCfg.id.lane = asicLaneStart + i;
    portCfg.pins.iphy.push_back(iphyCfg);

    phy::PinConfig xphySysCfg;
    xphySysCfg.id.chip = "XPHY8";
    xphySysCfg.id.lane = xphySysLaneStart + i;
    xphySys.push_back(xphySysCfg);

    for (int j = 0; j < 2; j++) {
      phy::PinConfig xphyLineCfg;
      xphyLineCfg.id.chip = "XPHY8";
      xphyLineCfg.id.lane = xphyLineLaneStart + i * 2 + j;
      xphyLine.push_back(xphyLineCfg);

      phy::PinConfig tcvrCfg;
      tcvrCfg.id.chip = "eth2/2";
      tcvrCfg.id.lane = i * 2 + j;
      transceiver.push_back(tcvrCfg);
    }
  }
  portCfg.pins.set_transceiver(transceiver);
  portCfg.pins.set_xphySys(xphySys);
  portCfg.pins.set_xphyLine(xphyLine);
  entry.supportedProfiles[cfg::PortProfileID::PROFILE_100G_4_NRZ_RS528] =
      portCfg;

  return entry;
}

cfg::PlatformPortEntry getPlatformPortEntryWithoutXPHY() {
  cfg::PlatformPortEntry entry;

  entry.mapping.id = 4;
  entry.mapping.name = "eth2/2/1";
  entry.mapping.controllingPort = 3;

  int asicLaneStart = 6;
  for (int i = 0; i < 4; i++) {
    phy::PinConnection pin;
    phy::PinID iphy;
    iphy.chip = "BC0";
    iphy.lane = asicLaneStart + i;
    pin.a = iphy;

    phy::Pin tcvrPin;
    phy::PinID tcvr;
    tcvr.chip = "eth2/2";
    tcvr.lane = i;
    tcvrPin.set_end(tcvr);
    pin.set_z(tcvrPin);

    entry.mapping.pins.push_back(pin);
  }

  // prepare PortPinConfig for profiles
  cfg::PlatformPortConfig portCfg;
  std::vector<phy::PinConfig> transceiver;
  for (int i = 0; i < 4; i++) {
    phy::PinConfig iphyCfg;
    iphyCfg.id.chip = "BC0";
    iphyCfg.id.lane = asicLaneStart + i;
    portCfg.pins.iphy.push_back(iphyCfg);

    phy::PinConfig tcvrCfg;
    tcvrCfg.id.chip = "eth2/2";
    tcvrCfg.id.lane = i;
    transceiver.push_back(tcvrCfg);
  }
  portCfg.pins.set_transceiver(transceiver);
  entry.supportedProfiles[cfg::PortProfileID::PROFILE_100G_4_NRZ_RS528] =
      portCfg;

  return entry;
}

std::vector<phy::DataPlanePhyChip> getPlatformChips() {
  std::vector<phy::DataPlanePhyChip> chips;
  phy::DataPlanePhyChip iphy;
  iphy.name = "BC0";
  iphy.type = phy::DataPlanePhyChipType::IPHY;
  iphy.physicalID = 0;
  chips.push_back(iphy);

  phy::DataPlanePhyChip xphy;
  xphy.name = "XPHY8";
  xphy.type = phy::DataPlanePhyChipType::XPHY;
  xphy.physicalID = 8;
  chips.push_back(xphy);

  phy::DataPlanePhyChip tcvr;
  tcvr.name = "eth2/2";
  tcvr.type = phy::DataPlanePhyChipType::TRANSCEIVER;
  tcvr.physicalID = 2;
  chips.push_back(tcvr);

  return chips;
}
} // namespace

namespace facebook {
namespace fboss {
namespace utility {

TEST(PlatformConfigUtilsTests, GetTransceiverLanesFromProfilesWithXPHY) {
  auto transeiverLanes = utility::getTransceiverLanes(
      getPlatformPortEntryWithXPHY(),
      getPlatformChips(),
      cfg::PortProfileID::PROFILE_100G_4_NRZ_RS528);
  EXPECT_EQ(transeiverLanes.size(), 4);
  for (int i = 0; i < 4; i++) {
    EXPECT_EQ(transeiverLanes[i].chip, "eth2/2");
    EXPECT_EQ(transeiverLanes[i].lane, i);
  }
}

TEST(PlatformConfigUtilsTests, GetTransceiverLanesFromProfilesWithoutXPHY) {
  auto transeiverLanes = utility::getTransceiverLanes(
      getPlatformPortEntryWithoutXPHY(),
      getPlatformChips(),
      cfg::PortProfileID::PROFILE_100G_4_NRZ_RS528);
  EXPECT_EQ(transeiverLanes.size(), 4);
  for (int i = 0; i < 4; i++) {
    EXPECT_EQ(transeiverLanes[i].chip, "eth2/2");
    EXPECT_EQ(transeiverLanes[i].lane, i);
  }
}

TEST(PlatformConfigUtilsTests, GetTransceiverLanesFromMappingWithXPHY) {
  auto transeiverLanes = utility::getTransceiverLanes(
      getPlatformPortEntryWithXPHY(), getPlatformChips());
  EXPECT_EQ(transeiverLanes.size(), 4);
  for (int i = 0; i < 4; i++) {
    EXPECT_EQ(transeiverLanes[i].chip, "eth2/2");
    EXPECT_EQ(transeiverLanes[i].lane, i);
  }
}

TEST(PlatformConfigUtilsTests, GetTransceiverLanesFromMappingWithoutXPHY) {
  auto transeiverLanes = utility::getTransceiverLanes(
      getPlatformPortEntryWithoutXPHY(), getPlatformChips());
  EXPECT_EQ(transeiverLanes.size(), 4);
  for (int i = 0; i < 4; i++) {
    EXPECT_EQ(transeiverLanes[i].chip, "eth2/2");
    EXPECT_EQ(transeiverLanes[i].lane, i);
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

} // namespace utility
} // namespace fboss
} // namespace facebook
