// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include <folly/logging/xlog.h>
#include "fboss/agent/AgentConfig.h"
#include "fboss/qsfp_service/test/hw_test/HwTest.h"

#include "fboss/qsfp_service/lib/QsfpConfigParserHelper.h"
#include "fboss/qsfp_service/test/hw_test/HwPortUtils.h"
#include "fboss/qsfp_service/test/hw_test/HwQsfpEnsemble.h"

namespace facebook::fboss {

TEST_F(HwTest, moduleConfigVerificationTest) {
  auto portMap = std::make_unique<WedgeManager::PortMap>();
  auto agentConfig = getHwQsfpEnsemble()->getWedgeManager()->getAgentConfig();
  auto qsfpConfig = getHwQsfpEnsemble()->getWedgeManager()->getQsfpConfig();
  auto& swConfig = *agentConfig->thrift.sw_ref();

  // Build port status' for syncPorts
  for (auto& port : *swConfig.ports_ref()) {
    if (*port.state_ref() != cfg::PortState::ENABLED) {
      continue;
    }
    auto portId = *port.logicalID_ref();
    auto portStatus =
        utility::getPortStatus(PortID(portId), getHwQsfpEnsemble());
    // Mark the port up to trigger module configuration
    portStatus.up_ref() = true;
    portMap->emplace(portId, portStatus);
  }
  auto wedgeManager = getHwQsfpEnsemble()->getWedgeManager();
  std::map<int32_t, TransceiverInfo> transceivers;
  wedgeManager->syncPorts(transceivers, std::move(portMap));

  // After syncPorts, the module should have the expected configuration settings
  // programmed.
  // Do a refresh and get the latest configured settings
  refreshTransceiversWithRetry();
  auto transceiverIds = std::make_unique<std::vector<int32_t>>();
  std::for_each(
      transceivers.begin(),
      transceivers.end(),
      [&transceiverIds](const auto& idAndInfo) {
        transceiverIds->push_back(idAndInfo.first);
      });
  transceivers.clear();
  wedgeManager->getTransceiversInfo(transceivers, std::move(transceiverIds));

  // Validate settings in QSFP config
  for (const auto& tcvr : transceivers) {
    auto transceiver = tcvr.second;
    auto mgmtInterface = apache::thrift::can_throw(
        *transceiver.transceiverManagementInterface_ref());
    cfg::TransceiverConfigOverrideFactor moduleFactor;
    auto settings = apache::thrift::can_throw(*transceiver.settings_ref());
    auto mediaIntefaces =
        apache::thrift::can_throw(*settings.mediaInterface_ref());
    if (mgmtInterface == TransceiverManagementInterface::CMIS) {
      moduleFactor.applicationCode_ref() =
          *mediaIntefaces[0].media_ref()->smfCode_ref();
    } else {
      EXPECT_TRUE(
          mgmtInterface == TransceiverManagementInterface::SFF ||
          mgmtInterface == TransceiverManagementInterface::SFF8472);
      // TODO: Update this test for SFF configurations
      continue;
    }

    auto hostSettings =
        apache::thrift::can_throw(*settings.hostLaneSettings_ref());

    // Check if the config has an override for this kind of transceiver
    for (const auto& cfgOverride :
         *(qsfpConfig->thrift.transceiverConfigOverrides_ref())) {
      if (overrideFactorMatchFound(*cfgOverride.factor_ref(), moduleFactor)) {
        if (mgmtInterface == TransceiverManagementInterface::CMIS) {
          // Validate RxEqualizerSettings for CMIS modules
          if (auto rxEqSetting =
                  cmisRxEqualizerSettingOverride(*cfgOverride.config_ref())) {
            for (const auto& setting : hostSettings) {
              XLOG(DBG2) << folly::sformat(
                  "Module : {:d}, Settings in the configuration : {:d}, {:d}, {:d}, Settings programmed in the module : {:d}, {:d}, {:d}",
                  *transceiver.port_ref(),
                  *(*rxEqSetting).preCursor_ref(),
                  *(*rxEqSetting).postCursor_ref(),
                  *(*rxEqSetting).mainAmplitude_ref(),
                  apache::thrift::can_throw(*setting.rxOutputPreCursor_ref()),
                  apache::thrift::can_throw(*setting.rxOutputPostCursor_ref()),
                  apache::thrift::can_throw(*setting.rxOutputAmplitude_ref()));
              EXPECT_TRUE(
                  apache::thrift::can_throw(*setting.rxOutputPreCursor_ref()) ==
                  *(*rxEqSetting).preCursor_ref());
            }
          }
        }
      }
    }
  }
}

} // namespace facebook::fboss
