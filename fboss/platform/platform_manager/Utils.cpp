// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/platform/platform_manager/Utils.h"

#include <gpiod.h>
#include <cctype>
#include <filesystem>
#include <stdexcept>

#include <folly/FileUtil.h>
#include <folly/logging/xlog.h>
#include <re2/re2.h>
#include <thrift/lib/cpp2/protocol/Serializer.h>

#include "fboss/lib/GpiodLine.h"
#include "fboss/platform/config_lib/ConfigLib.h"
#include "fboss/platform/helpers/PlatformNameLib.h"
#include "fboss/platform/platform_manager/ConfigValidator.h"

namespace fs = std::filesystem;
using namespace facebook::fboss::platform;

namespace {
const re2::RE2 kPmDeviceParseRe{"(?P<SlotPath>.*)\\[(?P<DeviceName>.*)\\]"};
const re2::RE2 kGpioChipNameRe{"gpiochip\\d+"};
const std::string kGpioChip = "gpiochip";

const re2::RE2 kWatchdogNameRe{"watchdog(\\d+)"};
const std::string kWatchdog = "watchdog";
} // namespace

namespace facebook::fboss::platform::platform_manager {

// Verify that the platform name from the config and dmidecode match.  This
// is necessary to prevent an incorrect config from being used on any platform.
void verifyPlatformNameMatches(
    const std::string& platformNameInConfig,
    const std::string& platformNameFromBios) {
  std::string platformNameInConfigUpper(platformNameInConfig);
  std::transform(
      platformNameInConfigUpper.begin(),
      platformNameInConfigUpper.end(),
      platformNameInConfigUpper.begin(),
      ::toupper);

  if (platformNameInConfigUpper == platformNameFromBios) {
    return;
  }

#ifndef IS_OSS
  if (platformNameFromBios == "NOT SPECIFIED") {
    XLOG(ERR)
        << "Platform name is not specified in BIOS. Skipping comparison with config.";
    return;
  }
#endif

  XLOGF(
      FATAL,
      "Platform name in config does not match the inferred platform name from "
      "bios. Config: {}, Inferred name from BIOS {} ",
      platformNameInConfigUpper,
      platformNameFromBios);
}

PlatformConfig Utils::getConfig() {
  std::string platformNameFromBios =
      helpers::PlatformNameLib().getPlatformNameFromBios(true);
  std::string configJson =
      ConfigLib().getPlatformManagerConfig(platformNameFromBios);
  PlatformConfig config;
  try {
    apache::thrift::SimpleJSONSerializer::deserialize<PlatformConfig>(
        configJson, config);
  } catch (const std::exception& e) {
    XLOG(ERR) << "Failed to deserialize platform config: " << e.what();
    throw;
  }
  XLOG(DBG2) << apache::thrift::SimpleJSONSerializer::serialize<std::string>(
      config);

  verifyPlatformNameMatches(*config.platformName(), platformNameFromBios);

  if (!ConfigValidator().isValid(config)) {
    XLOG(ERR) << "Invalid platform config";
    throw std::runtime_error("Invalid platform config");
  }
  return config;
}

std::pair<std::string, std::string> Utils::parseDevicePath(
    const std::string& devicePath) {
  if (!ConfigValidator().isValidDevicePath(devicePath)) {
    throw std::runtime_error(fmt::format("Invalid DevicePath {}", devicePath));
  }
  std::string slotPath, deviceName;
  CHECK(RE2::FullMatch(devicePath, kPmDeviceParseRe, &slotPath, &deviceName));
  // Remove trailling '/' (e.g /abc/dfg/)
  CHECK_EQ(slotPath.back(), '/');
  if (slotPath.length() > 1) {
    slotPath.pop_back();
  }
  return {slotPath, deviceName};
}

std::string Utils::createDevicePath(
    const std::string& slotPath,
    const std::string& deviceName) {
  return fmt::format("{}/[{}]", slotPath == "/" ? "" : slotPath, deviceName);
}

std::string Utils::resolveGpioChipCharDevPath(const std::string& sysfsPath) {
  auto failMsg = "Fail to resolve GpioChip CharDevPath";
  if (!fs::exists(sysfsPath)) {
    throw std::runtime_error(
        fmt::format("{}. Reason: {} doesn't exist", failMsg, sysfsPath));
  }
  std::optional<uint16_t> gpioChipNum{std::nullopt};
  for (const auto& dirEntry : fs::directory_iterator(sysfsPath)) {
    if (re2::RE2::FullMatch(
            dirEntry.path().filename().string(), kGpioChipNameRe)) {
      gpioChipNum = folly::to<uint16_t>(
          dirEntry.path().filename().string().substr(kGpioChip.length()));
    }
  }
  if (!gpioChipNum) {
    throw std::runtime_error(fmt::format(
        "{}. Reason: Couldn't find gpio chip under {}", failMsg, sysfsPath));
  }
  auto charDevPath = fmt::format("/dev/gpiochip{}", *gpioChipNum);
  if (!fs::exists(charDevPath)) {
    throw std::runtime_error(fmt::format(
        "{}. Reason: {} does not exist in the system", failMsg, charDevPath));
  }
  return charDevPath;
}

std::string Utils::resolveWatchdogCharDevPath(const std::string& sysfsPath) {
  auto failMsg = "Failed to resolve Watchdog CharDevPath";
  if (!fs::exists(sysfsPath)) {
    throw std::runtime_error(
        fmt::format("{}. Reason: {} doesn't exist", failMsg, sysfsPath));
  }

  fs::path watchdogDir = fs::path(sysfsPath) / "watchdog";
  if (!fs::exists(watchdogDir)) {
    throw std::runtime_error(fmt::format(
        "{}. Reason: Couldn't find watchdog directory under {}",
        failMsg,
        sysfsPath));
  }

  std::optional<uint16_t> watchdogNum{std::nullopt};
  for (const auto& dirEntry : fs::directory_iterator(watchdogDir)) {
    if (re2::RE2::FullMatch(
            dirEntry.path().filename().string(), kWatchdogNameRe)) {
      watchdogNum = folly::to<uint16_t>(
          dirEntry.path().filename().string().substr(kWatchdog.length()));
    }
  }
  if (!watchdogNum) {
    throw std::runtime_error(fmt::format(
        "{}. Reason: Couldn't find watchdog under {}", failMsg, sysfsPath));
  }
  auto charDevPath = fmt::format("/dev/watchdog{}", *watchdogNum);
  if (!fs::exists(charDevPath)) {
    throw std::runtime_error(fmt::format(
        "{}. Reason: {} does not exist in the system", failMsg, charDevPath));
  }
  return charDevPath;
}

bool Utils::checkDeviceReadiness(
    std::function<bool()>&& isDeviceReadyFunc,
    const std::string& onWaitMsg,
    std::chrono::seconds maxWaitSecs) {
  if (isDeviceReadyFunc()) {
    return true;
  }
  XLOG(INFO) << onWaitMsg;
  std::chrono::milliseconds waitIntervalMs = std::chrono::milliseconds(50);
  auto start = std::chrono::steady_clock::now();
  do {
    std::this_thread::sleep_for(waitIntervalMs);
    if (isDeviceReadyFunc()) {
      return true;
    }
  } while (std::chrono::steady_clock::now() <= start + maxWaitSecs);
  return false;
}

int Utils::getGpioLineValue(const std::string& charDevPath, int lineIndex)
    const {
  struct gpiod_chip* chip = gpiod_chip_open(charDevPath.c_str());
  // Ensure GpiodLine is destroyed before gpiod_chip_close
  int value = GpiodLine(chip, lineIndex, "gpioline").getValue();
  gpiod_chip_close(chip);
  return value;
};
} // namespace facebook::fboss::platform::platform_manager
