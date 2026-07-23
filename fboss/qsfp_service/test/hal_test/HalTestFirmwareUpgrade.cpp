// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <unistd.h>
#include <chrono>
#include <sstream>
#include <string>
#include <thread>

#include <fmt/format.h>
#include <folly/Random.h>
#include <folly/ScopeGuard.h>
#include <folly/io/IOBuf.h>
#include <folly/logging/xlog.h>

#include "fboss/qsfp_service/if/gen-cpp2/transceiver_types.h"
#include "fboss/qsfp_service/module/QsfpModule.h"
#include "fboss/qsfp_service/test/hal_test/BspTransceiverImpl.h"
#include "fboss/qsfp_service/test/hal_test/HalTest.h"
#include "fboss/qsfp_service/test/hal_test/HalTestUtils.h"

DECLARE_int32(firmware_upgrade_attempts);

namespace facebook::fboss {

namespace {
constexpr auto kUpgradeTimeout = std::chrono::minutes(6);

constexpr int kInterruptNumIterations = 5;
// Interrupt the upgrade at a random point in [min, max] seconds.
constexpr unsigned int kInterruptMinSec = 30;
constexpr unsigned int kInterruptMaxSec = 60;
constexpr unsigned int kPostInterruptSettleSec = 5;
constexpr int kRecoveryReadRetries = 10;

// Confirm the module is responsive after the interrupt by reading page0 offset0
// (the module identifier) without throwing. We don't check the value -- the
// interrupted upgrade should have left the running image intact, so recovery
// just means the module answers I2C again. Retries because the module needs a
// moment after the reset and concurrent cleanup can briefly return EBUSY.
bool readModuleRegisterSucceeds(QsfpModule* module, std::string& lastErr) {
  for (int i = 0; i < kRecoveryReadRetries; ++i) {
    try {
      // Refresh presence so readTransceiver actually issues the I2C read.
      module->detectPresence();
      TransceiverIOParameters param;
      param.offset() = 0; // module identifier byte
      auto buf = module->readTransceiver(std::move(param));
      if (buf && buf->length() > 0) {
        return true;
      }
      lastErr = "module not present / no data returned";
    } catch (const std::exception& e) {
      lastErr = e.what();
    }
    /* sleep override */
    sleep(1);
  }
  return false;
}
} // namespace

TEST_F(T0HalTest, FirmwareUpgrade) {
  forEachTransceiverParallel(
      [this](hal_test::TransceiverTestResult& result, int tcvrId) {
        const auto& config = getConfig();
        const HalTestTransceiverEntry* entry = nullptr;
        for (const auto& e : *config.transceivers()) {
          if (*e.id() == tcvrId) {
            entry = &e;
            break;
          }
        }
        const auto& currentFw = *entry->startupConfig()->firmware();
        const auto& previousFw = entry->previousFirmware().has_value()
            ? *entry->previousFirmware()
            : currentFw;

        // previousFirmware -> currentFirmware
        {
          XLOG(INFO) << "Transceiver " << tcvrId
                     << ": ensuring previousFirmware before upgrade";
          hal_test::upgradeFirmware(getModule(tcvrId), previousFw);

          auto start = std::chrono::steady_clock::now();
          XLOG(INFO) << "Transceiver " << tcvrId
                     << ": upgrading previousFirmware -> currentFirmware";
          HAL_CHECK_FATAL_VOID(
              result,
              hal_test::upgradeFirmware(getModule(tcvrId), currentFw),
              fmt::format(
                  "Transceiver {} failed to upgrade from previousFirmware to currentFirmware",
                  tcvrId));

          auto elapsed = std::chrono::steady_clock::now() - start;
          std::ostringstream oss;
          oss << "Transceiver " << tcvrId
              << " firmware upgrade previousFirmware -> currentFirmware exceeded "
              << std::chrono::duration_cast<std::chrono::seconds>(
                     kUpgradeTimeout)
                     .count()
              << "s timeout (took "
              << std::chrono::duration_cast<std::chrono::seconds>(elapsed)
                     .count()
              << "s)";
          HAL_CHECK(result, elapsed < kUpgradeTimeout, oss.str());
        }

        // currentFirmware -> previousFirmware
        {
          XLOG(INFO) << "Transceiver " << tcvrId
                     << ": ensuring currentFirmware before downgrade";
          hal_test::upgradeFirmware(getModule(tcvrId), currentFw);

          auto start = std::chrono::steady_clock::now();
          XLOG(INFO) << "Transceiver " << tcvrId
                     << ": upgrading currentFirmware -> previousFirmware";
          HAL_CHECK_FATAL_VOID(
              result,
              hal_test::upgradeFirmware(getModule(tcvrId), previousFw),
              fmt::format(
                  "Transceiver {} failed to upgrade from currentFirmware to previousFirmware",
                  tcvrId));

          auto elapsed = std::chrono::steady_clock::now() - start;
          std::ostringstream oss;
          oss << "Transceiver " << tcvrId
              << " firmware upgrade currentFirmware -> previousFirmware exceeded "
              << std::chrono::duration_cast<std::chrono::seconds>(
                     kUpgradeTimeout)
                     .count()
              << "s timeout (took "
              << std::chrono::duration_cast<std::chrono::seconds>(elapsed)
                     .count()
              << "s)";
          HAL_CHECK(result, elapsed < kUpgradeTimeout, oss.str());
        }
      },
      [this](int tcvrId) {
        const auto& config = getConfig();
        for (const auto& e : *config.transceivers()) {
          if (*e.id() == tcvrId) {
            return true;
          }
        }
        return false;
      });
}

// Verifies a CMIS optic recovers when a firmware upgrade is interrupted
// mid-flight by a hard reset. Each iteration starts a real firmware download on
// a worker thread and hard resets the module partway through. Retries are
// disabled (FLAGS_firmware_upgrade_attempts = 1) so the reset actually
// interrupts the upgrade; the interrupted upgrade is EXPECTED to fail, so that
// is not treated as a test failure. Recovery is verified by confirming the
// module still answers a register read afterward -- i.e. it fell back to its
// running image and is responsive, not bricked. Firmware retrieval and the
// upgrade reuse hal_test::upgradeFirmware exactly like the FirmwareUpgrade test
// above; the interrupting hard reset goes through BspTransceiverImpl.
TEST_F(T0HalTest, FirmwareUpgradeInterruptRecovery) {
  // Disable the upgrade retry loop for this test: we want a single hard reset
  // to actually interrupt the upgrade, not have it silently restart.
  auto prevAttempts = FLAGS_firmware_upgrade_attempts;
  FLAGS_firmware_upgrade_attempts = 1;
  SCOPE_EXIT {
    FLAGS_firmware_upgrade_attempts = prevAttempts;
  };

  forEachTransceiverParallel(
      [this](hal_test::TransceiverTestResult& result, int tcvrId) {
        auto* impl = getImpl(tcvrId);

        const auto& config = getConfig();
        const HalTestTransceiverEntry* entry = nullptr;
        for (const auto& e : *config.transceivers()) {
          if (*e.id() == tcvrId) {
            entry = &e;
            break;
          }
        }
        HAL_CHECK_FATAL_VOID(
            result,
            entry != nullptr,
            fmt::format("Transceiver {} has no config entry", tcvrId));

        // Always upgrade to the configured firmware. Because we interrupt every
        // upgrade, the module never completes it and keeps running its existing
        // image, so each iteration is a genuine download to interrupt.
        const auto& targetFw = *entry->startupConfig()->firmware();

        for (int iter = 1; iter <= kInterruptNumIterations; ++iter) {
          XLOG(INFO) << "=== Transceiver " << tcvrId << " iteration " << iter
                     << "/" << kInterruptNumIterations << " ===";

          // Each interrupting hard reset (step 2) leaves the module reset and
          // in low power, so bring it back to ready before every upgrade --
          // else upgradeFirmware's refresh reads fail and it throws before the
          // download even starts.
          hal_test::ensureModuleReady(getModule(tcvrId));

          // Step 1: start the firmware download on a worker thread. The
          // interrupt below is expected to make it fail; that failure is not a
          // test failure, so just log it.
          std::thread upgradeThread([&]() {
            try {
              hal_test::upgradeFirmware(getModule(tcvrId), targetFw);
            } catch (const std::exception& e) {
              XLOG(INFO) << "Transceiver " << tcvrId
                         << ": upgrade ended as expected after interrupt: "
                         << e.what();
            }
          });

          // Step 2: interrupt the download with a hard reset.
          const unsigned int interruptAfterSec =
              folly::Random::rand32(kInterruptMinSec, kInterruptMaxSec + 1);
          XLOG(INFO) << "Transceiver " << tcvrId << ": interrupting upgrade in "
                     << interruptAfterSec << "s";
          /* sleep override */
          sleep(interruptAfterSec);
          XLOG(INFO) << "Transceiver " << tcvrId
                     << ": issuing interrupting hard reset";
          impl->triggerQsfpHardReset();

          upgradeThread.join();

          // Step 3: recovery -- the module must still answer a register read
          // after the interrupted upgrade (it should fall back to its running
          // image). We don't assert on the value, only that the read succeeds.
          /* sleep override */
          sleep(kPostInterruptSettleSec);
          std::string readErr;
          bool recovered =
              readModuleRegisterSucceeds(getModule(tcvrId), readErr);
          HAL_CHECK(
              result,
              recovered,
              fmt::format(
                  "Transceiver {} iteration {}/{}: module did not respond to a "
                  "register read after interrupted upgrade: {}",
                  tcvrId,
                  iter,
                  kInterruptNumIterations,
                  readErr));
          if (!recovered) {
            XLOG(ERR) << "Transceiver " << tcvrId
                      << " failed to recover on iteration " << iter << "/"
                      << kInterruptNumIterations << ". Stopping.";
            return;
          }
          XLOG(INFO) << "Transceiver " << tcvrId << " iteration " << iter
                     << ": PASS";
        }
      },
      [this](int tcvrId) {
        // Only run on transceivers that have a firmware configured to upgrade
        // to.
        const auto& config = getConfig();
        for (const auto& e : *config.transceivers()) {
          if (*e.id() == tcvrId) {
            return !e.startupConfig()->firmware()->versions()->empty();
          }
        }
        return false;
      });
}

} // namespace facebook::fboss
