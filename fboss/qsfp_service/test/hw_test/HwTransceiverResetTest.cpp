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
#include "thrift/lib/cpp/util/EnumUtils.h"

constexpr static auto kMaxRefreshesForReadyState = 5;

namespace facebook::fboss {

TEST_F(HwTest, resetTranscieverAndDetectPresence) {
  auto portMap = std::make_unique<WedgeManager::PortMap>();
  auto agentConfig = getHwQsfpEnsemble()->getWedgeManager()->getAgentConfig();
  auto& swConfig = *agentConfig->thrift.sw_ref();
  for (auto& port : *swConfig.ports_ref()) {
    if (*port.state_ref() != cfg::PortState::ENABLED) {
      continue;
    }
    auto portId = *port.logicalID_ref();
    portMap->emplace(
        portId, utility::getPortStatus(PortID(portId), getHwQsfpEnsemble()));
  }
  auto wedgeManager = getHwQsfpEnsemble()->getWedgeManager();
  std::map<int32_t, TransceiverInfo> transceivers;
  wedgeManager->syncPorts(transceivers, std::move(portMap));

  // Validate that the power control register has been correctly set before we
  // begin resetting the modules
  auto transceiverIds = refreshTransceiversWithRetry();
  auto expectedIds =
      utility::getCabledPortTranceivers(*agentConfig, getHwQsfpEnsemble());
  EXPECT_TRUE(utility::containsSubset(transceiverIds, expectedIds));

  transceivers.clear();
  wedgeManager->getTransceiversInfo(
      transceivers,
      std::make_unique<std::vector<int32_t>>(
          utility::legacyTransceiverIds(expectedIds)));
  for (auto idAndTransceiver : transceivers) {
    if (*idAndTransceiver.second.present_ref()) {
      XLOG(INFO)
          << "Transceiver:" << idAndTransceiver.first
          << " before hard reset, power control="
          << apache::thrift::util::enumNameSafe(
                 *idAndTransceiver.second.settings_ref()->powerControl_ref());
      auto transmitterTech = *idAndTransceiver.second.cable_ref()
                                  .value_or({})
                                  .transmitterTech_ref();
      if (transmitterTech == TransmitterTechnology::COPPER) {
        EXPECT_TRUE(
            *idAndTransceiver.second.settings_ref()->powerControl_ref() ==
            PowerControlState::POWER_SET_BY_HW);
      } else {
        EXPECT_TRUE(
            *idAndTransceiver.second.settings_ref()->powerControl_ref() ==
                PowerControlState::POWER_OVERRIDE ||
            *idAndTransceiver.second.settings_ref()->powerControl_ref() ==
                PowerControlState::HIGH_POWER_OVERRIDE);
      }
    }
  }

  // Reset all transceivers
  for (auto idAndTransceiver : transceivers) {
    wedgeManager->triggerQsfpHardReset(idAndTransceiver.first);
  }
  // Clear reset on all transceivers
  wedgeManager->clearAllTransceiverReset();
  // Refresh
  refreshTransceiversWithRetry();

  std::map<int32_t, TransceiverInfo> transceiversAfterReset;
  wedgeManager->getTransceiversInfo(
      transceiversAfterReset,
      std::make_unique<std::vector<int32_t>>(
          utility::legacyTransceiverIds(expectedIds)));
  // Assert that we can detect all transceivers again

  for (auto idAndTransceiver : transceivers) {
    if (*idAndTransceiver.second.present_ref()) {
      XLOG(DBG2) << "Checking that transceiver : " << idAndTransceiver.first
                 << " was detected after reset";
      auto titr = transceiversAfterReset.find(idAndTransceiver.first);
      EXPECT_TRUE(titr != transceiversAfterReset.end());
      EXPECT_TRUE(*titr->second.present_ref());

      XLOG(INFO)
          << "Transceiver:" << idAndTransceiver.first
          << " before hard reset, power control="
          << apache::thrift::util::enumNameSafe(
                 *idAndTransceiver.second.settings_ref()->powerControl_ref())
          << ", after hard reset, power control="
          << apache::thrift::util::enumNameSafe(
                 *titr->second.settings_ref()->powerControl_ref());
      EXPECT_EQ(
          *idAndTransceiver.second.settings_ref()->powerControl_ref(),
          *titr->second.settings_ref()->powerControl_ref());
    }
  }
}

TEST_F(HwTest, resetTranscieverAndDetectStateChanged) {
  auto portMap = std::make_unique<WedgeManager::PortMap>();
  auto agentConfig = getHwQsfpEnsemble()->getWedgeManager()->getAgentConfig();
  auto& swConfig = *agentConfig->thrift.sw();
  for (auto& port : *swConfig.ports()) {
    if (*port.state() != cfg::PortState::ENABLED) {
      continue;
    }
    auto portId = *port.logicalID();
    portMap->emplace(
        portId, utility::getPortStatus(PortID(portId), getHwQsfpEnsemble()));
  }
  auto wedgeManager = getHwQsfpEnsemble()->getWedgeManager();
  std::map<int32_t, TransceiverInfo> transceivers;
  std::map<int32_t, ModuleStatus> moduleStatuses;
  wedgeManager->syncPorts(transceivers, std::move(portMap));

  auto expectedIds =
      utility::getCabledPortTranceivers(*agentConfig, getHwQsfpEnsemble());

  auto cmisModulesReady = [this, &transceivers, &wedgeManager, &expectedIds]() {
    auto allCmisModulesReady = true;
    refreshTransceiversWithRetry();
    transceivers.clear();
    wedgeManager->getTransceiversInfo(
        transceivers,
        std::make_unique<std::vector<int32_t>>(
            utility::legacyTransceiverIds(expectedIds)));
    for (auto idAndTransceiver : transceivers) {
      if (*idAndTransceiver.second.present()) {
        auto mgmtInterface =
            idAndTransceiver.second.transceiverManagementInterface();
        CHECK(mgmtInterface);
        if (mgmtInterface == TransceiverManagementInterface::CMIS) {
          auto state = idAndTransceiver.second.status()->cmisModuleState();
          CHECK(state);
          allCmisModulesReady &= (state == CmisModuleState::READY);
        }
      }
    }
    return allCmisModulesReady;
  };

  checkWithRetry(cmisModulesReady, kMaxRefreshesForReadyState);

  // clear existing module status flags from previous refreshes
  wedgeManager->getAndClearTransceiversModuleStatus(
      moduleStatuses,
      std::make_unique<std::vector<int32_t>>(
          utility::legacyTransceiverIds(expectedIds)));
  auto transceiverIds = refreshTransceiversWithRetry();
  EXPECT_TRUE(utility::containsSubset(transceiverIds, expectedIds));

  transceivers.clear();
  wedgeManager->getTransceiversInfo(
      transceivers,
      std::make_unique<std::vector<int32_t>>(
          utility::legacyTransceiverIds(expectedIds)));
  // get the module status flags from the previous refresh
  moduleStatuses.clear();
  wedgeManager->getAndClearTransceiversModuleStatus(
      moduleStatuses,
      std::make_unique<std::vector<int32_t>>(
          utility::legacyTransceiverIds(expectedIds)));

  // Validate that cmisStateChanged is 0 before running the test
  // TODO: also add check for SFF modules' PowerControl field
  for (auto idAndTransceiver : transceivers) {
    if (*idAndTransceiver.second.present()) {
      auto mgmtInterface =
          idAndTransceiver.second.transceiverManagementInterface();
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

  // Reset all transceivers
  for (auto idAndTransceiver : transceivers) {
    wedgeManager->triggerQsfpHardReset(idAndTransceiver.first);
  }
  // Clear reset on all transceivers
  wedgeManager->clearAllTransceiverReset();
  refreshTransceiversWithRetry();

  std::map<int32_t, TransceiverInfo> transceiversAfterReset;
  wedgeManager->getTransceiversInfo(
      transceiversAfterReset,
      std::make_unique<std::vector<int32_t>>(
          utility::legacyTransceiverIds(expectedIds)));
  moduleStatuses.clear();
  wedgeManager->getAndClearTransceiversModuleStatus(
      moduleStatuses,
      std::make_unique<std::vector<int32_t>>(
          utility::legacyTransceiverIds(expectedIds)));

  // Check that CMIS modules have cmisStateChanged flag set after reset
  // TODO: also add check for SFF modules' PowerControl field
  for (auto idAndTransceiver : transceivers) {
    if (*idAndTransceiver.second.present()) {
      auto titr = transceiversAfterReset.find(idAndTransceiver.first);
      EXPECT_TRUE(titr != transceiversAfterReset.end());
      auto mgmtInterface = titr->second.transceiverManagementInterface();
      CHECK(mgmtInterface);
      if (*mgmtInterface == TransceiverManagementInterface::SFF ||
          *mgmtInterface == TransceiverManagementInterface::SFF8472) {
      } else if (*mgmtInterface == TransceiverManagementInterface::CMIS) {
        auto status = moduleStatuses[idAndTransceiver.first];
        auto stateChanged = status.cmisStateChanged();
        CHECK(stateChanged);
        EXPECT_TRUE(*stateChanged)
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
