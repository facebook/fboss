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

#include <folly/gen/Base.h>
#include <folly/logging/xlog.h>
#include "fboss/agent/FbossError.h"
#include "fboss/lib/CommonFileUtils.h"
#include "fboss/lib/CommonUtils.h"
#include "fboss/lib/fpga/MultiPimPlatformSystemContainer.h"
#include "fboss/lib/phy/PhyManager.h"
#include "fboss/qsfp_service/QsfpServer.h"
#include "fboss/qsfp_service/test/hw_test/HwPortUtils.h"
#include "fboss/qsfp_service/test/hw_test/HwQsfpEnsemble.h"
#include "fboss/qsfp_service/test/hw_test/HwTransceiverUtils.h"

DEFINE_bool(
    setup_for_warmboot,
    false,
    "Set up test for QSFP warmboot. Useful for testing individual "
    "tests doing a full process warmboot and verifying expectations");

DEFINE_bool(
    list_production_feature,
    false,
    "List production feature needed for every single test.");

namespace facebook::fboss {

using namespace std::chrono_literals;

HwTest::HwTest(bool setupOverrideTcvrToPortAndProfile)
    : setupOverrideTcvrToPortAndProfile_(setupOverrideTcvrToPortAndProfile) {}

void HwTest::SetUp() {
  if (FLAGS_list_production_feature) {
    printProductionFeatures();
    return;
  }

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

  if (FLAGS_port_manager_mode) {
    // In PortManager mode, we need ports to be enabled to bring ports /
    // transceivers to programmed state.
    ensemble_->getQsfpServiceHandler()->setOverrideAgentPortStatusForTesting(
        true /* up */, true /* enabled */);
  }

  // Setting remediation paused as a default for 10 minutes
  ensemble_->getWedgeManager()->setPauseRemediation(600, nullptr);

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

  XLOG(INFO) << "HwTest::SetUp() complete";
}

void HwTest::TearDown() {
  // At the end of the test, expect the watchdog fired count to be 0 because we
  // don't expect any deadlocked threads

  // ensemble_ is nullptr when --list_production_feature feature is enabled,
  // because SetUp() code returns early after printing production features –
  // skipping over ensemble initialization.
  if (ensemble_ != nullptr) {
    EXPECT_EQ(
        ensemble_->getWedgeManager()
            ->getStateMachineThreadHeartbeatMissedCount(),
        0);
    ensemble_.reset();
  }
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
  auto expectedIds = utility::getCabledPortTranceivers(getHwQsfpEnsemble());
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
void HwTest::waitTillCabledTcvrProgrammed(int numRetries, bool portUp) {
  // First get all expected transceivers based on cabled ports from agent.conf
  const auto& expectedIds =
      utility::getCabledPortTranceivers(getHwQsfpEnsemble());
  const auto& expectedPorts = utility::getCabledPorts(getHwQsfpEnsemble());
  const PortStateMachineState expectedPortState = portUp
      ? PortStateMachineState::PORT_UP
      : PortStateMachineState::PORT_DOWN;

  // Due to some platforms are easy to have i2c issue which causes the current
  // refresh not work as expected. Adding enough retries to make sure that we
  // at least can secure TRANSCEIVER_PROGRAMMED after `numRetries` times.
  auto refreshStateMachinesTillTcvrProgrammed =
      [this, &expectedIds, &expectedPorts, expectedPortState]() {
        auto wedgeMgr = getHwQsfpEnsemble()->getWedgeManager();
        getHwQsfpEnsemble()->getQsfpServiceHandler()->refreshStateMachines();
        std::vector<TransceiverID> notProgrammedTcvrs;
        std::vector<PortID> notProgrammedPorts;
        bool retval{true};

        for (auto id : expectedIds) {
          auto curState = wedgeMgr->getCurrentState(id);
          // Statemachine can support transceiver programming (iphy/xphy/tcvr)
          // when `setupOverrideTcvrToPortAndProfile_` is true.
          if (setupOverrideTcvrToPortAndProfile_ &&
              curState !=
                  TransceiverStateMachineState::TRANSCEIVER_PROGRAMMED) {
            notProgrammedTcvrs.push_back(id);
            retval = false;
          }
        }

        if (!notProgrammedTcvrs.empty()) {
          XLOG(ERR) << "Transceivers that should be programmed but are not: "
                    << folly::join(",", notProgrammedTcvrs);
        }

        if (FLAGS_port_manager_mode) {
          auto portManager =
              getHwQsfpEnsemble()->getQsfpServiceHandler()->getPortManager();
          for (const auto& portId : expectedPorts) {
            auto curState = portManager->getPortState(portId);
            if (setupOverrideTcvrToPortAndProfile_ &&
                curState != expectedPortState) {
              notProgrammedPorts.push_back(portId);
              retval = false;
            }
          }
        }

        if (!notProgrammedPorts.empty()) {
          XLOG(ERR) << "Ports that did not reach "
                    << apache::thrift::util::enumNameSafe(expectedPortState)
                    << ": " << folly::join(",", notProgrammedPorts);
        }

        return retval;
      };

  // Retry until all state machines reach TRANSCEIVER_PROGRAMMED
  checkWithRetry(
      refreshStateMachinesTillTcvrProgrammed,
      numRetries /* retries */,
      std::chrono::milliseconds(5000) /* msBetweenRetry */,
      "Never got all transceivers programmed");
}

std::vector<int> HwTest::getCabledOpticalAndActiveTransceiverIDs() {
  auto transceivers = utility::legacyTransceiverIds(
      utility::getCabledPortTranceivers(getHwQsfpEnsemble()));
  std::map<int32_t, TransceiverInfo> transceiversInfo;
  getHwQsfpEnsemble()->getWedgeManager()->getTransceiversInfo(
      transceiversInfo, std::make_unique<std::vector<int32_t>>(transceivers));

  return folly::gen::from(transceivers) |
      folly::gen::filter([&transceiversInfo](int32_t tcvrId) {
           auto& tcvrInfo = transceiversInfo[tcvrId];
           return TransceiverManager::opticalOrActiveCable(
               *tcvrInfo.tcvrState());
         }) |
      folly::gen::as<std::vector>();
}

std::vector<qsfp_production_features::QsfpProductionFeature>
HwTest::getProductionFeatures() const {
  return {};
}

void HwTest::printProductionFeatures() const {
  std::vector<std::string> supportedFeatures;
  for (const auto& feature : getProductionFeatures()) {
    supportedFeatures.push_back(apache::thrift::util::enumNameSafe(feature));
  }
  std::cout << "Feature List: " << folly::join(",", supportedFeatures) << "\n";
  GTEST_SKIP();
}

TEST_F(HwTest, CheckTcvrNameAndInterfaces) {
  auto wedgeManager = getHwQsfpEnsemble()->getWedgeManager();
  std::map<int32_t, TransceiverInfo> insertedTransceiversMap;
  wedgeManager->getTransceiversInfo(
      insertedTransceiversMap, std::make_unique<std::vector<int32_t>>());
  CHECK(!insertedTransceiversMap.empty());
  auto platformMappingTransceiverMap =
      getHwQsfpEnsemble()->getWedgeManager()->getTcvrIdToTcvrNameMap();
  for (auto& [id, tcvr] : insertedTransceiversMap) {
    if (platformMappingTransceiverMap.find(static_cast<TransceiverID>(id)) ==
        platformMappingTransceiverMap.end()) {
      // It's possible that there are more transceivers on the system than the
      // one used by current config. For example, the BSP mapping may have more
      // transceiver slots corresponding to multiple NPUs, but in reality we are
      // using the single NPU mode and hence should not test the unused
      // transceivers in this test
      continue;
    }
    auto tcvrStateName = *tcvr.tcvrState()->tcvrName();
    auto tcvrStatsName = *tcvr.tcvrState()->tcvrName();
    XLOG(INFO) << "Checking transceiver name and interfaces for id: " << id
               << " " << tcvrStateName;
    EXPECT_EQ(tcvrStateName, tcvrStatsName);
    EXPECT_TRUE(
        tcvrStateName.starts_with("eth") || tcvrStateName.starts_with("fab"));
    auto tcvrPortName = wedgeManager->getPortName(TransceiverID(id));
    EXPECT_EQ(tcvrStateName + "/1", tcvrPortName);

    auto ports = wedgeManager->getPortNames(TransceiverID(id));
    EXPECT_FALSE(ports.empty());
    EXPECT_EQ(*tcvr.tcvrState()->interfaces(), ports);
    EXPECT_EQ(*tcvr.tcvrStats()->interfaces(), ports);
  }
}
} // namespace facebook::fboss
