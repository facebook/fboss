// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <chrono>
#include <sstream>

#include <folly/logging/xlog.h>

#include "fboss/qsfp_service/test/hal_test/HalTest.h"
#include "fboss/qsfp_service/test/hal_test/HalTestUtils.h"

namespace facebook::fboss {

namespace {
constexpr auto kUpgradeTimeout = std::chrono::minutes(6);
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

} // namespace facebook::fboss
