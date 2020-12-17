// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/test/HwPortUtils.h"
#include "fboss/agent/hw/test/HwTest.h"
#include "fboss/agent/hw/test/HwTestPortUtils.h"
#include "fboss/agent/state/Port.h"

#include "fboss/agent/hw/test/ConfigFactory.h"

namespace facebook::fboss {
template <cfg::PortProfileID Profile>
class HwPortProfileTest : public HwTest {
 protected:
  void SetUp() override {
    utility::enableTransceiverProgramming(true);
    HwTest::SetUp();
  }

  cfg::SwitchConfig initialConfig() const {
    auto lbMode = getPlatform()->getAsic()->desiredLoopbackMode();
    return utility::oneL3IntfTwoPortConfig(
        getHwSwitch(),
        masterLogicalPortIds()[0],
        masterLogicalPortIds()[1],
        lbMode);
  }

  bool skipTest() {
    if (!getPlatform()->getPortProfileConfig(PlatformPortProfileConfigMatcher(
            Profile, masterLogicalPortIds()[0])) ||
        !getPlatform()->getPortProfileConfig(PlatformPortProfileConfigMatcher(
            Profile, masterLogicalPortIds()[1]))) {
      return true;
    }
    auto& platformPorts = getPlatform()->getPlatformPorts();
    for (auto port : {masterLogicalPortIds()[0], masterLogicalPortIds()[1]}) {
      auto iter = platformPorts.find(port);
      if (iter == platformPorts.end()) {
        return true;
      }
      if (iter->second.supportedProfiles_ref()->find(Profile) ==
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

  void runTest() {
    if (skipTest()) {
// profile is not supported.
#if defined(GTEST_SKIP)
      GTEST_SKIP();
#endif
      return;
    }
    auto setup = [=]() {
      auto config = initialConfig();
      for (auto port : {masterLogicalPortIds()[0], masterLogicalPortIds()[1]}) {
        auto hwSwitch = getHwSwitch();
        utility::configurePortProfile(
            *hwSwitch, config, Profile, getAllPortsInGroup(port));
      }
      applyNewConfig(config);
    };
    auto verify = [=]() {
      bool up = true;
      for (auto portID :
           {masterLogicalPortIds()[0], masterLogicalPortIds()[1]}) {
        verifyPort(portID);
      }
    };
    // TODO: override transceiver info is not available across warm boots.
    setup();
    verify();
  }

  std::optional<TransceiverInfo> overrideTransceiverInfo() const override {
    auto tech = utility::getMediaType(Profile);
    return utility::getTransceiverInfo(masterLogicalPortIds()[0], tech);
  }
};

#define TEST_PROFILE(PROFILE)                                     \
  struct HwTest_##PROFILE                                         \
      : public HwPortProfileTest<cfg::PortProfileID::PROFILE> {}; \
  TEST_F(HwTest_##PROFILE, TestProfile) {                         \
    runTest();                                                    \
  }

TEST_PROFILE(PROFILE_10G_1_NRZ_NOFEC_COPPER)

TEST_PROFILE(PROFILE_10G_1_NRZ_NOFEC_OPTICAL)

TEST_PROFILE(PROFILE_25G_1_NRZ_NOFEC_COPPER)

TEST_PROFILE(PROFILE_25G_1_NRZ_CL74_COPPER)

TEST_PROFILE(PROFILE_25G_1_NRZ_RS528_COPPER)

TEST_PROFILE(PROFILE_40G_4_NRZ_NOFEC_COPPER)

TEST_PROFILE(PROFILE_40G_4_NRZ_NOFEC_OPTICAL)

TEST_PROFILE(PROFILE_50G_2_NRZ_NOFEC_COPPER)

TEST_PROFILE(PROFILE_50G_2_NRZ_CL74_COPPER)

TEST_PROFILE(PROFILE_50G_2_NRZ_RS528_COPPER)

TEST_PROFILE(PROFILE_100G_4_NRZ_RS528_COPPER)

TEST_PROFILE(PROFILE_100G_4_NRZ_RS528_OPTICAL)

TEST_PROFILE(PROFILE_100G_4_NRZ_CL91_COPPER)

TEST_PROFILE(PROFILE_100G_4_NRZ_CL91_OPTICAL)

TEST_PROFILE(PROFILE_25G_1_NRZ_NOFEC_OPTICAL)

TEST_PROFILE(PROFILE_50G_2_NRZ_NOFEC_OPTICAL)

TEST_PROFILE(PROFILE_100G_4_NRZ_NOFEC_COPPER)

// TODO: investigate and fix failures
// TEST_PROFILE(PROFILE_20G_2_NRZ_NOFEC_COPPER)
// TEST_PROFILE(PROFILE_20G_2_NRZ_NOFEC_OPTICAL)

TEST_PROFILE(PROFILE_200G_4_PAM4_RS544X2N_COPPER)

TEST_PROFILE(PROFILE_200G_4_PAM4_RS544X2N_OPTICAL)

TEST_PROFILE(PROFILE_400G_8_PAM4_RS544X2N_OPTICAL)

TEST_PROFILE(PROFILE_100G_4_NRZ_RS528)

TEST_PROFILE(PROFILE_40G_4_NRZ_NOFEC)

TEST_PROFILE(PROFILE_200G_4_PAM4_RS544X2N)

} // namespace facebook::fboss
