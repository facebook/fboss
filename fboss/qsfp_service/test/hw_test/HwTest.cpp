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

#include <folly/logging/xlog.h>
#include "fboss/agent/FbossError.h"
#include "fboss/lib/CommonFileUtils.h"
#include "fboss/lib/CommonUtils.h"
#include "fboss/lib/fpga/MultiPimPlatformSystemContainer.h"
#include "fboss/lib/phy/PhyManager.h"
#include "fboss/lib/platforms/PlatformProductInfo.h"
#include "fboss/qsfp_service/QsfpServer.h"
#include "fboss/qsfp_service/test/hw_test/HwPortUtils.h"
#include "fboss/qsfp_service/test/hw_test/HwQsfpEnsemble.h"

DEFINE_bool(
    setup_for_warmboot,
    false,
    "Set up test for QSFP warmboot. Useful for testing individual "
    "tests doing a full process warmboot and verifying expectations");

namespace facebook::fboss {

using namespace std::chrono_literals;

HwTest::HwTest(bool setupOverrideTcvrToPortAndProfile)
    : setupOverrideTcvrToPortAndProfile_(setupOverrideTcvrToPortAndProfile) {}

void HwTest::SetUp() {
  // First use QsfpConfig to init default command line arguments
  initFlagDefaultsFromQsfpConfig();

  // Change the default remediation interval to 0 to avoid waiting
  gflags::SetCommandLineOptionWithMode(
      "remediate_interval", "0", gflags::SET_FLAGS_DEFAULT);
  gflags::SetCommandLineOptionWithMode(
      "initial_remediate_interval", "0", gflags::SET_FLAGS_DEFAULT);

  // This is used to set up the override TransceiveToPortAndProfile so that we
  // don't have to rely on wedge_agent::programInternalPhyPorts() in our
  // qsfp_hw_test
  if (setupOverrideTcvrToPortAndProfile_) {
    gflags::SetCommandLineOptionWithMode(
        "override_program_iphy_ports_for_test", "1", gflags::SET_FLAGS_DEFAULT);
  }

  ensemble_ = std::make_unique<HwQsfpEnsemble>();
  ensemble_->init();

  // Allow back to back refresh and customizations in test
  gflags::SetCommandLineOptionWithMode(
      "qsfp_data_refresh_interval", "0", gflags::SET_FLAGS_DEFAULT);
  gflags::SetCommandLineOptionWithMode(
      "customize_interval", "0", gflags::SET_FLAGS_DEFAULT);

  waitTillCabledTcvrProgrammed();

  // It also takes ~5 seconds sometimes for the CMIS modules to be
  // functional after a data path deinit, that can happen in the customize call
  if (!didWarmBoot()) {
    // Warmboot shouldn't configure the transceiver again so we shouldn't
    // require a wait here
    sleep(5);
  }
}

void HwTest::TearDown() {
  // At the end of the test, expect the watchdog fired count to be 0 because we
  // don't expect any deadlocked threads
  EXPECT_EQ(
      ensemble_->getWedgeManager()->getStateMachineThreadHeartbeatMissedCount(),
      0);
  ensemble_.reset();
  // We expect that any coldboot flag set during the test should be cleared
  EXPECT_FALSE(checkFileExists(TransceiverManager::forceColdBootFileName()));
}

bool HwTest::didWarmBoot() const {
  return ensemble_->didWarmBoot();
}

MultiPimPlatformPimContainer* HwTest::getPimContainer(int pimID) {
  return ensemble_->getPhyManager()->getSystemContainer()->getPimContainer(
      pimID);
}

void HwTest::setupForWarmboot() const {
  ensemble_->setupForWarmboot();
}

// Refresh transceivers and retry after some unsuccessful attempts
std::vector<TransceiverID> HwTest::refreshTransceiversWithRetry(
    int numRetries) {
  auto agentConfig = ensemble_->getWedgeManager()->getAgentConfig();
  auto expectedIds =
      utility::getCabledPortTranceivers(*agentConfig, getHwQsfpEnsemble());
  std::map<int32_t, TransceiverInfo> transceiversBeforeRefresh;
  std::vector<TransceiverID> transceiverIds;
  ensemble_->getWedgeManager()->getTransceiversInfo(
      transceiversBeforeRefresh,
      std::make_unique<std::vector<int32_t>>(
          utility::legacyTransceiverIds(expectedIds)));

  // Lambda to do a refresh and confirm refresh actually happened
  auto refresh =
      [this, &expectedIds, &transceiversBeforeRefresh, &transceiverIds]() {
        transceiverIds = ensemble_->getWedgeManager()->refreshTransceivers();
        std::map<int32_t, TransceiverInfo> transceiversAfterRefresh;
        ensemble_->getWedgeManager()->getTransceiversInfo(
            transceiversAfterRefresh,
            std::make_unique<std::vector<int32_t>>(
                utility::legacyTransceiverIds(expectedIds)));
        for (auto id : expectedIds) {
          // Confirm all the cabled transceivers were returned by
          // getTransceiversInfo
          if (transceiversAfterRefresh.find(id) ==
              transceiversAfterRefresh.end()) {
            XLOG(WARN) << "TransceiverID : " << id
                       << ": transceiverInfo not returned after refresh";
            return false;
          }
          // It's possible that there were no transceivers to return before a
          // refresh
          if (transceiversBeforeRefresh.find(id) ==
              transceiversBeforeRefresh.end()) {
            continue;
          }
          auto timeAfter =
              *transceiversAfterRefresh[id].tcvrState()->timeCollected();
          auto timeBefore =
              *transceiversBeforeRefresh[id].tcvrState()->timeCollected();
          // Confirm that the timestamp advanced which will only happen when
          // refresh is successful
          if (timeAfter <= timeBefore) {
            XLOG(WARN) << "TransceiverID : " << id
                       << ": timestamp in the TransceiverInfo after refresh = "
                       << timeAfter
                       << ", timestamp in the TransceiverInfo before refresh = "
                       << timeBefore;
            return false;
          }
        }
        return true;
      };
  checkWithRetry(
      refresh, numRetries, 1s, "Never refreshed all expected transceivers");

  return transceiverIds;
}

// This function is only used if new port programming feature is enabled
// We will wait till all the cabled transceivers reach the
// TRANSCEIVER_PROGRAMMED state by retrying `numRetries` times of
// TransceiverManager::refreshStateMachines()
void HwTest::waitTillCabledTcvrProgrammed(int numRetries) {
  // First get all expected transceivers based on cabled ports from agent.conf
  auto agentConfig = ensemble_->getWedgeManager()->getAgentConfig();
  const auto& expectedIds =
      utility::getCabledPortTranceivers(*agentConfig, getHwQsfpEnsemble());

  // Due to some platforms are easy to have i2c issue which causes the current
  // refresh not work as expected. Adding enough retries to make sure that we
  // at least can secure TRANSCEIVER_PROGRAMMED after `numRetries` times.
  auto refreshStateMachinesTillTcvrProgrammed = [this, &expectedIds]() {
    auto wedgeMgr = getHwQsfpEnsemble()->getWedgeManager();
    wedgeMgr->refreshStateMachines();
    for (auto id : expectedIds) {
      auto curState = wedgeMgr->getCurrentState(id);
      // Statemachine can support transceiver programming (iphy/xphy/tcvr) when
      // `setupOverrideTcvrToPortAndProfile_` is true.
      if (setupOverrideTcvrToPortAndProfile_ &&
          curState != TransceiverStateMachineState::TRANSCEIVER_PROGRAMMED) {
        return false;
      }
    }
    return true;
  };

  // Retry until all state machines reach TRANSCEIVER_PROGRAMMED
  checkWithRetry(
      refreshStateMachinesTillTcvrProgrammed,
      numRetries /* retries */,
      std::chrono::milliseconds(5000) /* msBetweenRetry */,
      "Never got all transceivers programmed");
}
} // namespace facebook::fboss
