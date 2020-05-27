/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/platforms/wedge/tests/PlatformMappingTest.h"

#include "fboss/agent/platforms/common/galaxy/GalaxyFCPlatformMapping.h"
#include "fboss/agent/platforms/common/galaxy/GalaxyLCPlatformMapping.h"
#include "fboss/agent/platforms/common/wedge100/Wedge100PlatformMapping.h"
#include "fboss/agent/platforms/common/wedge40/Wedge40PlatformMapping.h"
#include "fboss/agent/platforms/wedge/minipack/Minipack16QPimPlatformMapping.h"
#include "fboss/agent/platforms/wedge/wedge400/Wedge400PlatformMapping.h"
#include "fboss/agent/platforms/wedge/yamp/YampPlatformMapping.h"

#include <gtest/gtest.h>

namespace facebook {
namespace fboss {
namespace test {

TEST_F(PlatformMappingTest, VerifyWedge400PlatformMapping) {
  // supported profiles
  std::vector<cfg::PortProfileID> expectedProfiles = {
      cfg::PortProfileID::PROFILE_10G_1_NRZ_NOFEC_COPPER,
      cfg::PortProfileID::PROFILE_25G_1_NRZ_NOFEC_COPPER,
      cfg::PortProfileID::PROFILE_40G_4_NRZ_NOFEC_COPPER,
      cfg::PortProfileID::PROFILE_50G_2_NRZ_NOFEC_COPPER,
      cfg::PortProfileID::PROFILE_100G_4_NRZ_RS528_COPPER,
      cfg::PortProfileID::PROFILE_100G_4_NRZ_RS528_OPTICAL,
      cfg::PortProfileID::PROFILE_200G_4_PAM4_RS544X2N_COPPER,
      cfg::PortProfileID::PROFILE_200G_4_PAM4_RS544X2N_OPTICAL,
      cfg::PortProfileID::PROFILE_400G_8_PAM4_RS544X2N_OPTICAL};

  // Wedge400 has 16 uplinks + 4 * 32 downlink ports = 144
  // 32 TH3 Blackhawk cores + 48 transceivers
  setExpection(144, 32, 0, 48, expectedProfiles);

  auto mapping = std::make_unique<Wedge400PlatformMapping>();
  verify(mapping.get());
}

TEST_F(PlatformMappingTest, VerifyWedge400PortIphyPinConfigs) {
  // All the downlink ports should be using the same tx_settings: nrz:-8:89:0
  auto mapping = std::make_unique<Wedge400PlatformMapping>();
  for (auto port : mapping->getPlatformPorts()) {
    const auto& profiles = port.second.supportedProfiles;
    // Only downlink ports has 10G profile
    if (profiles.find(cfg::PortProfileID::PROFILE_10G_1_NRZ_NOFEC_COPPER) ==
        profiles.end()) {
      continue;
    }
    for (auto profile : profiles) {
      auto pinCfgs =
          mapping->getPortIphyPinConfigs(PortID(port.first), profile.first);
      EXPECT_TRUE(pinCfgs.size() > 0);

      auto itProfileCfg = mapping->getSupportedProfiles().find(profile.first);
      EXPECT_TRUE(itProfileCfg != mapping->getSupportedProfiles().end());

      for (auto pinCfg : pinCfgs) {
        auto tx = pinCfg.tx_ref();
        // Only NRZ mode has override tx
        if (itProfileCfg->second.iphy.modulation == phy::IpModulation::NRZ) {
          EXPECT_TRUE(tx.has_value());
          EXPECT_EQ(tx->pre2, 0);
          EXPECT_EQ(tx->pre, -8);
          EXPECT_EQ(tx->main, 89);
          EXPECT_EQ(tx->post, 0);
          EXPECT_EQ(tx->post2, 0);
          EXPECT_EQ(tx->post3, 0);
        } else {
          EXPECT_FALSE(tx.has_value());
        }
      }
    }
  }

  // Not override case will read directly from PlatformPortConfig
  std::unordered_map<PortID, std::vector<int>> uplinkTxMap = {
      {PortID(36), {0, -6, 92, -24, 0, 0}},
      {PortID(37), {0, -2, 90, -18, 0, 0}},
      {PortID(56), {0, -2, 90, -18, 0, 0}},
      {PortID(57), {0, -2, 90, -18, 0, 0}},
      {PortID(17), {0, -6, 92, -24, 0, 0}},
      {PortID(18), {0, -2, 90, -22, 0, 0}},
      {PortID(76), {0, -2, 78, -10, 0, 0}},
      {PortID(77), {0, -2, 66, -8, 0, 0}},
      {PortID(96), {0, -2, 66, -8, 0, 0}},
      {PortID(97), {0, -2, 78, -10, 0, 0}},
      {PortID(156), {0, -2, 88, -20, 0, 0}},
      {PortID(157), {0, -6, 92, -24, 0, 0}},
      {PortID(116), {0, -2, 90, -18, 0, 0}},
      {PortID(117), {0, -2, 90, -18, 0, 0}},
      {PortID(136), {0, -2, 88, -20, 0, 0}},
      {PortID(137), {0, -6, 92, -24, 0, 0}},

  };
  for (auto uplinkTx : uplinkTxMap) {
    const auto& pinCfgs = mapping->getPortIphyPinConfigs(
        uplinkTx.first, cfg::PortProfileID::PROFILE_100G_4_NRZ_RS528_OPTICAL);
    EXPECT_EQ(pinCfgs.size(), 4);
    for (auto pinCfg : pinCfgs) {
      if (auto tx = pinCfg.tx_ref()) {
        EXPECT_EQ(tx->pre2, uplinkTx.second[0]);
        EXPECT_EQ(tx->pre, uplinkTx.second[1]);
        EXPECT_EQ(tx->main, uplinkTx.second[2]);
        EXPECT_EQ(tx->post, uplinkTx.second[3]);
        EXPECT_EQ(tx->post2, uplinkTx.second[4]);
        EXPECT_EQ(tx->post3, uplinkTx.second[5]);
      }
    }
  }
}

TEST_F(PlatformMappingTest, VerifyYampPlatformMapping) {
  // supported profiles
  std::vector<cfg::PortProfileID> expectedProfiles = {
      cfg::PortProfileID::PROFILE_40G_4_NRZ_NOFEC,
      cfg::PortProfileID::PROFILE_100G_4_NRZ_RS528,
      cfg::PortProfileID::PROFILE_200G_4_PAM4_RS544X2N};

  // Yamp has 128 ports
  // 32 TH3 Blackhawk cores + 128 transceivers + 64 xphy
  setExpection(128, 32, 64, 128, expectedProfiles);

  auto mapping = std::make_unique<YampPlatformMapping>();
  verify(mapping.get());
}

TEST_F(PlatformMappingTest, VerifyMinipack16QPlatformMapping) {
  // supported profiles
  std::vector<cfg::PortProfileID> expectedProfiles = {
      cfg::PortProfileID::PROFILE_40G_4_NRZ_NOFEC,
      cfg::PortProfileID::PROFILE_100G_4_NRZ_RS528};

  // Minipack16Q has 128 ports
  // 32 TH3 Blackhawk cores + 128 transceivers + 32 xphy
  setExpection(128, 32, 32, 128, expectedProfiles);

  auto mapping = std::make_unique<Minipack16QPimPlatformMapping>();
  verify(mapping.get());
}

TEST_F(PlatformMappingTest, VerifyWedge40PlatformMapping) {
  // supported profiles
  std::vector<cfg::PortProfileID> expectedProfiles = {
      cfg::PortProfileID::PROFILE_10G_1_NRZ_NOFEC,
      cfg::PortProfileID::PROFILE_20G_2_NRZ_NOFEC,
      cfg::PortProfileID::PROFILE_40G_4_NRZ_NOFEC,
      cfg::PortProfileID::PROFILE_10G_1_NRZ_NOFEC_COPPER,
      cfg::PortProfileID::PROFILE_10G_1_NRZ_NOFEC_OPTICAL,
      cfg::PortProfileID::PROFILE_20G_2_NRZ_NOFEC_COPPER,
      cfg::PortProfileID::PROFILE_20G_2_NRZ_NOFEC_OPTICAL,
      cfg::PortProfileID::PROFILE_40G_4_NRZ_NOFEC_COPPER,
      cfg::PortProfileID::PROFILE_40G_4_NRZ_NOFEC_OPTICAL};

  // Wedge40 has 16 * 4 = 64 logical ports
  // 16 TD2 Warp cores + 16 transceivers
  setExpection(64, 16, 0, 16, expectedProfiles);

  auto mapping = std::make_unique<Wedge40PlatformMapping>();
  verify(mapping.get());
}

TEST_F(PlatformMappingTest, VerifyWedge100PlatformMapping) {
  // supported profiles
  std::vector<cfg::PortProfileID> expectedProfiles = {
      cfg::PortProfileID::PROFILE_10G_1_NRZ_NOFEC,
      cfg::PortProfileID::PROFILE_20G_2_NRZ_NOFEC,
      cfg::PortProfileID::PROFILE_25G_1_NRZ_NOFEC,
      cfg::PortProfileID::PROFILE_40G_4_NRZ_NOFEC,
      cfg::PortProfileID::PROFILE_50G_2_NRZ_NOFEC,
      cfg::PortProfileID::PROFILE_100G_4_NRZ_CL91,
      cfg::PortProfileID::PROFILE_10G_1_NRZ_NOFEC_COPPER,
      cfg::PortProfileID::PROFILE_10G_1_NRZ_NOFEC_OPTICAL,
      cfg::PortProfileID::PROFILE_20G_2_NRZ_NOFEC_COPPER,
      cfg::PortProfileID::PROFILE_20G_2_NRZ_NOFEC_OPTICAL,
      cfg::PortProfileID::PROFILE_25G_1_NRZ_NOFEC_COPPER,
      cfg::PortProfileID::PROFILE_25G_1_NRZ_CL74_COPPER,
      cfg::PortProfileID::PROFILE_25G_1_NRZ_NOFEC_OPTICAL,
      cfg::PortProfileID::PROFILE_40G_4_NRZ_NOFEC_COPPER,
      cfg::PortProfileID::PROFILE_40G_4_NRZ_NOFEC_OPTICAL,
      cfg::PortProfileID::PROFILE_50G_2_NRZ_NOFEC_COPPER,
      cfg::PortProfileID::PROFILE_50G_2_NRZ_CL74_COPPER,
      cfg::PortProfileID::PROFILE_50G_2_NRZ_NOFEC_OPTICAL,
      cfg::PortProfileID::PROFILE_100G_4_NRZ_CL91_COPPER,
      cfg::PortProfileID::PROFILE_100G_4_NRZ_CL91_OPTICAL};

  // Wedge40 has 32 * 4 = 128 logical ports
  // 32 TH Falcon cores + 32 transceivers
  setExpection(128, 32, 0, 32, expectedProfiles);

  auto mapping = std::make_unique<Wedge100PlatformMapping>();
  verify(mapping.get());
}

TEST_F(PlatformMappingTest, VerifyGalaxyFCPlatformMapping) {
  // supported profiles
  std::vector<cfg::PortProfileID> expectedProfiles = {
      cfg::PortProfileID::PROFILE_10G_1_NRZ_NOFEC,
      cfg::PortProfileID::PROFILE_25G_1_NRZ_NOFEC,
      cfg::PortProfileID::PROFILE_40G_4_NRZ_NOFEC,
      cfg::PortProfileID::PROFILE_50G_2_NRZ_NOFEC,
      cfg::PortProfileID::PROFILE_100G_4_NRZ_CL91,
      cfg::PortProfileID::PROFILE_10G_1_NRZ_NOFEC_OPTICAL,
      cfg::PortProfileID::PROFILE_25G_1_NRZ_NOFEC_COPPER,
      cfg::PortProfileID::PROFILE_25G_1_NRZ_NOFEC_OPTICAL,
      cfg::PortProfileID::PROFILE_40G_4_NRZ_NOFEC_OPTICAL,
      cfg::PortProfileID::PROFILE_50G_2_NRZ_NOFEC_COPPER,
      cfg::PortProfileID::PROFILE_50G_2_NRZ_NOFEC_OPTICAL,
      cfg::PortProfileID::PROFILE_100G_4_NRZ_CL91_OPTICAL,
      cfg::PortProfileID::PROFILE_100G_4_NRZ_NOFEC_COPPER};

  // Galaxy FC has 32 * 4 = 128 logical ports
  // 32 TH Falcon cores + 0 transceivers
  setExpection(128, 32, 0, 0, expectedProfiles);

  auto mapping = std::make_unique<GalaxyFCPlatformMapping>("fc003");
  verify(mapping.get());

  // also check all the port name should starts with fab3
  for (const auto& port : mapping->getPlatformPorts()) {
    EXPECT_EQ(port.second.mapping.name.rfind("fab3", 0), 0);
  }
}

TEST_F(PlatformMappingTest, VerifyGalaxyLCPlatformMapping) {
  // supported profiles
  std::vector<cfg::PortProfileID> expectedProfiles = {
      cfg::PortProfileID::PROFILE_10G_1_NRZ_NOFEC,
      cfg::PortProfileID::PROFILE_25G_1_NRZ_NOFEC,
      cfg::PortProfileID::PROFILE_40G_4_NRZ_NOFEC,
      cfg::PortProfileID::PROFILE_50G_2_NRZ_NOFEC,
      cfg::PortProfileID::PROFILE_100G_4_NRZ_CL91,
      cfg::PortProfileID::PROFILE_10G_1_NRZ_NOFEC_OPTICAL,
      cfg::PortProfileID::PROFILE_25G_1_NRZ_NOFEC_COPPER,
      cfg::PortProfileID::PROFILE_25G_1_NRZ_NOFEC_OPTICAL,
      cfg::PortProfileID::PROFILE_40G_4_NRZ_NOFEC_OPTICAL,
      cfg::PortProfileID::PROFILE_50G_2_NRZ_NOFEC_COPPER,
      cfg::PortProfileID::PROFILE_50G_2_NRZ_NOFEC_OPTICAL,
      cfg::PortProfileID::PROFILE_100G_4_NRZ_CL91_OPTICAL,
      cfg::PortProfileID::PROFILE_100G_4_NRZ_NOFEC_COPPER};

  // Galaxy LC has 32 * 4 = 128 logical ports
  // 32 TH Falcon cores + 16 transceivers
  setExpection(128, 32, 0, 16, expectedProfiles);

  auto mapping = std::make_unique<GalaxyLCPlatformMapping>("lc301");
  verify(mapping.get());

  // also check all the port name should starts with fab3/eth301
  for (const auto& port : mapping->getPlatformPorts()) {
    EXPECT_TRUE(
        port.second.mapping.name.rfind("fab301", 0) == 0 ||
        port.second.mapping.name.rfind("eth301", 0) == 0);
  }
}
} // namespace test
} // namespace fboss
} // namespace facebook
