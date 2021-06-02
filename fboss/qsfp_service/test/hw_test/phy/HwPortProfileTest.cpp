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

namespace facebook::fboss {

template <cfg::PortProfileID Profile>
class HwPortProfileTest : public HwTest {
 private:
  struct Ports {
    std::vector<PortID> xphyPorts;
    std::vector<PortID> iphyPorts;
  };
  Ports findAvailablePorts() {
    std::set<PortID> xPhyPorts;
    const auto& platformPorts =
        getHwQsfpEnsemble()->getPlatformMapping()->getPlatformPorts();
    const auto& chips = getHwQsfpEnsemble()->getPlatformMapping()->getChips();
    Ports ports;
    auto agentConfig = getHwQsfpEnsemble()->getWedgeManager()->getAgentConfig();
    auto& swConfig = *agentConfig->thrift.sw_ref();
    for (auto& port : *swConfig.ports_ref()) {
      if (*port.profileID_ref() != Profile ||
          *port.state_ref() != cfg::PortState::ENABLED) {
        continue;
      }
      const auto& xphy = utility::getDataPlanePhyChips(
          platformPorts.find(*port.logicalID_ref())->second,
          chips,
          phy::DataPlanePhyChipType::XPHY);
      if (!xphy.empty()) {
        ports.xphyPorts.emplace_back(PortID(*port.logicalID_ref()));
      }
      // All ports have iphys
      ports.iphyPorts.emplace_back(PortID(*port.logicalID_ref()));
    }
    return ports;
  }

  void verifyPhyPort(PortID portID) {
    utility::verifyPhyPortConfig(
        portID,
        Profile,
        getHwQsfpEnsemble()->getPlatformMapping(),
        getHwQsfpEnsemble()->getExternalPhy(portID));

    utility::verifyPhyPortConnector(portID, getHwQsfpEnsemble());
  }
  void verifyTransceiverSettings(
      const std::map<int32_t, TransceiverInfo>& /*transceivers*/) {
    // TODO
  }

 protected:
  void runTest() {
    auto ports = findAvailablePorts();
    EXPECT_TRUE(!(ports.xphyPorts.empty() && ports.iphyPorts.empty()));
    auto portMap = std::make_unique<WedgeManager::PortMap>();
    for (auto port : ports.xphyPorts) {
      if (getHwQsfpEnsemble()->getWedgeManager()->getPlatformMode() ==
          PlatformMode::ELBERT) {
        // Right now only Elbert supports programming PHYs via phy manager in
        // qsfp service.
        getHwQsfpEnsemble()->getPhyManager()->programOnePort(port, Profile);
        // Verify whether such profile has been programmed to the port
        verifyPhyPort(port);
      }
      portMap->emplace(port, getPortStatus(port));
    }
    for (auto port : ports.iphyPorts) {
      portMap->emplace(port, getPortStatus(port));
    }
    std::map<int32_t, TransceiverInfo> transceivers;
    getHwQsfpEnsemble()->getWedgeManager()->syncPorts(
        transceivers, std::move(portMap));
    verifyTransceiverSettings(transceivers);
  }

 private:
  PortStatus getPortStatus(PortID portId) {
    auto config = *getHwQsfpEnsemble()
                       ->getWedgeManager()
                       ->getAgentConfig()
                       ->thrift.sw_ref();
    std::optional<cfg::Port> portCfg;
    for (auto& port : *config.ports_ref()) {
      if (*port.logicalID_ref() == static_cast<uint16_t>(portId)) {
        portCfg = port;
        break;
      }
    }
    CHECK(portCfg);
    PortStatus status;
    status.enabled_ref() = *portCfg->state_ref() == cfg::PortState::ENABLED;
    // Mark port down to force transceiver programming
    status.up_ref() = false;
    status.speedMbps_ref() = static_cast<int64_t>(*portCfg->speed_ref());
    return status;
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
