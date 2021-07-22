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

      const auto& expectedPhyPortConfig =
          getHwQsfpEnsemble()->getPhyManager()->getDesiredPhyPortConfig(
              portID, profile, tcvrOpt);

      utility::verifyPhyPortConfig(
          portID, getHwQsfpEnsemble()->getPhyManager(), expectedPhyPortConfig);

      utility::verifyPhyPortConnector(portID, getHwQsfpEnsemble());
    }
  }

  void verifyTransceiverSettings(
      const std::map<int32_t, TransceiverInfo>& transceivers) {
    XLOG(INFO) << " Will verify transceiver settings for : "
               << transceivers.size() << " ports.";
    for (auto idAndTransceiver : transceivers) {
      auto& transceiver = idAndTransceiver.second;
      auto id = idAndTransceiver.first;
      if (!*transceiver.present_ref()) {
        XLOG(INFO) << " Skip verifying: " << id << ", not present";
        continue;
      }
      XLOG(INFO) << " Verifying: " << id;
      // Only testing QSFP transceivers right now
      EXPECT_EQ(*transceiver.transceiver_ref(), TransceiverType::QSFP);
      auto settings = apache::thrift::can_throw(*transceiver.settings_ref());
      // Disable low power mode
      EXPECT_TRUE(
          *settings.powerControl_ref() == PowerControlState::POWER_OVERRIDE ||
          *settings.powerControl_ref() ==
              PowerControlState::HIGH_POWER_OVERRIDE);
      EXPECT_EQ(*settings.cdrTx_ref(), FeatureState::ENABLED);
      EXPECT_EQ(*settings.cdrRx_ref(), FeatureState::ENABLED);
      for (auto& mediaLane :
           apache::thrift::can_throw(*settings.mediaLaneSettings_ref())) {
        EXPECT_FALSE(mediaLane.txDisable_ref().value());
        EXPECT_FALSE(mediaLane.txSquelch_ref().value());
      }
      for (auto& hostLane :
           apache::thrift::can_throw(*settings.hostLaneSettings_ref())) {
        EXPECT_FALSE(hostLane.rxSquelch_ref().value());
      }
      auto mgmtInterface = apache::thrift::can_throw(
          *transceiver.transceiverManagementInterface_ref());
      EXPECT_TRUE(
          mgmtInterface == TransceiverManagementInterface::SFF ||
          mgmtInterface == TransceiverManagementInterface::CMIS);
      auto mediaIntefaces =
          apache::thrift::can_throw(*settings.mediaInterface_ref());
      switch (Profile) {
        case cfg::PortProfileID::PROFILE_100G_4_NRZ_RS528:
        case cfg::PortProfileID::PROFILE_100G_4_NRZ_RS528_OPTICAL:
          for (const auto& mediaId : mediaIntefaces) {
            if (mgmtInterface == TransceiverManagementInterface::SFF) {
              auto specComplianceCode =
                  *mediaId.media_ref()
                       ->extendedSpecificationComplianceCode_ref();
              EXPECT_TRUE(
                  specComplianceCode ==
                      ExtendedSpecComplianceCode::CWDM4_100G ||
                  specComplianceCode == ExtendedSpecComplianceCode::FR1_100G);
            } else if (mgmtInterface == TransceiverManagementInterface::CMIS) {
              EXPECT_EQ(
                  *mediaId.media_ref()->smfCode_ref(),
                  SMFMediaInterfaceCode::CWDM4_100G);
            }
          }
          break;
        case cfg::PortProfileID::PROFILE_40G_4_NRZ_NOFEC:
        case cfg::PortProfileID::PROFILE_200G_4_PAM4_RS544X2N:
        case cfg::PortProfileID::PROFILE_200G_4_PAM4_RS544X2N_OPTICAL:
        case cfg::PortProfileID::PROFILE_400G_8_PAM4_RS544X2N_OPTICAL:
          // TODO
          break;
        default:
          throw FbossError("Unhandled profile ", Profile);
      }
    }
  }

 protected:
  void runTest() {
    auto platformMode =
        getHwQsfpEnsemble()->getWedgeManager()->getPlatformMode();
    const auto& ports =
        utility::findAvailablePorts(getHwQsfpEnsemble(), Profile);
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
      std::map<int32_t, TransceiverInfo> transceivers;
      for (auto& [port, profile] : ports.xphyPorts) {
        getHwQsfpEnsemble()->getWedgeManager()->programXphyPort(port, profile);
      }
      getHwQsfpEnsemble()->getWedgeManager()->syncPorts(
          transceivers, std::make_unique<WedgeManager::PortMap>(portMap));
    };
    auto verify = [&]() {
      auto transceiverIds =
          utility::getTransceiverIds(matchingPorts, getHwQsfpEnsemble());
      getHwQsfpEnsemble()->getWedgeManager()->refreshTransceivers();

      std::map<int32_t, TransceiverInfo> transceiversAfterRefresh;
      getHwQsfpEnsemble()->getWedgeManager()->getTransceiversInfo(
          transceiversAfterRefresh,
          std::make_unique<std::vector<int32_t>>(
              utility::legacyTransceiverIds(transceiverIds)));
      // Verify whether such profile has been programmed to all the xphy ports
      verifyXphyPorts(ports.xphyPorts, transceiversAfterRefresh);

      // Only needs to verify transceivers if there're transceivers in the
      // test system. getTransceiversInfo() will fetch all ports transceivers
      // and set present to false if there's no transceiver on such front panel
      // ports. But syncPorts won't set `transceivers` if there's no transceiver
      // there.
      // Assert that refresh caused transceiver info to be pulled
      // from HW
      verifyTransceiverSettings(transceiversAfterRefresh);
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

TEST_PROFILE(PROFILE_100G_4_NRZ_RS528_OPTICAL)

TEST_PROFILE(PROFILE_200G_4_PAM4_RS544X2N_OPTICAL)

TEST_PROFILE(PROFILE_400G_8_PAM4_RS544X2N_OPTICAL)

// The following profiles have been used by Minipack/Yamp.
// But we prefer a more detailed profile with the optics type in the name
// like PROFILE_100G_4_NRZ_RS528_OPTICAL
// For now, we keep support the following profiles until we can deprecate
// these old profiles.
TEST_PROFILE(PROFILE_40G_4_NRZ_NOFEC)

TEST_PROFILE(PROFILE_100G_4_NRZ_RS528)

TEST_PROFILE(PROFILE_200G_4_PAM4_RS544X2N)

} // namespace facebook::fboss
