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

#include "fboss/agent/AgentConfig.h"
#include "fboss/agent/platforms/common/PlatformMapping.h"
#include "fboss/agent/state/Port.h"
#include "fboss/lib/config/PlatformConfigUtils.h"
#include "fboss/lib/phy/PhyManager.h"
#include "fboss/qsfp_service/test/hw_test/HwQsfpEnsemble.h"
#include "fboss/qsfp_service/test/hw_test/phy/HwPortUtils.h"

#include <folly/logging/xlog.h>

namespace facebook::fboss {

template <cfg::PortProfileID Profile>
class HwPortProfileTest : public HwTest {
 protected:
  std::vector<PortID> findAvailablePort() {
    std::set<PortID> xPhyPorts;
    const auto& platformPorts =
        getHwQsfpEnsemble()->getPlatformMapping()->getPlatformPorts();
    const auto& chips = getHwQsfpEnsemble()->getPlatformMapping()->getChips();
    for (auto idAndEntry : platformPorts) {
      const auto& xphy = utility::getDataPlanePhyChips(
          idAndEntry.second, chips, phy::DataPlanePhyChipType::XPHY);
      if (xphy.empty()) {
        continue;
      }
      xPhyPorts.emplace(PortID(idAndEntry.first));
    }
    std::vector<PortID> ports;
    auto agentConfig = getHwQsfpEnsemble()->getWedgeManager()->getAgentConfig();
    auto& swConfig = *agentConfig->thrift.sw_ref();
    for (auto& port : *swConfig.ports_ref()) {
      if (xPhyPorts.find(PortID(*port.logicalID_ref())) == xPhyPorts.end()) {
        // TODO: expand to non xphy ports too
        continue;
      }
      if (*port.profileID_ref() == Profile &&
          *port.state_ref() == cfg::PortState::ENABLED) {
        ports.emplace_back(PortID(*port.logicalID_ref()));
      }
    }
    return ports;
  }

  void verifyPort(PortID portID) {
    utility::verifyPhyPortConfig(
        portID,
        Profile,
        getHwQsfpEnsemble()->getPlatformMapping(),
        getHwQsfpEnsemble()->getExternalPhy(portID));

    utility::veridyPhyPortConnector(
        portID, getHwQsfpEnsemble()->getPhyManager());
  }

  void runTest() {
    auto ports = findAvailablePort();
    EXPECT_TRUE(!ports.empty());
    for (auto port : ports) {
      // Call PhyManager to program such port
      getHwQsfpEnsemble()->getPhyManager()->programOnePort(port, Profile);
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
