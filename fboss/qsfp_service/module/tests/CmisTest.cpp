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

#include "fboss/qsfp_service/module/cmis/CmisHelper.h"
#include "fboss/qsfp_service/module/cmis/CmisModule.h"
#include "fboss/qsfp_service/module/tests/FakeTransceiverImpl.h"
#include "fboss/qsfp_service/module/tests/TransceiverTestsHelper.h"
#include "fboss/qsfp_service/test/hw_test/HwTransceiverUtils.h"

namespace facebook::fboss {

class MockCmisModule : public CmisModule {
 public:
  template <typename XcvrImplT>
  explicit MockCmisModule(
      std::set<std::string> portNames,
      XcvrImplT* qsfpImpl,
      std::shared_ptr<const TransceiverConfig> cfgPtr,
      std::string tcvrName = "")
      : CmisModule(
            std::move(portNames),
            qsfpImpl,
            cfgPtr,
            true /*supportRemediate*/,
            tcvrName) {
    ON_CALL(*this, getModuleStateChanged).WillByDefault([this]() {
      // Only return true for the first read so that we can mimic the clear
      // on read register
      return (++moduleStateChangedReadTimes_) == 1;
    });
    ON_CALL(*this, ensureTransceiverReadyLocked(testing::_))
        .WillByDefault(testing::Return(true));
  }

  MOCK_METHOD0(getModuleStateChanged, bool());
  MOCK_METHOD1(ensureTransceiverReadyLocked, bool(bool));

  using CmisModule::frequencyGridToGridSelection;
  using CmisModule::getApplicationField;
  using CmisModule::getChannelNumFromFrequency;
  using CmisModule::getCurrentAppSelCode;
  using CmisModule::getInterfaceCodeForAppSel;
  using CmisModule::getTunableLaserStatus;
  using CmisModule::isTunableOptics;

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
    qsfpImpls_.push_back(
        std::make_unique<XcvrImplT>(id, transceiverManager_.get()));
    // This override function use ids starting from 1
    transceiverManager_->overrideMgmtInterface(
        static_cast<int>(id) + 1, uint8_t(identifier));
    auto xcvr = static_cast<MockCmisModule*>(
        transceiverManager_->overrideTransceiverForTesting(
            id,
            std::make_unique<MockCmisModule>(
                transceiverManager_->getPortNames(id),
                qsfpImpls_.back().get(),
                tcvrConfig_,
                transceiverManager_->getTransceiverName(id))));

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
  EXPECT_TRUE(info.tcvrState()->transceiverManagementInterface());
  EXPECT_EQ(
      info.tcvrState()->transceiverManagementInterface(),
      TransceiverManagementInterface::CMIS);
  EXPECT_EQ(xcvr->numHostLanes(), 4);
  EXPECT_EQ(xcvr->numMediaLanes(), 4);
  EXPECT_EQ(
      info.tcvrState()->moduleMediaInterface(), MediaInterfaceCode::FR4_200G);
  EXPECT_EQ(
      xcvr->numMediaLanes(),
      info.tcvrState()->settings()->mediaInterface().value_or({}).size());
  for (auto& media : *info.tcvrState()->settings()->mediaInterface()) {
    EXPECT_EQ(media.media()->get_smfCode(), SMFMediaInterfaceCode::FR4_200G);
    EXPECT_EQ(media.code(), MediaInterfaceCode::FR4_200G);
  }
  testCachedMediaSignals(xcvr);
  // Check cmisStateChanged
  EXPECT_TRUE(
      info.tcvrState()->status() &&
      info.tcvrState()->status()->cmisStateChanged() &&
      *info.tcvrState()->status()->cmisStateChanged());

  utility::HwTransceiverUtils::verifyDiagsCapability(
      *info.tcvrState(),
      transceiverManager_->getDiagsCapability(xcvrID),
      false /* skipCheckingIndividualCapability */);

  // Verify update qsfp data logic
  if (auto status = info.tcvrState()->status();
      status && status->cmisModuleState()) {
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

  EXPECT_EQ(
      xcvr->numHostLanes(),
      info.tcvrState()->hostLaneSignals().value_or({}).size());
  for (auto& signal : *info.tcvrState()->hostLaneSignals()) {
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

  auto settings = info.tcvrState()->settings().value_or({});
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
    EXPECT_EQ(applicationField->hostStartLanes, std::vector<int>{0});
    EXPECT_EQ(applicationField->mediaStartLanes, std::vector<int>{0});
    for (uint8_t lane = 1; lane < 7; lane++) {
      EXPECT_EQ(
          xcvr->getApplicationField(
              static_cast<uint8_t>(supportedApplication), lane),
          std::nullopt);
    }
  }

  EXPECT_FALSE(diagsCap.value().vdm().value());
  EXPECT_TRUE(diagsCap.value().cdb().value());
  EXPECT_TRUE(diagsCap.value().prbsLine().value());
  EXPECT_TRUE(diagsCap.value().prbsSystem().value());
  EXPECT_TRUE(diagsCap.value().loopbackLine().value());
  EXPECT_TRUE(diagsCap.value().loopbackSystem().value());
  EXPECT_TRUE(diagsCap.value().txOutputControl().value());
  EXPECT_TRUE(diagsCap.value().rxOutputControl().value());
  EXPECT_FALSE(diagsCap.value().snrLine().value());
  EXPECT_FALSE(diagsCap.value().snrSystem().value());
  EXPECT_FALSE(xcvr->isVdmSupported());
  EXPECT_TRUE(xcvr->isPrbsSupported(phy::Side::LINE));
  EXPECT_TRUE(xcvr->isPrbsSupported(phy::Side::SYSTEM));
  EXPECT_FALSE(xcvr->isSnrSupported(phy::Side::LINE));
  EXPECT_FALSE(xcvr->isSnrSupported(phy::Side::SYSTEM));

  TransceiverPortState goodPortState1{
      "", 0, cfg::PortSpeed::TWOHUNDREDG, 4, TransmitterTechnology::OPTICAL};
  TransceiverPortState goodPortState2{
      "", 0, cfg::PortSpeed::HUNDREDG, 4, TransmitterTechnology::OPTICAL};
  for (auto portState : {goodPortState1, goodPortState2}) {
    EXPECT_TRUE(xcvr->tcvrPortStateSupported(portState));
  }

  TransceiverPortState badPortState1{
      "",
      0,
      cfg::PortSpeed::HUNDREDG,
      4,
      TransmitterTechnology::COPPER}; // Copper not supported
  TransceiverPortState badPortState2{
      "",
      0,
      cfg::PortSpeed::FOURHUNDREDG,
      8,
      TransmitterTechnology::OPTICAL}; // 400G not supported
  TransceiverPortState badPortState3{
      "",
      1,
      cfg::PortSpeed::HUNDREDG,
      4,
      TransmitterTechnology::OPTICAL}; // BAD START LANE
  for (auto portState : {badPortState1, badPortState2, badPortState3}) {
    EXPECT_FALSE(xcvr->tcvrPortStateSupported(portState));
  }
  EXPECT_TRUE(info.tcvrState()->errorStates()->empty());
}

TEST_F(CmisTest, cmis400GLr4TransceiverInfoTest) {
  auto xcvrID = TransceiverID(1);
  auto xcvr = overrideCmisModule<Cmis400GLr4Transceiver>(xcvrID);
  const auto& info = xcvr->getTransceiverInfo();
  EXPECT_TRUE(info.tcvrState()->transceiverManagementInterface());
  EXPECT_EQ(
      info.tcvrState()->transceiverManagementInterface(),
      TransceiverManagementInterface::CMIS);
  EXPECT_EQ(xcvr->numHostLanes(), 8);
  EXPECT_EQ(xcvr->numMediaLanes(), 4);
  EXPECT_EQ(
      info.tcvrState()->moduleMediaInterface(),
      MediaInterfaceCode::LR4_400G_10KM);
  for (auto& media : *info.tcvrState()->settings()->mediaInterface()) {
    EXPECT_EQ(media.media()->get_smfCode(), SMFMediaInterfaceCode::LR4_10_400G);
    EXPECT_EQ(media.code(), MediaInterfaceCode::LR4_400G_10KM);
  }
  // Check cmisStateChanged
  EXPECT_TRUE(
      info.tcvrState()->status() &&
      info.tcvrState()->status()->cmisStateChanged() &&
      *info.tcvrState()->status()->cmisStateChanged());

  utility::HwTransceiverUtils::verifyDiagsCapability(
      *info.tcvrState(),
      transceiverManager_->getDiagsCapability(xcvrID),
      false /* skipCheckingIndividualCapability */);

  // Verify update qsfp data logic
  if (auto status = info.tcvrState()->status();
      status && status->cmisModuleState()) {
    EXPECT_EQ(*status->cmisModuleState(), CmisModuleState::READY);
    auto cmisData = xcvr->getDOMDataUnion().get_cmis();
    // NOTE the following cmis data are specifically set to 0x11 in
    // FakeTransceiverImpl
    EXPECT_TRUE(cmisData.page14());
    // SNR will be set
    EXPECT_EQ(cmisData.page14()->data()[0], 0x06);
    // Check VDM cache
    EXPECT_TRUE(xcvr->isVdmSupported());
    EXPECT_FALSE(xcvr->isVdmSupported(3));
    EXPECT_TRUE(cmisData.page20());
    EXPECT_EQ(cmisData.page20()->data()[0], 0x00);
    EXPECT_TRUE(cmisData.page21());
    EXPECT_EQ(cmisData.page21()->data()[0], 0x00);
    EXPECT_TRUE(cmisData.page24());
    EXPECT_EQ(cmisData.page24()->data()[0], 0x15);
    EXPECT_TRUE(cmisData.page25());
    EXPECT_EQ(cmisData.page25()->data()[0], 0x00);
    EXPECT_EQ(
        int(info.tcvrStats()
                ->vdmDiagsStats()
                .value()
                .errFrameMediaMin()
                .value() *
            10e9),
        736);
    EXPECT_EQ(
        int(info.tcvrStats()
                ->vdmDiagsStats()
                .value()
                .errFrameMediaMax()
                .value() *
            10e9),
        743);
    EXPECT_EQ(
        int(info.tcvrStats()
                ->vdmDiagsStats()
                .value()
                .errFrameMediaAvg()
                .value() *
            10e9),
        738);
    EXPECT_EQ(
        int(info.tcvrStats()
                ->vdmDiagsStats()
                .value()
                .errFrameMediaCur()
                .value() *
            10e9),
        741);
    EXPECT_EQ(
        int(info.tcvrStats()
                ->vdmDiagsStats()
                .value()
                .errFrameHostMin()
                .value() *
            10e10),
        597);
    EXPECT_EQ(
        int(info.tcvrStats()
                ->vdmDiagsStats()
                .value()
                .errFrameHostMax()
                .value() *
            10e10),
        598);
    EXPECT_EQ(
        int(info.tcvrStats()
                ->vdmDiagsStats()
                .value()
                .errFrameHostAvg()
                .value() *
            10e8),
        6);
    EXPECT_EQ(
        int(info.tcvrStats()
                ->vdmDiagsStats()
                .value()
                .errFrameHostCur()
                .value() *
            10e10),
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
    EXPECT_EQ(applicationField->hostStartLanes, std::vector<int>{0});
    EXPECT_EQ(applicationField->mediaStartLanes, std::vector<int>{0});
    for (uint8_t lane = 1; lane < 7; lane++) {
      EXPECT_EQ(
          xcvr->getApplicationField(
              static_cast<uint8_t>(supportedApplication), lane),
          std::nullopt);
    }
  }

  TransceiverPortState goodPortState1{
      "", 0, cfg::PortSpeed::TWOHUNDREDG, 4, TransmitterTechnology::OPTICAL};
  TransceiverPortState goodPortState2{
      "", 0, cfg::PortSpeed::FOURHUNDREDG, 8, TransmitterTechnology::OPTICAL};
  for (auto portState : {goodPortState1, goodPortState2}) {
    EXPECT_TRUE(xcvr->tcvrPortStateSupported(portState));
  }

  TransceiverPortState badPortState1{
      "",
      0,
      cfg::PortSpeed::HUNDREDG,
      4,
      TransmitterTechnology::COPPER}; // Copper not supported
  TransceiverPortState badPortState2{
      "",
      0,
      cfg::PortSpeed::HUNDREDG,
      4,
      TransmitterTechnology::OPTICAL}; // 100G not supported
  TransceiverPortState badPortState3{
      "",
      1,
      cfg::PortSpeed::HUNDREDG,
      4,
      TransmitterTechnology::OPTICAL}; // BAD START LANE
  for (auto portState : {badPortState1, badPortState2, badPortState3}) {
    EXPECT_FALSE(xcvr->tcvrPortStateSupported(portState));
  }
  EXPECT_TRUE(info.tcvrState()->errorStates()->empty());
}

TEST_F(CmisTest, flatMemTransceiverInfoTest) {
  auto xcvr = overrideCmisModule<CmisFlatMemTransceiver>(TransceiverID(1));
  const auto& info = xcvr->getTransceiverInfo();
  EXPECT_TRUE(info.tcvrState()->transceiverManagementInterface());
  EXPECT_EQ(
      info.tcvrState()->transceiverManagementInterface(),
      TransceiverManagementInterface::CMIS);
  EXPECT_EQ(xcvr->numHostLanes(), 0); // Unknown MediaInterface
  EXPECT_EQ(xcvr->numMediaLanes(), 0); // Unknown MediaInterface

  TransceiverTestsHelper tests(info);
  tests.verifyVendorName("FACETEST");
  EXPECT_TRUE(info.tcvrState()->errorStates()->empty());
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
  EXPECT_TRUE(info.tcvrState()->transceiverManagementInterface());
  EXPECT_EQ(
      info.tcvrState()->transceiverManagementInterface(),
      TransceiverManagementInterface::CMIS);
  EXPECT_EQ(xcvr->numHostLanes(), 8);
  EXPECT_EQ(xcvr->numMediaLanes(), 8);
  EXPECT_EQ(
      info.tcvrState()->moduleMediaInterface(), MediaInterfaceCode::CR8_400G);
  for (auto& media : *info.tcvrState()->settings()->mediaInterface()) {
    EXPECT_EQ(
        media.media()->get_passiveCuCode(),
        PassiveCuMediaInterfaceCode::COPPER);
    EXPECT_EQ(media.code(), MediaInterfaceCode::CR8_400G);
  }
  // Check cmisStateChanged
  EXPECT_TRUE(
      info.tcvrState()->status() &&
      info.tcvrState()->status()->cmisStateChanged() &&
      *info.tcvrState()->status()->cmisStateChanged());

  // Verify update qsfp data logic
  if (auto status = info.tcvrState()->status();
      status && status->cmisModuleState()) {
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
    EXPECT_EQ(applicationField->hostStartLanes, std::vector<int>{0});
    EXPECT_EQ(applicationField->mediaStartLanes, std::vector<int>{});
    for (uint8_t lane = 1; lane < 7; lane++) {
      EXPECT_EQ(
          xcvr->getApplicationField(
              static_cast<uint8_t>(supportedApplication), lane),
          std::nullopt);
    }
  }
  EXPECT_TRUE(info.tcvrState()->errorStates()->empty());
}

TEST_F(CmisTest, cmis2x400GFr4TransceiverInfoTest) {
  auto xcvrID = TransceiverID(1);
  auto xcvr = overrideCmisModule<Cmis2x400GFr4Transceiver>(
      xcvrID, TransceiverModuleIdentifier::OSFP);
  const auto& info = xcvr->getTransceiverInfo();
  EXPECT_TRUE(info.tcvrState()->transceiverManagementInterface());
  EXPECT_EQ(
      info.tcvrState()->transceiverManagementInterface(),
      TransceiverManagementInterface::CMIS);
  EXPECT_EQ(xcvr->numHostLanes(), 8);
  EXPECT_EQ(xcvr->numMediaLanes(), 8);
  EXPECT_EQ(
      info.tcvrState()->moduleMediaInterface(), MediaInterfaceCode::FR4_2x400G);
  for (auto& media : *info.tcvrState()->settings()->mediaInterface()) {
    EXPECT_EQ(media.media()->get_smfCode(), SMFMediaInterfaceCode::FR4_400G);
    EXPECT_EQ(media.code(), MediaInterfaceCode::FR4_400G);
  }

  // Check cmisStateChanged
  EXPECT_TRUE(
      info.tcvrState()->status() &&
      info.tcvrState()->status()->cmisStateChanged() &&
      *info.tcvrState()->status()->cmisStateChanged());

  utility::HwTransceiverUtils::verifyDiagsCapability(
      *info.tcvrState(),
      transceiverManager_->getDiagsCapability(xcvrID),
      false /* skipCheckingIndividualCapability */);

  TransceiverTestsHelper tests(info);
  tests.verifyVendorName("FACETEST");

  auto diagsCap = transceiverManager_->getDiagsCapability(xcvrID);
  EXPECT_TRUE(diagsCap.has_value());
  std::vector<prbs::PrbsPolynomial> expectedSysPolynomials = {
      prbs::PrbsPolynomial::PRBS31Q,
      prbs::PrbsPolynomial::PRBS23Q,
      prbs::PrbsPolynomial::PRBS15Q,
      prbs::PrbsPolynomial::PRBS13Q,
      prbs::PrbsPolynomial::PRBS9Q,
      prbs::PrbsPolynomial::PRBS7Q,
      prbs::PrbsPolynomial::PRBS31,
      prbs::PrbsPolynomial::PRBS23,
      prbs::PrbsPolynomial::PRBS15,
      prbs::PrbsPolynomial::PRBS13,
      prbs::PrbsPolynomial::PRBS9,
      prbs::PrbsPolynomial::PRBS7};
  std::vector<prbs::PrbsPolynomial> expectedLinePolynomials = {
      prbs::PrbsPolynomial::PRBS31Q,
      prbs::PrbsPolynomial::PRBS23Q,
      prbs::PrbsPolynomial::PRBS15Q,
      prbs::PrbsPolynomial::PRBS13Q,
      prbs::PrbsPolynomial::PRBS9Q,
      prbs::PrbsPolynomial::PRBS7Q,
      prbs::PrbsPolynomial::PRBS31,
      prbs::PrbsPolynomial::PRBS23,
      prbs::PrbsPolynomial::PRBS15,
      prbs::PrbsPolynomial::PRBS13,
      prbs::PrbsPolynomial::PRBS9,
      prbs::PrbsPolynomial::PRBS7};

  auto linePrbsCapability = *(*diagsCap).prbsLineCapabilities();
  auto sysPrbsCapability = *(*diagsCap).prbsSystemCapabilities();

  tests.verifyPrbsPolynomials(expectedLinePolynomials, linePrbsCapability);
  tests.verifyPrbsPolynomials(expectedSysPolynomials, sysPrbsCapability);

  for (auto unsupportedApplication : {SMFMediaInterfaceCode::LR4_10_400G}) {
    EXPECT_EQ(
        xcvr->getApplicationField(
            static_cast<uint8_t>(unsupportedApplication), 0),
        std::nullopt);
  }

  for (auto supportedApplication :
       {SMFMediaInterfaceCode::FR4_400G,
        SMFMediaInterfaceCode::FR1_100G,
        SMFMediaInterfaceCode::FR4_200G,
        SMFMediaInterfaceCode::CWDM4_100G}) {
    auto applicationField = xcvr->getApplicationField(
        static_cast<uint8_t>(supportedApplication), 0);
    EXPECT_NE(applicationField, std::nullopt);
    std::vector<int> expectedStartLanes;
    if (supportedApplication == SMFMediaInterfaceCode::FR1_100G) {
      expectedStartLanes = {0, 1, 2, 3, 4, 5, 6, 7};
    } else {
      expectedStartLanes = {0, 4};
    }
    EXPECT_EQ(applicationField->hostStartLanes, expectedStartLanes);
    EXPECT_EQ(applicationField->mediaStartLanes, expectedStartLanes);
    for (uint8_t lane = 0; lane <= 7; lane++) {
      if (std::find(
              expectedStartLanes.begin(), expectedStartLanes.end(), lane) !=
          expectedStartLanes.end()) {
        continue;
      }
      // For lanes that are not expected to be start lanes, getApplicationField
      // should return nullopt
      EXPECT_EQ(
          xcvr->getApplicationField(
              static_cast<uint8_t>(supportedApplication), lane),
          std::nullopt);
    }
  }

  EXPECT_TRUE(diagsCap.value().vdm().value());
  EXPECT_TRUE(diagsCap.value().cdb().value());
  EXPECT_TRUE(diagsCap.value().prbsLine().value());
  EXPECT_TRUE(diagsCap.value().prbsSystem().value());
  EXPECT_TRUE(diagsCap.value().loopbackLine().value());
  EXPECT_TRUE(diagsCap.value().loopbackSystem().value());
  EXPECT_TRUE(diagsCap.value().txOutputControl().value());
  EXPECT_TRUE(diagsCap.value().rxOutputControl().value());
  EXPECT_TRUE(diagsCap.value().snrLine().value());
  EXPECT_TRUE(diagsCap.value().snrSystem().value());
  EXPECT_TRUE(xcvr->isVdmSupported());
  EXPECT_TRUE(xcvr->isVdmSupported(3));
  EXPECT_TRUE(xcvr->isPrbsSupported(phy::Side::LINE));
  EXPECT_TRUE(xcvr->isPrbsSupported(phy::Side::SYSTEM));
  EXPECT_TRUE(xcvr->isSnrSupported(phy::Side::LINE));
  EXPECT_TRUE(xcvr->isSnrSupported(phy::Side::SYSTEM));

  TransceiverPortState goodPortState1{
      "", 0, cfg::PortSpeed::TWOHUNDREDG, 4, TransmitterTechnology::OPTICAL};
  TransceiverPortState goodPortState2{
      "", 0, cfg::PortSpeed::HUNDREDG, 4, TransmitterTechnology::OPTICAL};
  TransceiverPortState goodPortState3{
      "", 0, cfg::PortSpeed::FOURHUNDREDG, 4, TransmitterTechnology::OPTICAL};
  TransceiverPortState goodPortState4{
      "", 4, cfg::PortSpeed::FOURHUNDREDG, 4, TransmitterTechnology::OPTICAL};
  for (auto portState :
       {goodPortState1, goodPortState2, goodPortState3, goodPortState4}) {
    EXPECT_TRUE(xcvr->tcvrPortStateSupported(portState));
  }

  TransceiverPortState badPortState1{
      "",
      0,
      cfg::PortSpeed::HUNDREDG,
      4,
      TransmitterTechnology::COPPER}; // Copper not supported
  TransceiverPortState badPortState2{
      "",
      0,
      cfg::PortSpeed::FORTYG,
      4,
      TransmitterTechnology::OPTICAL}; // 40G not supported
  TransceiverPortState badPortState3{
      "",
      1,
      cfg::PortSpeed::HUNDREDG,
      4,
      TransmitterTechnology::OPTICAL}; // BAD START LANE
  for (auto portState : {badPortState1, badPortState2, badPortState3}) {
    EXPECT_FALSE(xcvr->tcvrPortStateSupported(portState));
  }
  EXPECT_TRUE(info.tcvrState()->errorStates()->empty());
}

TEST_F(CmisTest, cmis2x400GFr4LiteTransceiverInfoTest) {
  auto xcvrID = TransceiverID(1);
  auto xcvr = overrideCmisModule<Cmis2x400GFr4LiteTransceiver>(
      xcvrID, TransceiverModuleIdentifier::OSFP);
  const auto& info = xcvr->getTransceiverInfo();
  EXPECT_TRUE(info.tcvrState()->transceiverManagementInterface());
  EXPECT_EQ(
      info.tcvrState()->transceiverManagementInterface(),
      TransceiverManagementInterface::CMIS);
  EXPECT_EQ(xcvr->numHostLanes(), 8);
  EXPECT_EQ(xcvr->numMediaLanes(), 8);
  EXPECT_EQ(
      info.tcvrState()->moduleMediaInterface(),
      MediaInterfaceCode::FR4_LITE_2x400G);
  for (auto& media : *info.tcvrState()->settings()->mediaInterface()) {
    EXPECT_EQ(media.media()->get_smfCode(), SMFMediaInterfaceCode::FR4_400G);
    EXPECT_EQ(media.code(), MediaInterfaceCode::FR4_400G);
  }

  // Check cmisStateChanged
  EXPECT_TRUE(
      info.tcvrState()->status() &&
      info.tcvrState()->status()->cmisStateChanged() &&
      *info.tcvrState()->status()->cmisStateChanged());

  utility::HwTransceiverUtils::verifyDiagsCapability(
      *info.tcvrState(),
      transceiverManager_->getDiagsCapability(xcvrID),
      false /* skipCheckingIndividualCapability */);

  TransceiverTestsHelper tests(info);
  tests.verifyVendorName("FACETEST");

  auto diagsCap = transceiverManager_->getDiagsCapability(xcvrID);
  EXPECT_TRUE(diagsCap.has_value());
  std::vector<prbs::PrbsPolynomial> expectedSysPolynomials = {
      prbs::PrbsPolynomial::PRBS31Q,
      prbs::PrbsPolynomial::PRBS23Q,
      prbs::PrbsPolynomial::PRBS15Q,
      prbs::PrbsPolynomial::PRBS13Q,
      prbs::PrbsPolynomial::PRBS9Q,
      prbs::PrbsPolynomial::PRBS7Q,
      prbs::PrbsPolynomial::PRBS31,
      prbs::PrbsPolynomial::PRBS23,
      prbs::PrbsPolynomial::PRBS15,
      prbs::PrbsPolynomial::PRBS13,
      prbs::PrbsPolynomial::PRBS9,
      prbs::PrbsPolynomial::PRBS7,
      prbs::PrbsPolynomial::PRBSSSPRQ};
  std::vector<prbs::PrbsPolynomial> expectedLinePolynomials = {
      prbs::PrbsPolynomial::PRBS31Q,
      prbs::PrbsPolynomial::PRBS23Q,
      prbs::PrbsPolynomial::PRBS15Q,
      prbs::PrbsPolynomial::PRBS13Q,
      prbs::PrbsPolynomial::PRBS9Q,
      prbs::PrbsPolynomial::PRBS7Q,
      prbs::PrbsPolynomial::PRBS31,
      prbs::PrbsPolynomial::PRBS23,
      prbs::PrbsPolynomial::PRBS15,
      prbs::PrbsPolynomial::PRBS13,
      prbs::PrbsPolynomial::PRBS9,
      prbs::PrbsPolynomial::PRBS7,
      prbs::PrbsPolynomial::PRBSSSPRQ};

  auto linePrbsCapability = *(*diagsCap).prbsLineCapabilities();
  auto sysPrbsCapability = *(*diagsCap).prbsSystemCapabilities();

  tests.verifyPrbsPolynomials(expectedLinePolynomials, linePrbsCapability);
  tests.verifyPrbsPolynomials(expectedSysPolynomials, sysPrbsCapability);

  for (auto unsupportedApplication : {SMFMediaInterfaceCode::LR4_10_400G}) {
    EXPECT_EQ(
        xcvr->getApplicationField(
            static_cast<uint8_t>(unsupportedApplication), 0),
        std::nullopt);
  }

  for (auto supportedApplication :
       {SMFMediaInterfaceCode::FR4_400G,
        SMFMediaInterfaceCode::FR1_100G,
        SMFMediaInterfaceCode::FR4_200G,
        SMFMediaInterfaceCode::CWDM4_100G}) {
    auto applicationField = xcvr->getApplicationField(
        static_cast<uint8_t>(supportedApplication), 0);
    EXPECT_NE(applicationField, std::nullopt);
    std::vector<int> expectedStartLanes;
    if (supportedApplication == SMFMediaInterfaceCode::FR1_100G) {
      expectedStartLanes = {0, 1, 2, 3, 4, 5, 6, 7};
    } else {
      expectedStartLanes = {0, 4};
    }
    EXPECT_EQ(applicationField->hostStartLanes, expectedStartLanes);
    EXPECT_EQ(applicationField->mediaStartLanes, expectedStartLanes);
    for (uint8_t lane = 0; lane <= 7; lane++) {
      if (std::find(
              expectedStartLanes.begin(), expectedStartLanes.end(), lane) !=
          expectedStartLanes.end()) {
        continue;
      }
      // For lanes that are not expected to be start lanes, getApplicationField
      // should return nullopt
      EXPECT_EQ(
          xcvr->getApplicationField(
              static_cast<uint8_t>(supportedApplication), lane),
          std::nullopt);
    }
  }

  EXPECT_TRUE(diagsCap.value().vdm().value());
  EXPECT_TRUE(diagsCap.value().cdb().value());
  EXPECT_TRUE(diagsCap.value().prbsLine().value());
  EXPECT_TRUE(diagsCap.value().prbsSystem().value());
  EXPECT_TRUE(diagsCap.value().loopbackLine().value());
  EXPECT_TRUE(diagsCap.value().loopbackSystem().value());
  EXPECT_TRUE(diagsCap.value().txOutputControl().value());
  EXPECT_TRUE(diagsCap.value().rxOutputControl().value());
  EXPECT_TRUE(diagsCap.value().snrLine().value());
  EXPECT_TRUE(diagsCap.value().snrSystem().value());
  EXPECT_TRUE(xcvr->isVdmSupported());
  EXPECT_TRUE(xcvr->isVdmSupported(3));
  EXPECT_TRUE(xcvr->isPrbsSupported(phy::Side::LINE));
  EXPECT_TRUE(xcvr->isPrbsSupported(phy::Side::SYSTEM));
  EXPECT_TRUE(xcvr->isSnrSupported(phy::Side::LINE));
  EXPECT_TRUE(xcvr->isSnrSupported(phy::Side::SYSTEM));

  TransceiverPortState goodPortState1{
      "", 0, cfg::PortSpeed::TWOHUNDREDG, 4, TransmitterTechnology::OPTICAL};
  TransceiverPortState goodPortState2{
      "", 0, cfg::PortSpeed::HUNDREDG, 4, TransmitterTechnology::OPTICAL};
  TransceiverPortState goodPortState3{
      "", 0, cfg::PortSpeed::FOURHUNDREDG, 4, TransmitterTechnology::OPTICAL};
  TransceiverPortState goodPortState4{
      "", 4, cfg::PortSpeed::FOURHUNDREDG, 4, TransmitterTechnology::OPTICAL};
  for (auto portState :
       {goodPortState1, goodPortState2, goodPortState3, goodPortState4}) {
    EXPECT_TRUE(xcvr->tcvrPortStateSupported(portState));
  }

  TransceiverPortState badPortState1{
      "",
      0,
      cfg::PortSpeed::HUNDREDG,
      4,
      TransmitterTechnology::COPPER}; // Copper not supported
  TransceiverPortState badPortState2{
      "",
      0,
      cfg::PortSpeed::FORTYG,
      4,
      TransmitterTechnology::OPTICAL}; // 40G not supported
  TransceiverPortState badPortState3{
      "",
      1,
      cfg::PortSpeed::HUNDREDG,
      4,
      TransmitterTechnology::OPTICAL}; // BAD START LANE
  for (auto portState : {badPortState1, badPortState2, badPortState3}) {
    EXPECT_FALSE(xcvr->tcvrPortStateSupported(portState));
  }
  EXPECT_TRUE(info.tcvrState()->errorStates()->empty());
}

TEST_F(CmisTest, cmis2x400GFr4LpoTransceiverInfoTest) {
  auto xcvrID = TransceiverID(1);
  auto xcvr = overrideCmisModule<Cmis2x400GFr4LpoTransceiver>(
      xcvrID, TransceiverModuleIdentifier::OSFP);
  const auto& info = xcvr->getTransceiverInfo();
  EXPECT_TRUE(info.tcvrState()->transceiverManagementInterface());
  EXPECT_EQ(
      info.tcvrState()->transceiverManagementInterface(),
      TransceiverManagementInterface::CMIS);
  EXPECT_EQ(xcvr->numHostLanes(), 8);
  EXPECT_EQ(xcvr->numMediaLanes(), 8);
  EXPECT_EQ(
      info.tcvrState()->moduleMediaInterface(),
      MediaInterfaceCode::FR4_LPO_2x400G);
  for (auto& media : *info.tcvrState()->settings()->mediaInterface()) {
    EXPECT_EQ(media.media()->get_smfCode(), SMFMediaInterfaceCode::FR4_400G);
    EXPECT_EQ(media.code(), MediaInterfaceCode::FR4_400G);
  }

  // Check cmisStateChanged
  EXPECT_TRUE(
      info.tcvrState()->status() &&
      info.tcvrState()->status()->cmisStateChanged() &&
      *info.tcvrState()->status()->cmisStateChanged());

  std::optional<DiagsCapability> diagsCapability =
      transceiverManager_->getDiagsCapability(xcvrID);

  // LPO Has limited diags capability, pass skipCheckingIndividualCapability as
  // true.
  utility::HwTransceiverUtils::verifyDiagsCapability(
      *info.tcvrState(),
      diagsCapability,
      true /* skipCheckingIndividualCapability */);

  // Diags Capabilities available in LPO Modules
  EXPECT_TRUE(*diagsCapability->cdb());
  EXPECT_TRUE(*diagsCapability->txOutputControl());

  TransceiverTestsHelper tests(info);
  tests.verifyVendorName("FACETESTLPO");

  auto diagsCap = transceiverManager_->getDiagsCapability(xcvrID);
  EXPECT_TRUE(diagsCap.has_value());
  std::vector<prbs::PrbsPolynomial> expectedSysPolynomials = {};
  std::vector<prbs::PrbsPolynomial> expectedLinePolynomials = {};

  auto linePrbsCapability = *(*diagsCap).prbsLineCapabilities();
  auto sysPrbsCapability = *(*diagsCap).prbsSystemCapabilities();

  tests.verifyPrbsPolynomials(expectedLinePolynomials, linePrbsCapability);
  tests.verifyPrbsPolynomials(expectedSysPolynomials, sysPrbsCapability);

  for (auto unsupportedApplication : {SMFMediaInterfaceCode::LR4_10_400G}) {
    EXPECT_EQ(
        xcvr->getApplicationField(
            static_cast<uint8_t>(unsupportedApplication), 0),
        std::nullopt);
  }

  // TODO T230016502: Add SMFMediaInterfaceCode::FR8_800G once we have new
  // EEPROM and modules supporting 800G ?
  for (auto supportedApplication :
       {SMFMediaInterfaceCode::FR4_400G, SMFMediaInterfaceCode::FR1_100G}) {
    auto applicationField = xcvr->getApplicationField(
        static_cast<uint8_t>(supportedApplication), 0);
    EXPECT_NE(applicationField, std::nullopt);
    std::vector<int> expectedStartLanes;
    switch (supportedApplication) {
      case SMFMediaInterfaceCode::FR8_800G:
        expectedStartLanes = {0};
        break;
      case SMFMediaInterfaceCode::FR4_400G:
        expectedStartLanes = {0, 4};
        break;
      case SMFMediaInterfaceCode::FR1_100G:
        expectedStartLanes = {0, 1, 2, 3, 4, 5, 6, 7};
        break;
      default:
        throw FbossError(
            "Unhandled application ",
            apache::thrift::util::enumNameSafe(supportedApplication));
    }
    EXPECT_EQ(applicationField->hostStartLanes, expectedStartLanes);
    EXPECT_EQ(applicationField->mediaStartLanes, expectedStartLanes);
    for (uint8_t lane = 0; lane <= 7; lane++) {
      if (std::find(
              expectedStartLanes.begin(), expectedStartLanes.end(), lane) !=
          expectedStartLanes.end()) {
        continue;
      }
      // For lanes that are not expected to be start lanes, getApplicationField
      // should return nullopt
      EXPECT_EQ(
          xcvr->getApplicationField(
              static_cast<uint8_t>(supportedApplication), lane),
          std::nullopt);
    }
  }

  EXPECT_TRUE(diagsCap.value().cdb().value());
  EXPECT_TRUE(diagsCap.value().txOutputControl().value());

  // 8x100G
  TransceiverPortState goodPortState1{
      "", 0, cfg::PortSpeed::HUNDREDG, 1, TransmitterTechnology::OPTICAL};
  TransceiverPortState goodPortState2{
      "", 1, cfg::PortSpeed::HUNDREDG, 1, TransmitterTechnology::OPTICAL};
  TransceiverPortState goodPortState3{
      "", 2, cfg::PortSpeed::HUNDREDG, 1, TransmitterTechnology::OPTICAL};
  TransceiverPortState goodPortState4{
      "", 3, cfg::PortSpeed::HUNDREDG, 1, TransmitterTechnology::OPTICAL};
  TransceiverPortState goodPortState5{
      "", 4, cfg::PortSpeed::HUNDREDG, 1, TransmitterTechnology::OPTICAL};
  TransceiverPortState goodPortState6{
      "", 5, cfg::PortSpeed::HUNDREDG, 1, TransmitterTechnology::OPTICAL};
  TransceiverPortState goodPortState7{
      "", 6, cfg::PortSpeed::HUNDREDG, 1, TransmitterTechnology::OPTICAL};
  TransceiverPortState goodPortState8{
      "", 7, cfg::PortSpeed::HUNDREDG, 1, TransmitterTechnology::OPTICAL};
  // 2x400G
  TransceiverPortState goodPortState9{
      "", 0, cfg::PortSpeed::FOURHUNDREDG, 4, TransmitterTechnology::OPTICAL};
  TransceiverPortState goodPortState10{
      "", 4, cfg::PortSpeed::FOURHUNDREDG, 4, TransmitterTechnology::OPTICAL};
  for (auto portState : {
           goodPortState1,
           goodPortState2,
           goodPortState3,
           goodPortState4,
           goodPortState5,
           goodPortState6,
           goodPortState7,
           goodPortState8,
           goodPortState9,
           goodPortState10,
       }) {
    EXPECT_TRUE(xcvr->tcvrPortStateSupported(portState));
  }

  TransceiverPortState badPortState1{
      "",
      0,
      cfg::PortSpeed::HUNDREDG,
      4,
      TransmitterTechnology::COPPER}; // Copper not supported
  TransceiverPortState badPortState2{
      "",
      0,
      cfg::PortSpeed::FORTYG,
      4,
      TransmitterTechnology::OPTICAL}; // 40G not supported
  TransceiverPortState badPortState3{
      "",
      1,
      cfg::PortSpeed::HUNDREDG,
      4,
      TransmitterTechnology::OPTICAL}; // BAD START LANE
  for (auto portState : {badPortState1, badPortState2, badPortState3}) {
    EXPECT_FALSE(xcvr->tcvrPortStateSupported(portState));
  }
  EXPECT_TRUE(info.tcvrState()->errorStates()->empty());
}

// TODO: T232388340 Further enhancement for full EEPROM support.
TEST_F(CmisTest, cmis800GZrTransceiverInfoTest) {
  auto xcvrID = TransceiverID(1);
  auto xcvr = overrideCmisModule<Cmis800GZrTransceiver>(
      xcvrID, TransceiverModuleIdentifier::OSFP);
  const auto& info = xcvr->getTransceiverInfo();
  EXPECT_TRUE(info.tcvrState()->transceiverManagementInterface());
  EXPECT_EQ(
      info.tcvrState()->transceiverManagementInterface(),
      TransceiverManagementInterface::CMIS);
  EXPECT_EQ(xcvr->numHostLanes(), 8);
  EXPECT_EQ(xcvr->numMediaLanes(), 1);
  EXPECT_EQ(
      info.tcvrState()->moduleMediaInterface(), MediaInterfaceCode::ZR_800G);
  for (auto& media : *info.tcvrState()->settings()->mediaInterface()) {
    EXPECT_EQ(
        media.media()->get_smfCode(),
        SMFMediaInterfaceCode::ZR_OROADM_FLEXO_8E_DPO_800G);
    EXPECT_EQ(media.code(), MediaInterfaceCode::ZR_800G);
  }

  // Check cmisStateChanged
  EXPECT_TRUE(
      info.tcvrState()->status() &&
      info.tcvrState()->status()->cmisStateChanged() &&
      *info.tcvrState()->status()->cmisStateChanged());

  std::optional<DiagsCapability> diagsCapability =
      transceiverManager_->getDiagsCapability(xcvrID);

  utility::HwTransceiverUtils::verifyDiagsCapability(
      *info.tcvrState(), diagsCapability, false);

  EXPECT_TRUE(*diagsCapability->cdb());
  EXPECT_TRUE(*diagsCapability->txOutputControl());

  TransceiverTestsHelper tests(info);
  tests.verifyVendorName("METAZR");

  auto diagsCap = transceiverManager_->getDiagsCapability(xcvrID);
  EXPECT_TRUE(diagsCap.has_value());
  std::vector<prbs::PrbsPolynomial> expectedSysPolynomials = {
      prbs::PrbsPolynomial::PRBS31Q,
      prbs::PrbsPolynomial::PRBS23Q,
      prbs::PrbsPolynomial::PRBS15Q,
      prbs::PrbsPolynomial::PRBS13Q,
      prbs::PrbsPolynomial::PRBS9Q,
      prbs::PrbsPolynomial::PRBS7Q,
      prbs::PrbsPolynomial::PRBS31,
      prbs::PrbsPolynomial::PRBS23,
      prbs::PrbsPolynomial::PRBS15,
      prbs::PrbsPolynomial::PRBS13,
      prbs::PrbsPolynomial::PRBS9,
      prbs::PrbsPolynomial::PRBS7};
  std::vector<prbs::PrbsPolynomial> expectedLinePolynomials = {
      prbs::PrbsPolynomial::PRBS31,
      prbs::PrbsPolynomial::PRBS23,
      prbs::PrbsPolynomial::PRBS15,
      prbs::PrbsPolynomial::PRBS7};

  auto linePrbsCapability = *(*diagsCap).prbsLineCapabilities();
  auto sysPrbsCapability = *(*diagsCap).prbsSystemCapabilities();

  tests.verifyPrbsPolynomials(expectedLinePolynomials, linePrbsCapability);
  tests.verifyPrbsPolynomials(expectedSysPolynomials, sysPrbsCapability);

  for (auto unsupportedApplication : {SMFMediaInterfaceCode::LR4_10_400G}) {
    EXPECT_EQ(
        xcvr->getApplicationField(
            static_cast<uint8_t>(unsupportedApplication), 0),
        std::nullopt);
  }

  for (auto supportedApplication :
       {SMFMediaInterfaceCode::ZR_OROADM_FLEXO_8E_DPO_800G}) {
    auto applicationField = xcvr->getApplicationField(
        static_cast<uint8_t>(supportedApplication), 0);
    EXPECT_NE(applicationField, std::nullopt);
    std::vector<int> expectedStartLanes;
    switch (supportedApplication) {
      case SMFMediaInterfaceCode::ZR_OROADM_FLEXO_8E_DPO_800G:
        expectedStartLanes = {0};
        break;
      default:
        throw FbossError(
            "Unhandled application ",
            apache::thrift::util::enumNameSafe(supportedApplication));
    }
    EXPECT_EQ(applicationField->hostStartLanes, expectedStartLanes);
    EXPECT_EQ(applicationField->mediaStartLanes, expectedStartLanes);
  }

  EXPECT_TRUE(diagsCap.value().cdb().value());
  EXPECT_TRUE(diagsCap.value().prbsLine().value());
  EXPECT_TRUE(diagsCap.value().prbsSystem().value());
  EXPECT_TRUE(diagsCap.value().loopbackLine().value());
  EXPECT_TRUE(diagsCap.value().loopbackSystem().value());
  EXPECT_TRUE(diagsCap.value().txOutputControl().value());
  EXPECT_TRUE(diagsCap.value().rxOutputControl().value());

  // 1x800G
  TransceiverPortState goodPortState1{
      "", 0, cfg::PortSpeed::EIGHTHUNDREDG, 8, TransmitterTechnology::OPTICAL};
  for (auto portState : {
           goodPortState1,
       }) {
    EXPECT_TRUE(xcvr->tcvrPortStateSupported(portState));
  }

  TransceiverPortState badPortState1{
      "",
      0,
      cfg::PortSpeed::HUNDREDG,
      4,
      TransmitterTechnology::COPPER}; // Copper not supported
  TransceiverPortState badPortState2{
      "",
      0,
      cfg::PortSpeed::FORTYG,
      4,
      TransmitterTechnology::OPTICAL}; // 40G not supported
  TransceiverPortState badPortState3{
      "",
      1,
      cfg::PortSpeed::HUNDREDG,
      4,
      TransmitterTechnology::OPTICAL}; // BAD START LANE
  for (auto portState : {badPortState1, badPortState2, badPortState3}) {
    EXPECT_FALSE(xcvr->tcvrPortStateSupported(portState));
  }

  // tunable optics check
  EXPECT_TRUE(xcvr->isTunableOptics());

  // Test all supported frequency grids map to correct grid selection values
  EXPECT_EQ(
      0x00, xcvr->frequencyGridToGridSelection(FrequencyGrid::LASER_3P125GHZ));
  EXPECT_EQ(
      0x10, xcvr->frequencyGridToGridSelection(FrequencyGrid::LASER_6P25GHZ));
  EXPECT_EQ(
      0x20, xcvr->frequencyGridToGridSelection(FrequencyGrid::LASER_12P5GHZ));
  EXPECT_EQ(
      0x30, xcvr->frequencyGridToGridSelection(FrequencyGrid::LASER_25GHZ));
  EXPECT_EQ(
      0x40, xcvr->frequencyGridToGridSelection(FrequencyGrid::LASER_50GHZ));
  EXPECT_EQ(
      0x50, xcvr->frequencyGridToGridSelection(FrequencyGrid::LASER_100GHZ));
  EXPECT_EQ(
      0x60, xcvr->frequencyGridToGridSelection(FrequencyGrid::LASER_33GHZ));
  EXPECT_EQ(
      0x70, xcvr->frequencyGridToGridSelection(FrequencyGrid::LASER_75GHZ));
  EXPECT_EQ(
      0x80, xcvr->frequencyGridToGridSelection(FrequencyGrid::LASER_150GHZ));

  // Test getChannelNumFromFrequency: base frequency → channel 0
  EXPECT_EQ(
      xcvr->getChannelNumFromFrequency(
          193100000, FrequencyGrid::LASER_3P125GHZ),
      0);
  EXPECT_EQ(
      xcvr->getChannelNumFromFrequency(193100000, FrequencyGrid::LASER_6P25GHZ),
      0);

  // Test getChannelNumFromFrequency: positive channel numbers
  EXPECT_EQ(
      xcvr->getChannelNumFromFrequency(193775000, FrequencyGrid::LASER_6P25GHZ),
      108);
  EXPECT_EQ(
      xcvr->getChannelNumFromFrequency(193925000, FrequencyGrid::LASER_6P25GHZ),
      132);

  // Test getChannelNumFromFrequency: negative channel numbers (L-band)
  EXPECT_EQ(
      xcvr->getChannelNumFromFrequency(186125000, FrequencyGrid::LASER_6P25GHZ),
      -1116);
  EXPECT_EQ(
      xcvr->getChannelNumFromFrequency(191375000, FrequencyGrid::LASER_6P25GHZ),
      -276);

  // Test getChannelNumFromFrequency across all grids to verify
  // no floating-point truncation (the off-by-one bug)
  // 150 GHz grid: n = (diffMhz * 40) / 1000000 - 3; channel 1 at 193200000
  EXPECT_EQ(
      xcvr->getChannelNumFromFrequency(193200000, FrequencyGrid::LASER_150GHZ),
      1);
  EXPECT_EQ(
      xcvr->getChannelNumFromFrequency(193200000, FrequencyGrid::LASER_100GHZ),
      1);
  // 75 GHz grid: n = (diffMhz * 40) / 1000000; channel 1 at 193125000
  EXPECT_EQ(
      xcvr->getChannelNumFromFrequency(193125000, FrequencyGrid::LASER_75GHZ),
      1);
  EXPECT_EQ(
      xcvr->getChannelNumFromFrequency(193150000, FrequencyGrid::LASER_50GHZ),
      1);
  // 33 GHz grid: n = (diffMhz * 30) / 1000000; channel 3 at 193200000
  EXPECT_EQ(
      xcvr->getChannelNumFromFrequency(193200000, FrequencyGrid::LASER_33GHZ),
      3);
  EXPECT_EQ(
      xcvr->getChannelNumFromFrequency(193125000, FrequencyGrid::LASER_25GHZ),
      1);
  EXPECT_EQ(
      xcvr->getChannelNumFromFrequency(193112500, FrequencyGrid::LASER_12P5GHZ),
      1);
  EXPECT_EQ(
      xcvr->getChannelNumFromFrequency(193106250, FrequencyGrid::LASER_6P25GHZ),
      1);
  EXPECT_EQ(
      xcvr->getChannelNumFromFrequency(
          193103125, FrequencyGrid::LASER_3P125GHZ),
      1);

  auto tunableLaserStatus = xcvr->getTunableLaserStatus();
  EXPECT_NE(tunableLaserStatus, std::nullopt);
  EXPECT_EQ(
      tunableLaserStatus->laserFrequencyMhz(),
      CmisModule::kDefaultFrequencyMhz);
  EXPECT_EQ(
      tunableLaserStatus->tuningStatus(),
      LaserStatusBitMask::LASER_TUNE_NOT_IN_PROGRESS);
  EXPECT_EQ(
      tunableLaserStatus->wavelengthLockingStatus(),
      LaserStatusBitMask::WAVELENGTH_LOCKED);
  EXPECT_EQ(
      xcvr->getInterfaceCodeForAppSel(1, 1),
      static_cast<uint8_t>(SMFMediaInterfaceCode::ZR_OROADM_FLEXO_8E_DPO_800G));
  EXPECT_EQ(xcvr->getCurrentAppSelCode(1), 0x1);
  EXPECT_TRUE(info.tcvrState()->errorStates()->empty());
}

// Test coherent FEC Performance Monitoring stats from C-CMIS page 34h
// on 800G ZR modules. Validates that fillVdmPerfMonitorFecPm correctly
// decodes all FEC PM counters from the cached page34 data.
TEST_F(CmisTest, cmis800GZrFecPmTest) {
  auto xcvrID = TransceiverID(1);
  auto xcvr = overrideCmisModule<Cmis800GZrTransceiver>(
      xcvrID, TransceiverModuleIdentifier::OSFP);
  const auto& info = xcvr->getTransceiverInfo();

  ASSERT_TRUE(xcvr->isVdmSupported());
  ASSERT_TRUE(xcvr->isTunableOptics());

  // Program transceiver with OpticalChannelConfig for tunable optics
  // to populate port-to-media-lane mappings needed for FEC PM stats
  ProgramTransceiverState programTcvrState;
  TransceiverPortState portState;
  portState.portName = "eth1/1/1";
  portState.startHostLane = 0;
  portState.speed = cfg::PortSpeed::EIGHTHUNDREDG;
  portState.numHostLanes = 8;

  cfg::OpticalChannelConfig optChanConfig;
  cfg::FrequencyConfig freqConfig;
  freqConfig.frequencyGrid() = FrequencyGrid::LASER_6P25GHZ;
  cfg::CenterFrequencyConfig centerFreq;
  centerFreq.set_frequencyMhz(CmisModule::kDefaultFrequencyMhz);
  freqConfig.centerFrequencyConfig() = centerFreq;
  optChanConfig.frequencyConfig() = freqConfig;
  optChanConfig.txPower0P01Dbm() = 0;
  optChanConfig.appSelCode() = 1;
  portState.opticalChannelConfig = optChanConfig;

  programTcvrState.ports.emplace(portState.portName, portState);
  xcvr->programTransceiver(programTcvrState, false);

  std::vector<int32_t> xcvrIds = {xcvrID};
  transceiverManager_->triggerVdmStatsCapture(xcvrIds);
  transceiverManager_->refreshStateMachines();

  const auto& newInfo = xcvr->getTransceiverInfo();
  ASSERT_TRUE(newInfo.tcvrStats()->vdmPerfMonitorStats().has_value());
  auto& vdmPerfMonStats = newInfo.tcvrStats()->vdmPerfMonitorStats().value();
  ASSERT_FALSE(vdmPerfMonStats.mediaPortVdmStats()->empty());

  // Validate coherent FEC PM values from C-CMIS page 34h
  // Expected values are decoded from kCmis800GZrPage34 fake data
  auto portIt = vdmPerfMonStats.mediaPortVdmStats()->find("eth1/1/1");
  ASSERT_NE(portIt, vdmPerfMonStats.mediaPortVdmStats()->end());
  ASSERT_TRUE(portIt->second.coherentVdmStats().has_value());
  ASSERT_TRUE(portIt->second.coherentVdmStats()->fecPm().has_value());
  auto& fecPm = portIt->second.coherentVdmStats()->fecPm().value();

  // U64 FEC PM counters
  EXPECT_EQ(*fecPm.rxBitsPm(), 61440720961536);
  EXPECT_EQ(*fecPm.rxBitsSubIntPm(), 1809317888);
  EXPECT_EQ(*fecPm.rxCorrBitsPm(), 20766418419);
  EXPECT_EQ(*fecPm.rxMinCorrBitsSubIntPm(), 323226814);
  EXPECT_EQ(*fecPm.rxMaxCorrBitsSubIntPm(), 395888028);

  // U32 FEC PM counters
  EXPECT_EQ(*fecPm.rxFramesPm(), 44643381);
  EXPECT_EQ(*fecPm.rxFramesSubIntPm(), 744056);
  EXPECT_EQ(*fecPm.rxFramesUncorrErrPm(), 0);
  EXPECT_EQ(*fecPm.rxMinFramesUncorrErrSubIntPm(), 0);
  EXPECT_EQ(*fecPm.rxMaxFramesUncorrErrSubIntPm(), 0);
}

TEST_F(CmisTest, cmis800GZrLinePmTest) {
  auto xcvrID = TransceiverID(1);
  auto xcvr = overrideCmisModule<Cmis800GZrTransceiver>(
      xcvrID, TransceiverModuleIdentifier::OSFP);
  const auto& info = xcvr->getTransceiverInfo();

  ASSERT_TRUE(xcvr->isVdmSupported());
  ASSERT_TRUE(xcvr->isTunableOptics());

  // Program transceiver with OpticalChannelConfig for tunable optics
  // to populate port-to-media-lane mappings needed for Link PM stats
  ProgramTransceiverState programTcvrState;
  TransceiverPortState portState;
  portState.portName = "eth1/1/1";
  portState.startHostLane = 0;
  portState.speed = cfg::PortSpeed::EIGHTHUNDREDG;
  portState.numHostLanes = 8;

  cfg::OpticalChannelConfig optChanConfig;
  cfg::FrequencyConfig freqConfig;
  freqConfig.frequencyGrid() = FrequencyGrid::LASER_6P25GHZ;
  cfg::CenterFrequencyConfig centerFreq;
  centerFreq.set_frequencyMhz(CmisModule::kDefaultFrequencyMhz);
  freqConfig.centerFrequencyConfig() = centerFreq;
  optChanConfig.frequencyConfig() = freqConfig;
  optChanConfig.txPower0P01Dbm() = 0;
  optChanConfig.appSelCode() = 1;
  portState.opticalChannelConfig = optChanConfig;

  programTcvrState.ports.emplace(portState.portName, portState);
  xcvr->programTransceiver(programTcvrState, false);

  std::vector<int32_t> xcvrIds = {xcvrID};
  transceiverManager_->triggerVdmStatsCapture(xcvrIds);
  transceiverManager_->refreshStateMachines();

  const auto& newInfo = xcvr->getTransceiverInfo();
  ASSERT_TRUE(newInfo.tcvrStats()->vdmPerfMonitorStats().has_value());
  auto& vdmPerfMonStats = newInfo.tcvrStats()->vdmPerfMonitorStats().value();
  ASSERT_FALSE(vdmPerfMonStats.mediaPortVdmStats()->empty());

  // Validate coherent Link PM values from C-CMIS page 35h
  // Expected values are decoded from kCmis800GZrPage35 fake data
  auto portIt = vdmPerfMonStats.mediaPortVdmStats()->find("eth1/1/1");
  ASSERT_NE(portIt, vdmPerfMonStats.mediaPortVdmStats()->end());
  ASSERT_TRUE(portIt->second.coherentVdmStats().has_value());
  ASSERT_TRUE(portIt->second.coherentVdmStats()->linkPm().has_value());
  auto& linkPm = portIt->second.coherentVdmStats()->linkPm().value();

  // CD: S32 at bytes 128-139, LSB=1.0 ps/nm (exact integer values)
  ASSERT_TRUE(linkPm.cd().has_value());
  EXPECT_EQ(*linkPm.cd()->avg(), -1.0);
  EXPECT_EQ(*linkPm.cd()->min(), -3.0);
  EXPECT_EQ(*linkPm.cd()->max(), 1.0);

  // DGD: U16 at bytes 140-145, LSB=0.01 ps
  ASSERT_TRUE(linkPm.dgd().has_value());
  EXPECT_DOUBLE_EQ(*linkPm.dgd()->avg(), 1.09);
  EXPECT_DOUBLE_EQ(*linkPm.dgd()->min(), 0.95);
  EXPECT_DOUBLE_EQ(*linkPm.dgd()->max(), 1.31);

  // SOPMD: U16 at bytes 146-151, LSB=0.01 ps^2
  ASSERT_TRUE(linkPm.sopmd().has_value());
  EXPECT_DOUBLE_EQ(*linkPm.sopmd()->avg(), 65.0);
  EXPECT_DOUBLE_EQ(*linkPm.sopmd()->min(), 8.0);
  EXPECT_DOUBLE_EQ(*linkPm.sopmd()->max(), 211.0);

  // PDL: U16 at bytes 152-157, LSB=0.1 dB
  ASSERT_TRUE(linkPm.pdl().has_value());
  EXPECT_DOUBLE_EQ(*linkPm.pdl()->avg(), 0.8);
  EXPECT_DOUBLE_EQ(*linkPm.pdl()->min(), 0.7);
  EXPECT_DOUBLE_EQ(*linkPm.pdl()->max(), 0.9);

  // OSNR: U16 at bytes 158-163, LSB=0.1 dB
  ASSERT_TRUE(linkPm.osnr().has_value());
  EXPECT_DOUBLE_EQ(*linkPm.osnr()->avg(), 33.8);
  EXPECT_DOUBLE_EQ(*linkPm.osnr()->min(), 32.8);
  EXPECT_DOUBLE_EQ(*linkPm.osnr()->max(), 34.8);

  // eSNR: U16 at bytes 164-169, LSB=0.1 dB
  ASSERT_TRUE(linkPm.esnr().has_value());
  EXPECT_DOUBLE_EQ(*linkPm.esnr()->avg(), 15.6);
  EXPECT_DOUBLE_EQ(*linkPm.esnr()->min(), 15.5);
  EXPECT_DOUBLE_EQ(*linkPm.esnr()->max(), 15.6);

  // CFO: S16 at bytes 170-175, LSB=1.0 MHz (exact integer values)
  ASSERT_TRUE(linkPm.cfo().has_value());
  EXPECT_EQ(*linkPm.cfo()->avg(), 119.0);
  EXPECT_EQ(*linkPm.cfo()->min(), 58.0);
  EXPECT_EQ(*linkPm.cfo()->max(), 205.0);

  // EVM: U16 at bytes 176-181, LSB=100.0/65535.0 % (irrational LSB, use NEAR)
  ASSERT_TRUE(linkPm.evmModem().has_value());
  EXPECT_NEAR(*linkPm.evmModem()->avg(), 10268.0 * 100.0 / 65535.0, 0.01);
  EXPECT_NEAR(*linkPm.evmModem()->min(), 10258.0 * 100.0 / 65535.0, 0.01);
  EXPECT_NEAR(*linkPm.evmModem()->max(), 10453.0 * 100.0 / 65535.0, 0.01);

  // TxPower: S16 at bytes 182-187, LSB=0.01 dBm
  ASSERT_TRUE(linkPm.txPower().has_value());
  EXPECT_DOUBLE_EQ(*linkPm.txPower()->avg(), -1.99);
  EXPECT_DOUBLE_EQ(*linkPm.txPower()->min(), -2.05);
  EXPECT_DOUBLE_EQ(*linkPm.txPower()->max(), -1.92);

  // RxTotalPower: S16 at bytes 188-193, LSB=0.01 dBm
  ASSERT_TRUE(linkPm.rxPower().has_value());
  EXPECT_DOUBLE_EQ(*linkPm.rxPower()->avg(), -2.04);
  EXPECT_DOUBLE_EQ(*linkPm.rxPower()->min(), -2.14);
  EXPECT_DOUBLE_EQ(*linkPm.rxPower()->max(), -1.93);

  // RxSigPower: S16 at bytes 194-199, LSB=0.01 dBm
  ASSERT_TRUE(linkPm.rxSigPower().has_value());
  EXPECT_DOUBLE_EQ(*linkPm.rxSigPower()->avg(), -2.94);
  EXPECT_DOUBLE_EQ(*linkPm.rxSigPower()->min(), -3.12);
  EXPECT_DOUBLE_EQ(*linkPm.rxSigPower()->max(), -2.69);

  // SOP ROC: S16 at bytes 200-205, LSB=1.0 krad/s (exact integer values)
  ASSERT_TRUE(linkPm.sopcr().has_value());
  EXPECT_EQ(*linkPm.sopcr()->avg(), 4.0);
  EXPECT_EQ(*linkPm.sopcr()->min(), 0.0);
  EXPECT_EQ(*linkPm.sopcr()->max(), 14.0);

  // MER: U16 at bytes 206-211, LSB=0.1 dB
  ASSERT_TRUE(linkPm.mer().has_value());
  EXPECT_DOUBLE_EQ(*linkPm.mer()->avg(), 16.0);
  EXPECT_DOUBLE_EQ(*linkPm.mer()->min(), 15.8);
  EXPECT_DOUBLE_EQ(*linkPm.mer()->max(), 16.0);

  // ClockRecoveryLoop: S16 at bytes 212-217, LSB=100.0/32767.0 % (irrational)
  ASSERT_TRUE(linkPm.clockRecoveryLoop().has_value());
  EXPECT_NEAR(*linkPm.clockRecoveryLoop()->avg(), 83.0 * 100.0 / 32767.0, 0.01);
  EXPECT_EQ(*linkPm.clockRecoveryLoop()->min(), 0.0);
  EXPECT_NEAR(
      *linkPm.clockRecoveryLoop()->max(), 183.0 * 100.0 / 32767.0, 0.01);

  // SNR Margin: S16 at bytes 224-229, LSB=0.1 dB
  ASSERT_TRUE(linkPm.snrMargin().has_value());
  EXPECT_DOUBLE_EQ(*linkPm.snrMargin()->avg(), 4.1);
  EXPECT_DOUBLE_EQ(*linkPm.snrMargin()->min(), 4.1);
  EXPECT_DOUBLE_EQ(*linkPm.snrMargin()->max(), 4.1);

  // Q-factor: U16 at bytes 230-235, LSB=0.1 dB
  ASSERT_TRUE(linkPm.qFactor().has_value());
  EXPECT_DOUBLE_EQ(*linkPm.qFactor()->avg(), 10.6);
  EXPECT_DOUBLE_EQ(*linkPm.qFactor()->min(), 10.6);
  EXPECT_DOUBLE_EQ(*linkPm.qFactor()->max(), 10.7);

  // Q-margin: S16 at bytes 236-241, LSB=0.1 dB
  ASSERT_TRUE(linkPm.qMargin().has_value());
  EXPECT_DOUBLE_EQ(*linkPm.qMargin()->avg(), 4.1);
  EXPECT_DOUBLE_EQ(*linkPm.qMargin()->min(), 4.0);
  EXPECT_DOUBLE_EQ(*linkPm.qMargin()->max(), 4.2);
}

// Test for VDM support on 800G ZR modules
TEST_F(CmisTest, cmis800GZrVdmTest) {
  auto xcvrID = TransceiverID(1);
  auto xcvr = overrideCmisModule<Cmis800GZrTransceiver>(
      xcvrID, TransceiverModuleIdentifier::OSFP);

  EXPECT_TRUE(xcvr->isVdmSupported());
  // Verify all 4 VDM groups are supported (VDMSupport bits 1-0 = 0x03)
  EXPECT_TRUE(xcvr->isVdmSupported(4));

  // Verify coherent VDM parameters are found in VDM config (page 23h)
  // with data values on page 27h
  auto modulatorBiasXI =
      xcvr->getVdmDiagsValLocation(VdmConfigType::MODULATOR_BIAS_XI);
  EXPECT_TRUE(modulatorBiasXI.vdmConfImplementedByModule);
  EXPECT_EQ(modulatorBiasXI.vdmValPage, CmisPages::PAGE27);

  auto modulatorBiasXQ =
      xcvr->getVdmDiagsValLocation(VdmConfigType::MODULATOR_BIAS_XQ);
  EXPECT_TRUE(modulatorBiasXQ.vdmConfImplementedByModule);
  EXPECT_EQ(modulatorBiasXQ.vdmValPage, CmisPages::PAGE27);

  auto modulatorBiasYI =
      xcvr->getVdmDiagsValLocation(VdmConfigType::MODULATOR_BIAS_YI);
  EXPECT_TRUE(modulatorBiasYI.vdmConfImplementedByModule);
  EXPECT_EQ(modulatorBiasYI.vdmValPage, CmisPages::PAGE27);

  auto modulatorBiasYQ =
      xcvr->getVdmDiagsValLocation(VdmConfigType::MODULATOR_BIAS_YQ);
  EXPECT_TRUE(modulatorBiasYQ.vdmConfImplementedByModule);
  EXPECT_EQ(modulatorBiasYQ.vdmValPage, CmisPages::PAGE27);

  auto modulatorBiasXPhase =
      xcvr->getVdmDiagsValLocation(VdmConfigType::MODULATOR_BIAS_X_PHASE);
  EXPECT_TRUE(modulatorBiasXPhase.vdmConfImplementedByModule);
  EXPECT_EQ(modulatorBiasXPhase.vdmValPage, CmisPages::PAGE27);

  auto modulatorBiasYPhase =
      xcvr->getVdmDiagsValLocation(VdmConfigType::MODULATOR_BIAS_Y_PHASE);
  EXPECT_TRUE(modulatorBiasYPhase.vdmConfImplementedByModule);
  EXPECT_EQ(modulatorBiasYPhase.vdmValPage, CmisPages::PAGE27);

  auto cdLowGran =
      xcvr->getVdmDiagsValLocation(VdmConfigType::CD_LOW_GRANULARITY);
  EXPECT_TRUE(cdLowGran.vdmConfImplementedByModule);
  EXPECT_EQ(cdLowGran.vdmValPage, CmisPages::PAGE27);

  auto sopmdLowGran =
      xcvr->getVdmDiagsValLocation(VdmConfigType::SOPMD_LOW_GRANULARITY);
  EXPECT_TRUE(sopmdLowGran.vdmConfImplementedByModule);
  EXPECT_EQ(sopmdLowGran.vdmValPage, CmisPages::PAGE27);

  // Program transceiver to populate port-to-media-lane mappings
  ProgramTransceiverState programTcvrState;
  TransceiverPortState portState;
  portState.portName = "eth1/1/1";
  portState.startHostLane = 0;
  portState.speed = cfg::PortSpeed::EIGHTHUNDREDG;
  portState.numHostLanes = 8;

  cfg::OpticalChannelConfig optChanConfig;
  cfg::FrequencyConfig freqConfig;
  freqConfig.frequencyGrid() = FrequencyGrid::LASER_6P25GHZ;
  cfg::CenterFrequencyConfig centerFreq;
  centerFreq.set_frequencyMhz(CmisModule::kDefaultFrequencyMhz);
  freqConfig.centerFrequencyConfig() = centerFreq;
  optChanConfig.frequencyConfig() = freqConfig;
  optChanConfig.txPower0P01Dbm() = 0;
  optChanConfig.appSelCode() = 1;
  portState.opticalChannelConfig = optChanConfig;

  programTcvrState.ports.emplace(portState.portName, portState);
  xcvr->programTransceiver(programTcvrState, false);

  std::vector<int32_t> xcvrIds = {xcvrID};
  transceiverManager_->triggerVdmStatsCapture(xcvrIds);
  transceiverManager_->refreshStateMachines();

  const auto& newInfo = xcvr->getTransceiverInfo();
  ASSERT_TRUE(newInfo.tcvrStats()->vdmPerfMonitorStats().has_value());
  auto& vdmPerfMonStats = newInfo.tcvrStats()->vdmPerfMonitorStats().value();
  ASSERT_FALSE(vdmPerfMonStats.mediaPortVdmStats()->empty());

  // Validate coherent VDM values from page 0x27 (kCmis800GZrPage27)
  // Values are computed as: rawU16 * LSB (or rawS16 * LSB for signed)
  auto portIt = vdmPerfMonStats.mediaPortVdmStats()->find("eth1/1/1");
  ASSERT_NE(portIt, vdmPerfMonStats.mediaPortVdmStats()->end());
  ASSERT_TRUE(portIt->second.coherentVdmStats().has_value());
  auto& coherentVdm = portIt->second.coherentVdmStats().value();

  // Modulator Bias XI: raw U16 = 0x758b, LSB = 100/65535
  EXPECT_EQ(int(*coherentVdm.modulatorBiasXI() * 100), 4591);
  // Modulator Bias XQ: raw U16 = 0x6118, LSB = 100/65535
  EXPECT_EQ(int(*coherentVdm.modulatorBiasXQ() * 100), 3792);
  // Modulator Bias YI: raw U16 = 0x7764, LSB = 100/65535
  EXPECT_EQ(int(*coherentVdm.modulatorBiasYI() * 100), 4663);
  // Modulator Bias YQ: raw U16 = 0x63d1, LSB = 100/65535
  EXPECT_EQ(int(*coherentVdm.modulatorBiasYQ() * 100), 3899);
  // Modulator Bias X Phase: raw U16 = 0x6a5f, LSB = 100/65535
  EXPECT_EQ(int(*coherentVdm.modulatorBiasXPhase() * 100), 4155);
  // Modulator Bias Y Phase: raw U16 = 0x661b, LSB = 100/65535
  EXPECT_EQ(int(*coherentVdm.modulatorBiasYPhase() * 100), 3988);
  // CD low granularity: raw S16 = 0x0000, LSB = 20 ps/nm
  EXPECT_EQ(*coherentVdm.cdLowGranularity(), 0.0);
  // SOPMD low granularity: raw U16 = 0x0045 = 69, LSB = 1 ps^2
  EXPECT_EQ(*coherentVdm.sopmdLowGranularity(), 69.0);
}

TEST_F(CmisTest, cmisCredo800AecInfoTest) {
  auto xcvrID = TransceiverID(1);
  auto xcvr = overrideCmisModule<CmisCredo800AEC>(
      xcvrID, TransceiverModuleIdentifier::OSFP);
  const auto& info = xcvr->getTransceiverInfo();
  EXPECT_TRUE(info.tcvrState()->transceiverManagementInterface());
  EXPECT_EQ(
      info.tcvrState()->transceiverManagementInterface(),
      TransceiverManagementInterface::CMIS);
  EXPECT_EQ(xcvr->numHostLanes(), 8);
  EXPECT_EQ(xcvr->numMediaLanes(), 8);
  EXPECT_EQ(
      info.tcvrState()->moduleMediaInterface(), MediaInterfaceCode::CR8_800G);
  for (auto& media : *info.tcvrState()->settings()->mediaInterface()) {
    EXPECT_EQ(
        media.media()->get_activeCuCode(),
        ActiveCuHostInterfaceCode::AUI_PAM4_8S_800G);
    EXPECT_EQ(media.code(), MediaInterfaceCode::CR8_800G);
  }

  // Check cmisStateChanged
  EXPECT_TRUE(
      info.tcvrState()->status() &&
      info.tcvrState()->status()->cmisStateChanged() &&
      *info.tcvrState()->status()->cmisStateChanged());

  utility::HwTransceiverUtils::verifyDiagsCapability(
      *info.tcvrState(),
      transceiverManager_->getDiagsCapability(xcvrID),
      false /* skipCheckingIndividualCapability */);

  TransceiverTestsHelper tests(info);
  tests.verifyVendorName("FACETEST");

  auto diagsCap = transceiverManager_->getDiagsCapability(xcvrID);
  EXPECT_TRUE(diagsCap.has_value());
  std::vector<prbs::PrbsPolynomial> expectedSysPolynomials = {
      prbs::PrbsPolynomial::PRBS31Q,
      prbs::PrbsPolynomial::PRBS15Q,
      prbs::PrbsPolynomial::PRBS13Q,
      prbs::PrbsPolynomial::PRBS9Q,
      prbs::PrbsPolynomial::PRBS31,
      prbs::PrbsPolynomial::PRBS15,
      prbs::PrbsPolynomial::PRBS13,
      prbs::PrbsPolynomial::PRBS9};
  std::vector<prbs::PrbsPolynomial> expectedLinePolynomials = {
      prbs::PrbsPolynomial::PRBS31Q,
      prbs::PrbsPolynomial::PRBS15Q,
      prbs::PrbsPolynomial::PRBS13Q,
      prbs::PrbsPolynomial::PRBS9Q,
      prbs::PrbsPolynomial::PRBS31,
      prbs::PrbsPolynomial::PRBS15,
      prbs::PrbsPolynomial::PRBS13,
      prbs::PrbsPolynomial::PRBS9};

  auto linePrbsCapability = *(*diagsCap).prbsLineCapabilities();
  auto sysPrbsCapability = *(*diagsCap).prbsSystemCapabilities();

  tests.verifyPrbsPolynomials(expectedLinePolynomials, linePrbsCapability);
  tests.verifyPrbsPolynomials(expectedSysPolynomials, sysPrbsCapability);

  for (auto unsupportedApplication : {SMFMediaInterfaceCode::LR4_10_400G}) {
    EXPECT_EQ(
        xcvr->getApplicationField(
            static_cast<uint8_t>(unsupportedApplication), 0),
        std::nullopt);
  }

  for (auto supportedApplication :
       {ActiveCuHostInterfaceCode::AUI_PAM4_8S_800G,
        ActiveCuHostInterfaceCode::AUI_PAM4_4S_400G}) {
    auto applicationField = xcvr->getApplicationField(
        static_cast<uint8_t>(supportedApplication), 0);
    EXPECT_NE(applicationField, std::nullopt);
    std::vector<int> expectedStartLanes;
    if (supportedApplication == ActiveCuHostInterfaceCode::AUI_PAM4_8S_800G) {
      expectedStartLanes = {0};
    } else {
      expectedStartLanes = {0, 4};
    }
    EXPECT_EQ(applicationField->hostStartLanes, expectedStartLanes);
    EXPECT_EQ(applicationField->mediaStartLanes, expectedStartLanes);
    for (uint8_t lane = 0; lane <= 7; lane++) {
      if (std::find(
              expectedStartLanes.begin(), expectedStartLanes.end(), lane) !=
          expectedStartLanes.end()) {
        continue;
      }
      // For lanes that are not expected to be start lanes, getApplicationField
      // should return nullopt
      EXPECT_EQ(
          xcvr->getApplicationField(
              static_cast<uint8_t>(supportedApplication), lane),
          std::nullopt);
    }
  }

  EXPECT_TRUE(diagsCap.value().cdb().value());
  EXPECT_TRUE(diagsCap.value().prbsLine().value());
  EXPECT_TRUE(diagsCap.value().prbsSystem().value());
  EXPECT_TRUE(diagsCap.value().loopbackLine().value());
  EXPECT_TRUE(diagsCap.value().loopbackSystem().value());
  EXPECT_TRUE(diagsCap.value().txOutputControl().value());
  EXPECT_TRUE(diagsCap.value().rxOutputControl().value());
  EXPECT_TRUE(diagsCap.value().snrLine().value());
  EXPECT_TRUE(diagsCap.value().snrSystem().value());
  EXPECT_TRUE(xcvr->isPrbsSupported(phy::Side::LINE));
  EXPECT_TRUE(xcvr->isPrbsSupported(phy::Side::SYSTEM));
  EXPECT_TRUE(xcvr->isSnrSupported(phy::Side::LINE));
  EXPECT_TRUE(xcvr->isSnrSupported(phy::Side::SYSTEM));

  // 1x800G
  TransceiverPortState goodPortState1{
      "", 0, cfg::PortSpeed::EIGHTHUNDREDG, 8, TransmitterTechnology::COPPER};

  // 2x400G
  TransceiverPortState goodPortState2{
      "", 0, cfg::PortSpeed::FOURHUNDREDG, 4, TransmitterTechnology::COPPER};
  TransceiverPortState goodPortState3{
      "", 4, cfg::PortSpeed::FOURHUNDREDG, 4, TransmitterTechnology::COPPER};
  for (auto portState : {goodPortState1, goodPortState2, goodPortState3}) {
    EXPECT_TRUE(xcvr->tcvrPortStateSupported(portState));
  }

  TransceiverPortState badPortState1{
      "",
      0,
      cfg::PortSpeed::HUNDREDG,
      4,
      TransmitterTechnology::OPTICAL}; // OPTICAL not supported
  TransceiverPortState badPortState2{
      "",
      0,
      cfg::PortSpeed::FORTYG,
      4,
      TransmitterTechnology::OPTICAL}; // 40G not supported
  TransceiverPortState badPortState3{
      "",
      1,
      cfg::PortSpeed::HUNDREDG,
      4,
      TransmitterTechnology::OPTICAL}; // BAD START LANE
  for (auto portState : {badPortState1, badPortState2, badPortState3}) {
    EXPECT_FALSE(xcvr->tcvrPortStateSupported(portState));
  }
  EXPECT_TRUE(info.tcvrState()->errorStates()->empty());
}

TEST_F(CmisTest, cmis2x400GFr4TransceiverVdmTest) {
  auto xcvrID = TransceiverID(1);
  auto xcvr = overrideCmisModule<Cmis2x400GFr4Transceiver>(
      xcvrID, TransceiverModuleIdentifier::OSFP);
  const auto& info = xcvr->getTransceiverInfo();
  EXPECT_TRUE(info.tcvrState()->transceiverManagementInterface());
  EXPECT_EQ(
      info.tcvrState()->transceiverManagementInterface(),
      TransceiverManagementInterface::CMIS);
  EXPECT_EQ(xcvr->numHostLanes(), 8);
  EXPECT_EQ(xcvr->numMediaLanes(), 8);
  EXPECT_EQ(
      info.tcvrState()->moduleMediaInterface(), MediaInterfaceCode::FR4_2x400G);

  utility::HwTransceiverUtils::verifyDiagsCapability(
      *info.tcvrState(),
      transceiverManager_->getDiagsCapability(xcvrID),
      false /* skipCheckingIndividualCapability */);

  TransceiverTestsHelper tests(info);
  tests.verifyVendorName("FACETEST");

  auto diagsCap = transceiverManager_->getDiagsCapability(xcvrID);
  EXPECT_TRUE(diagsCap.has_value());

  EXPECT_TRUE(diagsCap.value().vdm().value());
  EXPECT_TRUE(xcvr->isVdmSupported());
  EXPECT_TRUE(xcvr->isVdmSupported(3));

  auto vdmLocationInfo = xcvr->getVdmDiagsValLocation(SNR_MEDIA_IN);
  EXPECT_TRUE(vdmLocationInfo.vdmConfImplementedByModule);
  EXPECT_EQ(vdmLocationInfo.vdmValPage, static_cast<CmisPages>(0x24));
  EXPECT_EQ(vdmLocationInfo.vdmValOffset, 128);
  EXPECT_EQ(vdmLocationInfo.vdmValLength, 16);
  vdmLocationInfo = xcvr->getVdmDiagsValLocation(PAM4_LTP_MEDIA_IN);
  EXPECT_TRUE(vdmLocationInfo.vdmConfImplementedByModule);
  EXPECT_EQ(vdmLocationInfo.vdmValPage, static_cast<CmisPages>(0x24));
  EXPECT_EQ(vdmLocationInfo.vdmValOffset, 144);
  EXPECT_EQ(vdmLocationInfo.vdmValLength, 16);
  vdmLocationInfo = xcvr->getVdmDiagsValLocation(PRE_FEC_BER_MEDIA_IN_MAX);
  EXPECT_TRUE(vdmLocationInfo.vdmConfImplementedByModule);
  EXPECT_EQ(vdmLocationInfo.vdmValPage, static_cast<CmisPages>(0x24));
  EXPECT_EQ(vdmLocationInfo.vdmValOffset, 160);
  EXPECT_EQ(vdmLocationInfo.vdmValLength, 16);
  vdmLocationInfo = xcvr->getVdmDiagsValLocation(PRE_FEC_BER_MEDIA_IN_AVG);
  EXPECT_TRUE(vdmLocationInfo.vdmConfImplementedByModule);
  EXPECT_EQ(vdmLocationInfo.vdmValPage, static_cast<CmisPages>(0x24));
  EXPECT_EQ(vdmLocationInfo.vdmValOffset, 176);
  EXPECT_EQ(vdmLocationInfo.vdmValLength, 16);
  vdmLocationInfo = xcvr->getVdmDiagsValLocation(ERR_FRAME_MEDIA_IN_MAX);
  EXPECT_TRUE(vdmLocationInfo.vdmConfImplementedByModule);
  EXPECT_EQ(vdmLocationInfo.vdmValPage, static_cast<CmisPages>(0x24));
  EXPECT_EQ(vdmLocationInfo.vdmValOffset, 192);
  EXPECT_EQ(vdmLocationInfo.vdmValLength, 16);
  vdmLocationInfo = xcvr->getVdmDiagsValLocation(ERR_FRAME_MEDIA_IN_AVG);
  EXPECT_TRUE(vdmLocationInfo.vdmConfImplementedByModule);
  EXPECT_EQ(vdmLocationInfo.vdmValPage, static_cast<CmisPages>(0x24));
  EXPECT_EQ(vdmLocationInfo.vdmValOffset, 208);
  EXPECT_EQ(vdmLocationInfo.vdmValLength, 16);
  vdmLocationInfo = xcvr->getVdmDiagsValLocation(FEC_TAIL_MEDIA_IN_MAX);
  EXPECT_TRUE(vdmLocationInfo.vdmConfImplementedByModule);
  EXPECT_EQ(vdmLocationInfo.vdmValPage, static_cast<CmisPages>(0x24));
  EXPECT_EQ(vdmLocationInfo.vdmValOffset, 224);
  EXPECT_EQ(vdmLocationInfo.vdmValLength, 16);
  vdmLocationInfo = xcvr->getVdmDiagsValLocation(FEC_TAIL_MEDIA_IN_CURR);
  EXPECT_TRUE(vdmLocationInfo.vdmConfImplementedByModule);
  EXPECT_EQ(vdmLocationInfo.vdmValPage, static_cast<CmisPages>(0x24));
  EXPECT_EQ(vdmLocationInfo.vdmValOffset, 240);
  EXPECT_EQ(vdmLocationInfo.vdmValLength, 16);
  vdmLocationInfo = xcvr->getVdmDiagsValLocation(SNR_HOST_IN);
  EXPECT_TRUE(vdmLocationInfo.vdmConfImplementedByModule);
  EXPECT_EQ(vdmLocationInfo.vdmValPage, static_cast<CmisPages>(0x25));
  EXPECT_EQ(vdmLocationInfo.vdmValOffset, 128);
  EXPECT_EQ(vdmLocationInfo.vdmValLength, 16);
  vdmLocationInfo = xcvr->getVdmDiagsValLocation(PRE_FEC_BER_HOST_IN_MAX);
  EXPECT_TRUE(vdmLocationInfo.vdmConfImplementedByModule);
  EXPECT_EQ(vdmLocationInfo.vdmValPage, static_cast<CmisPages>(0x25));
  EXPECT_EQ(vdmLocationInfo.vdmValOffset, 160);
  EXPECT_EQ(vdmLocationInfo.vdmValLength, 16);
  vdmLocationInfo = xcvr->getVdmDiagsValLocation(PRE_FEC_BER_HOST_IN_AVG);
  EXPECT_TRUE(vdmLocationInfo.vdmConfImplementedByModule);
  EXPECT_EQ(vdmLocationInfo.vdmValPage, static_cast<CmisPages>(0x25));
  EXPECT_EQ(vdmLocationInfo.vdmValOffset, 176);
  EXPECT_EQ(vdmLocationInfo.vdmValLength, 16);
  vdmLocationInfo = xcvr->getVdmDiagsValLocation(ERR_FRAME_HOST_IN_MAX);
  EXPECT_TRUE(vdmLocationInfo.vdmConfImplementedByModule);
  EXPECT_EQ(vdmLocationInfo.vdmValPage, static_cast<CmisPages>(0x25));
  EXPECT_EQ(vdmLocationInfo.vdmValOffset, 192);
  EXPECT_EQ(vdmLocationInfo.vdmValLength, 16);
  vdmLocationInfo = xcvr->getVdmDiagsValLocation(ERR_FRAME_HOST_IN_AVG);
  EXPECT_TRUE(vdmLocationInfo.vdmConfImplementedByModule);
  EXPECT_EQ(vdmLocationInfo.vdmValPage, static_cast<CmisPages>(0x25));
  EXPECT_EQ(vdmLocationInfo.vdmValOffset, 208);
  EXPECT_EQ(vdmLocationInfo.vdmValLength, 16);
  vdmLocationInfo = xcvr->getVdmDiagsValLocation(FEC_TAIL_HOST_IN_MAX);
  EXPECT_TRUE(vdmLocationInfo.vdmConfImplementedByModule);
  EXPECT_EQ(vdmLocationInfo.vdmValPage, static_cast<CmisPages>(0x25));
  EXPECT_EQ(vdmLocationInfo.vdmValOffset, 224);
  EXPECT_EQ(vdmLocationInfo.vdmValLength, 16);
  vdmLocationInfo = xcvr->getVdmDiagsValLocation(FEC_TAIL_HOST_IN_CURR);
  EXPECT_TRUE(vdmLocationInfo.vdmConfImplementedByModule);
  EXPECT_EQ(vdmLocationInfo.vdmValPage, static_cast<CmisPages>(0x25));
  EXPECT_EQ(vdmLocationInfo.vdmValOffset, 240);
  EXPECT_EQ(vdmLocationInfo.vdmValLength, 16);
  vdmLocationInfo =
      xcvr->getVdmDiagsValLocation(PAM4_LEVEL0_STANDARD_DEVIATION_LINE);
  EXPECT_TRUE(vdmLocationInfo.vdmConfImplementedByModule);
  EXPECT_EQ(vdmLocationInfo.vdmValPage, static_cast<CmisPages>(0x26));
  EXPECT_EQ(vdmLocationInfo.vdmValOffset, 128);
  EXPECT_EQ(vdmLocationInfo.vdmValLength, 16);
  vdmLocationInfo =
      xcvr->getVdmDiagsValLocation(PAM4_LEVEL1_STANDARD_DEVIATION_LINE);
  EXPECT_TRUE(vdmLocationInfo.vdmConfImplementedByModule);
  EXPECT_EQ(vdmLocationInfo.vdmValPage, static_cast<CmisPages>(0x26));
  EXPECT_EQ(vdmLocationInfo.vdmValOffset, 144);
  EXPECT_EQ(vdmLocationInfo.vdmValLength, 16);
  vdmLocationInfo =
      xcvr->getVdmDiagsValLocation(PAM4_LEVEL2_STANDARD_DEVIATION_LINE);
  EXPECT_TRUE(vdmLocationInfo.vdmConfImplementedByModule);
  EXPECT_EQ(vdmLocationInfo.vdmValPage, static_cast<CmisPages>(0x26));
  EXPECT_EQ(vdmLocationInfo.vdmValOffset, 160);
  EXPECT_EQ(vdmLocationInfo.vdmValLength, 16);
  vdmLocationInfo =
      xcvr->getVdmDiagsValLocation(PAM4_LEVEL3_STANDARD_DEVIATION_LINE);
  EXPECT_TRUE(vdmLocationInfo.vdmConfImplementedByModule);
  EXPECT_EQ(vdmLocationInfo.vdmValPage, static_cast<CmisPages>(0x26));
  EXPECT_EQ(vdmLocationInfo.vdmValOffset, 176);
  EXPECT_EQ(vdmLocationInfo.vdmValLength, 16);
  vdmLocationInfo = xcvr->getVdmDiagsValLocation(PAM4_MPI_LINE);
  EXPECT_TRUE(vdmLocationInfo.vdmConfImplementedByModule);
  EXPECT_EQ(vdmLocationInfo.vdmValPage, static_cast<CmisPages>(0x26));
  EXPECT_EQ(vdmLocationInfo.vdmValOffset, 192);
  EXPECT_EQ(vdmLocationInfo.vdmValLength, 16);

  // Check the VDM stats values now
  ProgramTransceiverState programTcvrState;
  TransceiverPortState portState;
  portState.portName = "eth1/1/1";
  portState.startHostLane = 0;
  portState.speed = cfg::PortSpeed::FOURHUNDREDG;
  portState.numHostLanes = 4;
  programTcvrState.ports.emplace(portState.portName, portState);
  portState.portName = "eth1/1/5";
  portState.startHostLane = 4;
  portState.speed = cfg::PortSpeed::FOURHUNDREDG;
  portState.numHostLanes = 4;
  programTcvrState.ports.emplace(portState.portName, portState);

  xcvr->programTransceiver(programTcvrState, false);

  std::vector<int32_t> xcvrIds = {xcvrID};
  transceiverManager_->triggerVdmStatsCapture(xcvrIds);

  transceiverManager_->refreshStateMachines();
  const auto& newInfo = xcvr->getTransceiverInfo();
  EXPECT_TRUE(newInfo.tcvrStats()->vdmPerfMonitorStats().has_value());
  EXPECT_EQ(
      newInfo.tcvrStats()
          ->vdmPerfMonitorStats()
          ->mediaPortVdmStats()
          .value()
          .size(),
      2);
  EXPECT_EQ(
      newInfo.tcvrStats()
          ->vdmPerfMonitorStats()
          ->hostPortVdmStats()
          .value()
          .size(),
      2);
  for (auto& [pName, stats] : newInfo.tcvrStats()
                                  ->vdmPerfMonitorStats()
                                  ->mediaPortVdmStats()
                                  .value()) {
    EXPECT_EQ(stats.laneSNR().value().size(), 4);
  }
  EXPECT_TRUE(newInfo.tcvrStats()->vdmPerfMonitorStatsForOds().has_value());
  EXPECT_EQ(
      newInfo.tcvrStats()
          ->vdmPerfMonitorStatsForOds()
          ->mediaPortVdmStats()
          .value()
          .size(),
      2);
  EXPECT_EQ(
      int(newInfo.tcvrStats()
              ->vdmPerfMonitorStatsForOds()
              ->mediaPortVdmStats()
              .value()
              .begin()
              ->second.laneSNRMin()
              .value() *
          100),
      2103);
  EXPECT_EQ(
      int(newInfo.tcvrStats()
              ->vdmPerfMonitorStatsForOds()
              ->mediaPortVdmStats()
              .value()
              .begin()
              ->second.lanePam4Level0SDMax()
              .value() *
          100),
      178);
  EXPECT_EQ(
      int(newInfo.tcvrStats()
              ->vdmPerfMonitorStatsForOds()
              ->mediaPortVdmStats()
              .value()
              .begin()
              ->second.lanePam4Level1SDMax()
              .value() *
          100),
      184);
  EXPECT_EQ(
      int(newInfo.tcvrStats()
              ->vdmPerfMonitorStatsForOds()
              ->mediaPortVdmStats()
              .value()
              .begin()
              ->second.lanePam4Level2SDMax()
              .value() *
          100),
      217);
  EXPECT_EQ(
      int(newInfo.tcvrStats()
              ->vdmPerfMonitorStatsForOds()
              ->mediaPortVdmStats()
              .value()
              .begin()
              ->second.lanePam4Level3SDMax()
              .value() *
          100),
      213);
  EXPECT_EQ(
      int(newInfo.tcvrStats()
              ->vdmPerfMonitorStatsForOds()
              ->mediaPortVdmStats()
              .value()
              .begin()
              ->second.lanePam4MPIMax()
              .value() *
          100),
      132);
  EXPECT_EQ(
      newInfo.tcvrStats()
          ->vdmPerfMonitorStatsForOds()
          ->mediaPortVdmStats()
          .value()
          .begin()
          ->second.fecTailMax()
          .value(),
      2);
  EXPECT_EQ(
      newInfo.tcvrStats()
          ->vdmPerfMonitorStatsForOds()
          ->hostPortVdmStats()
          .value()
          .begin()
          ->second.fecTailMax()
          .value(),
      3);

  EXPECT_EQ(
      newInfo.tcvrStats()
          ->vdmPerfMonitorStatsForOds()
          ->hostPortVdmStats()
          .value()
          .size(),
      2);
}

TEST_F(CmisTest, cmis2x400GFr4DatapathProgramTest) {
  auto xcvrID = TransceiverID(1);
  auto xcvr = overrideCmisModule<Cmis2x400GFr4Transceiver>(
      xcvrID, TransceiverModuleIdentifier::OSFP);
  const auto& info = xcvr->getTransceiverInfo();
  EXPECT_TRUE(info.tcvrState()->transceiverManagementInterface());
  EXPECT_EQ(
      info.tcvrState()->transceiverManagementInterface(),
      TransceiverManagementInterface::CMIS);
  EXPECT_EQ(xcvr->numHostLanes(), 8);
  EXPECT_EQ(xcvr->numMediaLanes(), 8);

  EXPECT_TRUE(xcvr->isRequestValidMultiportSpeedConfig(
      cfg::PortSpeed::FOURHUNDREDG, 0, 4));
  EXPECT_TRUE(xcvr->isRequestValidMultiportSpeedConfig(
      cfg::PortSpeed::FOURHUNDREDG, 4, 4));
  EXPECT_TRUE(xcvr->isRequestValidMultiportSpeedConfig(
      cfg::PortSpeed::TWOHUNDREDG, 0, 4));
  EXPECT_TRUE(xcvr->isRequestValidMultiportSpeedConfig(
      cfg::PortSpeed::TWOHUNDREDG, 4, 4));
  EXPECT_FALSE(xcvr->isRequestValidMultiportSpeedConfig(
      cfg::PortSpeed::TWOHUNDREDG, 2, 4));
  EXPECT_FALSE(
      xcvr->isRequestValidMultiportSpeedConfig(cfg::PortSpeed::HUNDREDG, 0, 1));
  EXPECT_FALSE(
      xcvr->isRequestValidMultiportSpeedConfig(cfg::PortSpeed::HUNDREDG, 0, 4));
  EXPECT_FALSE(
      xcvr->isRequestValidMultiportSpeedConfig(cfg::PortSpeed::HUNDREDG, 4, 4));

  auto speedCfgCombo = CmisHelper::getValidMultiportSpeedConfig(
      cfg::PortSpeed::FOURHUNDREDG,
      0,
      4,
      CmisModule::laneMask(0, 4),
      "tcvr1",
      xcvr->getModuleCapabilities(),
      CmisHelper::getSmfValidSpeedCombinations(),
      CmisHelper::getSmfSpeedApplicationMapping());
  EXPECT_EQ(speedCfgCombo.size(), CmisModule::kMaxOsfpNumLanes);
  EXPECT_EQ(speedCfgCombo[0], (uint8_t)SMFMediaInterfaceCode::FR4_400G);

  speedCfgCombo = CmisHelper::getValidMultiportSpeedConfig(
      cfg::PortSpeed::FOURHUNDREDG,
      4,
      4,
      CmisModule::laneMask(4, 4),
      "tcvr1",
      xcvr->getModuleCapabilities(),
      CmisHelper::getSmfValidSpeedCombinations(),
      CmisHelper::getSmfSpeedApplicationMapping());
  EXPECT_EQ(speedCfgCombo.size(), CmisModule::kMaxOsfpNumLanes);
  EXPECT_EQ(speedCfgCombo[4], (uint8_t)SMFMediaInterfaceCode::FR4_400G);

  speedCfgCombo = CmisHelper::getValidMultiportSpeedConfig(
      cfg::PortSpeed::HUNDREDG,
      4,
      4,
      CmisModule::laneMask(4, 4),
      "tcvr1",
      xcvr->getModuleCapabilities(),
      CmisHelper::getSmfValidSpeedCombinations(),
      CmisHelper::getSmfSpeedApplicationMapping());
  EXPECT_EQ(speedCfgCombo.size(), CmisModule::kMaxOsfpNumLanes);
  EXPECT_EQ(speedCfgCombo[4], (uint8_t)SMFMediaInterfaceCode::CWDM4_100G);

  speedCfgCombo = CmisHelper::getValidMultiportSpeedConfig(
      cfg::PortSpeed::HUNDREDG,
      5,
      1,
      CmisModule::laneMask(4, 4),
      "tcvr1",
      xcvr->getModuleCapabilities(),
      CmisHelper::getSmfValidSpeedCombinations(),
      CmisHelper::getSmfSpeedApplicationMapping());
  EXPECT_EQ(speedCfgCombo.size(), CmisModule::kMaxOsfpNumLanes);
  for (auto& speed : speedCfgCombo) {
    EXPECT_EQ(speed, (uint8_t)SMFMediaInterfaceCode::FR1_100G);
  }
  EXPECT_TRUE(info.tcvrState()->errorStates()->empty());
}

TEST_F(CmisTest, cmis2x400GDr4TransceiverInfoTest) {
  auto xcvrID = TransceiverID(1);
  auto xcvr = overrideCmisModule<Cmis2x400GDr4Transceiver>(
      xcvrID, TransceiverModuleIdentifier::OSFP);
  const auto& info = xcvr->getTransceiverInfo();
  EXPECT_TRUE(info.tcvrState()->transceiverManagementInterface());
  EXPECT_EQ(
      info.tcvrState()->transceiverManagementInterface(),
      TransceiverManagementInterface::CMIS);
  EXPECT_EQ(xcvr->numHostLanes(), 8);
  EXPECT_EQ(xcvr->numMediaLanes(), 8);
  EXPECT_EQ(
      info.tcvrState()->moduleMediaInterface(), MediaInterfaceCode::DR4_2x400G);
  for (auto& media : *info.tcvrState()->settings()->mediaInterface()) {
    EXPECT_EQ(media.media()->get_smfCode(), SMFMediaInterfaceCode::DR4_400G);
    EXPECT_EQ(media.code(), MediaInterfaceCode::DR4_400G);
  }

  // Check cmisStateChanged
  EXPECT_TRUE(
      info.tcvrState()->status() &&
      info.tcvrState()->status()->cmisStateChanged() &&
      *info.tcvrState()->status()->cmisStateChanged());

  utility::HwTransceiverUtils::verifyDiagsCapability(
      *info.tcvrState(),
      transceiverManager_->getDiagsCapability(xcvrID),
      false /* skipCheckingIndividualCapability */);

  TransceiverTestsHelper tests(info);
  tests.verifyVendorName("FACETEST");

  auto diagsCap = transceiverManager_->getDiagsCapability(xcvrID);
  EXPECT_TRUE(diagsCap.has_value());
  std::vector<prbs::PrbsPolynomial> expectedSysPolynomials = {
      prbs::PrbsPolynomial::PRBS31Q,
      prbs::PrbsPolynomial::PRBS23Q,
      prbs::PrbsPolynomial::PRBS15Q,
      prbs::PrbsPolynomial::PRBS13Q,
      prbs::PrbsPolynomial::PRBS9Q,
      prbs::PrbsPolynomial::PRBS7Q,
      prbs::PrbsPolynomial::PRBS31,
      prbs::PrbsPolynomial::PRBS23,
      prbs::PrbsPolynomial::PRBS15,
      prbs::PrbsPolynomial::PRBS13,
      prbs::PrbsPolynomial::PRBS9,
      prbs::PrbsPolynomial::PRBS7};
  std::vector<prbs::PrbsPolynomial> expectedLinePolynomials = {
      prbs::PrbsPolynomial::PRBS31Q,
      prbs::PrbsPolynomial::PRBS23Q,
      prbs::PrbsPolynomial::PRBS15Q,
      prbs::PrbsPolynomial::PRBS13Q,
      prbs::PrbsPolynomial::PRBS9Q,
      prbs::PrbsPolynomial::PRBS7Q,
      prbs::PrbsPolynomial::PRBS31,
      prbs::PrbsPolynomial::PRBS23,
      prbs::PrbsPolynomial::PRBS15,
      prbs::PrbsPolynomial::PRBS13,
      prbs::PrbsPolynomial::PRBS9,
      prbs::PrbsPolynomial::PRBS7};

  auto linePrbsCapability = *(*diagsCap).prbsLineCapabilities();
  auto sysPrbsCapability = *(*diagsCap).prbsSystemCapabilities();

  tests.verifyPrbsPolynomials(expectedLinePolynomials, linePrbsCapability);
  tests.verifyPrbsPolynomials(expectedSysPolynomials, sysPrbsCapability);

  for (auto unsupportedApplication :
       {SMFMediaInterfaceCode::LR4_10_400G,
        SMFMediaInterfaceCode::FR4_400G,
        SMFMediaInterfaceCode::FR1_100G,
        SMFMediaInterfaceCode::FR4_200G,
        SMFMediaInterfaceCode::CWDM4_100G}) {
    EXPECT_EQ(
        xcvr->getApplicationField(
            static_cast<uint8_t>(unsupportedApplication), 0),
        std::nullopt);
  }

  for (auto supportedApplication : {SMFMediaInterfaceCode::DR4_400G}) {
    auto applicationField = xcvr->getApplicationField(
        static_cast<uint8_t>(supportedApplication), 0);
    EXPECT_NE(applicationField, std::nullopt);
    std::vector<int> expectedStartLanes = {0, 4};
    EXPECT_EQ(applicationField->hostStartLanes, expectedStartLanes);
    EXPECT_EQ(applicationField->mediaStartLanes, expectedStartLanes);
  }

  EXPECT_TRUE(diagsCap.value().vdm().value());
  EXPECT_TRUE(diagsCap.value().cdb().value());
  EXPECT_TRUE(diagsCap.value().prbsLine().value());
  EXPECT_TRUE(diagsCap.value().prbsSystem().value());
  EXPECT_TRUE(diagsCap.value().loopbackLine().value());
  EXPECT_TRUE(diagsCap.value().loopbackSystem().value());
  EXPECT_TRUE(diagsCap.value().txOutputControl().value());
  EXPECT_TRUE(diagsCap.value().rxOutputControl().value());
  EXPECT_TRUE(diagsCap.value().snrLine().value());
  EXPECT_TRUE(diagsCap.value().snrSystem().value());
  EXPECT_TRUE(xcvr->isVdmSupported());
  EXPECT_TRUE(xcvr->isPrbsSupported(phy::Side::LINE));
  EXPECT_TRUE(xcvr->isPrbsSupported(phy::Side::SYSTEM));
  EXPECT_TRUE(xcvr->isSnrSupported(phy::Side::LINE));
  EXPECT_TRUE(xcvr->isSnrSupported(phy::Side::SYSTEM));
  EXPECT_TRUE(info.tcvrState()->errorStates()->empty());
}

TEST_F(CmisTest, vdmPam4MpiAlarmsTest) {
  // This test verifies that PAM4 MPI alarm flags are not raised by default
  auto xcvrID = TransceiverID(1);
  auto xcvr = overrideCmisModule<Cmis2x400GFr4Transceiver>(
      xcvrID, TransceiverModuleIdentifier::OSFP);
  const auto& info = xcvr->getTransceiverInfo();
  EXPECT_TRUE(info.tcvrState()->transceiverManagementInterface());
  EXPECT_EQ(
      info.tcvrState()->transceiverManagementInterface(),
      TransceiverManagementInterface::CMIS);
  EXPECT_EQ(xcvr->numHostLanes(), 8);
  EXPECT_EQ(xcvr->numMediaLanes(), 8);
  EXPECT_EQ(
      info.tcvrState()->moduleMediaInterface(), MediaInterfaceCode::FR4_2x400G);
  // Verify that VDM group 3 is supported
  EXPECT_TRUE(xcvr->isVdmSupported());
  EXPECT_TRUE(xcvr->isVdmSupported(3));

  ProgramTransceiverState programTcvrState;
  TransceiverPortState portState;
  portState.portName = "eth1/1/1";
  portState.startHostLane = 0;
  portState.speed = cfg::PortSpeed::FOURHUNDREDG;
  portState.numHostLanes = 4;
  programTcvrState.ports.emplace(portState.portName, portState);
  portState.portName = "eth1/1/5";
  portState.startHostLane = 4;
  portState.speed = cfg::PortSpeed::FOURHUNDREDG;
  portState.numHostLanes = 4;
  programTcvrState.ports.emplace(portState.portName, portState);

  xcvr->programTransceiver(programTcvrState, false);

  // Trigger VDM stats capture
  std::vector<int32_t> xcvrIds = {xcvrID};
  transceiverManager_->triggerVdmStatsCapture(xcvrIds);

  transceiverManager_->refreshStateMachines();
  const auto& newInfo = xcvr->getTransceiverInfo();

  // Verify that VDM performance monitor stats are available
  ASSERT_TRUE(newInfo.tcvrStats()->vdmPerfMonitorStats().has_value());

  // Verify that we have stats for both ports
  ASSERT_EQ(
      newInfo.tcvrStats()
          ->vdmPerfMonitorStats()
          ->mediaPortVdmStats()
          .value()
          .size(),
      2);

  // Assert that there are 4 keys in lanePam4MPIFlags().value() for each port
  for (const auto& [portName, portStats] : newInfo.tcvrStats()
                                               ->vdmPerfMonitorStats()
                                               ->mediaPortVdmStats()
                                               .value()) {
    EXPECT_EQ(portStats.lanePam4MPIFlags().value().size(), 4);
  }

  // Check that PAM4 MPI flags are not raised by default
  for (const auto& [portName, portStats] : newInfo.tcvrStats()
                                               ->vdmPerfMonitorStats()
                                               ->mediaPortVdmStats()
                                               .value()) {
    // Check each lane's flags
    for (const auto& [lane, flags] : portStats.lanePam4MPIFlags().value()) {
      // Verify that the flags are not raised
      EXPECT_FALSE(*flags.alarm()->high());
      EXPECT_FALSE(*flags.warn()->high());

      // Low flags should always be false for MPI
      EXPECT_FALSE(*flags.alarm()->low());
      EXPECT_FALSE(*flags.warn()->low());
    }
  }
}

TEST_F(CmisTest, vdmPam4MpiAlarmsRaisedTest) {
  // This test verifies that PAM4 MPI alarm/warning flags are correctly read
  auto xcvrID = TransceiverID(1);
  auto xcvr = overrideCmisModule<Cmis2x400GFr4WithMpiAlarmsTransceiver>(
      xcvrID, TransceiverModuleIdentifier::OSFP);

  // Verify that VDM group 3 is supported
  EXPECT_TRUE(xcvr->isVdmSupported());
  EXPECT_TRUE(xcvr->isVdmSupported(3));

  ProgramTransceiverState programTcvrState;
  TransceiverPortState portState;
  portState.portName = "eth1/1/1";
  portState.startHostLane = 0;
  portState.speed = cfg::PortSpeed::FOURHUNDREDG;
  portState.numHostLanes = 4;
  programTcvrState.ports.emplace(portState.portName, portState);
  portState.portName = "eth1/1/5";
  portState.startHostLane = 4;
  portState.speed = cfg::PortSpeed::FOURHUNDREDG;
  portState.numHostLanes = 4;
  programTcvrState.ports.emplace(portState.portName, portState);

  xcvr->programTransceiver(programTcvrState, false);

  // Trigger VDM stats capture
  std::vector<int32_t> xcvrIds = {xcvrID};
  transceiverManager_->triggerVdmStatsCapture(xcvrIds);

  transceiverManager_->refreshStateMachines();

  const auto& info = xcvr->getTransceiverInfo();

  ASSERT_TRUE(info.tcvrStats()->vdmPerfMonitorStats().has_value());

  // Verify that we have stats for both ports
  ASSERT_EQ(
      info.tcvrStats()
          ->vdmPerfMonitorStats()
          ->mediaPortVdmStats()
          .value()
          .size(),
      2);

  // Check that PAM4 MPI flags are raised as expected
  for (const auto& [portName, portStats] :
       info.tcvrStats()->vdmPerfMonitorStats()->mediaPortVdmStats().value()) {
    // Check each lane's flags
    for (const auto& [lane, flags] : portStats.lanePam4MPIFlags().value()) {
      // Verify that the high alarm and high warning flags are raised
      EXPECT_TRUE(*flags.alarm()->high());
      EXPECT_TRUE(*flags.warn()->high());

      // Low flags should always be false for MPI
      EXPECT_FALSE(*flags.alarm()->low());
      EXPECT_FALSE(*flags.warn()->low());
    }
  }
}

TEST_F(CmisTest, cmis400GDr4TransceiverInfoTest) {
  auto xcvrID = TransceiverID(1);
  auto xcvr = overrideCmisModule<Cmis400GDr4Transceiver>(xcvrID);
  const auto& info = xcvr->getTransceiverInfo();
  EXPECT_TRUE(info.tcvrState()->transceiverManagementInterface());
  EXPECT_EQ(
      info.tcvrState()->transceiverManagementInterface(),
      TransceiverManagementInterface::CMIS);
  EXPECT_EQ(xcvr->numHostLanes(), 8);
  EXPECT_EQ(xcvr->numMediaLanes(), 4);
  EXPECT_EQ(
      info.tcvrState()->moduleMediaInterface(), MediaInterfaceCode::DR4_400G);
  for (auto& media : *info.tcvrState()->settings()->mediaInterface()) {
    EXPECT_EQ(media.media()->get_smfCode(), SMFMediaInterfaceCode::DR4_400G);
    EXPECT_EQ(media.code(), MediaInterfaceCode::DR4_400G);
  }
  // Check cmisStateChanged
  EXPECT_TRUE(
      info.tcvrState()->status() &&
      info.tcvrState()->status()->cmisStateChanged() &&
      *info.tcvrState()->status()->cmisStateChanged());

  utility::HwTransceiverUtils::verifyDiagsCapability(
      *info.tcvrState(),
      transceiverManager_->getDiagsCapability(xcvrID),
      false /* skipCheckingIndividualCapability */);

  TransceiverTestsHelper tests(info);
  tests.verifyVendorName("FACETEST");

  auto diagsCap = transceiverManager_->getDiagsCapability(xcvrID);
  EXPECT_TRUE(diagsCap.has_value());
  std::vector<prbs::PrbsPolynomial> expectedSysPolynomials = {
      prbs::PrbsPolynomial::PRBS31Q,
      prbs::PrbsPolynomial::PRBS23Q,
      prbs::PrbsPolynomial::PRBS15Q,
      prbs::PrbsPolynomial::PRBS13Q,
      prbs::PrbsPolynomial::PRBS9Q,
      prbs::PrbsPolynomial::PRBS7Q};
  std::vector<prbs::PrbsPolynomial> expectedLinePolynomials = {
      prbs::PrbsPolynomial::PRBS31Q,
      prbs::PrbsPolynomial::PRBS23Q,
      prbs::PrbsPolynomial::PRBS15Q,
      prbs::PrbsPolynomial::PRBS13Q,
      prbs::PrbsPolynomial::PRBS9Q,
      prbs::PrbsPolynomial::PRBS7Q};

  auto linePrbsCapability = *(*diagsCap).prbsLineCapabilities();
  auto sysPrbsCapability = *(*diagsCap).prbsSystemCapabilities();

  tests.verifyPrbsPolynomials(expectedLinePolynomials, linePrbsCapability);
  tests.verifyPrbsPolynomials(expectedSysPolynomials, sysPrbsCapability);

  for (auto unsupportedApplication :
       {SMFMediaInterfaceCode::CWDM4_100G,
        SMFMediaInterfaceCode::FR4_400G,
        SMFMediaInterfaceCode::LR4_10_400G,
        SMFMediaInterfaceCode::FR4_200G}) {
    EXPECT_EQ(
        xcvr->getApplicationField(
            static_cast<uint8_t>(unsupportedApplication), 0),
        std::nullopt);
  }
  for (auto supportedApplication : {SMFMediaInterfaceCode::DR4_400G}) {
    auto applicationField = xcvr->getApplicationField(
        static_cast<uint8_t>(supportedApplication), 0);
    EXPECT_NE(applicationField, std::nullopt);
    EXPECT_EQ(applicationField->hostStartLanes, std::vector<int>{0});
    EXPECT_EQ(applicationField->mediaStartLanes, std::vector<int>{0});
    for (uint8_t lane = 1; lane < 7; lane++) {
      EXPECT_EQ(
          xcvr->getApplicationField(
              static_cast<uint8_t>(supportedApplication), lane),
          std::nullopt);
    }
  }
  EXPECT_TRUE(info.tcvrState()->errorStates()->empty());
}

TEST_F(CmisTest, cmis2x800GDr4TransceiverInfoTest) {
  auto xcvrID = TransceiverID(1);
  auto xcvr = overrideCmisModule<Cmis2x800GDr4Transceiver>(
      xcvrID, TransceiverModuleIdentifier::OSFP);
  const auto& info = xcvr->getTransceiverInfo();
  EXPECT_TRUE(info.tcvrState()->transceiverManagementInterface());
  EXPECT_EQ(
      info.tcvrState()->transceiverManagementInterface(),
      TransceiverManagementInterface::CMIS);
  EXPECT_EQ(xcvr->numHostLanes(), 8);
  EXPECT_EQ(xcvr->numMediaLanes(), 8);
  EXPECT_EQ(
      info.tcvrState()->moduleMediaInterface(), MediaInterfaceCode::DR4_2x800G);
  for (auto& media : *info.tcvrState()->settings()->mediaInterface()) {
    EXPECT_EQ(media.media()->get_smfCode(), SMFMediaInterfaceCode::DR4_800G);
    EXPECT_EQ(media.code(), MediaInterfaceCode::DR4_800G);
  }

  // Check cmisStateChanged
  EXPECT_TRUE(
      info.tcvrState()->status() &&
      info.tcvrState()->status()->cmisStateChanged() &&
      *info.tcvrState()->status()->cmisStateChanged());

  utility::HwTransceiverUtils::verifyDiagsCapability(
      *info.tcvrState(),
      transceiverManager_->getDiagsCapability(xcvrID),
      false /* skipCheckingIndividualCapability */);

  TransceiverTestsHelper tests(info);
  tests.verifyVendorName("FACETEST");

  auto diagsCap = transceiverManager_->getDiagsCapability(xcvrID);
  EXPECT_TRUE(diagsCap.has_value());
  std::vector<prbs::PrbsPolynomial> expectedSysPolynomials = {
      prbs::PrbsPolynomial::PRBS31Q,
      prbs::PrbsPolynomial::PRBS23Q,
      prbs::PrbsPolynomial::PRBS13Q,
  };
  std::vector<prbs::PrbsPolynomial> expectedLinePolynomials = {
      prbs::PrbsPolynomial::PRBS31Q,
      prbs::PrbsPolynomial::PRBS23Q,
      prbs::PrbsPolynomial::PRBS15Q,
      prbs::PrbsPolynomial::PRBS13Q,
  };

  auto linePrbsCapability = *(*diagsCap).prbsLineCapabilities();
  auto sysPrbsCapability = *(*diagsCap).prbsSystemCapabilities();

  tests.verifyPrbsPolynomials(expectedLinePolynomials, linePrbsCapability);
  tests.verifyPrbsPolynomials(expectedSysPolynomials, sysPrbsCapability);

  for (auto unsupportedApplication :
       {SMFMediaInterfaceCode::LR4_10_400G,
        SMFMediaInterfaceCode::FR4_400G,
        SMFMediaInterfaceCode::FR1_100G,
        SMFMediaInterfaceCode::FR4_200G,
        SMFMediaInterfaceCode::CWDM4_100G}) {
    EXPECT_EQ(
        xcvr->getApplicationField(
            static_cast<uint8_t>(unsupportedApplication), 0),
        std::nullopt);
  }

  for (auto supportedApplication : {SMFMediaInterfaceCode::DR4_800G}) {
    auto applicationField = xcvr->getApplicationField(
        static_cast<uint8_t>(supportedApplication), 0);
    EXPECT_NE(applicationField, std::nullopt);
    std::vector<int> expectedStartLanes = {0, 4};
    EXPECT_EQ(applicationField->hostStartLanes, expectedStartLanes);
    EXPECT_EQ(applicationField->mediaStartLanes, expectedStartLanes);
  }

  EXPECT_TRUE(diagsCap.value().vdm().value());
  EXPECT_TRUE(diagsCap.value().cdb().value());
  EXPECT_TRUE(diagsCap.value().prbsLine().value());
  EXPECT_TRUE(diagsCap.value().prbsSystem().value());
  EXPECT_TRUE(diagsCap.value().loopbackLine().value());
  EXPECT_TRUE(diagsCap.value().loopbackSystem().value());
  EXPECT_TRUE(diagsCap.value().txOutputControl().value());
  EXPECT_TRUE(diagsCap.value().rxOutputControl().value());
  EXPECT_TRUE(diagsCap.value().snrLine().value());
  EXPECT_TRUE(diagsCap.value().snrSystem().value());
  EXPECT_TRUE(xcvr->isVdmSupported());
  EXPECT_TRUE(xcvr->isPrbsSupported(phy::Side::LINE));
  EXPECT_TRUE(xcvr->isPrbsSupported(phy::Side::SYSTEM));
  EXPECT_TRUE(xcvr->isSnrSupported(phy::Side::LINE));
  EXPECT_TRUE(xcvr->isSnrSupported(phy::Side::SYSTEM));
  EXPECT_TRUE(info.tcvrState()->errorStates()->empty());
}

TEST_F(CmisTest, cmisInvalidDatapathTransceiverInfoTest) {
  auto xcvrID = TransceiverID(1);
  auto xcvr = overrideCmisModule<InvalidDatapathLaneStateTransceiver>(
      xcvrID, TransceiverModuleIdentifier::OSFP);
  const auto& info = xcvr->getTransceiverInfo();
  EXPECT_TRUE(info.tcvrState()->transceiverManagementInterface());
  std::set<TransceiverErrorState> expectedErrorStates = {
      TransceiverErrorState::INVALID_DATA_PATH_LANE_STATE};
  EXPECT_EQ(info.tcvrState()->errorStates(), expectedErrorStates);
}
} // namespace facebook::fboss
