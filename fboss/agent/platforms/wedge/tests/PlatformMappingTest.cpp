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

#include "fboss/agent/FbossError.h"
#include "fboss/agent/platforms/common/PlatformMode.h"
#include "fboss/agent/platforms/common/elbert/Elbert16QPimPlatformMapping.h"
#include "fboss/agent/platforms/common/galaxy/GalaxyFCPlatformMapping.h"
#include "fboss/agent/platforms/common/galaxy/GalaxyLCPlatformMapping.h"
#include "fboss/agent/platforms/common/minipack/Minipack16QPimPlatformMapping.h"
#include "fboss/agent/platforms/common/wedge100/Wedge100PlatformMapping.h"
#include "fboss/agent/platforms/common/wedge40/Wedge40PlatformMapping.h"
#include "fboss/agent/platforms/common/wedge400/Wedge400PlatformMapping.h"
#include "fboss/agent/platforms/common/yamp/YampPlatformMapping.h"

#include <gtest/gtest.h>

namespace facebook::fboss::test {

cfg::PlatformPortProfileConfigEntry createPlatformPortProfileConfigEntry(
    cfg::PortProfileID profileID,
    std::set<int> pimIDs,
    cfg::PortSpeed speed,
    phy::FecMode iphyFec) {
  cfg::PlatformPortConfigFactor factor;
  factor.profileID_ref() = profileID;
  factor.pimIDs_ref() = pimIDs;

  phy::PortProfileConfig config;
  phy::ProfileSideConfig iphy;
  iphy.fec_ref() = iphyFec;
  config.iphy_ref() = iphy;
  config.speed_ref() = speed;

  cfg::PlatformPortProfileConfigEntry configEntry;
  configEntry.factor_ref() = factor;
  configEntry.profile_ref() = config;
  return configEntry;
}

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
      cfg::PortProfileID::PROFILE_400G_8_PAM4_RS544X2N_OPTICAL,
      cfg::PortProfileID::PROFILE_25G_1_NRZ_CL74_COPPER,
      cfg::PortProfileID::PROFILE_50G_2_NRZ_RS528_COPPER};

  // Wedge400 has 16 uplinks + 4 * 32 downlink ports = 144
  // 32 TH3 Blackhawk cores + 48 transceivers
  setExpectation(144, 32, 0, 48, expectedProfiles);

  auto mapping = std::make_unique<Wedge400PlatformMapping>();
  verify(mapping.get());
}

TEST_F(PlatformMappingTest, VerifyWedge400PortIphyPinConfigs) {
  // All the downlink ports should be using the same tx_settings: nrz:-8:89:0
  auto mapping = std::make_unique<Wedge400PlatformMapping>();
  for (auto port : mapping->getPlatformPorts()) {
    const auto& profiles = *port.second.supportedProfiles_ref();

    // Only downlink ports has 10G profile
    if (profiles.find(cfg::PortProfileID::PROFILE_10G_1_NRZ_NOFEC_COPPER) ==
        profiles.end()) {
      continue;
    }
    for (auto profile : profiles) {
      auto pinCfgs = mapping->getPortIphyPinConfigs(
          PlatformPortProfileConfigMatcher(profile.first, PortID(port.first)));
      EXPECT_TRUE(pinCfgs.size() > 0);

      auto itProfileCfg = mapping->getPortProfileConfig(
          PlatformPortProfileConfigMatcher(PlatformPortProfileConfigMatcher(
              profile.first, PortID(port.first))));
      EXPECT_TRUE(itProfileCfg.has_value());

      for (auto pinCfg : pinCfgs) {
        auto tx = pinCfg.tx_ref();
        if (*itProfileCfg->iphy_ref()->modulation_ref() ==
            phy::IpModulation::NRZ) {
          EXPECT_TRUE(tx.has_value());
          verifyTxSettings(*tx, {0, -8, 89, 0, 0, 0});
        } else if (
            *itProfileCfg->iphy_ref()->modulation_ref() ==
            phy::IpModulation::PAM4) {
          if (profile.first ==
              cfg::PortProfileID::PROFILE_200G_4_PAM4_RS544X2N_COPPER) {
            EXPECT_TRUE(tx.has_value());
            verifyTxSettings(*tx, {4, -20, 140, 0, 0, 0});
          } else {
            EXPECT_FALSE(tx.has_value());
          }
        } else {
          EXPECT_FALSE(tx.has_value());
        }
      }
    }
  }

  // Not override case will read directly from PlatformPortConfig
  std::unordered_map<PortID, std::array<int, 6>> uplinkTxMapForProfile23 = {
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

  for (auto uplinkTx : uplinkTxMapForProfile23) {
    // this is profile 23
    const auto& pinCfgs =
        mapping->getPortIphyPinConfigs(PlatformPortProfileConfigMatcher(
            cfg::PortProfileID::PROFILE_100G_4_NRZ_RS528_OPTICAL,
            uplinkTx.first));
    EXPECT_EQ(pinCfgs.size(), 4);
    for (auto pinCfg : pinCfgs) {
      auto tx = pinCfg.tx_ref();
      EXPECT_TRUE(tx.has_value());
      verifyTxSettings(*tx, uplinkTx.second);
    }
  }

  // profile25 has different tx settings per lane
  //  {portID: { laneId1: {txSettings1}, laneId2: {txSettings2} .... }}
  std::unordered_map<PortID, std::unordered_map<int, std::array<int, 6>>>
      uplinkTxMapForProfile25 = {
          {
              PortID(17),
              {
                  {0, {0, -12, 140, -16, 0, 0}},
                  {1, {0, -12, 140, -16, 0, 0}},
                  {2, {0, -12, 140, -16, 0, 0}},
                  {3, {0, -12, 140, -16, 0, 0}},
              },
          },
          {
              PortID(18),
              {
                  {0, {0, -12, 140, -12, 0, 0}},
                  {1, {0, -12, 140, -16, 0, 0}},
                  {2, {0, -12, 140, -16, 0, 0}},
                  {3, {0, -12, 140, -12, 0, 0}},
              },
          },
          {
              PortID(36),
              {
                  {0, {0, -12, 136, -16, 0, 0}},
                  {1, {0, -12, 136, -16, 0, 0}},
                  {2, {0, -12, 136, -16, 0, 0}},
                  {3, {0, -12, 132, -16, 0, 0}},
              },
          },
          {
              PortID(37),
              {
                  {0, {0, -16, 140, -12, 0, 0}},
                  {1, {0, -16, 140, -12, 0, 0}},
                  {2, {0, -16, 140, -12, 0, 0}},
                  {3, {0, -16, 136, -12, 0, 0}},
              },
          },
          {
              PortID(56),
              {
                  {0, {0, -8, 136, -16, 0, 0}},
                  {1, {0, -12, 140, -16, 0, 0}},
                  {2, {0, -12, 136, -12, 0, 0}},
                  {3, {0, -16, 140, -12, 0, 0}},
              },
          },
          {
              PortID(57),
              {
                  {0, {0, -12, 140, -12, 0, 0}},
                  {1, {0, -12, 136, -12, 0, 0}},
                  {2, {0, -12, 136, -12, 0, 0}},
                  {3, {0, -12, 136, -12, 0, 0}},
              },
          },
          {
              PortID(76),
              {
                  {0, {0, -12, 136, -12, 0, 0}},
                  {1, {0, -12, 136, -12, 0, 0}},
                  {2, {0, -12, 136, -12, 0, 0}},
                  {3, {0, -12, 136, -12, 0, 0}},
              },
          },
          {
              PortID(77),
              {
                  {0, {0, -8, 132, -8, 0, 0}},
                  {1, {0, -8, 136, -8, 0, 0}},
                  {2, {0, -8, 128, -4, 0, 0}},
                  {3, {0, -8, 136, -8, 0, 0}},
              },
          },
          {
              PortID(96),
              {
                  {0, {0, -8, 136, -8, 0, 0}},
                  {1, {0, -8, 136, -8, 0, 0}},
                  {2, {0, -8, 136, -8, 0, 0}},
                  {3, {0, -8, 136, -8, 0, 0}},
              },
          },
          {
              PortID(97),
              {
                  {0, {0, -8, 136, -8, 0, 0}},
                  {1, {0, -8, 136, -8, 0, 0}},
                  {2, {0, -8, 136, -8, 0, 0}},
                  {3, {0, -8, 136, -8, 0, 0}},
              },
          },
          {
              PortID(116),
              {
                  {0, {0, -12, 140, -12, 0, 0}},
                  {1, {0, -12, 136, -12, 0, 0}},
                  {2, {0, -12, 140, -12, 0, 0}},
                  {3, {0, -12, 136, -12, 0, 0}},
              },
          },
          {
              PortID(117),
              {
                  {0, {0, -12, 132, -12, 0, 0}},
                  {1, {0, -12, 132, -12, 0, 0}},
                  {2, {0, -12, 132, -12, 0, 0}},
                  {3, {0, -12, 132, -12, 0, 0}},
              },
          },
          {
              PortID(136),
              {
                  {0, {0, -12, 132, -12, 0, 0}},
                  {1, {0, -12, 132, -16, 0, 0}},
                  {2, {0, -12, 132, -12, 0, 0}},
                  {3, {0, -12, 132, -12, 0, 0}},
              },
          },
          {
              PortID(137),
              {
                  {0, {0, -16, 136, -12, 0, 0}},
                  {1, {0, -8, 128, -16, 0, 0}},
                  {2, {0, -12, 132, -16, 0, 0}},
                  {3, {0, -12, 140, -16, 0, 0}},
              },
          },
          {
              PortID(156),
              {
                  {0, {0, -12, 136, -12, 0, 0}},
                  {1, {0, -12, 136, -12, 0, 0}},
                  {2, {0, -12, 136, -12, 0, 0}},
                  {3, {0, -12, 136, -12, 0, 0}},
              },
          },
          {
              PortID(157),
              {
                  {0, {0, -16, 136, -12, 0, 0}},
                  {1, {0, -16, 136, -12, 0, 0}},
                  {2, {0, -16, 136, -12, 0, 0}},
                  {3, {0, -16, 136, -12, 0, 0}},
              },
          },
      };

  for (auto laneToUplinkTxSettingsToPortMap : uplinkTxMapForProfile25) {
    // this is profile 25
    const auto& pinCfgs =
        mapping->getPortIphyPinConfigs(PlatformPortProfileConfigMatcher(
            cfg::PortProfileID::PROFILE_200G_4_PAM4_RS544X2N_OPTICAL,
            laneToUplinkTxSettingsToPortMap.first));
    EXPECT_EQ(pinCfgs.size(), 4);
    for (auto pinCfg : pinCfgs) {
      auto pinId = *pinCfg.id_ref();
      auto laneId = *pinId.lane_ref();
      auto tx = pinCfg.tx_ref();
      EXPECT_TRUE(tx.has_value());
      auto laneToUplinkTxSettinsMap = laneToUplinkTxSettingsToPortMap.second;
      auto uplinkTxSettings = laneToUplinkTxSettinsMap[laneId];
      verifyTxSettings(*tx, uplinkTxSettings);
    }
  }
}

TEST_F(PlatformMappingTest, VerifyYampPortProfileConfigOverride) {
  auto mapping = std::make_unique<YampPlatformMapping>();
  TransceiverInfo transceiverInfo = TransceiverInfo();
  transceiverInfo.extendedSpecificationComplianceCode_ref() =
      ExtendedSpecComplianceCode::FR1_100G;
  for (auto port : mapping->getPlatformPorts()) {
    auto portProfileConfig =
        mapping->getPortProfileConfig(PlatformPortProfileConfigMatcher(
            cfg::PortProfileID::PROFILE_40G_4_NRZ_NOFEC, PortID(port.first)));
    EXPECT_TRUE(
        *portProfileConfig->xphyLine_ref()->fec_ref() == phy::FecMode::NONE);

    portProfileConfig =
        mapping->getPortProfileConfig(PlatformPortProfileConfigMatcher(
            cfg::PortProfileID::PROFILE_100G_4_NRZ_RS528, PortID(port.first)));
    EXPECT_TRUE(
        *portProfileConfig->xphyLine_ref()->fec_ref() == phy::FecMode::RS528);

    portProfileConfig =
        mapping->getPortProfileConfig(PlatformPortProfileConfigMatcher(
            cfg::PortProfileID::PROFILE_100G_4_NRZ_RS528, transceiverInfo));
    EXPECT_TRUE(
        *portProfileConfig->xphyLine_ref()->fec_ref() == phy::FecMode::NONE);

    portProfileConfig =
        mapping->getPortProfileConfig(PlatformPortProfileConfigMatcher(
            cfg::PortProfileID::PROFILE_200G_4_PAM4_RS544X2N,
            PortID(port.first)));
    EXPECT_TRUE(
        *portProfileConfig->xphyLine_ref()->fec_ref() == phy::FecMode::NONE);
  }
}

TEST_F(PlatformMappingTest, VerifyYampPortXphyLinePinConfigOverride) {
  auto mapping = std::make_unique<YampPlatformMapping>();
  TransceiverInfo transceiverInfo = TransceiverInfo();

  std::array<int, 6> YampPort100GSffXphyLinePinConfig = {0, -4, 23, -12, 0, 0};
  std::array<int, 6> YampPort100GCmisXphyLinePinConfig = {0, -2, 15, -7, 0, 0};

  for (auto port : mapping->getPlatformPorts()) {
    transceiverInfo.transceiverManagementInterface_ref() =
        TransceiverManagementInterface::SFF;
    auto portXphyLinePinConfigs = mapping->getPortXphySidePinConfigs(
        PlatformPortProfileConfigMatcher(
            cfg::PortProfileID::PROFILE_100G_4_NRZ_RS528,
            PortID(port.first),
            transceiverInfo),
        phy::Side::LINE);

    EXPECT_EQ(portXphyLinePinConfigs.size(), 4);
    for (auto portXphyLinePinConfig : portXphyLinePinConfigs) {
      auto pinId = *portXphyLinePinConfig.id_ref();
      if (auto tx = portXphyLinePinConfig.tx_ref()) {
        verifyTxSettings(*tx, YampPort100GSffXphyLinePinConfig);
      }
    }

    transceiverInfo.transceiverManagementInterface_ref() =
        TransceiverManagementInterface::CMIS;
    FLAGS_override_cmis_tx_setting = false;
    portXphyLinePinConfigs = mapping->getPortXphySidePinConfigs(
        PlatformPortProfileConfigMatcher(
            cfg::PortProfileID::PROFILE_100G_4_NRZ_RS528,
            PortID(port.first),
            transceiverInfo),
        phy::Side::LINE);

    EXPECT_EQ(portXphyLinePinConfigs.size(), 4);
    for (auto portXphyLinePinConfig : portXphyLinePinConfigs) {
      auto pinId = *portXphyLinePinConfig.id_ref();
      if (auto tx = portXphyLinePinConfig.tx_ref()) {
        verifyTxSettings(*tx, YampPort100GSffXphyLinePinConfig);
      }
    }

    FLAGS_override_cmis_tx_setting = true;
    portXphyLinePinConfigs = mapping->getPortXphySidePinConfigs(
        PlatformPortProfileConfigMatcher(
            cfg::PortProfileID::PROFILE_100G_4_NRZ_RS528,
            PortID(port.first),
            transceiverInfo),
        phy::Side::LINE);

    EXPECT_EQ(portXphyLinePinConfigs.size(), 4);
    for (auto portXphyLinePinConfig : portXphyLinePinConfigs) {
      auto pinId = *portXphyLinePinConfig.id_ref();
      if (auto tx = portXphyLinePinConfig.tx_ref()) {
        verifyTxSettings(*tx, YampPort100GCmisXphyLinePinConfig);
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
  setExpectation(128, 32, 64, 128, expectedProfiles);

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
  setExpectation(128, 32, 32, 128, expectedProfiles);

  // both xphy version should pass the same standard
  for (auto xphyVersion :
       {ExternalPhyVersion::MILN4_2, ExternalPhyVersion::MILN5_2}) {
    auto mapping = std::make_unique<Minipack16QPimPlatformMapping>(xphyVersion);
    verify(mapping.get());
  }
}

TEST_F(PlatformMappingTest, VerifyOverrideMerge) {
  auto miln4_2 = std::make_unique<Minipack16QPimPlatformMapping>(
      ExternalPhyVersion::MILN4_2);
  auto miln5_2 = std::make_unique<Minipack16QPimPlatformMapping>(
      ExternalPhyVersion::MILN5_2);
  for (const auto& port : miln5_2->getPlatformPorts()) {
    miln4_2->mergePortConfigOverrides(
        port.first, miln5_2->getPortConfigOverrides(port.first));
  }

  // Now 4.2 should be exactly the same as 5.2
  EXPECT_EQ(miln4_2->getPortConfigOverrides().size(), 1);
  for (auto miln4_2Override : miln4_2->getPortConfigOverrides()) {
    auto overrideProfileList = miln4_2Override.factor_ref()->profiles_ref();
    EXPECT_TRUE(overrideProfileList);
    EXPECT_TRUE(
        std::find(
            overrideProfileList->begin(),
            overrideProfileList->end(),
            cfg::PortProfileID::PROFILE_100G_4_NRZ_RS528) !=
        overrideProfileList->end());

    auto overridePortList = miln4_2Override.factor_ref()->ports_ref();
    EXPECT_EQ(overridePortList->size(), miln4_2->getPlatformPorts().size());
    // Now all the port should use this override
    for (const auto& port : miln4_2->getPlatformPorts()) {
      EXPECT_TRUE(
          std::find(
              overridePortList->begin(), overridePortList->end(), port.first) !=
          overridePortList->end());

      auto pinCfgs =
          miln4_2->getPortIphyPinConfigs(PlatformPortProfileConfigMatcher(
              cfg::PortProfileID::PROFILE_100G_4_NRZ_RS528,
              PortID(port.first)));
      for (auto pinCfg : pinCfgs) {
        auto tx = pinCfg.tx_ref();
        EXPECT_TRUE(tx.has_value());
        verifyTxSettings(*tx, {0, 0, 128, 0, 0, 0});
      }
    }
  }
}

TEST_F(PlatformMappingTest, VerifyWedge40PlatformMapping) {
  // supported profiles
  std::vector<cfg::PortProfileID> expectedProfiles = {
      cfg::PortProfileID::PROFILE_10G_1_NRZ_NOFEC_COPPER,
      cfg::PortProfileID::PROFILE_10G_1_NRZ_NOFEC_OPTICAL,
      cfg::PortProfileID::PROFILE_20G_2_NRZ_NOFEC_COPPER,
      cfg::PortProfileID::PROFILE_20G_2_NRZ_NOFEC_OPTICAL,
      cfg::PortProfileID::PROFILE_40G_4_NRZ_NOFEC_COPPER,
      cfg::PortProfileID::PROFILE_40G_4_NRZ_NOFEC_OPTICAL};

  // Wedge40 has 16 * 4 = 64 logical ports
  // 16 TD2 Warp cores + 16 transceivers
  setExpectation(64, 16, 0, 16, expectedProfiles);

  auto mapping = std::make_unique<Wedge40PlatformMapping>();
  verify(mapping.get());
}

TEST_F(PlatformMappingTest, VerifyWedge100PlatformMapping) {
  // supported profiles
  std::vector<cfg::PortProfileID> expectedProfiles = {
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
  setExpectation(128, 32, 0, 32, expectedProfiles);

  auto mapping = std::make_unique<Wedge100PlatformMapping>();
  verify(mapping.get());
}

TEST_F(PlatformMappingTest, VerifyGalaxyFCPlatformMapping) {
  // supported profiles
  std::vector<cfg::PortProfileID> expectedProfiles = {
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
  setExpectation(128, 32, 0, 0, expectedProfiles);

  auto mapping = std::make_unique<GalaxyFCPlatformMapping>("fc003");
  verify(mapping.get());

  // also check all the port name should starts with fab3
  for (const auto& port : mapping->getPlatformPorts()) {
    EXPECT_EQ(port.second.mapping_ref()->name_ref()->rfind("fab3", 0), 0);
  }
}

TEST_F(PlatformMappingTest, VerifyGalaxyLCPlatformMapping) {
  // supported profiles
  std::vector<cfg::PortProfileID> expectedProfiles = {
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
  setExpectation(128, 32, 0, 16, expectedProfiles);

  auto mapping = std::make_unique<GalaxyLCPlatformMapping>("lc301");
  verify(mapping.get());

  // also check all the port name should starts with fab3/eth301
  for (const auto& port : mapping->getPlatformPorts()) {
    EXPECT_TRUE(
        port.second.mapping_ref()->name_ref()->rfind("fab301", 0) == 0 ||
        port.second.mapping_ref()->name_ref()->rfind("eth301", 0) == 0);
  }
}

// Array of tx setting override groups for wedge100
// Each group is a mapping of cableLength => {driveCurrent_ref().value(),
// pre, main, post}
static const std::vector<std::map<double, std::pair<std::array<int, 6>, int>>>
    kWedge100DownlinkTxGroups = {
        {
            {1.0, {{0, 0x4, 0x3c, 0x30, 0, 0}, 0xa}},
            {1.5, {{0, 0x4, 0x3c, 0x30, 0, 0}, 0xa}},
            {2.0, {{0, 0x4, 0x3c, 0x30, 0, 0}, 0xa}},
            {2.5, {{0, 0x6, 0x3e, 0x32, 0, 0}, 0xc}},
            {3.0, {{0, 0x6, 0x3e, 0x32, 0, 0}, 0xc}},
        },
        {
            {1.0, {{0, 0x6, 0x40, 0x2a, 0, 0}, 0xa}},
            {1.5, {{0, 0x7, 0x3e, 0x2b, 0, 0}, 0xa}},
            {2.0, {{0, 0x8, 0x3c, 0x2c, 0, 0}, 0xb}},
            {2.5, {{0, 0x7, 0x3d, 0x2c, 0, 0}, 0xc}},
            {3.0, {{0, 0x6, 0x3c, 0x2e, 0, 0}, 0xc}},
        },
        {
            {1.0, {{0, 0x8, 0x42, 0x26, 0, 0}, 0x9}},
            {1.5, {{0, 0x9, 0x41, 0x26, 0, 0}, 0x9}},
            {2.0, {{0, 0x9, 0x40, 0x27, 0, 0}, 0x9}},
            {2.5, {{0, 0x9, 0x3f, 0x28, 0, 0}, 0x9}},
            {3.0, {{0, 0x8, 0x40, 0x28, 0, 0}, 0xa}},
        },
        {
            {1.0, {{0, 0x6, 0x46, 0x24, 0, 0}, 0x8}},
            {1.5, {{0, 0x6, 0x46, 0x24, 0, 0}, 0x9}},
            {2.0, {{0, 0x7, 0x45, 0x24, 0, 0}, 0x9}},
            {2.5, {{0, 0x8, 0x43, 0x25, 0, 0}, 0x9}},
            {3.0, {{0, 0x8, 0x43, 0x25, 0, 0}, 0xa}},
        },
        {
            {1.0, {{0, 0x6, 0x4c, 0x1e, 0, 0}, 0x8}},
            {1.5, {{0, 0x7, 0x4b, 0x1e, 0, 0}, 0x9}},
            {2.0, {{0, 0x7, 0x4b, 0x1e, 0, 0}, 0x9}},
            {2.5, {{0, 0x8, 0x49, 0x1f, 0, 0}, 0x9}},
            {3.0, {{0, 0x8, 0x48, 0x20, 0, 0}, 0xa}},
        },
        {
            {1.0, {{0, 0x6, 0x4e, 0x1c, 0, 0}, 0x8}},
            {1.5, {{0, 0x6, 0x4d, 0x1d, 0, 0}, 0x9}},
            {2.0, {{0, 0x7, 0x4b, 0x1e, 0, 0}, 0xa}},
            {2.5, {{0, 0x8, 0x49, 0x1f, 0, 0}, 0xa}},
            {3.0, {{0, 0x8, 0x48, 0x20, 0, 0}, 0xa}},
        },
        {
            {1.0, {{0, 0x6, 0x50, 0x1a, 0, 0}, 0x8}},
            {1.5, {{0, 0x6, 0x4e, 0x1c, 0, 0}, 0x9}},
            {2.0, {{0, 0x6, 0x4e, 0x1c, 0, 0}, 0x9}},
            {2.5, {{0, 0x7, 0x4b, 0x1e, 0, 0}, 0x9}},
            {3.0, {{0, 0x8, 0x4a, 0x1e, 0, 0}, 0x9}},
        },
};

// Each front panel port maps to one trace group in the above
// vector. The index is the TransceiverID, the value is the index
// for which set of overrides to use from kWedge100DownlinkTxGroups.
static const std::array<uint8_t, 28> kWedge100DownlinkPortGroupMapping = {{
    1, 0, 1, 1, 2, 2, 3, 3, 4, 4, 4, 4, 5, 5,
    6, 6, 6, 5, 5, 5, 4, 4, 3, 3, 3, 3, 2, 2,
}};

TEST_F(PlatformMappingTest, VerifyWedge100DownlinkPortIphyPinConfigs) {
  std::unordered_set<cfg::PortProfileID> downlinkProfiles({
      cfg::PortProfileID::PROFILE_10G_1_NRZ_NOFEC_COPPER,
      cfg::PortProfileID::PROFILE_20G_2_NRZ_NOFEC_COPPER,
      cfg::PortProfileID::PROFILE_25G_1_NRZ_NOFEC_COPPER,
      cfg::PortProfileID::PROFILE_25G_1_NRZ_CL74_COPPER,
      cfg::PortProfileID::PROFILE_40G_4_NRZ_NOFEC_COPPER,
      cfg::PortProfileID::PROFILE_50G_2_NRZ_NOFEC_COPPER,
      cfg::PortProfileID::PROFILE_50G_2_NRZ_CL74_COPPER,
      cfg::PortProfileID::PROFILE_100G_4_NRZ_CL91_COPPER,
  });
  auto mapping = std::make_unique<Wedge100PlatformMapping>();
  for (auto& port : mapping->getPlatformPorts()) {
    const auto& chipName =
        port.second.get_mapping().get_pins()[0].get_z()->get_end().get_chip();
    const auto& chip = mapping->getChips().at(chipName);
    auto transID = chip.get_physicalID();
    // Skip uplinks
    if (transID >= kWedge100DownlinkPortGroupMapping.size()) {
      continue;
    }

    const auto& profiles = *port.second.supportedProfiles_ref();
    for (auto profile : profiles) {
      // skip uplink profiles
      if (downlinkProfiles.find(profile.first) == downlinkProfiles.end()) {
        continue;
      }
      auto itProfileCfg = mapping->getPortProfileConfig(
          PlatformPortProfileConfigMatcher(profile.first, PortID(port.first)));
      EXPECT_TRUE(itProfileCfg.has_value());

      const auto& txSettingsGroup =
          kWedge100DownlinkTxGroups[kWedge100DownlinkPortGroupMapping[transID]];
      for (auto& setting : txSettingsGroup) {
        TransceiverInfo transceiverInfo = TransceiverInfo();
        Cable cable = Cable();
        cable.length_ref() = setting.first;
        transceiverInfo.cable_ref() = cable;
        auto expectedTx = setting.second.first;
        auto expectedDriveCurrent = setting.second.second;
        auto pinCfgs =
            mapping->getPortIphyPinConfigs(PlatformPortProfileConfigMatcher(
                profile.first, PortID(port.first), transceiverInfo));
        auto pinCfg = pinCfgs.at(0);
        auto tx = pinCfg.tx_ref();
        EXPECT_TRUE(tx.has_value());
        verifyTxSettings(*tx, expectedTx, true);
        EXPECT_EQ(*tx->driveCurrent_ref(), expectedDriveCurrent);
      }
    }
  }
}

// Array of expected tx settings for wedge100 uplinks for 100G Optical
// Index is transciever Id - 24 (i.e first element is tcvr 24, the first
// uplink) value is {driveCurrent, pre, main, post}
const std::vector<std::array<int, 6>> kWedge100UplinkTxSettings = {
    {0, 2, 72, 38, 0, 0},
    {0, 2, 66, 44, 0, 0},
    {0, 4, 68, 40, 0, 0},
    {0, 2, 68, 42, 0, 0},
    {0, 4, 64, 44, 0, 0},
    {0, 2, 64, 46, 0, 0},
    {0, 2, 62, 48, 0, 0},
    {0, 2, 62, 48, 0, 0}};
const auto kWedge100UplinkDriveCurrent = 0x8;

TEST_F(PlatformMappingTest, VerifyWedge100UplinkPortIphyPinConfigs) {
  auto mapping = std::make_unique<Wedge100PlatformMapping>();
  for (auto& port : mapping->getPlatformPorts()) {
    const auto& chipName =
        port.second.get_mapping().get_pins()[0].get_z()->get_end().get_chip();
    const auto& chip = mapping->getChips().at(chipName);
    auto transID = chip.get_physicalID();
    // skip downlinks
    if (transID < 24) {
      continue;
    }
    const auto& profiles = *port.second.supportedProfiles_ref();
    for (auto profile : profiles) {
      auto pinCfgs = mapping->getPortIphyPinConfigs(
          PlatformPortProfileConfigMatcher(profile.first, PortID(port.first)));
      for (auto pinCfg : pinCfgs) {
        auto tx = pinCfg.tx_ref();
        // Only the this profile should have tx settings
        if (profile.first ==
            cfg::PortProfileID::PROFILE_100G_4_NRZ_CL91_OPTICAL) {
          auto expectedTx = kWedge100UplinkTxSettings[transID - 24];
          EXPECT_TRUE(tx.has_value());
          verifyTxSettings(*tx, expectedTx, true);
          EXPECT_EQ(*tx->driveCurrent_ref(), kWedge100UplinkDriveCurrent);
        } else {
          EXPECT_FALSE(tx.has_value());
        }
      }
    }
  }
}

TEST_F(PlatformMappingTest, VerifyElbert16QPlatformMapping) {
  // supported profiles
  std::vector<cfg::PortProfileID> expectedProfiles = {
      cfg::PortProfileID::PROFILE_100G_4_NRZ_RS528_OPTICAL,
      cfg::PortProfileID::PROFILE_200G_4_PAM4_RS544X2N_OPTICAL,
      cfg::PortProfileID::PROFILE_400G_8_PAM4_RS544X2N_OPTICAL};

  // Elbert16Q has 128 ports
  // 64 TH3 Blackhawk cores + 128 transceivers + no xphy
  setExpectation(128, 64, 0, 128, expectedProfiles);

  // both xphy version should pass the same standard
  auto mapping = std::make_unique<Elbert16QPimPlatformMapping>();
  verify(mapping.get());
}

TEST_F(PlatformMappingTest, VerifyElbert16QIphyPinConfigs) {
  auto mapping = std::make_unique<Elbert16QPimPlatformMapping>();
  std::map<cfg::PortProfileID, std::array<int, 6>> expectedTxSettings = {
      {cfg::PortProfileID::PROFILE_100G_4_NRZ_RS528_OPTICAL,
       {0, -14, 87, 0, 0, 0}},
      {cfg::PortProfileID::PROFILE_200G_4_PAM4_RS544X2N_OPTICAL,
       {0, -24, 132, -12, 0, 0}},
      {cfg::PortProfileID::PROFILE_400G_8_PAM4_RS544X2N_OPTICAL,
       {0, -24, 132, -12, 0, 0}},
  };
  verifyTxSettingsByProfile(
      mapping.get(), mapping->getPlatformPorts(), expectedTxSettings);
}

TEST_F(PlatformMappingTest, VerifyPlatformSupportedProfileMerge) {
  auto platformMapping = PlatformMapping();
  cfg::PlatformPortProfileConfigEntry configEntry1 =
      createPlatformPortProfileConfigEntry(
          cfg::PortProfileID::PROFILE_100G_4_NRZ_CL91_OPTICAL,
          {1, 2},
          cfg::PortSpeed::HUNDREDG,
          phy::FecMode::CL74);
  cfg::PlatformPortProfileConfigEntry configEntry2 =
      createPlatformPortProfileConfigEntry(
          cfg::PortProfileID::PROFILE_100G_4_NRZ_CL91_OPTICAL,
          {1, 3},
          cfg::PortSpeed::HUNDREDG,
          phy::FecMode::CL74);
  cfg::PlatformPortProfileConfigEntry configEntry3 =
      createPlatformPortProfileConfigEntry(
          cfg::PortProfileID::PROFILE_25G_1_NRZ_NOFEC_OPTICAL,
          {1, 2, 3},
          cfg::PortSpeed::HUNDREDG,
          phy::FecMode::CL74);
  cfg::PlatformPortProfileConfigEntry configEntry4 =
      createPlatformPortProfileConfigEntry(
          cfg::PortProfileID::PROFILE_100G_4_NRZ_CL91_OPTICAL,
          {2},
          cfg::PortSpeed::HUNDREDG,
          phy::FecMode::RS528);

  platformMapping.mergePlatformSupportedProfile(configEntry1);
  platformMapping.mergePlatformSupportedProfile(configEntry2);
  platformMapping.mergePlatformSupportedProfile(configEntry3);

  // pims 1-3 with PROFILE_100G_4_NRZ_CL91_OPTICAL
  for (auto pimID : {1, 2, 3}) {
    auto profile =
        platformMapping.getPortProfileConfig(PlatformPortProfileConfigMatcher(
            cfg::PortProfileID::PROFILE_100G_4_NRZ_CL91_OPTICAL, PimID(pimID)));
    EXPECT_TRUE(profile.has_value());
    EXPECT_EQ(profile.value(), configEntry1.profile_ref());
  }

  // pims 1-3 with PROFILE_25G_1_NRZ_NOFEC_OPTICAL
  for (auto pimID : {1, 2, 3}) {
    auto profile =
        platformMapping.getPortProfileConfig(PlatformPortProfileConfigMatcher(
            cfg::PortProfileID::PROFILE_25G_1_NRZ_NOFEC_OPTICAL, PimID(pimID)));
    EXPECT_TRUE(profile.has_value());
    EXPECT_EQ(profile.value(), configEntry3.profile_ref());
  }

  // No match at pim 4
  auto profileDoesNotExists =
      platformMapping.getPortProfileConfig(PlatformPortProfileConfigMatcher(
          cfg::PortProfileID::PROFILE_100G_4_NRZ_CL91_OPTICAL, PimID(4)));
  EXPECT_FALSE(profileDoesNotExists.has_value());

  // config 4 has the same profile id and overlapping pimID as config 1, but
  // different profile config. There's no reasonable merge in this case so
  // expect an error
  EXPECT_THROW(
      platformMapping.mergePlatformSupportedProfile(configEntry4), FbossError);
}
} // namespace facebook::fboss::test
