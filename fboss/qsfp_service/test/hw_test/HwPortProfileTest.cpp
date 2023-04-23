/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/qsfp_service/if/gen-cpp2/transceiver_types.h"
#include "fboss/qsfp_service/test/hw_test/HwTest.h"

#include "fboss/agent/AgentConfig.h"
#include "fboss/agent/platforms/common/PlatformMapping.h"
#include "fboss/agent/state/Port.h"
#include "fboss/lib/config/PlatformConfigUtils.h"
#include "fboss/lib/phy/PhyManager.h"
#include "fboss/qsfp_service/test/hw_test/HwPortUtils.h"
#include "fboss/qsfp_service/test/hw_test/HwQsfpEnsemble.h"
#include "fboss/qsfp_service/test/hw_test/HwTransceiverUtils.h"

#include <folly/logging/xlog.h>

namespace facebook::fboss {

template <cfg::PortProfileID Profile>
class HwPortProfileTest : public HwTest {
 private:
  void verifyXphyPorts(
      const std::vector<std::pair<PortID, cfg::PortProfileID>>& xphyPorts,
      const std::map<int32_t, TransceiverInfo>& transceivers) {
    const auto* platformMapping = getHwQsfpEnsemble()->getPlatformMapping();
    for (auto& [portID, profile] : xphyPorts) {
      // Check whether transceiver exist, which will affect xphy config
      // Get the transceiver id for the given port id
      auto platformPortEntry = platformMapping->getPlatformPorts().find(portID);
      if (platformPortEntry == platformMapping->getPlatformPorts().end()) {
        throw FbossError(
            "Can't find the platform port entry in platform mapping for port:",
            portID);
      }
      auto tcvrID = utility::getTransceiverId(
          platformPortEntry->second, platformMapping->getChips());
      if (!tcvrID) {
        throw FbossError(
            "Can't find the transceiver id in platform mapping for port:",
            portID);
      }
      std::optional<TransceiverInfo> tcvrOpt;
      if (auto tcvr = transceivers.find(*tcvrID); tcvr != transceivers.end()) {
        tcvrOpt = tcvr->second;
      }

      utility::verifyXphyPort(portID, profile, tcvrOpt, getHwQsfpEnsemble());
    }
  }

  void verifyTransceiverSettings(
      const std::map<int32_t, TransceiverInfo>& transceivers) {
    XLOG(INFO) << " Will verify transceiver settings for : "
               << transceivers.size() << " ports.";
    for (auto idAndTransceiver : transceivers) {
      utility::HwTransceiverUtils::verifyTransceiverSettings(
          *idAndTransceiver.second.tcvrState(), Profile);
    }
  }

 protected:
  void runTest() {
    const auto& ports =
        utility::findAvailablePorts(getHwQsfpEnsemble(), Profile, true);
    EXPECT_TRUE(!(ports.xphyPorts.empty() && ports.iphyPorts.empty()));
    WedgeManager::PortMap portMap;
    std::vector<PortID> matchingPorts;
    // Program xphy
    for (auto& [port, _] : ports.xphyPorts) {
      portMap.emplace(port, utility::getPortStatus(port, getHwQsfpEnsemble()));
      matchingPorts.push_back(port);
    }
    for (auto& [port, _] : ports.iphyPorts) {
      portMap.emplace(port, utility::getPortStatus(port, getHwQsfpEnsemble()));
      matchingPorts.push_back(port);
    }

    auto setup = [this, &ports, &portMap]() {
      // New port programming will use state machine to program xphy ports and
      // transceivers automatically, no need to call program xphy port again
      for (auto& [port, profile] : ports.xphyPorts) {
        // Program the same port with the same profile twice, the second time
        // should be idempotent.
        getHwQsfpEnsemble()->getWedgeManager()->programXphyPort(port, profile);
      }
    };
    auto verify = [&]() {
      auto transceiverIds =
          utility::getTransceiverIds(matchingPorts, getHwQsfpEnsemble());

      std::map<int32_t, TransceiverInfo> transceivers;
      getHwQsfpEnsemble()->getWedgeManager()->getTransceiversInfo(
          transceivers,
          std::make_unique<std::vector<int32_t>>(
              utility::legacyTransceiverIds(transceiverIds)));
      // Verify whether such profile has been programmed to all the xphy ports
      verifyXphyPorts(ports.xphyPorts, transceivers);

      // Only needs to verify transceivers if there're transceivers in the
      // test system. getTransceiversInfo() will fetch all ports transceivers
      // and set present to false if there's no transceiver on such front panel
      // ports. But syncPorts won't set `transceivers` if there's no transceiver
      // there.
      // Assert that refresh caused transceiver info to be pulled
      // from HW
      verifyTransceiverSettings(transceivers);
      utility::HwTransceiverUtils::verifyPortNameToLaneMap(
          matchingPorts,
          Profile,
          getHwQsfpEnsemble()->getPlatformMapping(),
          transceivers);
    };
    verifyAcrossWarmBoots(setup, verify);
  }
};

#define TEST_PROFILE(PROFILE)                                     \
  struct HwTest_##PROFILE                                         \
      : public HwPortProfileTest<cfg::PortProfileID::PROFILE> {}; \
  TEST_F(HwTest_##PROFILE, TestProfile) {                         \
    runTest();                                                    \
  }

TEST_PROFILE(PROFILE_10G_1_NRZ_NOFEC_OPTICAL)

TEST_PROFILE(PROFILE_100G_4_NRZ_RS528_OPTICAL)

TEST_PROFILE(PROFILE_200G_4_PAM4_RS544X2N_OPTICAL)

TEST_PROFILE(PROFILE_200G_4_PAM4_RS544X2N_COPPER)

TEST_PROFILE(PROFILE_400G_8_PAM4_RS544X2N_OPTICAL)

// The following profiles have been used by Minipack/Yamp.
// But we prefer a more detailed profile with the optics type in the name
// like PROFILE_100G_4_NRZ_RS528_OPTICAL
// For now, we keep support the following profiles until we can deprecate
// these old profiles.
TEST_PROFILE(PROFILE_40G_4_NRZ_NOFEC)

TEST_PROFILE(PROFILE_100G_4_NRZ_RS528)

TEST_PROFILE(PROFILE_200G_4_PAM4_RS544X2N)

TEST_PROFILE(PROFILE_53POINT125G_1_PAM4_RS545_COPPER)

TEST_PROFILE(PROFILE_53POINT125G_1_PAM4_RS545_OPTICAL)

} // namespace facebook::fboss
