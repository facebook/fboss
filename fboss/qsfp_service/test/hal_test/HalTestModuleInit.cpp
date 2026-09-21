// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <fmt/core.h>

#include "fboss/qsfp_service/if/gen-cpp2/transceiver_types.h"
#include "fboss/qsfp_service/module/cmis/CmisModule.h"
#include "fboss/qsfp_service/test/hal_test/HalTest.h"

namespace facebook::fboss {

// Verify that a CMIS module reaches READY state after low power mode is
// cleared. The test flow for each present transceiver is:
//   1. Confirm the module is not ready after the hard reset performed in SetUp.
//   2. Explicitly set low power mode and confirm the module remains not ready.
//   3. Clear the low power bit and poll until the module reports READY,
//      ensuring the full low-power-to-ready transition works end to end.
TEST_F(T1HalTest, verifyModuleReachesReadyAfterLowPowerCleared) {
  forEachTransceiverParallel([this](
                                 hal_test::TransceiverTestResult& result,
                                 int tcvrId) {
    auto* cmisModule = dynamic_cast<CmisModule*>(getModule(tcvrId));
    HAL_CHECK_FATAL_VOID(
        result,
        cmisModule != nullptr,
        fmt::format("Transceiver {} is not CMIS", tcvrId));

    // Step 1: After hard reset (done in SetUp), module should NOT be ready
    HAL_CHECK(
        result,
        !cmisModule->isModuleInReadyState(),
        fmt::format(
            "Transceiver {} should not be in ready state after hard reset",
            tcvrId));

    // Step 2: Set module to low power mode
    cmisModule->setModuleLowPowerModeLocked();

    // Step 3: Verify module is still NOT in ready state in low power mode
    HAL_CHECK(
        result,
        !cmisModule->isModuleInReadyState(),
        fmt::format(
            "Transceiver {} should not be ready in low power mode", tcvrId));

    // Step 4: Release low power mode
    cmisModule->releaseModuleLowPowerModeLocked();

    // Step 5: Poll until module reaches ready state
    HAL_CHECK(
        result,
        cmisModule->moduleReadyStatePoll(),
        fmt::format(
            "Transceiver {} did not reach ready state after releasing low power",
            tcvrId));
  });
}

// Verify that a ZR module comes out of low power with RX output squelch already
// disabled. rxSquelch reflects the RX Output Squelch Disable bits (Page 10h,
// Byte 139), so every host lane is expected to report it set before
// qsfp_service programs any port configuration. Non-ZR transceivers are
// skipped: they are expected to keep squelch enabled, which is what
// QsfpModule::ensureRxOutputSquelchEnabled() enforces at programming time.
TEST_F(T1HalTest, verifyRxSquelchDisabledByDefaultOnZr) {
  forEachTransceiverParallel(
      [this](hal_test::TransceiverTestResult& result, int tcvrId) {
        auto* cmisModule = dynamic_cast<CmisModule*>(getModule(tcvrId));
        HAL_CHECK_FATAL_VOID(
            result,
            cmisModule != nullptr,
            fmt::format("Transceiver {} is not CMIS", tcvrId));

        // Bring the module out of the low power state it enters after the hard
        // reset done in SetUp. This writes Module Control (Lower Page, Byte
        // 26), whose bit 5 selects the squelch method (OMA vs Pave), but it
        // does not touch RX Output Squelch Disable (Page 10h, Byte 139); nor
        // does refresh(), which only reads. The one thing that would set byte
        // 139, disableTxRxSquelchForTunableOptics(), runs from the port
        // programming path this test never reaches. So the values read below
        // are the module's own power-on defaults.
        cmisModule->releaseModuleLowPowerModeLocked();
        HAL_CHECK_FATAL_VOID(
            result,
            cmisModule->moduleReadyStatePoll(),
            fmt::format(
                "Transceiver {} did not reach ready state after releasing low power",
                tcvrId));

        cmisModule->detectPresence();
        cmisModule->refresh();

        const auto settings =
            *cmisModule->getTransceiverInfo().tcvrState()->settings();
        const auto& hostLaneSettings = settings.hostLaneSettings();
        HAL_CHECK_FATAL_VOID(
            result,
            hostLaneSettings.has_value() && !hostLaneSettings->empty(),
            fmt::format(
                "Transceiver {} reported no host lane settings", tcvrId));

        for (const auto& hostLane : *hostLaneSettings) {
          // Report an absent rxSquelch distinctly: that means we could not read
          // the module's state, which is a different problem from reading it
          // and finding squelch still enabled.
          HAL_CHECK(
              result,
              hostLane.rxSquelch().has_value(),
              fmt::format(
                  "Transceiver {} lane {}: module did not report rxSquelch",
                  tcvrId,
                  *hostLane.lane()));
          HAL_CHECK(
              result,
              hostLane.rxSquelch().value_or(false),
              fmt::format(
                  "Transceiver {} lane {}: RX squelch is not disabled by default",
                  tcvrId,
                  *hostLane.lane()));
        }
      },
      [this](int tcvrId) {
        // Only ZR optics are in scope for this check.
        auto* module = getModule(tcvrId);
        module->detectPresence();
        module->updateQsfpData();
        return hal_test::isZrModule(module);
      });
}

} // namespace facebook::fboss
