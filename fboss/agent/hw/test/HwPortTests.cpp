// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/test/HwPortUtils.h"
#include "fboss/agent/hw/test/HwTest.h"
#include "fboss/agent/state/Port.h"

#include "fboss/agent/hw/test/ConfigFactory.h"

namespace facebook::fboss {
class HwPortTest : public HwTest {
 protected:
  cfg::SwitchConfig initialConfig() const {
    return utility::oneL3IntfTwoPortConfig(
        getHwSwitch(), masterLogicalPortIds()[0], masterLogicalPortIds()[1]);
  }

  void testProfile(cfg::PortProfileID profile) {
    auto profiles = getPlatform()->getSupportedProfiles();
    if (profiles.find(profile) == profiles.end()) {
      return;
    }
    auto setup = []() {
      // TODO
    };
    auto verify = []() {
      // TODO
    };
    verifyAcrossWarmBoots(setup, verify);
  }
};

TEST_F(HwPortTest, PROFILE_10G_1_NRZ_NOFEC_COPPER) {
  testProfile(cfg::PortProfileID::PROFILE_10G_1_NRZ_NOFEC_COPPER);
}

TEST_F(HwPortTest, PROFILE_10G_1_NRZ_NOFEC_OPTICAL) {
  testProfile(cfg::PortProfileID::PROFILE_10G_1_NRZ_NOFEC_OPTICAL);
}

TEST_F(HwPortTest, PROFILE_20G_2_NRZ_NOFEC_COPPER) {
  testProfile(cfg::PortProfileID::PROFILE_20G_2_NRZ_NOFEC_COPPER);
}

TEST_F(HwPortTest, PROFILE_25G_1_NRZ_NOFEC_COPPER) {
  testProfile(cfg::PortProfileID::PROFILE_25G_1_NRZ_NOFEC_COPPER);
}

TEST_F(HwPortTest, PROFILE_25G_1_NRZ_CL74_COPPER) {
  testProfile(cfg::PortProfileID::PROFILE_25G_1_NRZ_CL74_COPPER);
}

TEST_F(HwPortTest, PROFILE_25G_1_NRZ_RS528_COPPER) {
  testProfile(cfg::PortProfileID::PROFILE_25G_1_NRZ_RS528_COPPER);
}

TEST_F(HwPortTest, PROFILE_40G_4_NRZ_NOFEC_COPPER) {
  testProfile(cfg::PortProfileID::PROFILE_40G_4_NRZ_NOFEC_COPPER);
}

TEST_F(HwPortTest, PROFILE_40G_4_NRZ_NOFEC_OPTICAL) {
  testProfile(cfg::PortProfileID::PROFILE_40G_4_NRZ_NOFEC_OPTICAL);
}

TEST_F(HwPortTest, PROFILE_50G_2_NRZ_NOFEC_COPPER) {
  testProfile(cfg::PortProfileID::PROFILE_50G_2_NRZ_NOFEC_COPPER);
}

TEST_F(HwPortTest, PROFILE_50G_2_NRZ_CL74_COPPER) {
  testProfile(cfg::PortProfileID::PROFILE_50G_2_NRZ_CL74_COPPER);
}

TEST_F(HwPortTest, PROFILE_50G_2_NRZ_RS528_COPPER) {
  testProfile(cfg::PortProfileID::PROFILE_50G_2_NRZ_RS528_COPPER);
}

TEST_F(HwPortTest, PROFILE_100G_4_NRZ_RS528_COPPER) {
  testProfile(cfg::PortProfileID::PROFILE_100G_4_NRZ_RS528_COPPER);
}

TEST_F(HwPortTest, PROFILE_100G_4_NRZ_RS528_OPTICAL) {
  testProfile(cfg::PortProfileID::PROFILE_100G_4_NRZ_RS528_OPTICAL);
}

/*
TEST_F(HwPortTest, PROFILE_200G_4_PAM4_RS544X2N_COPPER) {
  testProfile(cfg::PortProfileID::PROFILE_200G_4_PAM4_RS544X2N_COPPER);
}

TEST_F(HwPortTest, PROFILE_200G_4_PAM4_RS544X2N_OPTICAL) {
  testProfile(cfg::PortProfileID::PROFILE_200G_4_PAM4_RS544X2N_OPTICAL);
}

TEST_F(HwPortTest, PROFILE_400G_8_PAM4_RS544X2N_OPTICAL) {
  testProfile(cfg::PortProfileID::PROFILE_400G_8_PAM4_RS544X2N_OPTICAL);
}
*/

TEST_F(HwPortTest, PROFILE_100G_4_NRZ_CL91_COPPER) {
  testProfile(cfg::PortProfileID::PROFILE_100G_4_NRZ_CL91_COPPER);
}

TEST_F(HwPortTest, PROFILE_100G_4_NRZ_CL91_OPTICAL) {
  testProfile(cfg::PortProfileID::PROFILE_100G_4_NRZ_CL91_OPTICAL);
}

TEST_F(HwPortTest, PROFILE_20G_2_NRZ_NOFEC_OPTICAL) {
  testProfile(cfg::PortProfileID::PROFILE_20G_2_NRZ_NOFEC_OPTICAL);
}

TEST_F(HwPortTest, PROFILE_25G_1_NRZ_NOFEC_OPTICAL) {
  testProfile(cfg::PortProfileID::PROFILE_25G_1_NRZ_NOFEC_OPTICAL);
}

TEST_F(HwPortTest, PROFILE_50G_2_NRZ_NOFEC_OPTICAL) {
  testProfile(cfg::PortProfileID::PROFILE_50G_2_NRZ_NOFEC_OPTICAL);
}

TEST_F(HwPortTest, PROFILE_100G_4_NRZ_NOFEC_COPPER) {
  testProfile(cfg::PortProfileID::PROFILE_100G_4_NRZ_NOFEC_COPPER);
}

TEST_F(HwPortTest, AssertMode) {
  auto setup = [this]() { applyNewConfig(initialConfig()); };
  auto verify = [this]() {
    for (const auto& port : *getProgrammedState()->getPorts()) {
      if (!port->isEnabled()) {
        continue;
      }
      utility::verifyInterfaceMode(
          port->getID(), port->getProfileID(), getPlatform());
    }
  };
  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(HwPortTest, AssertTxSetting) {
  auto setup = [this]() { applyNewConfig(initialConfig()); };
  auto verify = [this]() {
    for (const auto& port : *getProgrammedState()->getPorts()) {
      if (!port->isEnabled()) {
        continue;
      }
      utility::verifyTxSettting(
          port->getID(), port->getProfileID(), getPlatform());
    }
  };
  verifyAcrossWarmBoots(setup, verify);
}

} // namespace facebook::fboss
