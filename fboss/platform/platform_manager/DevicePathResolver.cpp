// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/platform/platform_manager/DevicePathResolver.h"

#include <chrono>
#include <filesystem>
#include <stdexcept>
#include <thread>

#include <folly/logging/xlog.h>

#include "fboss/platform/platform_manager/PciExplorer.h"
#include "fboss/platform/platform_manager/Utils.h"

namespace fs = std::filesystem;

namespace {
const re2::RE2 kHwmonRe{"hwmon\\d+"};
const re2::RE2 kIioDeviceRe{"iio:device\\d+"};

// Retry timeout configuration for sensor device path resolution
// Initial delay = 10ms, max exp backoff delay = 500ms, max total delay = 5s
constexpr int kInitialRetryDelayMs = 10;
constexpr int kMaxRetryDelayMs = 500;
constexpr int kTotalTimeoutMs = 5000;
} // namespace

namespace facebook::fboss::platform::platform_manager {

DevicePathResolver::DevicePathResolver(const DataStore& dataStore)
    : dataStore_(dataStore) {}

std::string DevicePathResolver::resolveSensorPath(
    const std::string& devicePath) const {
  // Retry with exponential backoff to handle kernel driver probe delays in
  // subsystem device creation
  int retryDelayMs = kInitialRetryDelayMs;
  int totalElapsedMs = 0;
  int retryCount = 0;
  auto sensorPath = dataStore_.getSysfsPath(devicePath);

  while (totalElapsedMs < kTotalTimeoutMs) {
    try {
      if (sensorPath.find("/hwmon") == std::string::npos &&
          fs::exists(fmt::format("{}/hwmon", sensorPath))) {
        sensorPath = fmt::format("{}/hwmon", sensorPath);
      }
      for (const auto& dirEntry : fs::directory_iterator(sensorPath)) {
        auto dirName = dirEntry.path().filename();
        if (re2::RE2::FullMatch(dirName.string(), kHwmonRe) ||
            re2::RE2::FullMatch(dirName.string(), kIioDeviceRe)) {
          if (retryCount > 0) {
            XLOG(INFO) << fmt::format(
                "Found sensor device for {} after {} retries ({} ms)",
                devicePath,
                retryCount,
                totalElapsedMs);
          }
          return dirEntry.path();
        }
      }
    } catch (const fs::filesystem_error& e) {
      XLOG(DBG3) << fmt::format(
          "Filesystem error while checking {}: {}", sensorPath, e.what());
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(retryDelayMs));
    totalElapsedMs += retryDelayMs;
    retryCount++;
    retryDelayMs = std::min(retryDelayMs * 2, kMaxRetryDelayMs);

    if (retryCount == 1 || retryCount % 10 == 0) {
      XLOG(INFO) << fmt::format(
          "Waiting for sensor device for {} (retry {}, elapsed {} ms)",
          devicePath,
          retryCount,
          totalElapsedMs);
    }
  }
  throw std::runtime_error(
      fmt::format(
          "Couldn't find hwmon[num] nor iio:device[num] folder under {} for {} "
          "after {} seconds",
          sensorPath,
          devicePath,
          kTotalTimeoutMs / 1000));
}

std::string DevicePathResolver::resolveEepromPath(
    const std::string& devicePath) {
  auto i2cDevicPath = dataStore_.getSysfsPath(devicePath);
  auto eepromPath = i2cDevicPath + "/eeprom";
  if (!fs::exists(eepromPath)) {
    throw std::runtime_error(
        fmt::format(
            "Fail to resolve EepromPath for {} - {} doesn't exist",
            devicePath,
            eepromPath));
  }
  return eepromPath;
}

std::string DevicePathResolver::resolveI2cBusPath(
    const std::string& devicePath) {
  const auto [slotPath, deviceName] = Utils().parseDevicePath(devicePath);
  return fmt::format(
      "/dev/i2c-{}", dataStore_.getI2cBusNum(slotPath, deviceName));
}

std::string DevicePathResolver::resolvePciSubDevSysfsPath(
    const std::string& devicePath) {
  auto sysfsPath = dataStore_.getSysfsPath(devicePath);
  if (!fs::exists(sysfsPath)) {
    throw std::runtime_error(
        fmt::format(
            "Fail to resolve PciSubDevice's SysfsPath for {} - {} no longer exists",
            devicePath,
            sysfsPath));
  }
  return sysfsPath;
}

std::string DevicePathResolver::resolvePciSubDevCharDevPath(
    const std::string& devicePath) const {
  auto charDevPath = dataStore_.getCharDevPath(devicePath);
  if (!fs::exists(charDevPath)) {
    throw std::runtime_error(
        fmt::format(
            "Fail to resolve PciSubDevice's CharDevPath for {} - {} no longer exists",
            devicePath,
            charDevPath));
  }
  return charDevPath;
}

std::optional<std::string> DevicePathResolver::tryResolveI2cDevicePath(
    const std::string& devicePath) {
  try {
    return dataStore_.getSysfsPath(devicePath);
  } catch (const std::exception& ex) {
    XLOG(ERR) << fmt::format(
        "Failed to resolve I2cDevicePath from {}. Reason: {}",
        devicePath,
        ex.what());
  }
  return std::nullopt;
}

std::string DevicePathResolver::resolvePciDevicePath(
    const std::string& devicePath) {
  // Check if sysfs path is stored in DataStore (e.g info-rom)
  // Otherwise, try to construct PciDevice sysfs path via config.
  // TODO: rely on dataStore_ as part of efforts in D52785459
  if (dataStore_.hasSysfsPath(devicePath)) {
    return dataStore_.getSysfsPath(devicePath);
  }
  const auto [slotPath, deviceName] = Utils().parseDevicePath(devicePath);
  auto pciDeviceConfig = getPciDeviceConfig(slotPath, deviceName);
  auto pciDevice = PciDevice(pciDeviceConfig);
  if (!fs::exists(pciDevice.sysfsPath())) {
    throw std::runtime_error(
        fmt::format("{} is not plugged-in to the platform", deviceName));
  }
  return pciDevice.sysfsPath();
}

std::optional<std::string> DevicePathResolver::resolvePresencePath(
    const std::string& devicePath,
    const std::string& presenceFileName) const {
  XLOG(INFO) << fmt::format(
      "Resolving SysfsPath for DevicePath {}", devicePath);
  if (!dataStore_.hasSysfsPath(devicePath)) {
    XLOG(INFO) << fmt::format(
        "Couldn't find SysfsPath for DevicePath {}", devicePath);
    return std::nullopt;
  }
  fs::path sysfsPath = dataStore_.getSysfsPath(devicePath);
  if (fs::exists(sysfsPath / presenceFileName)) {
    XLOG(INFO) << fmt::format(
        "Resolved PresencePath to {}", (sysfsPath / presenceFileName).string());
    return sysfsPath / presenceFileName;
  }
  try {
    auto sensorPath = fs::path(resolveSensorPath(devicePath));
    XLOG(INFO) << fmt::format(
        "Resolved PresencePath to {}",
        (sensorPath / presenceFileName).string());
    return sensorPath / presenceFileName;
  } catch (const std::exception& e) {
    XLOG(ERR) << fmt::format(
        "Couldn't find PresencePath under /hwmon/hwmon[n]. Reason: {}",
        e.what());
  }
  XLOG(ERR) << fmt::format(
      "Failed to resolve PresencePath under ({}) nor ({})",
      (sysfsPath / presenceFileName).string(),
      (sysfsPath / "/hwmon/hwmon[n]").string());
  return std::nullopt;
}

PciDeviceConfig DevicePathResolver::getPciDeviceConfig(
    const std::string& slotPath,
    const std::string& deviceName) {
  auto pmUnitConfig = dataStore_.resolvePmUnitConfig(slotPath);
  auto pciDeviceConfig = std::find_if(
      pmUnitConfig.pciDeviceConfigs()->begin(),
      pmUnitConfig.pciDeviceConfigs()->end(),
      [&](auto pciDeviceConfig) {
        return *pciDeviceConfig.pmUnitScopedName() == deviceName;
      });
  if (pciDeviceConfig == pmUnitConfig.pciDeviceConfigs()->end()) {
    throw std::runtime_error(
        fmt::format(
            "Couldn't find PciDeviceConfig for {} at {}",
            deviceName,
            slotPath));
  }
  return *pciDeviceConfig;
}
} // namespace facebook::fboss::platform::platform_manager
