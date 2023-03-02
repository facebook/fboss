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

#include "fboss/agent/AgentConfig.h"
#include "fboss/agent/platforms/common/PlatformMapping.h"

#include <folly/logging/xlog.h>
#include "fboss/lib/CommonUtils.h"
#include "fboss/qsfp_service/module/QsfpModule.h"
#include "fboss/qsfp_service/test/hw_test/HwPortUtils.h"
#include "fboss/qsfp_service/test/hw_test/HwQsfpEnsemble.h"
#include "fboss/qsfp_service/test/hw_test/HwTransceiverTest.h"
#include "thrift/lib/cpp/util/EnumUtils.h"

constexpr static auto kMaxRefreshesForReadyState = 5;

namespace facebook::fboss {

class HwTransceiverResetTest : public HwTransceiverTest {
 public:
  // Mark port down to allow tcvr removal later when issuing hard reset
  HwTransceiverResetTest() : HwTransceiverTest(false) {}

  void resetAllTransceiversAndWaitForStable(
      const std::map<int32_t, TransceiverInfo>& transceivers) {
    auto wedgeManager = getHwQsfpEnsemble()->getWedgeManager();
    std::vector<int32_t> hardresetTcvrs;
    // Reset all transceivers
    for (auto idAndTransceiver : transceivers) {
      wedgeManager->triggerQsfpHardReset(idAndTransceiver.first);
      hardresetTcvrs.push_back(idAndTransceiver.first);
    }
    // Clear reset on all transceivers
    wedgeManager->clearAllTransceiverReset();
    XLOG(INFO) << "Trigger QSFP Hard reset for tranceivers: ["
               << folly::join(",", hardresetTcvrs) << "]";

    waitTillCabledTcvrProgrammed();
  }
};

TEST_F(HwTransceiverResetTest, resetTranscieverAndDetectPresence) {
  // Validate that the power control register has been correctly set before we
  // begin resetting the modules
  auto wedgeManager = getHwQsfpEnsemble()->getWedgeManager();
  std::map<int32_t, TransceiverInfo> transceivers;
  wedgeManager->getTransceiversInfo(
      transceivers, getExpectedLegacyTransceiverIds());
  for (auto idAndTransceiver : transceivers) {
    auto& tcvrState =
        apache::thrift::can_throw(*idAndTransceiver.second.tcvrState());
    if (*tcvrState.present()) {
      XLOG(INFO) << "Transceiver:" << idAndTransceiver.first
                 << " before hard reset, power control="
                 << apache::thrift::util::enumNameSafe(
                        *tcvrState.settings()->powerControl());
      auto transmitterTech = *tcvrState.cable().value_or({}).transmitterTech();
      auto mgmtInterface = apache::thrift::can_throw(
          *tcvrState.transceiverManagementInterface());
      if (transmitterTech == TransmitterTechnology::COPPER) {
        if (mgmtInterface == TransceiverManagementInterface::CMIS) {
          EXPECT_TRUE(
              *tcvrState.settings()->powerControl() ==
              PowerControlState::HIGH_POWER_OVERRIDE);
        } else {
          EXPECT_TRUE(
              *tcvrState.settings()->powerControl() ==
              PowerControlState::POWER_SET_BY_HW);
        }
      } else {
        EXPECT_TRUE(
            *tcvrState.settings()->powerControl() ==
                PowerControlState::POWER_OVERRIDE ||
            *tcvrState.settings()->powerControl() ==
                PowerControlState::HIGH_POWER_OVERRIDE);
      }
    }
  }

  resetAllTransceiversAndWaitForStable(transceivers);

  std::map<int32_t, TransceiverInfo> transceiversAfterReset;
  wedgeManager->getTransceiversInfo(
      transceiversAfterReset, getExpectedLegacyTransceiverIds());
  // Assert that we can detect all transceivers again
  for (auto idAndTransceiver : transceivers) {
    auto& tcvrState =
        apache::thrift::can_throw(*idAndTransceiver.second.tcvrState());
    if (*tcvrState.present()) {
      XLOG(DBG2) << "Checking that transceiver : " << idAndTransceiver.first
                 << " was detected after reset";
      auto titr = transceiversAfterReset.find(idAndTransceiver.first);
      EXPECT_TRUE(titr != transceiversAfterReset.end());
      EXPECT_TRUE(*titr->second.present());

      XLOG(INFO) << "Transceiver:" << idAndTransceiver.first
                 << " before hard reset, power control="
                 << apache::thrift::util::enumNameSafe(
                        *tcvrState.settings()->powerControl())
                 << ", after hard reset, power control="
                 << apache::thrift::util::enumNameSafe(
                        *titr->second.settings()->powerControl());
      EXPECT_EQ(
          *tcvrState.settings()->powerControl(),
          *titr->second.settings()->powerControl());
    }
  }
}

TEST_F(HwTransceiverResetTest, resetTranscieverAndDetectStateChanged) {
  std::map<int32_t, TransceiverInfo> transceivers;
  std::map<int32_t, ModuleStatus> moduleStatuses;

  auto wedgeManager = getHwQsfpEnsemble()->getWedgeManager();
  WITH_RETRIES_N(kMaxRefreshesForReadyState, {
    refreshTransceiversWithRetry();
    transceivers.clear();
    wedgeManager->getTransceiversInfo(
        transceivers, getExpectedLegacyTransceiverIds());
    EXPECT_EVENTUALLY_GT(transceivers.size(), 0);

    for (auto idAndTransceiver : transceivers) {
      auto& tcvrState =
          apache::thrift::can_throw(*idAndTransceiver.second.tcvrState());
      if (*tcvrState.present()) {
        auto mgmtInterface = tcvrState.transceiverManagementInterface();
        CHECK(mgmtInterface);
        if (mgmtInterface == TransceiverManagementInterface::CMIS) {
          auto state = tcvrState.status()->cmisModuleState();
          CHECK(state);
          EXPECT_EVENTUALLY_EQ(state, CmisModuleState::READY)
              << "Cmis module not ready on tcvr " << idAndTransceiver.first;
        }
      }
    }
  });

  // clear existing module status flags from previous refreshes
  wedgeManager->getAndClearTransceiversModuleStatus(
      moduleStatuses, getExpectedLegacyTransceiverIds());
  auto transceiverIds = refreshTransceiversWithRetry();
  EXPECT_TRUE(
      utility::containsSubset(transceiverIds, getExpectedTransceivers()));

  transceivers.clear();
  wedgeManager->getTransceiversInfo(
      transceivers, getExpectedLegacyTransceiverIds());
  // get the module status flags from the previous refresh
  moduleStatuses.clear();
  wedgeManager->getAndClearTransceiversModuleStatus(
      moduleStatuses, getExpectedLegacyTransceiverIds());

  // Validate that cmisStateChanged is 0 before running the test
  // TODO: also add check for SFF modules' PowerControl field
  for (auto idAndTransceiver : transceivers) {
    auto& tcvrState =
        apache::thrift::can_throw(*idAndTransceiver.second.tcvrState());
    if (*tcvrState.present()) {
      auto mgmtInterface = tcvrState.transceiverManagementInterface();
      CHECK(mgmtInterface);
      if (*mgmtInterface == TransceiverManagementInterface::SFF ||
          *mgmtInterface == TransceiverManagementInterface::SFF8472) {
      } else if (*mgmtInterface == TransceiverManagementInterface::CMIS) {
        auto status = moduleStatuses[idAndTransceiver.first];
        auto stateChanged = status.cmisStateChanged();
        CHECK(stateChanged);
        EXPECT_FALSE(*stateChanged)
            << "Wrong stateChanged on module " << idAndTransceiver.first;
      } else {
        throw FbossError(
            "Invalid transceiver type: ",
            apache::thrift::util::enumNameSafe(*mgmtInterface));
      }
    }
  }

  resetAllTransceiversAndWaitForStable(transceivers);

  std::map<int32_t, TransceiverInfo> transceiversAfterReset;
  wedgeManager->getTransceiversInfo(
      transceiversAfterReset, getExpectedLegacyTransceiverIds());
  moduleStatuses.clear();
  wedgeManager->getAndClearTransceiversModuleStatus(
      moduleStatuses, getExpectedLegacyTransceiverIds());

  // Check that CMIS modules have cmisStateChanged flag set after reset
  // TODO: also add check for SFF modules' PowerControl field
  for (auto idAndTransceiver : transceivers) {
    auto& tcvrState =
        apache::thrift::can_throw(*idAndTransceiver.second.tcvrState());
    if (*tcvrState.present()) {
      auto titr = transceiversAfterReset.find(idAndTransceiver.first);
      EXPECT_TRUE(titr != transceiversAfterReset.end());
      auto& tcvrStateAfterReset =
          apache::thrift::can_throw(*titr->second.tcvrState());
      auto mgmtInterface = tcvrStateAfterReset.transceiverManagementInterface();
      auto transmitterTech =
          *tcvrStateAfterReset.cable().value_or({}).transmitterTech();
      CHECK(mgmtInterface);
      if (*mgmtInterface == TransceiverManagementInterface::SFF ||
          *mgmtInterface == TransceiverManagementInterface::SFF8472) {
      } else if (*mgmtInterface == TransceiverManagementInterface::CMIS) {
        auto status = moduleStatuses[idAndTransceiver.first];
        auto stateChanged = status.cmisStateChanged();
        CHECK(stateChanged);
        // Copper cables don't set the state changed flag
        EXPECT_TRUE(
            *stateChanged == (transmitterTech != TransmitterTechnology::COPPER))
            << " Failed comparison for transceiver " << idAndTransceiver.first;
      } else {
        throw FbossError(
            "Invalid transceiver type: ",
            apache::thrift::util::enumNameSafe(*mgmtInterface));
      }
    }
  }
}
} // namespace facebook::fboss
