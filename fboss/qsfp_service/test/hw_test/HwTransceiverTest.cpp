/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/qsfp_service/test/hw_test/HwTransceiverTest.h"

#include "fboss/qsfp_service/test/hw_test/HwPortUtils.h"
#include "fboss/qsfp_service/test/hw_test/HwQsfpEnsemble.h"

namespace facebook::fboss {
void HwTransceiverTest::SetUp() {
  HwTest::SetUp();

  auto agentConfig = getHwQsfpEnsemble()->getWedgeManager()->getAgentConfig();
  auto wedgeManager = getHwQsfpEnsemble()->getWedgeManager();
  // Without new-port-programming enabled, we need to explicitly call
  // syncPorts to trigger TransceiverManager to program transceivers to the
  // correct mode
  if (FLAGS_use_new_state_machine) {
    wedgeManager->setOverrideAgentPortStatusForTesting(
        isPortUp_ /* up */, true /* enabled */);
    wedgeManager->refreshStateMachines();
    wedgeManager->setOverrideAgentPortStatusForTesting(
        isPortUp_ /* up */, true /* enabled */, true /* clearOnly */);
  } else {
    auto portMap = std::make_unique<WedgeManager::PortMap>();
    auto& swConfig = *agentConfig->thrift.sw_ref();
    for (auto& port : *swConfig.ports_ref()) {
      if (*port.state_ref() != cfg::PortState::ENABLED) {
        continue;
      }
      auto portId = *port.logicalID_ref();
      auto portStatus =
          utility::getPortStatus(PortID(portId), getHwQsfpEnsemble());
      portStatus.up_ref() = isPortUp_;
      portMap->emplace(portId, portStatus);
    }
    std::map<int32_t, TransceiverInfo> transceivers;
    wedgeManager->syncPorts(transceivers, std::move(portMap));
  }

  expectedTcvrs_ =
      utility::getCabledPortTranceivers(*agentConfig, getHwQsfpEnsemble());
  auto transceiverIds = refreshTransceiversWithRetry();
  EXPECT_TRUE(utility::containsSubset(transceiverIds, expectedTcvrs_));
}

std::unique_ptr<std::vector<int32_t>>
HwTransceiverTest::getExpectedLegacyTransceiverIds() const {
  return std::make_unique<std::vector<int32_t>>(
      utility::legacyTransceiverIds(expectedTcvrs_));
}
} // namespace facebook::fboss
