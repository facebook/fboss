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
#include "fboss/agent/platforms/common/PlatformProductInfo.h"
#include "fboss/lib/CommonUtils.h"
#include "fboss/lib/fpga/MultiPimPlatformSystemContainer.h"
#include "fboss/lib/phy/PhyManager.h"
#include "fboss/qsfp_service/test/hw_test/HwPortUtils.h"
#include "fboss/qsfp_service/test/hw_test/HwQsfpEnsemble.h"

DEFINE_bool(
    setup_for_warmboot,
    false,
    "Set up test for QSFP warmboot. Useful for testing individual "
    "tests doing a full process warmboot and verifying expectations");

namespace facebook::fboss {

HwTest::HwTest(bool useNewStateMachine, bool setupOverrideTcvrToPortAndProfile)
    : useNewStateMachine_(useNewStateMachine),
      setupOverrideTcvrToPortAndProfile_(setupOverrideTcvrToPortAndProfile) {}

void HwTest::SetUp() {
  // Set initializing pim xphys to 1 so we always initialize xphy chips
  gflags::SetCommandLineOptionWithMode(
      "init_pim_xphys", "1", gflags::SET_FLAGS_DEFAULT);

  // Only enable using new state machine on demand since the whole migration
  // is still on-going. Switching the whole qsfp_service logic to use unfinished
  // new state machine logic will break a lot of existing Transceiver tests.
  // Eventually we'll get rid of this check and use new state machine only.
  if (useNewStateMachine_) {
    gflags::SetCommandLineOptionWithMode(
        "use_new_state_machine", "1", gflags::SET_FLAGS_DEFAULT);
    // Change the default remediation interval to 0 to avoid waiting
    gflags::SetCommandLineOptionWithMode(
        "remediate_interval", "0", gflags::SET_FLAGS_DEFAULT);
    gflags::SetCommandLineOptionWithMode(
        "initial_remediate_interval", "0", gflags::SET_FLAGS_DEFAULT);
  }
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

  // Do an initial refresh so that the customization is done before the test
  // starts. It also takes ~5 seconds sometimes for the CMIS modules to be
  // functional after a data path deinit, that can happen in the customize call
  refreshTransceiversWithRetry();
  if (!didWarmBoot()) {
    // Warmboot shouldn't configure the transceiver again so we shouldn't
    // require a wait here
    sleep(5);
  }
}

void HwTest::TearDown() {
  ensemble_.reset();
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
              transceiversAfterRefresh[id].timeCollected_ref().value_or({});
          auto timeBefore =
              transceiversBeforeRefresh[id].timeCollected_ref().value_or({});
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
  checkWithRetry(refresh, numRetries);

  return transceiverIds;
}
} // namespace facebook::fboss
