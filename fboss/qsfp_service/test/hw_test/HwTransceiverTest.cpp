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

  auto wedgeManager = getHwQsfpEnsemble()->getWedgeManager();

  // Set override agent port status so that we can update the active state
  // via refreshStateMachines()
  wedgeManager->setOverrideAgentPortStatusForTesting(
      isPortUp_ /* up */, true /* enabled */);
  // Pause remediation if isPortUp_ == false to avoid unnecessary remediation
  if (!isPortUp_) {
    wedgeManager->setPauseRemediation(600, nullptr);
  }
  wedgeManager->refreshStateMachines();
  wedgeManager->setOverrideAgentPortStatusForTesting(
      isPortUp_ /* up */, true /* enabled */, true /* clearOnly */);

  expectedTcvrs_ = utility::getCabledPortTranceivers(getHwQsfpEnsemble());
  auto transceiverIds = refreshTransceiversWithRetry();
  EXPECT_TRUE(utility::containsSubset(transceiverIds, expectedTcvrs_));
}

std::unique_ptr<std::vector<int32_t>>
HwTransceiverTest::getExpectedLegacyTransceiverIds() const {
  return std::make_unique<std::vector<int32_t>>(
      utility::legacyTransceiverIds(expectedTcvrs_));
}
} // namespace facebook::fboss
