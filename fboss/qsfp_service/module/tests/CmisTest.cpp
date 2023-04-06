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

#include "fboss/qsfp_service/module/cmis/CmisModule.h"
#include "fboss/qsfp_service/module/tests/FakeTransceiverImpl.h"
#include "fboss/qsfp_service/module/tests/TransceiverTestsHelper.h"
#include "fboss/qsfp_service/test/hw_test/HwTransceiverUtils.h"

namespace facebook::fboss {

class MockCmisModule : public CmisModule {
 public:
  template <typename XcvrImplT>
  explicit MockCmisModule(
      TransceiverManager* transceiverManager,
      std::unique_ptr<XcvrImplT> qsfpImpl)
      : CmisModule(transceiverManager, std::move(qsfpImpl)) {
    ON_CALL(*this, getModuleStateChanged).WillByDefault([this]() {
      // Only return true for the first read so that we can mimic the clear
      // on read register
      return (++moduleStateChangedReadTimes_) == 1;
    });
    ON_CALL(*this, ensureTransceiverReadyLocked())
        .WillByDefault(testing::Return(true));
  }

  MOCK_METHOD0(getModuleStateChanged, bool());
  MOCK_METHOD0(ensureTransceiverReadyLocked, bool());

  using CmisModule::getApplicationField;

 private:
  uint8_t moduleStateChangedReadTimes_{0};
};

class CmisTest : public TransceiverManagerTestHelper {
 public:
  template <typename XcvrImplT>
  MockCmisModule* overrideCmisModule(
      TransceiverID id,
      TransceiverModuleIdentifier identifier =
          TransceiverModuleIdentifier::QSFP_PLUS_CMIS) {
    auto xcvrImpl = std::make_unique<XcvrImplT>(id);
    // This override function use ids starting from 1
    transceiverManager_->overrideMgmtInterface(
        static_cast<int>(id) + 1, uint8_t(identifier));

    auto xcvr = static_cast<MockCmisModule*>(
        transceiverManager_->overrideTransceiverForTesting(
            id,
            std::make_unique<MockCmisModule>(
                transceiverManager_.get(), std::move(xcvrImpl))));

    // Refresh once to make sure the override transceiver finishes refresh
    transceiverManager_->refreshStateMachines();

    return xcvr;
  }
};

// Tests that the transceiverInfo object is correctly populated
TEST_F(CmisTest, cmis200GTransceiverInfoTest) {
  auto xcvrID = TransceiverID(0);
  auto xcvr = overrideCmisModule<Cmis200GTransceiver>(xcvrID);

  const auto& info = xcvr->getTransceiverInfo();
  EXPECT_TRUE(info.transceiverManagementInterface());
  EXPECT_EQ(
      info.transceiverManagementInterface(),
      TransceiverManagementInterface::CMIS);
  EXPECT_EQ(xcvr->numHostLanes(), 4);
  EXPECT_EQ(xcvr->numMediaLanes(), 4);
  EXPECT_EQ(info.moduleMediaInterface(), MediaInterfaceCode::FR4_200G);
  EXPECT_EQ(
      xcvr->numMediaLanes(),
      info.settings()->mediaInterface().value_or({}).size());
  for (auto& media : *info.settings()->mediaInterface()) {
    EXPECT_EQ(media.media()->get_smfCode(), SMFMediaInterfaceCode::FR4_200G);
    EXPECT_EQ(media.code(), MediaInterfaceCode::FR4_200G);
  }
  testCachedMediaSignals(xcvr);
  // Check cmisStateChanged
  EXPECT_TRUE(
      info.status() && info.status()->cmisStateChanged() &&
      *info.status()->cmisStateChanged());

  utility::HwTransceiverUtils::verifyDiagsCapability(
      *info.tcvrState(),
      transceiverManager_->getDiagsCapability(xcvrID),
      false /* skipCheckingIndividualCapability */);

  // Verify update qsfp data logic
  if (auto status = info.status(); status && status->cmisModuleState()) {
    EXPECT_EQ(*status->cmisModuleState(), CmisModuleState::READY);
    auto cmisData = xcvr->getDOMDataUnion().get_cmis();
    // NOTE the following cmis data are specifically set to 0x11 in
    // FakeTransceiverImpl
    EXPECT_TRUE(cmisData.page14());
    // SNR will be set
    EXPECT_EQ(cmisData.page14()->data()[0], 0x06);
    // Check VDM cache
    EXPECT_FALSE(xcvr->isVdmSupported());
  } else {
    throw FbossError("Missing CMIS Module state");
  }

  TransceiverTestsHelper tests(info);
  tests.verifyVendorName("FACETEST");
  tests.verifyFwInfo("2.7", "1.10", "3.101");
  tests.verifyTemp(40.26953125);
  tests.verifyVcc(3.3020);

  std::map<std::string, std::vector<double>> laneDom = {
      {"TxBias", {48.442, 50.082, 53.516, 50.028}},
      {"TxPwr", {2.13, 2.0748, 2.0512, 2.1027}},
      {"RxPwr", {0.4032, 0.3969, 0.5812, 0.5176}},
  };
  tests.verifyLaneDom(laneDom, xcvr->numMediaLanes());

  std::map<std::string, std::vector<bool>> expectedMediaSignals = {
      {"Tx_Los", {1, 1, 0, 1}},
      {"Rx_Los", {1, 0, 1, 0}},
      {"Tx_Lol", {0, 0, 1, 1}},
      {"Rx_Lol", {0, 1, 1, 0}},
      {"Tx_Fault", {0, 1, 0, 1}},
      {"Tx_AdaptFault", {1, 0, 1, 1}},
  };
  tests.verifyLaneSignals(
      expectedMediaSignals, xcvr->numHostLanes(), xcvr->numMediaLanes());

  std::array<bool, 4> expectedDatapathDeinit = {0, 1, 1, 0};
  std::array<CmisLaneState, 4> expectedLaneState = {
      CmisLaneState::ACTIVATED,
      CmisLaneState::DATAPATHINIT,
      CmisLaneState::TX_ON,
      CmisLaneState::DEINIT};

  EXPECT_EQ(xcvr->numHostLanes(), info.hostLaneSignals().value_or({}).size());
  for (auto& signal : *info.hostLaneSignals()) {
    EXPECT_EQ(
        expectedDatapathDeinit[*signal.lane()],
        signal.dataPathDeInit().value_or({}));
    EXPECT_EQ(
        expectedLaneState[*signal.lane()], signal.cmisLaneState().value_or({}));
  }

  std::map<std::string, std::vector<bool>> expectedMediaLaneSettings = {
      {"TxDisable", {0, 1, 0, 1}},
      {"TxSqDisable", {1, 1, 0, 1}},
      {"TxForcedSq", {0, 0, 1, 1}},
  };

  std::map<std::string, std::vector<uint8_t>> expectedHostLaneSettings = {
      {"RxOutDisable", {1, 1, 0, 0}},
      {"RxSqDisable", {0, 0, 1, 1}},
      {"RxEqPrecursor", {2, 2, 2, 2}},
      {"RxEqPostcursor", {0, 0, 0, 0}},
      {"RxEqMain", {3, 3, 3, 3}},
  };

  auto settings = info.settings().value_or({});
  tests.verifyMediaLaneSettings(
      expectedMediaLaneSettings, xcvr->numMediaLanes());
  tests.verifyHostLaneSettings(expectedHostLaneSettings, xcvr->numHostLanes());

  EXPECT_EQ(PowerControlState::HIGH_POWER_OVERRIDE, settings.powerControl());

  std::map<std::string, std::vector<bool>> laneInterrupts = {
      {"TxPwrHighAlarm", {0, 1, 0, 0}},
      {"TxPwrHighWarn", {0, 0, 1, 0}},
      {"TxPwrLowAlarm", {1, 1, 0, 0}},
      {"TxPwrLowWarn", {1, 0, 1, 0}},
      {"RxPwrHighAlarm", {0, 1, 0, 1}},
      {"RxPwrHighWarn", {0, 0, 1, 1}},
      {"RxPwrLowAlarm", {1, 1, 0, 1}},
      {"RxPwrLowWarn", {1, 0, 1, 1}},
      {"TxBiasHighAlarm", {0, 1, 1, 0}},
      {"TxBiasHighWarn", {0, 0, 0, 1}},
      {"TxBiasLowAlarm", {1, 1, 1, 0}},
      {"TxBiasLowWarn", {1, 0, 0, 1}},
  };
  tests.verifyLaneInterrupts(laneInterrupts, xcvr->numMediaLanes());
  tests.verifyGlobalInterrupts("temp", 1, 1, 0, 1);
  tests.verifyGlobalInterrupts("vcc", 1, 0, 1, 0);

  auto diagsCap = transceiverManager_->getDiagsCapability(xcvrID);
  EXPECT_TRUE(diagsCap.has_value());
  std::vector<prbs::PrbsPolynomial> expectedPolynomials = {
      prbs::PrbsPolynomial::PRBS31Q,
      prbs::PrbsPolynomial::PRBS23Q,
      prbs::PrbsPolynomial::PRBS15Q,
      prbs::PrbsPolynomial::PRBS13Q,
      prbs::PrbsPolynomial::PRBS9Q,
      prbs::PrbsPolynomial::PRBS7Q};

  auto linePrbsCapability = *(*diagsCap).prbsLineCapabilities();
  auto sysPrbsCapability = *(*diagsCap).prbsSystemCapabilities();
  tests.verifyPrbsPolynomials(expectedPolynomials, linePrbsCapability);
  tests.verifyPrbsPolynomials(expectedPolynomials, sysPrbsCapability);

  for (auto unsupportedApplication :
       {SMFMediaInterfaceCode::LR4_10_400G, SMFMediaInterfaceCode::FR4_400G}) {
    EXPECT_EQ(
        xcvr->getApplicationField(
            static_cast<uint8_t>(unsupportedApplication), 0),
        std::nullopt);
  }
  for (auto supportedApplication :
       {SMFMediaInterfaceCode::FR4_200G, SMFMediaInterfaceCode::CWDM4_100G}) {
    auto applicationField = xcvr->getApplicationField(
        static_cast<uint8_t>(supportedApplication), 0);
    EXPECT_NE(applicationField, std::nullopt);
    EXPECT_EQ(applicationField->hostStartLanes, std::unordered_set<int>{0});
    for (uint8_t lane = 1; lane < 7; lane++) {
      EXPECT_EQ(
          xcvr->getApplicationField(
              static_cast<uint8_t>(supportedApplication), lane),
          std::nullopt);
    }
  }
}

TEST_F(CmisTest, cmis400GLr4TransceiverInfoTest) {
  auto xcvrID = TransceiverID(1);
  auto xcvr = overrideCmisModule<Cmis400GLr4Transceiver>(xcvrID);
  const auto& info = xcvr->getTransceiverInfo();
  EXPECT_TRUE(info.transceiverManagementInterface());
  EXPECT_EQ(
      info.transceiverManagementInterface(),
      TransceiverManagementInterface::CMIS);
  EXPECT_EQ(xcvr->numHostLanes(), 8);
  EXPECT_EQ(xcvr->numMediaLanes(), 4);
  EXPECT_EQ(info.moduleMediaInterface(), MediaInterfaceCode::LR4_400G_10KM);
  for (auto& media : *info.settings()->mediaInterface()) {
    EXPECT_EQ(media.media()->get_smfCode(), SMFMediaInterfaceCode::LR4_10_400G);
    EXPECT_EQ(media.code(), MediaInterfaceCode::LR4_400G_10KM);
  }
  // Check cmisStateChanged
  EXPECT_TRUE(
      info.status() && info.status()->cmisStateChanged() &&
      *info.status()->cmisStateChanged());

  utility::HwTransceiverUtils::verifyDiagsCapability(
      *info.tcvrState(),
      transceiverManager_->getDiagsCapability(xcvrID),
      false /* skipCheckingIndividualCapability */);

  // Verify update qsfp data logic
  if (auto status = info.status(); status && status->cmisModuleState()) {
    EXPECT_EQ(*status->cmisModuleState(), CmisModuleState::READY);
    auto cmisData = xcvr->getDOMDataUnion().get_cmis();
    // NOTE the following cmis data are specifically set to 0x11 in
    // FakeTransceiverImpl
    EXPECT_TRUE(cmisData.page14());
    // SNR will be set
    EXPECT_EQ(cmisData.page14()->data()[0], 0x06);
    // Check VDM cache
    EXPECT_TRUE(xcvr->isVdmSupported());
    EXPECT_TRUE(cmisData.page20());
    EXPECT_EQ(cmisData.page20()->data()[0], 0x00);
    EXPECT_TRUE(cmisData.page21());
    EXPECT_EQ(cmisData.page21()->data()[0], 0x00);
    EXPECT_TRUE(cmisData.page24());
    EXPECT_EQ(cmisData.page24()->data()[0], 0x15);
    EXPECT_TRUE(cmisData.page25());
    EXPECT_EQ(cmisData.page25()->data()[0], 0x00);
    EXPECT_EQ(
        int(info.vdmDiagsStats().value().errFrameMediaMin().value() * 10e9),
        736);
    EXPECT_EQ(
        int(info.vdmDiagsStats().value().errFrameMediaMax().value() * 10e9),
        743);
    EXPECT_EQ(
        int(info.vdmDiagsStats().value().errFrameMediaAvg().value() * 10e9),
        738);
    EXPECT_EQ(
        int(info.vdmDiagsStats().value().errFrameMediaCur().value() * 10e9),
        741);
    EXPECT_EQ(
        int(info.vdmDiagsStats().value().errFrameHostMin().value() * 10e10),
        597);
    EXPECT_EQ(
        int(info.vdmDiagsStats().value().errFrameHostMax().value() * 10e10),
        598);
    EXPECT_EQ(
        int(info.vdmDiagsStats().value().errFrameHostAvg().value() * 10e8), 6);
    EXPECT_EQ(
        int(info.vdmDiagsStats().value().errFrameHostCur().value() * 10e10),
        601);
  } else {
    throw FbossError("Missing CMIS Module state");
  }

  TransceiverTestsHelper tests(info);
  tests.verifyVendorName("FACETEST");

  auto diagsCap = transceiverManager_->getDiagsCapability(xcvrID);
  EXPECT_TRUE(diagsCap.has_value());
  std::vector<prbs::PrbsPolynomial> expectedSysPolynomials = {
      prbs::PrbsPolynomial::PRBS31Q,
      prbs::PrbsPolynomial::PRBS23Q,
      prbs::PrbsPolynomial::PRBS15Q,
      prbs::PrbsPolynomial::PRBS9Q,
      prbs::PrbsPolynomial::PRBS7Q};
  std::vector<prbs::PrbsPolynomial> expectedLinePolynomials = {
      prbs::PrbsPolynomial::PRBS31Q,
      prbs::PrbsPolynomial::PRBS23Q,
      prbs::PrbsPolynomial::PRBS9Q,
      prbs::PrbsPolynomial::PRBS7Q};

  auto linePrbsCapability = *(*diagsCap).prbsLineCapabilities();
  auto sysPrbsCapability = *(*diagsCap).prbsSystemCapabilities();

  tests.verifyPrbsPolynomials(expectedLinePolynomials, linePrbsCapability);
  tests.verifyPrbsPolynomials(expectedSysPolynomials, sysPrbsCapability);

  for (auto unsupportedApplication :
       {SMFMediaInterfaceCode::CWDM4_100G, SMFMediaInterfaceCode::FR4_400G}) {
    EXPECT_EQ(
        xcvr->getApplicationField(
            static_cast<uint8_t>(unsupportedApplication), 0),
        std::nullopt);
  }
  for (auto supportedApplication :
       {SMFMediaInterfaceCode::LR4_10_400G, SMFMediaInterfaceCode::FR4_200G}) {
    auto applicationField = xcvr->getApplicationField(
        static_cast<uint8_t>(supportedApplication), 0);
    EXPECT_NE(applicationField, std::nullopt);
    EXPECT_EQ(applicationField->hostStartLanes, std::unordered_set<int>{0});
    for (uint8_t lane = 1; lane < 7; lane++) {
      EXPECT_EQ(
          xcvr->getApplicationField(
              static_cast<uint8_t>(supportedApplication), lane),
          std::nullopt);
    }
  }
}

TEST_F(CmisTest, flatMemTransceiverInfoTest) {
  auto xcvr = overrideCmisModule<CmisFlatMemTransceiver>(TransceiverID(1));
  const auto& info = xcvr->getTransceiverInfo();
  EXPECT_TRUE(info.transceiverManagementInterface());
  EXPECT_EQ(
      info.transceiverManagementInterface(),
      TransceiverManagementInterface::CMIS);
  EXPECT_EQ(xcvr->numHostLanes(), 4);
  EXPECT_EQ(xcvr->numMediaLanes(), 0);

  TransceiverTestsHelper tests(info);
  tests.verifyVendorName("FACETEST");
}

TEST_F(CmisTest, moduleEepromChecksumTest) {
  // Create CMIS 200G FR4 module
  auto xcvrCmis200GFr4 =
      overrideCmisModule<Cmis200GTransceiver>(TransceiverID(1));
  // Verify EEPROM checksum for CMIS 200G FR4 module
  bool csumValid = xcvrCmis200GFr4->verifyEepromChecksums();
  EXPECT_TRUE(csumValid);

  // Create CMIS 400G LR4 module
  auto xcvrCmis400GLr4 =
      overrideCmisModule<Cmis400GLr4Transceiver>(TransceiverID(2));
  // Verify EEPROM checksum for CMIS 400G LR4 module
  csumValid = xcvrCmis400GLr4->verifyEepromChecksums();
  EXPECT_TRUE(csumValid);

  // Create CMIS 200G FR4 Bad module
  auto xcvrCmis200GFr4Bad =
      overrideCmisModule<BadCmis200GTransceiver>(TransceiverID(3));
  // Verify EEPROM checksum Invalid for CMIS 200G FR4 Bad module
  csumValid = xcvrCmis200GFr4Bad->verifyEepromChecksums();
  EXPECT_FALSE(csumValid);
}

TEST_F(CmisTest, cmis400GCr8TransceiverInfoTest) {
  auto xcvrID = TransceiverID(1);
  auto xcvr = overrideCmisModule<Cmis400GCr8Transceiver>(xcvrID);
  const auto& info = xcvr->getTransceiverInfo();
  EXPECT_TRUE(info.transceiverManagementInterface());
  EXPECT_EQ(
      info.transceiverManagementInterface(),
      TransceiverManagementInterface::CMIS);
  EXPECT_EQ(xcvr->numHostLanes(), 8);
  EXPECT_EQ(xcvr->numMediaLanes(), 8);
  EXPECT_EQ(info.moduleMediaInterface(), MediaInterfaceCode::CR8_400G);
  for (auto& media : *info.settings()->mediaInterface()) {
    EXPECT_EQ(
        media.media()->get_passiveCuCode(),
        PassiveCuMediaInterfaceCode::COPPER);
    EXPECT_EQ(media.code(), MediaInterfaceCode::CR8_400G);
  }
  // Check cmisStateChanged
  EXPECT_TRUE(
      info.status() && info.status()->cmisStateChanged() &&
      *info.status()->cmisStateChanged());

  // Verify update qsfp data logic
  if (auto status = info.status(); status && status->cmisModuleState()) {
    EXPECT_EQ(*status->cmisModuleState(), CmisModuleState::READY);
  } else {
    throw FbossError("Missing CMIS Module state");
  }

  TransceiverTestsHelper tests(info);
  tests.verifyVendorName("FACETEST");

  auto diagsCap = transceiverManager_->getDiagsCapability(xcvrID);
  EXPECT_FALSE(diagsCap.has_value());

  for (auto unsupportedApplication :
       {SMFMediaInterfaceCode::CWDM4_100G,
        SMFMediaInterfaceCode::FR4_200G,
        SMFMediaInterfaceCode::FR4_400G,
        SMFMediaInterfaceCode::LR4_10_400G}) {
    EXPECT_EQ(
        xcvr->getApplicationField(
            static_cast<uint8_t>(unsupportedApplication), 0),
        std::nullopt);
  }
  for (auto supportedApplication : {PassiveCuMediaInterfaceCode::COPPER}) {
    EXPECT_NE(
        xcvr->getApplicationField(
            static_cast<uint8_t>(supportedApplication), 0),
        std::nullopt);
    auto applicationField = xcvr->getApplicationField(
        static_cast<uint8_t>(supportedApplication), 0);
    EXPECT_NE(applicationField, std::nullopt);
    EXPECT_EQ(applicationField->hostStartLanes, std::unordered_set<int>{0});
    for (uint8_t lane = 1; lane < 7; lane++) {
      EXPECT_EQ(
          xcvr->getApplicationField(
              static_cast<uint8_t>(supportedApplication), lane),
          std::nullopt);
    }
  }
}
} // namespace facebook::fboss
