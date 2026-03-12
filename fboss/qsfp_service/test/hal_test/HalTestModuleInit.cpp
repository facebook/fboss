// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/qsfp_service/module/cmis/CmisModule.h"
#include "fboss/qsfp_service/test/hal_test/HalTest.h"

namespace facebook::fboss {

// Verify that a CMIS module reaches READY state after low power mode is
// cleared. The test flow for each present transceiver is:
//   1. Confirm the module is not ready after the hard reset performed in SetUp.
//   2. Explicitly set low power mode and confirm the module remains not ready.
//   3. Clear the low power bit and poll until the module reports READY,
//      ensuring the full low-power-to-ready transition works end to end.
TEST_F(HalTest, verifyModuleReachesReadyAfterLowPowerCleared) {
  for (auto tcvrId : getPresentTransceiverIds()) {
    auto* cmisModule = dynamic_cast<CmisModule*>(getModule(tcvrId));
    ASSERT_NE(cmisModule, nullptr)
        << "Transceiver " << tcvrId << " is not CMIS";

    // Step 1: After hard reset (done in SetUp), module should NOT be ready
    EXPECT_FALSE(cmisModule->isModuleInReadyState())
        << "Transceiver " << tcvrId
        << " should not be in ready state after hard reset";

    // Step 2: Set module to low power mode
    cmisModule->setModuleLowPowerModeLocked();

    // Step 3: Verify module is still NOT in ready state in low power mode
    EXPECT_FALSE(cmisModule->isModuleInReadyState())
        << "Transceiver " << tcvrId << " should not be ready in low power mode";

    // Step 4: Release low power mode
    cmisModule->releaseModuleLowPowerModeLocked();

    // Step 5: Poll until module reaches ready state
    EXPECT_TRUE(cmisModule->moduleReadyStatePoll())
        << "Transceiver " << tcvrId
        << " did not reach ready state after releasing low power";
  }
}

} // namespace facebook::fboss
