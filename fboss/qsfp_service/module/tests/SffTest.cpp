/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/qsfp_service/test/TransceiverManagerTestHelper.h"

#include "fboss/qsfp_service/module/sff/Sff8472Module.h"
#include "fboss/qsfp_service/module/sff/SffModule.h"
#include "fboss/qsfp_service/module/tests/FakeTransceiverImpl.h"
#include "fboss/qsfp_service/module/tests/TransceiverTestsHelper.h"
#include "fboss/qsfp_service/test/hw_test/HwTransceiverUtils.h"

namespace facebook::fboss {

class SffTest : public TransceiverManagerTestHelper {
 public:
  template <typename XcvrImplT>
  SffModule* overrideSffModule(TransceiverID id, bool willRefresh = true) {
    auto xcvrImpl = std::make_unique<XcvrImplT>(id);

    auto xcvr = static_cast<SffModule*>(
        transceiverManager_->overrideTransceiverForTesting(
            id,
            std::make_unique<SffModule>(
                transceiverManager_.get(), std::move(xcvrImpl))));

    if (willRefresh) {
      // Refresh once to make sure the override transceiver finishes refresh
      transceiverManager_->refreshStateMachines();
    }

    return xcvr;
  }
};

// Tests that the transceiverInfo object is correctly populated
TEST_F(SffTest, cwdm4TransceiverInfoTest) {
  auto xcvrID = TransceiverID(1);
  auto xcvr = overrideSffModule<SffCwdm4Transceiver>(xcvrID);

  // Verify SffModule logic
  EXPECT_EQ(xcvr->numHostLanes(), 4);
  EXPECT_EQ(xcvr->numMediaLanes(), 4);

  // Verify getTransceiverInfo() result
  const auto& info = xcvr->getTransceiverInfo();
  EXPECT_EQ(
      *info.extendedSpecificationComplianceCode(),
      ExtendedSpecComplianceCode::CWDM4_100G);
  EXPECT_EQ(info.moduleMediaInterface(), MediaInterfaceCode::CWDM4_100G);
  for (auto& media : *info.settings()->mediaInterface()) {
    EXPECT_EQ(
        *media.media()->extendedSpecificationComplianceCode_ref(),
        ExtendedSpecComplianceCode::CWDM4_100G);
    EXPECT_EQ(media.code(), MediaInterfaceCode::CWDM4_100G);
  }

  EXPECT_EQ(100, info.cable().value_or({}).om3().value_or({}));
  EXPECT_EQ(true, info.status().value_or({}).interruptL().value_or({}));

  utility::HwTransceiverUtils::verifyDiagsCapability(
      *info.tcvrState(),
      transceiverManager_->getDiagsCapability(xcvrID),
      false /* skipCheckingIndividualCapability */);

  // Using TransceiverTestsHelper to verify TransceiverInfo
  TransceiverTestsHelper tests(info);
  tests.verifyVendorName("FACETEST");
  tests.verifyTemp(31.015625);
  tests.verifyVcc(3.2989);
  std::map<std::string, std::vector<double>> laneDom = {
      {"TxBias", {10.28, 8.738, 17.476, 26.214}},
      {"TxPwr", {1.6705, 1.6962, 1.7219, 1.7476}},
      {"RxPwr", {0.4643, 0.9012, 0.4913, 0.4626}},
  };
  tests.verifyLaneDom(laneDom, xcvr->numMediaLanes());

  std::map<std::string, std::vector<bool>> expectedMediaSignals = {
      {"Tx_Los", {0, 1, 0, 1}},
      {"Rx_Los", {1, 1, 0, 1}},
      {"Tx_Lol", {0, 0, 1, 1}},
      {"Rx_Lol", {1, 0, 1, 1}},
      {"Tx_Fault", {0, 0, 1, 1}},
      {"Tx_AdaptFault", {1, 1, 0, 1}},
  };
  tests.verifyLaneSignals(
      expectedMediaSignals, xcvr->numHostLanes(), xcvr->numMediaLanes());

  std::map<std::string, std::vector<bool>> expectedMediaLaneSettings = {
      {"TxDisable", {0, 1, 1, 1}},
      {"TxSqDisable", {0, 0, 1, 1}},
      {"TxAdaptEqCtrl", {1, 0, 1, 1}},
  };
  std::map<std::string, std::vector<uint8_t>> expectedHostLaneSettings = {
      {"RxOutDisable", {1, 1, 0, 0}},
      {"RxSqDisable", {0, 1, 0, 0}},
      {"TxInputEq", {1, 2, 3, 4}},
      {"RxOutputEmph", {2, 1, 1, 3}},
      {"RxOutputAmp", {1, 3, 2, 3}},
  };
  auto settings = info.settings().value_or({});
  tests.verifyMediaLaneSettings(
      expectedMediaLaneSettings, xcvr->numMediaLanes());
  tests.verifyHostLaneSettings(expectedHostLaneSettings, xcvr->numHostLanes());

  std::map<std::string, std::vector<bool>> laneInterrupts = {
      {"TxPwrHighAlarm", {1, 0, 0, 0}},
      {"TxPwrLowAlarm", {1, 1, 0, 0}},
      {"TxPwrHighWarn", {0, 0, 0, 1}},
      {"TxPwrLowWarn", {0, 1, 0, 1}},
      {"RxPwrHighAlarm", {1, 0, 1, 0}},
      {"RxPwrLowAlarm", {1, 1, 1, 0}},
      {"RxPwrHighWarn", {0, 0, 1, 1}},
      {"RxPwrLowWarn", {0, 1, 1, 1}},
      {"TxBiasHighAlarm", {1, 0, 0, 1}},
      {"TxBiasLowAlarm", {1, 1, 0, 1}},
      {"TxBiasHighWarn", {0, 0, 1, 0}},
      {"TxBiasLowWarn", {0, 1, 1, 0}},
  };
  tests.verifyLaneInterrupts(laneInterrupts, xcvr->numMediaLanes());

  tests.verifyGlobalInterrupts("temp", 1, 1, 0, 0);
  tests.verifyGlobalInterrupts("vcc", 0, 0, 1, 1);

  tests.verifyThresholds("temp", 75, -5, 70, 0);
  tests.verifyThresholds("vcc", 3.80, 2.84, 3.45, 3.15);
  tests.verifyThresholds("rxPwr", 0.6683, 0.7197, 0.7711, 0.437);
  tests.verifyThresholds("txPwr", 0.4386, 1.3124, 2.1862, 3.06);
  tests.verifyThresholds("txBias", 0.034, 17.51, 34.986, 52.462);

  // This test will write registers, save it to the last
  testCachedMediaSignals(xcvr);
}

// Tests that a SFF DAC module can properly refresh
TEST_F(SffTest, dacTransceiverInfoTest) {
  auto xcvrID = TransceiverID(1);
  auto xcvr = overrideSffModule<SffDacTransceiver>(xcvrID);

  // Verify SffModule logic
  EXPECT_EQ(xcvr->numHostLanes(), 4);
  EXPECT_EQ(xcvr->numMediaLanes(), 4);

  // Verify getTransceiverInfo() result
  const auto& info = xcvr->getTransceiverInfo();
  EXPECT_EQ(
      *info.extendedSpecificationComplianceCode(),
      ExtendedSpecComplianceCode::CR4_100G);
  EXPECT_EQ(info.moduleMediaInterface(), MediaInterfaceCode::CR4_100G);
  for (auto& media : *info.settings()->mediaInterface()) {
    EXPECT_EQ(
        *media.media()->extendedSpecificationComplianceCode_ref(),
        ExtendedSpecComplianceCode::CR4_100G);
    EXPECT_EQ(media.code(), MediaInterfaceCode::CR4_100G);
  }

  utility::HwTransceiverUtils::verifyDiagsCapability(
      *info.tcvrState(),
      transceiverManager_->getDiagsCapability(xcvrID),
      false /* skipCheckingIndividualCapability */);

  // Using TransceiverTestsHelper to verify TransceiverInfo
  TransceiverTestsHelper tests(info);
  tests.verifyVendorName("FACETEST");

  // This test will write registers, save it to the last
  testCachedMediaSignals(xcvr);
}

// Tests that a SFF Fr1 module can properly refresh
TEST_F(SffTest, fr1TransceiverInfoTest) {
  auto xcvrID = TransceiverID(1);
  auto xcvr = overrideSffModule<SffFr1Transceiver>(xcvrID);

  // Verify SffModule logic
  EXPECT_EQ(xcvr->numHostLanes(), 4);
  EXPECT_EQ(xcvr->numMediaLanes(), 1);

  // Verify getTransceiverInfo() result
  const auto& info = xcvr->getTransceiverInfo();
  EXPECT_EQ(
      *info.extendedSpecificationComplianceCode(),
      ExtendedSpecComplianceCode::FR1_100G);
  EXPECT_EQ(info.moduleMediaInterface(), MediaInterfaceCode::FR1_100G);
  for (auto& media : *info.settings()->mediaInterface()) {
    EXPECT_EQ(
        media.media()->get_extendedSpecificationComplianceCode(),
        ExtendedSpecComplianceCode::FR1_100G);
    EXPECT_EQ(media.code(), MediaInterfaceCode::FR1_100G);
  }

  auto diagsCap = transceiverManager_->getDiagsCapability(xcvrID);
  utility::HwTransceiverUtils::verifyDiagsCapability(
      *info.tcvrState(),
      diagsCap,
      false /* skipCheckingIndividualCapability */);
  EXPECT_TRUE(diagsCap.has_value());
  std::vector<prbs::PrbsPolynomial> expectedCapabilities = {
      prbs::PrbsPolynomial::PRBS31};
  const auto& prbsSystemCaps = *diagsCap->prbsSystemCapabilities();
  EXPECT_TRUE(std::equal(
      prbsSystemCaps.begin(),
      prbsSystemCaps.end(),
      expectedCapabilities.begin()));
  const auto& prbsLineCaps = *diagsCap->prbsLineCapabilities();
  EXPECT_TRUE(std::equal(
      prbsLineCaps.begin(), prbsLineCaps.end(), expectedCapabilities.begin()));

  // Using TransceiverTestsHelper to verify TransceiverInfo
  TransceiverTestsHelper tests(info);
  tests.verifyVendorName("FACETEST");

  // This test will write registers, save it to the last
  testCachedMediaSignals(xcvr);
}

// Tests that a miniphoton module can properly refresh
TEST_F(SffTest, miniphotonTransceiverInfoTest) {
  auto xcvrID = TransceiverID(1);
  auto xcvr = overrideSffModule<MiniphotonOBOTransceiver>(xcvrID);

  // Verify SffModule logic
  EXPECT_EQ(xcvr->numHostLanes(), 4);
  EXPECT_EQ(xcvr->numMediaLanes(), 4);

  // Verify getTransceiverInfo() result
  const auto& info = xcvr->getTransceiverInfo();
  EXPECT_EQ(
      *info.extendedSpecificationComplianceCode(),
      ExtendedSpecComplianceCode::CWDM4_100G);
  EXPECT_EQ(info.moduleMediaInterface(), MediaInterfaceCode::CWDM4_100G);
  for (auto& media : *info.settings()->mediaInterface()) {
    EXPECT_EQ(
        media.media()->get_extendedSpecificationComplianceCode(),
        ExtendedSpecComplianceCode::CWDM4_100G);
    EXPECT_EQ(media.code(), MediaInterfaceCode::CWDM4_100G);
  }

  utility::HwTransceiverUtils::verifyDiagsCapability(
      *info.tcvrState(),
      transceiverManager_->getDiagsCapability(xcvrID),
      false /* skipCheckingIndividualCapability */);

  // Using TransceiverTestsHelper to verify TransceiverInfo
  TransceiverTestsHelper tests(info);
  tests.verifyVendorName("FACETEST");
}

TEST_F(SffTest, unknownTransceiverInfoTest) {
  auto xcvrID = TransceiverID(1);
  auto xcvr = overrideSffModule<UnknownModuleIdentifierTransceiver>(xcvrID);

  // Verify SffModule logic
  EXPECT_EQ(xcvr->numHostLanes(), 4);
  EXPECT_EQ(xcvr->numMediaLanes(), 4);

  const auto& info = xcvr->getTransceiverInfo();
  EXPECT_EQ(
      *info.extendedSpecificationComplianceCode(),
      ExtendedSpecComplianceCode::CWDM4_100G);
  EXPECT_EQ(info.moduleMediaInterface(), MediaInterfaceCode::CWDM4_100G);
  for (auto& media : *info.settings()->mediaInterface()) {
    EXPECT_EQ(
        media.media()->get_extendedSpecificationComplianceCode(),
        ExtendedSpecComplianceCode::CWDM4_100G);
    EXPECT_EQ(media.code(), MediaInterfaceCode::CWDM4_100G);
  }

  TransceiverTestsHelper tests(info);
  tests.verifyVendorName("FACETEST");
}

// Tests that a badly programmed module throws an exception
TEST_F(SffTest, badTransceiverSimpleRead) {
  auto xcvr = overrideSffModule<BadSffCwdm4Transceiver>(
      TransceiverID(1), false /* willRefresh */);
  EXPECT_THROW(xcvr->refresh(), QsfpModuleError);
}

TEST_F(SffTest, moduleEepromChecksumTest) {
  // Create SFF FR1 module
  auto xcvr100GFr1 = overrideSffModule<SffFr1Transceiver>(TransceiverID(1));
  // Verify EEPROM checksum for SFF FR1 module
  bool csumValid = xcvr100GFr1->verifyEepromChecksums();
  EXPECT_TRUE(csumValid);

  // Create CWDM4 module
  auto xcvrCwdm4 = overrideSffModule<SffCwdm4Transceiver>(TransceiverID(2));
  // Verify EEPROM checksum for CWDM4 module
  csumValid = xcvrCwdm4->verifyEepromChecksums();
  EXPECT_TRUE(csumValid);

  // Create CWDM4 Bad module
  auto xcvrCwdm4Bad =
      overrideSffModule<BadEepromSffCwdm4Transceiver>(TransceiverID(3));
  // Verify EEPROM checksum Invalid for CWDM4 Bad module
  csumValid = xcvrCwdm4Bad->verifyEepromChecksums();
  EXPECT_FALSE(csumValid);
}

class SfpTest : public TransceiverManagerTestHelper {
 public:
  template <typename XcvrImplT>
  Sff8472Module* overrideSfpModule(TransceiverID id) {
    auto xcvrImpl = std::make_unique<XcvrImplT>(id);
    // This override function use ids starting from 1
    transceiverManager_->overrideMgmtInterface(
        static_cast<int>(id) + 1,
        uint8_t(TransceiverModuleIdentifier::SFP_PLUS));

    auto xcvr = static_cast<Sff8472Module*>(
        transceiverManager_->overrideTransceiverForTesting(
            id,
            std::make_unique<Sff8472Module>(
                transceiverManager_.get(), std::move(xcvrImpl))));
    // Refresh once to make sure the override transceiver finishes refresh
    transceiverManager_->refreshStateMachines();

    return xcvr;
  }
};

// Tests that a SFP module can properly refresh
TEST_F(SfpTest, sfp10GTransceiverInfoTest) {
  auto xcvrID = TransceiverID(1);
  auto xcvr = overrideSfpModule<Sfp10GTransceiver>(xcvrID);

  // Verify SffModule logic
  EXPECT_EQ(xcvr->numHostLanes(), 1);
  EXPECT_EQ(xcvr->numMediaLanes(), 1);

  // Verify getTransceiverInfo() result
  const auto& info = xcvr->getTransceiverInfo();
  EXPECT_EQ((*info.settings()->mediaInterface()).size(), xcvr->numMediaLanes());
  EXPECT_EQ(info.moduleMediaInterface(), MediaInterfaceCode::LR_10G);
  for (auto& media : *info.settings()->mediaInterface()) {
    EXPECT_EQ(
        media.media()->get_ethernet10GComplianceCode(),
        Ethernet10GComplianceCode::LR_10G);
    EXPECT_EQ(media.code(), MediaInterfaceCode::LR_10G);
  }

  utility::HwTransceiverUtils::verifyDiagsCapability(
      *info.tcvrState(),
      transceiverManager_->getDiagsCapability(xcvrID),
      false /* skipCheckingIndividualCapability */);

  // Using TransceiverTestsHelper to verify TransceiverInfo
  TransceiverTestsHelper tests(info);
  tests.verifyTemp(18.203125);
  tests.verifyVcc(2.2136);
  std::map<std::string, std::vector<double>> laneDom = {
      {"TxBias", {79.224}},
      {"TxPwr", {5.6849}},
      {"RxPwr", {0.8755}},
  };
  tests.verifyLaneDom(laneDom, xcvr->numMediaLanes());

  std::map<std::string, std::vector<bool>> expectedMediaSignals = {
      {"Rx_Los", {1}},
      {"Tx_Fault", {1}},
  };
  // Only check media signals, we don't have any host signals on sfp10g
  tests.verifyMediaLaneSignals(expectedMediaSignals, xcvr->numMediaLanes());

  std::map<std::string, std::vector<bool>> expectedMediaLaneSettings = {
      {"TxDisable", {1}},
  };
  tests.verifyMediaLaneSettings(
      expectedMediaLaneSettings, xcvr->numMediaLanes());
  tests.verifyVendorName("FACETEST");
}

TEST_F(SffTest, 200GCr4TransceiverInfoTest) {
  auto xcvrID = TransceiverID(1);
  auto xcvr = overrideSffModule<Sff200GCr4Transceiver>(xcvrID);

  // Verify SffModule logic
  EXPECT_EQ(xcvr->numHostLanes(), 4);
  EXPECT_EQ(xcvr->numMediaLanes(), 4);

  // Verify getTransceiverInfo() result
  const auto& info = xcvr->getTransceiverInfo();
  EXPECT_EQ(
      *info.extendedSpecificationComplianceCode(),
      ExtendedSpecComplianceCode::CR_50G_CHANNELS);
  EXPECT_EQ(info.moduleMediaInterface(), MediaInterfaceCode::CR4_200G);
  for (auto& media : *info.settings()->mediaInterface()) {
    EXPECT_EQ(
        media.media()->get_extendedSpecificationComplianceCode(),
        ExtendedSpecComplianceCode::CR_50G_CHANNELS);
    EXPECT_EQ(media.code(), MediaInterfaceCode::CR4_200G);
  }

  utility::HwTransceiverUtils::verifyDiagsCapability(
      *info.tcvrState(),
      transceiverManager_->getDiagsCapability(xcvrID),
      false /* skipCheckingIndividualCapability */);

  // Using TransceiverTestsHelper to verify TransceiverInfo
  TransceiverTestsHelper tests(info);
  tests.verifyVendorName("FACETEST");
}

} // namespace facebook::fboss
