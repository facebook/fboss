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
#include "fboss/qsfp_service/module/cmis/CmisModule.h"
#include "fboss/qsfp_service/module/tests/FakeTransceiverImpl.h"
#include "fboss/qsfp_service/module/tests/TransceiverTestsHelper.h"

#include <gtest/gtest.h>

using namespace facebook::fboss;
using std::make_unique;

namespace {

// Tests that the transceiverInfo object is correctly populated
TEST(CmisTest, transceiverInfoTest) {
  int idx = 1;
  std::unique_ptr<Cmis200GTransceiver> qsfpImpl =
      std::make_unique<Cmis200GTransceiver>(idx);

  std::unique_ptr<CmisModule> xcvr =
      std::make_unique<CmisModule>(nullptr, std::move(qsfpImpl), 4);
  xcvr->refresh();

  TransceiverInfo info = xcvr->getTransceiverInfo();

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
  tests.verifyMediaLaneSignals(expectedMediaSignals, xcvr->numMediaLanes());

  std::array<bool, 4> expectedDatapathDeinit = {0, 1, 1, 0};
  std::array<CmisLaneState, 4> expectedLaneState = {
      CmisLaneState::ACTIVATED,
      CmisLaneState::DATAPATHINIT,
      CmisLaneState::TX_ON,
      CmisLaneState::DEINIT};

  EXPECT_EQ(
      xcvr->numHostLanes(), info.hostLaneSignals_ref().value_or({}).size());
  for (auto& signal : *info.hostLaneSignals_ref()) {
    EXPECT_EQ(
        expectedDatapathDeinit[*signal.lane_ref()],
        signal.dataPathDeInit_ref().value_or({}));
    EXPECT_EQ(
        expectedLaneState[*signal.lane_ref()],
        signal.cmisLaneState_ref().value_or({}));
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

  auto settings = info.settings_ref().value_or({});
  tests.verifyMediaLaneSettings(
      expectedMediaLaneSettings, xcvr->numMediaLanes());
  tests.verifyHostLaneSettings(expectedHostLaneSettings, xcvr->numHostLanes());

  EXPECT_EQ(
      PowerControlState::HIGH_POWER_OVERRIDE, settings.powerControl_ref());

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

  EXPECT_EQ(
      xcvr->numMediaLanes(),
      info.settings_ref()->mediaInterface_ref().value_or({}).size());
  for (auto& media : *info.settings_ref()->mediaInterface_ref()) {
    EXPECT_EQ(
        media.media_ref()->get_smfCode(), SMFMediaInterfaceCode::FR4_200G);
  }
  testCachedMediaSignals(xcvr.get());

  auto diagsCap = xcvr->moduleDiagsCapabilityGet();
  EXPECT_TRUE(diagsCap.has_value());
  EXPECT_TRUE(*diagsCap->diagnostics_ref());
  EXPECT_FALSE(*diagsCap->vdm_ref());
  EXPECT_TRUE(*diagsCap->cdb_ref());
  EXPECT_FALSE(*diagsCap->prbsLine_ref());
  EXPECT_FALSE(*diagsCap->prbsSystem_ref());
  EXPECT_FALSE(*diagsCap->loopbackLine_ref());
  EXPECT_TRUE(*diagsCap->loopbackSystem_ref());
  EXPECT_EQ(xcvr->numHostLanes(), 4);
  EXPECT_EQ(xcvr->numMediaLanes(), 4);
}

// MSM: Not_Present -> Present -> Discovered -> Inactive (on Agent timeout
//      event)
TEST(CmisTest, testStateToInactive) {
  int idx = 1;
  std::unique_ptr<Cmis200GTransceiver> qsfpImpl =
      std::make_unique<Cmis200GTransceiver>(idx);

  std::unique_ptr<CmisModule> qsfpMod =
      std::make_unique<CmisModule>(nullptr, std::move(qsfpImpl), 4);

  qsfpMod->moduleStateMachine_.get_attribute(qsfpModuleObjPtr) = qsfpMod.get();

  // MSM: Not_Present -> Present
  qsfpMod->stateUpdate(ModuleStateMachineEvent::OPTICS_DETECTED);
  int CurrState = qsfpMod->moduleStateMachine_.current_state()[0];
  EXPECT_EQ(CurrState, 1);

  // MSM: Present -> Discovered
  qsfpMod->stateUpdate(ModuleStateMachineEvent::EEPROM_READ);
  qsfpMod->refresh();
  CurrState = qsfpMod->moduleStateMachine_.current_state()[0];
  EXPECT_EQ(CurrState, 2);

  // MSM: Discovered -> Inactive (on Agent timeout event)
  qsfpMod->stateUpdate(ModuleStateMachineEvent::AGENT_SYNC_TIMEOUT);
  CurrState = qsfpMod->moduleStateMachine_.current_state()[0];
  EXPECT_EQ(CurrState, 3);
}

// MSM: Not_Present -> Present -> Discovered -> Inactive (On Agent timeout)
// Note: This test waits for 2 minutes to allow state machine transition to
//       new state
TEST(CmisTest, testStateTimeoutWaitInactive) {
  int idx = 1;
  std::unique_ptr<Cmis200GTransceiver> qsfpImpl =
      std::make_unique<Cmis200GTransceiver>(idx);

  std::unique_ptr<CmisModule> qsfpMod =
      std::make_unique<CmisModule>(nullptr, std::move(qsfpImpl), 4);

  qsfpMod->moduleStateMachine_.get_attribute(qsfpModuleObjPtr) = qsfpMod.get();

  // MSM: Not_Present -> Present -> Discovered -> Inactive (On Agent timeout)
  qsfpMod->stateUpdate(ModuleStateMachineEvent::OPTICS_DETECTED);

  qsfpMod->stateUpdate(ModuleStateMachineEvent::EEPROM_READ);
  qsfpMod->refresh();

  // Wait for 2 minutes for Agent sync timeout to happen and module SM
  // to transition to Inactive
  /* sleep override */
  sleep(125);
  int CurrState = qsfpMod->moduleStateMachine_.current_state()[0];
  EXPECT_EQ(CurrState, 3);
}

// MSM: Not_Present -> Present -> Discovered -> Inactive <-> Active
TEST(CmisTest, testPsmStates) {
  int idx = 1;
  std::unique_ptr<Cmis200GTransceiver> qsfpImpl =
      std::make_unique<Cmis200GTransceiver>(idx);

  std::unique_ptr<CmisModule> qsfpMod =
      std::make_unique<CmisModule>(nullptr, std::move(qsfpImpl), 4);

  qsfpMod->moduleStateMachine_.get_attribute(qsfpModuleObjPtr) = qsfpMod.get();

  // MSM: Not_Present -> Present -> Discovered -> Inactive (all port down)
  qsfpMod->stateUpdate(ModuleStateMachineEvent::OPTICS_DETECTED);
  qsfpMod->stateUpdate(ModuleStateMachineEvent::EEPROM_READ);
  qsfpMod->refresh();

  qsfpMod->portStateMachines_[0].process_event(
      MODULE_PORT_EVENT_AGENT_PORT_DOWN);
  qsfpMod->portStateMachines_[1].process_event(
      MODULE_PORT_EVENT_AGENT_PORT_DOWN);
  qsfpMod->portStateMachines_[2].process_event(
      MODULE_PORT_EVENT_AGENT_PORT_DOWN);
  qsfpMod->portStateMachines_[3].process_event(
      MODULE_PORT_EVENT_AGENT_PORT_DOWN);
  int CurrState = qsfpMod->moduleStateMachine_.current_state()[0];
  EXPECT_EQ(CurrState, 3);

  // One port Up -> Active
  qsfpMod->portStateMachines_[3].process_event(MODULE_PORT_EVENT_AGENT_PORT_UP);
  CurrState = qsfpMod->moduleStateMachine_.current_state()[0];
  EXPECT_EQ(CurrState, 4);

  // All ports down -> Inactive state
  qsfpMod->portStateMachines_[3].process_event(
      MODULE_PORT_EVENT_AGENT_PORT_DOWN);
  CurrState = qsfpMod->moduleStateMachine_.current_state()[0];
  EXPECT_EQ(CurrState, 3);

  // All port up -> Active state
  qsfpMod->portStateMachines_[0].process_event(MODULE_PORT_EVENT_AGENT_PORT_UP);
  qsfpMod->portStateMachines_[1].process_event(MODULE_PORT_EVENT_AGENT_PORT_UP);
  qsfpMod->portStateMachines_[2].process_event(MODULE_PORT_EVENT_AGENT_PORT_UP);
  qsfpMod->portStateMachines_[3].process_event(MODULE_PORT_EVENT_AGENT_PORT_UP);
  CurrState = qsfpMod->moduleStateMachine_.current_state()[0];
  EXPECT_EQ(CurrState, 4);

  // One port down, rest of ports up -> Active state
  qsfpMod->portStateMachines_[0].process_event(
      MODULE_PORT_EVENT_AGENT_PORT_DOWN);
  CurrState = qsfpMod->moduleStateMachine_.current_state()[0];
  EXPECT_EQ(CurrState, 4);
}

// MSM: Not_Present -> Present -> Discovered -> Inactive (Remediation)
TEST(CmisTest, testPsmRemediation) {
  int idx = 1;
  std::unique_ptr<Cmis200GTransceiver> qsfpImpl =
      std::make_unique<Cmis200GTransceiver>(idx);

  std::unique_ptr<CmisModule> qsfpMod =
      std::make_unique<CmisModule>(nullptr, std::move(qsfpImpl), 2);

  qsfpMod->moduleStateMachine_.get_attribute(qsfpModuleObjPtr) = qsfpMod.get();

  // MSM: Not_Present -> Present -> Discovered -> Inactive (all port down)
  qsfpMod->stateUpdate(ModuleStateMachineEvent::OPTICS_DETECTED);
  qsfpMod->stateUpdate(ModuleStateMachineEvent::EEPROM_READ);
  qsfpMod->refresh();

  EXPECT_EQ(qsfpMod->portStateMachines_.size(), 2);

  qsfpMod->portStateMachines_[0].process_event(
      MODULE_PORT_EVENT_AGENT_PORT_DOWN);
  qsfpMod->portStateMachines_[1].process_event(
      MODULE_PORT_EVENT_AGENT_PORT_DOWN);
  int CurrState = qsfpMod->moduleStateMachine_.current_state()[0];
  EXPECT_EQ(CurrState, 3);

  // Bring up done event -> Inactive state
  qsfpMod->stateUpdate(ModuleStateMachineEvent::BRINGUP_DONE);
  CurrState = qsfpMod->moduleStateMachine_.current_state()[0];
  EXPECT_EQ(CurrState, 3);

  // Remediation done event -> Inactive state
  qsfpMod->stateUpdate(ModuleStateMachineEvent::REMEDIATE_DONE);
  CurrState = qsfpMod->moduleStateMachine_.current_state()[0];
  EXPECT_EQ(CurrState, 3);
}

// MSM: Not_Present -> Present -> Discovered -> Inactive -> Upgrading
TEST(CmisTest, testMsmFwUpgrading) {
  int idx = 1;
  std::unique_ptr<Cmis200GTransceiver> qsfpImpl =
      std::make_unique<Cmis200GTransceiver>(idx);

  std::unique_ptr<CmisModule> qsfpMod =
      std::make_unique<CmisModule>(nullptr, std::move(qsfpImpl), 2);

  qsfpMod->moduleStateMachine_.get_attribute(qsfpModuleObjPtr) = qsfpMod.get();

  // MSM: Not_Present -> Present -> Discovered -> Inactive (all port down)
  qsfpMod->stateUpdate(ModuleStateMachineEvent::OPTICS_DETECTED);
  qsfpMod->stateUpdate(ModuleStateMachineEvent::EEPROM_READ);
  qsfpMod->refresh();

  qsfpMod->portStateMachines_[0].process_event(
      MODULE_PORT_EVENT_AGENT_PORT_DOWN);
  qsfpMod->portStateMachines_[1].process_event(MODULE_PORT_EVENT_AGENT_PORT_UP);
  int CurrState = qsfpMod->moduleStateMachine_.current_state()[0];
  EXPECT_EQ(CurrState, 4);

  // Upgrading event should get rejected
  qsfpMod->stateUpdate(ModuleStateMachineEvent::TRIGGER_UPGRADE);
  CurrState = qsfpMod->moduleStateMachine_.current_state()[0];
  EXPECT_EQ(CurrState, 4);

  // Bring module to Inactive state
  qsfpMod->portStateMachines_[1].process_event(
      MODULE_PORT_EVENT_AGENT_PORT_DOWN);
  CurrState = qsfpMod->moduleStateMachine_.current_state()[0];
  EXPECT_EQ(CurrState, 3);

  // Verify Upgrading event accepted and new state is Upgrading
  qsfpMod->stateUpdate(ModuleStateMachineEvent::TRIGGER_UPGRADE);
  CurrState = qsfpMod->moduleStateMachine_.current_state()[0];
  EXPECT_EQ(CurrState, 5);

  // Finish upgrading with module reset and move it to first state
  qsfpMod->stateUpdate(ModuleStateMachineEvent::OPTICS_RESET);
  CurrState = qsfpMod->moduleStateMachine_.current_state()[0];
  EXPECT_EQ(CurrState, 0);
}

// MSM: Not_Present -> Present -> Discovered -> Inactive -> Upgrading (Forced)
TEST(CmisTest, testMsmFwForceUpgrade) {
  int idx = 1;
  std::unique_ptr<Cmis200GTransceiver> qsfpImpl =
      std::make_unique<Cmis200GTransceiver>(idx);

  std::unique_ptr<CmisModule> qsfpMod =
      std::make_unique<CmisModule>(nullptr, std::move(qsfpImpl), 2);

  qsfpMod->moduleStateMachine_.get_attribute(qsfpModuleObjPtr) = qsfpMod.get();

  // MSM: Not_Present -> Present -> Discovered -> Inactive (all port down)
  qsfpMod->stateUpdate(ModuleStateMachineEvent::OPTICS_DETECTED);
  qsfpMod->stateUpdate(ModuleStateMachineEvent::EEPROM_READ);
  qsfpMod->refresh();

  qsfpMod->portStateMachines_[0].process_event(
      MODULE_PORT_EVENT_AGENT_PORT_DOWN);
  qsfpMod->portStateMachines_[1].process_event(MODULE_PORT_EVENT_AGENT_PORT_UP);
  int CurrState = qsfpMod->moduleStateMachine_.current_state()[0];
  EXPECT_EQ(CurrState, 4);

  // Check Forced upgrade event is accepted and module is in Upgrading state
  qsfpMod->stateUpdate(ModuleStateMachineEvent::FORCED_UPGRADE);
  CurrState = qsfpMod->moduleStateMachine_.current_state()[0];
  EXPECT_EQ(CurrState, 5);

  // Finish upgrading with module reset and move it to first state
  qsfpMod->stateUpdate(ModuleStateMachineEvent::OPTICS_RESET);
  CurrState = qsfpMod->moduleStateMachine_.current_state()[0];
  EXPECT_EQ(CurrState, 0);
}

// MSM: Check optics removal event from various states
TEST(CmisTest, testOpticsRemoval) {
  int idx = 1;
  std::unique_ptr<Cmis200GTransceiver> qsfpImpl =
      std::make_unique<Cmis200GTransceiver>(idx);

  std::unique_ptr<CmisModule> qsfpMod =
      std::make_unique<CmisModule>(nullptr, std::move(qsfpImpl), 2);

  qsfpMod->moduleStateMachine_.get_attribute(qsfpModuleObjPtr) = qsfpMod.get();

  // Bring up to discovered state and check optics removed event - new state
  // should be first one and PSM should get removed
  qsfpMod->stateUpdate(ModuleStateMachineEvent::OPTICS_DETECTED);
  qsfpMod->stateUpdate(ModuleStateMachineEvent::EEPROM_READ);
  qsfpMod->refresh();
  qsfpMod->stateUpdate(ModuleStateMachineEvent::OPTICS_REMOVED);
  int CurrState = qsfpMod->moduleStateMachine_.current_state()[0];
  EXPECT_EQ(CurrState, 0);
  EXPECT_EQ(qsfpMod->portStateMachines_.size(), 0);

  // Bring up to Active state and check optics removed event
  qsfpMod->stateUpdate(ModuleStateMachineEvent::OPTICS_DETECTED);
  qsfpMod->stateUpdate(ModuleStateMachineEvent::EEPROM_READ);
  qsfpMod->refresh();
  qsfpMod->portStateMachines_[0].process_event(MODULE_PORT_EVENT_AGENT_PORT_UP);
  qsfpMod->portStateMachines_[1].process_event(MODULE_PORT_EVENT_AGENT_PORT_UP);
  CurrState = qsfpMod->moduleStateMachine_.current_state()[0];
  EXPECT_EQ(CurrState, 4);
  qsfpMod->stateUpdate(ModuleStateMachineEvent::OPTICS_REMOVED);
  CurrState = qsfpMod->moduleStateMachine_.current_state()[0];
  EXPECT_EQ(CurrState, 0);

  // Bring up to Upgrading state and check optics removed event
  qsfpMod->stateUpdate(ModuleStateMachineEvent::OPTICS_DETECTED);
  qsfpMod->stateUpdate(ModuleStateMachineEvent::EEPROM_READ);
  qsfpMod->refresh();
  qsfpMod->portStateMachines_[0].process_event(
      MODULE_PORT_EVENT_AGENT_PORT_DOWN);
  qsfpMod->portStateMachines_[1].process_event(
      MODULE_PORT_EVENT_AGENT_PORT_DOWN);
  qsfpMod->stateUpdate(ModuleStateMachineEvent::TRIGGER_UPGRADE);
  CurrState = qsfpMod->moduleStateMachine_.current_state()[0];
  EXPECT_EQ(CurrState, 5);
  qsfpMod->stateUpdate(ModuleStateMachineEvent::OPTICS_REMOVED);
  CurrState = qsfpMod->moduleStateMachine_.current_state()[0];
  EXPECT_EQ(CurrState, 0);
}

TEST(Cmis400GLr4Test, transceiverInfoTest) {
  int idx = 1;
  std::unique_ptr<Cmis400GLr4Transceiver> qsfpImpl =
      std::make_unique<Cmis400GLr4Transceiver>(idx);

  std::unique_ptr<CmisModule> xcvr =
      std::make_unique<CmisModule>(nullptr, std::move(qsfpImpl), 4);
  xcvr->refresh();

  TransceiverInfo info = xcvr->getTransceiverInfo();

  TransceiverTestsHelper tests(info);

  tests.verifyVendorName("FACETEST");
  for (auto& media : *info.settings_ref()->mediaInterface_ref()) {
    EXPECT_EQ(
        media.media_ref()->get_smfCode(), SMFMediaInterfaceCode::LR4_10_400G);
  }
  EXPECT_EQ(xcvr->numHostLanes(), 8);
  EXPECT_EQ(xcvr->numMediaLanes(), 4);
}

TEST(CmisFlatMemTest, transceiverInfoTest) {
  int idx = 1;
  std::unique_ptr<CmisFlatMemTransceiver> qsfpImpl =
      std::make_unique<CmisFlatMemTransceiver>(idx);

  std::unique_ptr<CmisModule> xcvr =
      std::make_unique<CmisModule>(nullptr, std::move(qsfpImpl), 4);
  xcvr->refresh();

  TransceiverInfo info = xcvr->getTransceiverInfo();

  TransceiverTestsHelper tests(info);

  tests.verifyVendorName("FACETEST");
  EXPECT_EQ(xcvr->numHostLanes(), 4);
  EXPECT_EQ(xcvr->numMediaLanes(), 0);
}

} // namespace
