/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/qsfp_service/test/hw_test/HwTest.h"

#include "fboss/agent/platforms/common/PlatformMapping.h"
#include "fboss/agent/state/Port.h"
#include "fboss/lib/phy/PhyManager.h"
#include "fboss/qsfp_service/test/hw_test/phy/HwPhyEnsemble.h"
#include "fboss/qsfp_service/test/hw_test/phy/HwPortUtils.h"

namespace facebook::fboss {

template <cfg::PortProfileID Profile>
class HwPortProfileTest : public HwTest {
 protected:
  std::vector<PortID> findAvailablePort() {
    std::vector<PortID> ports;
    const auto& platformPorts =
        getHwPhyEnsemble()->getPlatformMapping()->getPlatformPorts();
    for (auto portID : getHwPhyEnsemble()->getTargetPorts()) {
      auto iter = platformPorts.find(portID);
      EXPECT_TRUE(iter != platformPorts.end());
      if (iter->second.supportedProfiles_ref()->find(Profile) !=
          iter->second.supportedProfiles_ref()->end()) {
        ports.emplace_back(portID);
      }
    }
    return ports;
  }

  void verifyPort(PortID portID) {
    utility::verifyPhyPortConfig(
        portID,
        Profile,
        getHwPhyEnsemble()->getPlatformMapping(),
        getHwPhyEnsemble()->getTargetExternalPhy());

    utility::veridyPhyPortConnector(
        portID, getHwPhyEnsemble()->getPhyManager());
  }

  void runTest() {
    auto ports = findAvailablePort();
    if (!ports.size()) {
// profile is not supported.
#if defined(GTEST_SKIP)
      GTEST_SKIP();
#endif
      return;
    }
    for (auto port : ports) {
      // Call PhyManager to program such port
      getHwPhyEnsemble()->getPhyManager()->programOnePort(port, Profile);
      // Verify whether such profile has been programmed to the port
      verifyPort(port);
    }
  }
};

#define TEST_PROFILE(PROFILE)                                     \
  struct HwTest_##PROFILE                                         \
      : public HwPortProfileTest<cfg::PortProfileID::PROFILE> {}; \
  TEST_F(HwTest_##PROFILE, TestProfile) {                         \
    runTest();                                                    \
  }

TEST_PROFILE(PROFILE_100G_4_NRZ_RS528_OPTICAL)

TEST_PROFILE(PROFILE_200G_4_PAM4_RS544X2N_OPTICAL)

TEST_PROFILE(PROFILE_400G_8_PAM4_RS544X2N_OPTICAL)
} // namespace facebook::fboss
