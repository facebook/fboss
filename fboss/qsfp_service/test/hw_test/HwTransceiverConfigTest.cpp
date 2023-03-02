// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include <folly/logging/xlog.h>
#include "fboss/agent/AgentConfig.h"
#include "fboss/qsfp_service/test/hw_test/HwTest.h"

#include "fboss/qsfp_service/lib/QsfpConfigParserHelper.h"
#include "fboss/qsfp_service/test/hw_test/HwPortUtils.h"
#include "fboss/qsfp_service/test/hw_test/HwQsfpEnsemble.h"
#include "fboss/qsfp_service/test/hw_test/HwTransceiverTest.h"

namespace facebook::fboss {

class HwTransceiverConfigTest : public HwTransceiverTest {
 public:
  // Mark the port up to trigger module configuration
  HwTransceiverConfigTest() : HwTransceiverTest(true) {}
};

TEST_F(HwTransceiverConfigTest, moduleConfigVerification) {
  auto wedgeManager = getHwQsfpEnsemble()->getWedgeManager();
  auto qsfpConfig = wedgeManager->getQsfpConfig();
  // After syncPorts, the module should have the expected configuration settings
  // programmed.
  std::map<int32_t, TransceiverInfo> transceivers;
  wedgeManager->getTransceiversInfo(
      transceivers, getExpectedLegacyTransceiverIds());

  // Validate settings in QSFP config
  for (const auto& tcvr : transceivers) {
    auto transceiver = tcvr.second;
    auto& tcvrState = *transceiver.tcvrState();
    auto mgmtInterface =
        apache::thrift::can_throw(*tcvrState.transceiverManagementInterface());
    cfg::TransceiverConfigOverrideFactor moduleFactor;
    auto settings = apache::thrift::can_throw(*tcvrState.settings());
    auto mediaIntefaces = apache::thrift::can_throw(*settings.mediaInterface());
    if (mgmtInterface == TransceiverManagementInterface::SFF8472) {
      // TODO: Nothing to verify for sff8472 modules
      continue;
    } else if (mgmtInterface == TransceiverManagementInterface::CMIS) {
      auto& cable = apache::thrift::can_throw(*tcvrState.cable());
      if (cable.transmitterTech() != TransmitterTechnology::COPPER) {
        moduleFactor.applicationCode() =
            *mediaIntefaces[0].media()->smfCode_ref();
      }
    } else {
      EXPECT_TRUE(mgmtInterface == TransceiverManagementInterface::SFF);
    }

    auto hostSettings = apache::thrift::can_throw(*settings.hostLaneSettings());

    // Check if the config has an override for this kind of transceiver
    for (const auto& cfgOverride :
         *(qsfpConfig->thrift.transceiverConfigOverrides())) {
      if (overrideFactorMatchFound(*cfgOverride.factor(), moduleFactor)) {
        if (mgmtInterface == TransceiverManagementInterface::CMIS) {
          // Validate RxEqualizerSettings for CMIS modules
          if (auto rxEqSetting =
                  cmisRxEqualizerSettingOverride(*cfgOverride.config())) {
            for (const auto& setting : hostSettings) {
              XLOG(DBG2) << folly::sformat(
                  "Module : {:d}, Settings in the configuration : {:d}, {:d}, {:d}, Settings programmed in the module : {:d}, {:d}, {:d}",
                  *tcvrState.port(),
                  *(*rxEqSetting).preCursor(),
                  *(*rxEqSetting).postCursor(),
                  *(*rxEqSetting).mainAmplitude(),
                  apache::thrift::can_throw(*setting.rxOutputPreCursor()),
                  apache::thrift::can_throw(*setting.rxOutputPostCursor()),
                  apache::thrift::can_throw(*setting.rxOutputAmplitude()));
              EXPECT_TRUE(
                  apache::thrift::can_throw(*setting.rxOutputPreCursor()) ==
                  *(*rxEqSetting).preCursor());
            }
          }
        } else if (mgmtInterface == TransceiverManagementInterface::SFF) {
          if (auto rxPreemphasis =
                  sffRxPreemphasisOverride(*cfgOverride.config())) {
            for (const auto& setting : hostSettings) {
              XLOG(DBG2) << folly::sformat(
                  "Module : {:d}, Preemphasis in the configuration : {:d}, Preemphasis programmed in the module : {:d}",
                  *tcvrState.port(),
                  *rxPreemphasis,
                  apache::thrift::can_throw(*setting.rxOutputEmphasis()));
              EXPECT_TRUE(
                  apache::thrift::can_throw(*setting.rxOutputEmphasis()) ==
                  *rxPreemphasis);
            }
          }
          if (auto txEqualization =
                  sffTxEqualizationOverride(*cfgOverride.config())) {
            for (const auto& setting : hostSettings) {
              XLOG(DBG2) << folly::sformat(
                  "Module : {:d}, TxEqualization in the configuration : {:d}, TxEqualization programmed in the module : {:d}",
                  *tcvrState.port(),
                  *txEqualization,
                  apache::thrift::can_throw(*setting.txInputEqualization()));
              EXPECT_TRUE(
                  apache::thrift::can_throw(*setting.txInputEqualization()) ==
                  *txEqualization);
            }
          }
          if (auto rxAmplitude =
                  sffRxAmplitudeOverride(*cfgOverride.config())) {
            for (const auto& setting : hostSettings) {
              XLOG(DBG2) << folly::sformat(
                  "Module : {:d}, RxAmplitude in the configuration : {:d}, RxAmplitude programmed in the module : {:d}",
                  *tcvrState.port(),
                  *rxAmplitude,
                  apache::thrift::can_throw(*setting.rxOutputAmplitude()));
              EXPECT_TRUE(
                  apache::thrift::can_throw(*setting.rxOutputAmplitude()) ==
                  *rxAmplitude);
            }
          }
        }
      }
    }
  }
}

} // namespace facebook::fboss
