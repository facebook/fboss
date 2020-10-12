// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/test/HwLinkStateDependentTest.h"
#include "fboss/agent/hw/test/HwPortUtils.h"
#include "fboss/agent/hw/test/HwTestPortUtils.h"
#include "fboss/agent/state/Port.h"

#include "fboss/agent/hw/test/ConfigFactory.h"

namespace facebook::fboss {
class HwPortProfileTest : public HwLinkStateDependentTest {
 protected:
  cfg::SwitchConfig initialConfig() const override {
    auto lbMode = getPlatform()->getAsic()->desiredLoopbackMode();
    return utility::oneL3IntfTwoPortConfig(
        getHwSwitch(),
        masterLogicalPortIds()[0],
        masterLogicalPortIds()[1],
        lbMode);
  }

  bool skipTest(cfg::PortProfileID profile) {
    if (!getPlatform()->getPortProfileConfig(profile)) {
      return true;
    }
    auto& platformPorts = getPlatform()->getPlatformPorts();
    for (auto port : {masterLogicalPortIds()[0], masterLogicalPortIds()[1]}) {
      auto iter = platformPorts.find(port);
      if (iter == platformPorts.end()) {
        return true;
      }
      if (iter->second.supportedProfiles_ref()->find(profile) ==
          iter->second.supportedProfiles_ref()->end()) {
        return true;
      }
    }
    return false;
  }

  void verifyPort(PortID portID) {
    auto port = getProgrammedState()->getPorts()->getPort(portID);
    // verify interface mode
    utility::verifyInterfaceMode(
        port->getID(), port->getProfileID(), getPlatform());
    // verify tx settings
    utility::verifyTxSettting(
        port->getID(), port->getProfileID(), getPlatform());
    // verify rx settings
    utility::verifyRxSettting(
        port->getID(), port->getProfileID(), getPlatform());
    // (TODO): verify speed
    utility::verifyFec(port->getID(), port->getProfileID(), getPlatform());
    // (TODO): verify lane count (for sai)
  }

  void setupPort2OverrideTransceiverInfo(cfg::PortProfileID profile) {
    auto tech = utility::getMediaType(profile);
    getPlatform()->setPort2OverrideTransceiverInfo(port2transceiverInfo(tech));
  }

  void testProfile(cfg::PortProfileID profile) {
    if (skipTest(profile)) {
// profile is not supported.
#if defined(GTEST_SKIP)
      GTEST_SKIP();
#endif
      return;
    }
    auto setup = [=]() {
      setupPort2OverrideTransceiverInfo(profile);
      auto config = initialConfig();
      for (auto port : {masterLogicalPortIds()[0], masterLogicalPortIds()[1]}) {
        auto hwSwitch = getHwSwitch();
        utility::configurePortProfile(
            *hwSwitch, config, profile, getAllPortsInGroup(port));
      }
      applyNewConfig(config);
    };
    auto verify = [=]() {
      bool up = true;
      for (auto portID :
           {masterLogicalPortIds()[0], masterLogicalPortIds()[1]}) {
        verifyPort(portID);
        bringDownPort(portID);
        utility::verifyLedStatus(getHwSwitchEnsemble(), portID, !up);
        bringUpPort(portID);
        utility::verifyLedStatus(getHwSwitchEnsemble(), portID, up);
      }
    };
    auto setupPostWb = [=]() { setupPort2OverrideTransceiverInfo(profile); };
    verifyAcrossWarmBoots(setup, verify, setupPostWb, verify);
  }

  std::map<PortID, TransceiverInfo> port2transceiverInfo(
      TransmitterTechnology tech) const {
    std::map<PortID, TransceiverInfo> result{};
    result.emplace(
        masterLogicalPortIds()[0],
        utility::getTransceiverInfo(masterLogicalPortIds()[0], tech));
    result.emplace(
        masterLogicalPortIds()[1],
        utility::getTransceiverInfo(masterLogicalPortIds()[1], tech));
    return result;
  }
};

TEST_F(HwPortProfileTest, PROFILE_10G_1_NRZ_NOFEC_COPPER) {
  testProfile(cfg::PortProfileID::PROFILE_10G_1_NRZ_NOFEC_COPPER);
}

TEST_F(HwPortProfileTest, PROFILE_10G_1_NRZ_NOFEC_OPTICAL) {
  testProfile(cfg::PortProfileID::PROFILE_10G_1_NRZ_NOFEC_OPTICAL);
}

TEST_F(HwPortProfileTest, PROFILE_25G_1_NRZ_NOFEC_COPPER) {
  testProfile(cfg::PortProfileID::PROFILE_25G_1_NRZ_NOFEC_COPPER);
}

TEST_F(HwPortProfileTest, PROFILE_25G_1_NRZ_CL74_COPPER) {
  testProfile(cfg::PortProfileID::PROFILE_25G_1_NRZ_CL74_COPPER);
}

TEST_F(HwPortProfileTest, PROFILE_25G_1_NRZ_RS528_COPPER) {
  testProfile(cfg::PortProfileID::PROFILE_25G_1_NRZ_RS528_COPPER);
}

TEST_F(HwPortProfileTest, PROFILE_40G_4_NRZ_NOFEC_COPPER) {
  testProfile(cfg::PortProfileID::PROFILE_40G_4_NRZ_NOFEC_COPPER);
}

TEST_F(HwPortProfileTest, PROFILE_40G_4_NRZ_NOFEC_OPTICAL) {
  testProfile(cfg::PortProfileID::PROFILE_40G_4_NRZ_NOFEC_OPTICAL);
}

TEST_F(HwPortProfileTest, PROFILE_50G_2_NRZ_NOFEC_COPPER) {
  testProfile(cfg::PortProfileID::PROFILE_50G_2_NRZ_NOFEC_COPPER);
}

TEST_F(HwPortProfileTest, PROFILE_50G_2_NRZ_CL74_COPPER) {
  testProfile(cfg::PortProfileID::PROFILE_50G_2_NRZ_CL74_COPPER);
}

TEST_F(HwPortProfileTest, PROFILE_50G_2_NRZ_RS528_COPPER) {
  testProfile(cfg::PortProfileID::PROFILE_50G_2_NRZ_RS528_COPPER);
}

TEST_F(HwPortProfileTest, PROFILE_100G_4_NRZ_RS528_COPPER) {
  testProfile(cfg::PortProfileID::PROFILE_100G_4_NRZ_RS528_COPPER);
}

TEST_F(HwPortProfileTest, PROFILE_100G_4_NRZ_RS528_OPTICAL) {
  testProfile(cfg::PortProfileID::PROFILE_100G_4_NRZ_RS528_OPTICAL);
}

TEST_F(HwPortProfileTest, PROFILE_100G_4_NRZ_CL91_COPPER) {
  testProfile(cfg::PortProfileID::PROFILE_100G_4_NRZ_CL91_COPPER);
}

TEST_F(HwPortProfileTest, PROFILE_100G_4_NRZ_CL91_OPTICAL) {
  testProfile(cfg::PortProfileID::PROFILE_100G_4_NRZ_CL91_OPTICAL);
}

TEST_F(HwPortProfileTest, PROFILE_25G_1_NRZ_NOFEC_OPTICAL) {
  testProfile(cfg::PortProfileID::PROFILE_25G_1_NRZ_NOFEC_OPTICAL);
}

TEST_F(HwPortProfileTest, PROFILE_50G_2_NRZ_NOFEC_OPTICAL) {
  testProfile(cfg::PortProfileID::PROFILE_50G_2_NRZ_NOFEC_OPTICAL);
}

TEST_F(HwPortProfileTest, PROFILE_100G_4_NRZ_NOFEC_COPPER) {
  testProfile(cfg::PortProfileID::PROFILE_100G_4_NRZ_NOFEC_COPPER);
}

/*
TEST_F(HwPortProfileTest, PROFILE_20G_2_NRZ_NOFEC_COPPER) {
  // TODO: investigate and fix failures
  testProfile(cfg::PortProfileID::PROFILE_20G_2_NRZ_NOFEC_COPPER);
}

TEST_F(HwPortProfileTest, PROFILE_20G_2_NRZ_NOFEC_OPTICAL) {
  // TODO: investigate and fix failures
  testProfile(cfg::PortProfileID::PROFILE_20G_2_NRZ_NOFEC_OPTICAL);
}
*/

TEST_F(HwPortProfileTest, PROFILE_200G_4_PAM4_RS544X2N_COPPER) {
  testProfile(cfg::PortProfileID::PROFILE_200G_4_PAM4_RS544X2N_COPPER);
}

TEST_F(HwPortProfileTest, PROFILE_200G_4_PAM4_RS544X2N_OPTICAL) {
  testProfile(cfg::PortProfileID::PROFILE_200G_4_PAM4_RS544X2N_OPTICAL);
}

TEST_F(HwPortProfileTest, PROFILE_400G_8_PAM4_RS544X2N_OPTICAL) {
  testProfile(cfg::PortProfileID::PROFILE_400G_8_PAM4_RS544X2N_OPTICAL);
}

} // namespace facebook::fboss
