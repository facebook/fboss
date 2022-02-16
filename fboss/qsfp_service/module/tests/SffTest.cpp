/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include <folly/Conv.h>
#include <folly/Memory.h>
#include <gflags/gflags.h>
#include <glog/logging.h>
#include <cstdint>
#include "fboss/qsfp_service/module/QsfpModule.h"
#include "fboss/qsfp_service/module/sff/Sff8472Module.h"
#include "fboss/qsfp_service/module/sff/SffModule.h"
#include "fboss/qsfp_service/module/tests/FakeTransceiverImpl.h"
#include "fboss/qsfp_service/module/tests/TransceiverTestsHelper.h"

#include <gtest/gtest.h>

using namespace facebook::fboss;
using std::make_unique;

namespace {

// Tests that the transceiverInfo object is correctly populated
TEST(SffTest, transceiverInfoTest) {
  int idx = 1;
  std::unique_ptr<SffCwdm4Transceiver> qsfpImpl =
      std::make_unique<SffCwdm4Transceiver>(idx);
  std::unique_ptr<SffModule> qsfp =
      std::make_unique<SffModule>(nullptr, std::move(qsfpImpl), 4);

  qsfp->refresh();

  TransceiverInfo info = qsfp->getTransceiverInfo();
  TransceiverTestsHelper tests(info);

  tests.verifyVendorName("FACETEST");
  EXPECT_EQ(
      *info.extendedSpecificationComplianceCode_ref(),
      ExtendedSpecComplianceCode::CWDM4_100G);
  tests.verifyTemp(31.015625);
  tests.verifyVcc(3.2989);
  std::map<std::string, std::vector<double>> laneDom = {
      {"TxBias", {10.28, 8.738, 17.476, 26.214}},
      {"TxPwr", {1.6705, 1.6962, 1.7219, 1.7476}},
      {"RxPwr", {0.4643, 0.9012, 0.4913, 0.4626}},
  };
  tests.verifyLaneDom(laneDom, qsfp->numMediaLanes());
  EXPECT_EQ(100, info.cable_ref().value_or({}).om3_ref().value_or({}));

  std::map<std::string, std::vector<bool>> expectedMediaSignals = {
      {"Tx_Los", {0, 1, 0, 1}},
      {"Rx_Los", {1, 1, 0, 1}},
      {"Tx_Lol", {0, 0, 1, 1}},
      {"Rx_Lol", {1, 0, 1, 1}},
      {"Tx_Fault", {0, 0, 1, 1}},
      {"Tx_AdaptFault", {1, 1, 0, 1}},
  };
  tests.verifyMediaLaneSignals(expectedMediaSignals, qsfp->numMediaLanes());

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

  auto settings = info.settings_ref().value_or({});
  tests.verifyMediaLaneSettings(
      expectedMediaLaneSettings, qsfp->numMediaLanes());
  tests.verifyHostLaneSettings(expectedHostLaneSettings, qsfp->numHostLanes());

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
  tests.verifyLaneInterrupts(laneInterrupts, qsfp->numMediaLanes());

  tests.verifyGlobalInterrupts("temp", 1, 1, 0, 0);
  tests.verifyGlobalInterrupts("vcc", 0, 0, 1, 1);

  tests.verifyThresholds("temp", 75, -5, 70, 0);
  tests.verifyThresholds("vcc", 3.80, 2.84, 3.45, 3.15);
  tests.verifyThresholds("rxPwr", 0.6683, 0.7197, 0.7711, 0.437);
  tests.verifyThresholds("txPwr", 0.4386, 1.3124, 2.1862, 3.06);
  tests.verifyThresholds("txBias", 0.034, 17.51, 34.986, 52.462);

  EXPECT_EQ(true, info.status_ref().value_or({}).interruptL_ref().value_or({}));
  EXPECT_EQ(qsfp->numHostLanes(), 4);
  EXPECT_EQ(qsfp->numMediaLanes(), 4);

  for (auto& media : *info.settings_ref()->mediaInterface_ref()) {
    EXPECT_EQ(
        media.media_ref()->get_extendedSpecificationComplianceCode(),
        ExtendedSpecComplianceCode::CWDM4_100G);
    EXPECT_EQ(media.code_ref(), MediaInterfaceCode::CWDM4_100G);
  }
  EXPECT_EQ(info.moduleMediaInterface_ref(), MediaInterfaceCode::CWDM4_100G);
  testCachedMediaSignals(qsfp.get());
  EXPECT_EQ(qsfp->moduleDiagsCapabilityGet(), std::nullopt);
}

// Tests that a SFF DAC module can properly refresh
TEST(SffDacTest, transceiverInfoTest) {
  int idx = 1;
  std::unique_ptr<SffDacTransceiver> qsfpImpl =
      std::make_unique<SffDacTransceiver>(idx);
  std::unique_ptr<SffModule> qsfp =
      std::make_unique<SffModule>(nullptr, std::move(qsfpImpl), 4);

  qsfp->refresh();
  TransceiverInfo info = qsfp->getTransceiverInfo();
  TransceiverTestsHelper tests(info);

  tests.verifyVendorName("FACETEST");
  EXPECT_EQ(
      *info.extendedSpecificationComplianceCode_ref(),
      ExtendedSpecComplianceCode::CR4_100G);
  EXPECT_EQ(qsfp->numHostLanes(), 4);
  EXPECT_EQ(qsfp->numMediaLanes(), 4);
  for (auto& media : *info.settings_ref()->mediaInterface_ref()) {
    EXPECT_EQ(
        media.media_ref()->get_extendedSpecificationComplianceCode(),
        ExtendedSpecComplianceCode::CR4_100G);
    EXPECT_EQ(media.code_ref(), MediaInterfaceCode::CR4_100G);
  }
  EXPECT_EQ(info.moduleMediaInterface_ref(), MediaInterfaceCode::CR4_100G);
  testCachedMediaSignals(qsfp.get());
  EXPECT_EQ(qsfp->moduleDiagsCapabilityGet(), std::nullopt);
}

// Tests that a SFF Fr1 module can properly refresh
TEST(SffFr1Test, transceiverInfoTest) {
  int idx = 1;
  std::unique_ptr<SffFr1Transceiver> qsfpImpl =
      std::make_unique<SffFr1Transceiver>(idx);
  std::unique_ptr<SffModule> qsfp =
      std::make_unique<SffModule>(nullptr, std::move(qsfpImpl), 4);

  qsfp->refresh();
  TransceiverInfo info = qsfp->getTransceiverInfo();
  TransceiverTestsHelper tests(info);

  tests.verifyVendorName("FACETEST");
  EXPECT_EQ(
      *info.extendedSpecificationComplianceCode_ref(),
      ExtendedSpecComplianceCode::FR1_100G);
  EXPECT_EQ(qsfp->numHostLanes(), 4);
  EXPECT_EQ(qsfp->numMediaLanes(), 1);
  for (auto& media : *info.settings_ref()->mediaInterface_ref()) {
    EXPECT_EQ(
        media.media_ref()->get_extendedSpecificationComplianceCode(),
        ExtendedSpecComplianceCode::FR1_100G);
    EXPECT_EQ(media.code_ref(), MediaInterfaceCode::FR1_100G);
  }
  EXPECT_EQ(info.moduleMediaInterface_ref(), MediaInterfaceCode::FR1_100G);
  testCachedMediaSignals(qsfp.get());

  auto diagsCap = qsfp->moduleDiagsCapabilityGet();
  EXPECT_TRUE(diagsCap);
  EXPECT_TRUE(*(*diagsCap).prbsLine_ref());
  EXPECT_TRUE(*(*diagsCap).prbsSystem_ref());
  auto prbsSystemCaps = (*diagsCap).get_prbsSystemCapabilities();
  auto prbsLineCaps = (*diagsCap).get_prbsLineCapabilities();
  std::vector<prbs::PrbsPolynomial> expectedCapabilities = {
      prbs::PrbsPolynomial::PRBS31};
  EXPECT_TRUE(std::equal(
      prbsSystemCaps.begin(),
      prbsSystemCaps.end(),
      expectedCapabilities.begin()));
  EXPECT_TRUE(std::equal(
      prbsLineCaps.begin(), prbsLineCaps.end(), expectedCapabilities.begin()));
}

// Tests that a miniphoton module can properly refresh
TEST(SffMiniphotonTest, transceiverInfoTest) {
  int idx = 1;
  std::unique_ptr<MiniphotonOBOTransceiver> qsfpImpl =
      std::make_unique<MiniphotonOBOTransceiver>(idx);
  std::unique_ptr<SffModule> qsfp =
      std::make_unique<SffModule>(nullptr, std::move(qsfpImpl), 4);

  qsfp->refresh();
  TransceiverInfo info = qsfp->getTransceiverInfo();
  TransceiverTestsHelper tests(info);

  tests.verifyVendorName("FACETEST");
  EXPECT_EQ(
      *info.extendedSpecificationComplianceCode_ref(),
      ExtendedSpecComplianceCode::CWDM4_100G);
  EXPECT_EQ(qsfp->numHostLanes(), 4);
  EXPECT_EQ(qsfp->numMediaLanes(), 4);
  for (auto& media : *info.settings_ref()->mediaInterface_ref()) {
    EXPECT_EQ(
        media.media_ref()->get_extendedSpecificationComplianceCode(),
        ExtendedSpecComplianceCode::CWDM4_100G);
    EXPECT_EQ(media.code_ref(), MediaInterfaceCode::CWDM4_100G);
  }
  EXPECT_EQ(info.moduleMediaInterface_ref(), MediaInterfaceCode::CWDM4_100G);
  EXPECT_EQ(qsfp->moduleDiagsCapabilityGet(), std::nullopt);
}

TEST(UnknownModuleIdentifierTest, transceiverInfoTest) {
  int idx = 1;
  std::unique_ptr<UnknownModuleIdentifierTransceiver> qsfpImpl =
      std::make_unique<UnknownModuleIdentifierTransceiver>(idx);
  std::unique_ptr<SffModule> qsfp =
      std::make_unique<SffModule>(nullptr, std::move(qsfpImpl), 4);

  qsfp->refresh();
  TransceiverInfo info = qsfp->getTransceiverInfo();
  TransceiverTestsHelper tests(info);

  tests.verifyVendorName("FACETEST");
  EXPECT_EQ(
      *info.extendedSpecificationComplianceCode_ref(),
      ExtendedSpecComplianceCode::CWDM4_100G);
  EXPECT_EQ(qsfp->numHostLanes(), 4);
  EXPECT_EQ(qsfp->numMediaLanes(), 4);
  for (auto& media : *info.settings_ref()->mediaInterface_ref()) {
    EXPECT_EQ(
        media.media_ref()->get_extendedSpecificationComplianceCode(),
        ExtendedSpecComplianceCode::CWDM4_100G);
    EXPECT_EQ(media.code_ref(), MediaInterfaceCode::CWDM4_100G);
  }
  EXPECT_EQ(info.moduleMediaInterface_ref(), MediaInterfaceCode::CWDM4_100G);
}

// Tests that a badly programmed module throws an exception
TEST(BadSffTest, simpleRead) {
  int idx = 1;
  std::unique_ptr<BadSffCwdm4Transceiver> qsfpImpl =
      std::make_unique<BadSffCwdm4Transceiver>(idx);
  std::unique_ptr<SffModule> qsfp =
      std::make_unique<SffModule>(nullptr, std::move(qsfpImpl), 4);

  EXPECT_THROW(qsfp->refresh(), QsfpModuleError);
}

// Tests that a SFP module can properly refresh
TEST(SfpTest, transceiverInfoTest) {
  int idx = 1;
  std::unique_ptr<Sfp10GTransceiver> qsfpImpl =
      std::make_unique<Sfp10GTransceiver>(idx);
  std::unique_ptr<Sff8472Module> sfp =
      std::make_unique<Sff8472Module>(nullptr, std::move(qsfpImpl), 1);

  sfp->refresh();
  TransceiverInfo info = sfp->getTransceiverInfo();
  TransceiverTestsHelper tests(info);

  EXPECT_EQ(sfp->numHostLanes(), 1);
  EXPECT_EQ(sfp->numMediaLanes(), 1);
  EXPECT_EQ(
      (*info.settings_ref()->mediaInterface_ref()).size(),
      sfp->numMediaLanes());
  for (auto& media : *info.settings_ref()->mediaInterface_ref()) {
    EXPECT_EQ(
        media.media_ref()->get_ethernet10GComplianceCode(),
        Ethernet10GComplianceCode::LR_10G);
    EXPECT_EQ(media.code_ref(), MediaInterfaceCode::LR_10G);
  }
  tests.verifyTemp(18.203125);
  tests.verifyVcc(2.2136);
  std::map<std::string, std::vector<double>> laneDom = {
      {"TxBias", {79.224}},
      {"TxPwr", {5.6849}},
      {"RxPwr", {0.8755}},
  };
  tests.verifyLaneDom(laneDom, sfp->numMediaLanes());

  std::map<std::string, std::vector<bool>> expectedMediaSignals = {
      {"Rx_Los", {1}},
      {"Tx_Fault", {1}},
  };
  tests.verifyMediaLaneSignals(expectedMediaSignals, sfp->numMediaLanes());

  std::map<std::string, std::vector<bool>> expectedMediaLaneSettings = {
      {"TxDisable", {1}},
  };
  tests.verifyMediaLaneSettings(
      expectedMediaLaneSettings, sfp->numMediaLanes());
  tests.verifyVendorName("FACETEST");
  EXPECT_EQ(info.moduleMediaInterface_ref(), MediaInterfaceCode::LR_10G);
  EXPECT_EQ(sfp->moduleDiagsCapabilityGet(), std::nullopt);
}

TEST(SfpTest, moduleEepromChecksumTest) {
  // Create SFF FR1 module
  int idx = 1;
  std::unique_ptr<SffFr1Transceiver> qsfpImplSff100GFr1 =
      std::make_unique<SffFr1Transceiver>(idx);
  std::unique_ptr<SffModule> qsfp100GFr1 =
      std::make_unique<SffModule>(nullptr, std::move(qsfpImplSff100GFr1), 4);

  qsfp100GFr1->refresh();
  // Verify EEPROM checksum for SFF FR1 module
  bool csumValid = qsfp100GFr1->verifyEepromChecksums();
  EXPECT_TRUE(csumValid);

  // Create CWDM4 module
  idx = 2;
  std::unique_ptr<SffCwdm4Transceiver> qsfpImplCwdm4 =
      std::make_unique<SffCwdm4Transceiver>(idx);
  std::unique_ptr<SffModule> qsfpCwdm4 =
      std::make_unique<SffModule>(nullptr, std::move(qsfpImplCwdm4), 4);

  qsfpCwdm4->refresh();
  // Verify EEPROM checksum for CWDM4 module
  csumValid = qsfpCwdm4->verifyEepromChecksums();
  EXPECT_TRUE(csumValid);

  // Create CWDM4 Bad module
  idx = 3;
  std::unique_ptr<BadEepromSffCwdm4Transceiver> qsfpImplCwdm4Bad =
      std::make_unique<BadEepromSffCwdm4Transceiver>(idx);
  std::unique_ptr<SffModule> qsfpCwdm4Bad =
      std::make_unique<SffModule>(nullptr, std::move(qsfpImplCwdm4Bad), 4);

  qsfpCwdm4Bad->refresh();
  // Verify EEPROM checksum Invalid for CWDM4 Bad module
  csumValid = qsfpCwdm4Bad->verifyEepromChecksums();
  EXPECT_FALSE(csumValid);
}

} // namespace
