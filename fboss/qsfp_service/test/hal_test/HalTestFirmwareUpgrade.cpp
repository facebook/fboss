// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <chrono>

#include <folly/logging/xlog.h>

#include "fboss/qsfp_service/test/hal_test/HalTest.h"
#include "fboss/qsfp_service/test/hal_test/HalTestUtils.h"

namespace facebook::fboss {

namespace {
constexpr auto kUpgradeTimeout = std::chrono::minutes(6);
} // namespace

TEST_F(T0HalTest, FirmwareUpgrade) {
  for (auto tcvrId : getPresentTransceiverIds()) {
    const auto& config = getConfig();
    const HalTestTransceiverEntry* entry = nullptr;
    for (const auto& e : *config.transceivers()) {
      if (*e.id() == tcvrId) {
        entry = &e;
        break;
      }
    }
    if (!entry) {
      continue;
    }

    const auto& currentFirmware = *entry->startupConfig()->firmware();
    const auto& previousFirmware = entry->previousFirmware().has_value()
        ? *entry->previousFirmware()
        : currentFirmware;

    // previousFirmware -> currentFirmware
    {
      auto start = std::chrono::steady_clock::now();

      XLOG(INFO) << "Transceiver " << tcvrId
                 << ": ensuring previousFirmware before upgrade";
      hal_test::upgradeFirmware(getModule(tcvrId), previousFirmware);

      XLOG(INFO) << "Transceiver " << tcvrId
                 << ": upgrading previousFirmware -> currentFirmware";
      ASSERT_TRUE(hal_test::upgradeFirmware(getModule(tcvrId), currentFirmware))
          << "Transceiver " << tcvrId
          << " failed to upgrade from previousFirmware to currentFirmware";

      auto elapsed = std::chrono::steady_clock::now() - start;
      EXPECT_LT(elapsed, kUpgradeTimeout)
          << "Transceiver " << tcvrId
          << " firmware upgrade previousFirmware -> currentFirmware exceeded "
          << std::chrono::duration_cast<std::chrono::seconds>(kUpgradeTimeout)
                 .count()
          << "s timeout (took "
          << std::chrono::duration_cast<std::chrono::seconds>(elapsed).count()
          << "s)";
    }

    // currentFirmware -> previousFirmware
    {
      auto start = std::chrono::steady_clock::now();

      XLOG(INFO) << "Transceiver " << tcvrId
                 << ": ensuring currentFirmware before downgrade";
      hal_test::upgradeFirmware(getModule(tcvrId), currentFirmware);

      XLOG(INFO) << "Transceiver " << tcvrId
                 << ": upgrading currentFirmware -> previousFirmware";
      ASSERT_TRUE(
          hal_test::upgradeFirmware(getModule(tcvrId), previousFirmware))
          << "Transceiver " << tcvrId
          << " failed to upgrade from currentFirmware to previousFirmware";

      auto elapsed = std::chrono::steady_clock::now() - start;
      EXPECT_LT(elapsed, kUpgradeTimeout)
          << "Transceiver " << tcvrId
          << " firmware upgrade currentFirmware -> previousFirmware exceeded "
          << std::chrono::duration_cast<std::chrono::seconds>(kUpgradeTimeout)
                 .count()
          << "s timeout (took "
          << std::chrono::duration_cast<std::chrono::seconds>(elapsed).count()
          << "s)";
    }
  }
}

} // namespace facebook::fboss
