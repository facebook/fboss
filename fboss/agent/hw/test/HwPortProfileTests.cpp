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

  cfg::SwitchConfig initialConfig(const std::vector<PortID>& ports) const {
    auto lbMode = getPlatform()->getAsic()->desiredLoopbackMode();
    return utility::oneL3IntfTwoPortConfig(
        getHwSwitch(), ports[0], ports[1], lbMode);
  }

  void verifyPort(PortID portID) {
    auto platformPort = getPlatform()->getPlatformPort(portID);
    EXPECT_EQ(portID, platformPort->getPortID());
    auto port = getProgrammedState()->getPorts()->getPort(portID);
    // verify interface mode
    utility::verifyInterfaceMode(
        port->getID(),
        port->getProfileID(),
        getPlatform(),
        port->getProfileConfig());
    // verify tx settings
    utility::verifyTxSettting(
        port->getID(),
        port->getProfileID(),
        getPlatform(),
        port->getPinConfigs());
    // verify rx settings
    utility::verifyRxSettting(
        port->getID(),
        port->getProfileID(),
        getPlatform(),
        port->getPinConfigs());
    // (TODO): verify speed
    utility::verifyFec(
        port->getID(),
        port->getProfileID(),
        getPlatform(),
        port->getProfileConfig());
    // (TODO): verify lane count (for sai)
  }

  void runTest() {
    const auto& availablePorts = findAvailablePorts();
    if (availablePorts.size() < 2) {
// profile is not supported.
#if defined(GTEST_SKIP)
      GTEST_SKIP();
#endif
      return;
    }
    auto setup = [=]() {
      auto config = initialConfig(availablePorts);
      for (auto port : {availablePorts[0], availablePorts[1]}) {
        auto hwSwitch = getHwSwitch();
        utility::configurePortProfile(
            *hwSwitch, config, Profile, getAllPortsInGroup(port), port);
      }
      applyNewConfig(config);
    };
    auto verify = [=]() {
      for (auto portID : {availablePorts[0], availablePorts[1]}) {
        verifyPort(portID);
      }
    };
    verifyAcrossWarmBoots(setup, verify);
  }

  std::optional<TransceiverInfo> overrideTransceiverInfo() const override {
    return utility::getTransceiverInfo(Profile);
  }

 private:
  std::vector<PortID> findAvailablePorts() {
    std::vector<PortID> availablePorts;
    auto& platformPorts = getPlatform()->getPlatformPorts();
    for (auto port : masterLogicalPortIds()) {
      auto iter = platformPorts.find(port);
      EXPECT_TRUE(iter != platformPorts.end());
      if (!getPlatform()->getPortProfileConfig(
              PlatformPortProfileConfigMatcher(Profile, port))) {
        continue;
      }
      if (iter->second.supportedProfiles_ref()->find(Profile) ==
          iter->second.supportedProfiles_ref()->end()) {
        continue;
      }
      availablePorts.push_back(port);
    }
    return availablePorts;
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

TEST_PROFILE(PROFILE_100G_4_NRZ_CL91_COPPER_RACK_YV3_T1)

TEST_PROFILE(PROFILE_25G_1_NRZ_NOFEC_COPPER_RACK_YV3_T1)

} // namespace facebook::fboss
