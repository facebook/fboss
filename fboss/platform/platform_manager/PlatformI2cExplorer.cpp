// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/platform/platform_manager/PlatformI2cExplorer.h"

#include <folly/FileUtil.h>
#include <folly/String.h>
#include <folly/logging/xlog.h>
#include <re2/re2.h>

#include <filesystem>
#include <stdexcept>

namespace fs = std::filesystem;

namespace {

const re2::RE2 kI2cBusNameRegex("i2c-\\d+");

bool isInI2cBusNameFormat(const std::string& busName) {
  return re2::RE2::FullMatch(busName, kI2cBusNameRegex);
}

int32_t getBusNum(const std::string& busName) {
  if (!isInI2cBusNameFormat(busName)) {
    XLOG(ERR) << "Invalid i2c bus name: " << busName;
    throw std::runtime_error("Invalid i2c bus name: " + busName);
  }
  return folly::to<int32_t>(busName.substr(busName.find_last_of("-") + 1));
}

std::string getBusName(const fs::path& busPath) {
  auto busNameFile = busPath / "name";
  if (!fs::exists(busNameFile)) {
    throw std::runtime_error(
        fmt::format("{} does not exist", busNameFile.string()));
  }
  std::string busName{};
  if (!folly::readFile(busNameFile.string().c_str(), busName)) {
    throw std::runtime_error(
        fmt::format("Could not read {}", busNameFile.string()));
  }
  return folly::trimWhitespace(busName).str();
}

} // namespace

namespace facebook::fboss::platform::platform_manager {

std::map<std::string, std::string> PlatformI2cExplorer::getKernelAssignedNames(
    const std::vector<std::string>& i2cBussesFromCpu) {
  std::map<std::string, std::string> kernelAssignedNames;
  auto deviceRoot = fs::path("/sys/bus/i2c/devices");
  for (const auto& dirEntry : fs::directory_iterator(deviceRoot)) {
    if (dirEntry.path().filename().string().rfind("i2c-", 0) == 0) {
      auto busName = getBusName(dirEntry.path());
      if (std::find(
              i2cBussesFromCpu.begin(), i2cBussesFromCpu.end(), busName) !=
          i2cBussesFromCpu.end()) {
        kernelAssignedNames[busName] = dirEntry.path().filename();
      }
    }
  }
  return kernelAssignedNames;
}

std::string PlatformI2cExplorer::getFruTypeName(const std::string&) {
  throw std::runtime_error("Not implemented yet.");
}

bool PlatformI2cExplorer::isI2cDevicePresent(
    const std::string& busName,
    uint8_t addr) {
  return fs::exists(fs::path(getDeviceI2cPath(busName, addr)) / "name");
}

std::optional<std::string> PlatformI2cExplorer::getI2cDeviceName(
    const std::string& busName,
    uint8_t addr) {
  std::string deviceName{};
  auto deviceNameFile = fs::path(getDeviceI2cPath(busName, addr)) / "name";
  if (!fs::exists(deviceNameFile)) {
    XLOG(ERR) << fmt::format("{} does not exist", deviceNameFile.string());
    return std::nullopt;
  }
  if (!folly::readFile(deviceNameFile.string().c_str(), deviceName)) {
    XLOG(ERR) << fmt::format("Could not read {}", deviceNameFile.string());
    return std::nullopt;
  }
  return folly::trimWhitespace(deviceName).str();
}

void PlatformI2cExplorer::createI2cDevice(
    const std::string& deviceName,
    const std::string& busName,
    uint8_t addr) {
  if (isI2cDevicePresent(busName, addr)) {
    auto existingDeviceName = getI2cDeviceName(busName, addr);
    if (existingDeviceName && existingDeviceName.value() == deviceName) {
      XLOG(INFO) << fmt::format(
          "Device {} already exists at bus: {}, addr: {}. Skipping creation.",
          deviceName,
          busName,
          addr);
      return;
    }
    XLOG(ERR) << fmt::format(
        "Creation of i2c device {} at bus: {}, addr: {} failed. "
        "Another device already present",
        deviceName,
        busName,
        addr);
    throw std::runtime_error("Creation of i2c device failed");
  }
  auto cmd = fmt::format(
      "echo {} {} > /sys/bus/i2c/devices/{}/new_device",
      deviceName,
      addr,
      busName);
  auto [exitStatus, standardOut] = platformUtils_->execCommand(cmd);
  XLOG_IF(INFO, !standardOut.empty()) << standardOut;
  if (exitStatus != 0) {
    XLOG(ERR) << fmt::format(
        "Creation of i2c device for {} at bus: {}, addr: {} "
        "failed with exit status {}",
        deviceName,
        busName,
        addr,
        exitStatus);
    throw std::runtime_error("Creation of i2c device failed");
  }
  XLOG(INFO) << fmt::format(
      "Created i2c device {} at {}",
      deviceName,
      getDeviceI2cPath(busName, addr));
}

std::vector<std::string> PlatformI2cExplorer::getMuxChannelI2CBuses(
    const std::string& busName,
    uint8_t addr) {
  auto devicePath = fs::path(getDeviceI2cPath(busName, addr));
  if (!fs::is_directory(devicePath)) {
    throw std::runtime_error(
        fmt::format("{} is not a directory.", devicePath.string()));
  }
  // This is an implementation of
  // find [devicePath] -type l -name channel* -exec readlink -f {} \; |
  // xargs --max-args 1 basename"
  std::vector<std::string> channelBusNames;
  for (const auto& dirEntry : fs::directory_iterator(devicePath)) {
    if (dirEntry.path().filename().string().rfind("channel-", 0) == 0) {
      if (!dirEntry.is_symlink()) {
        throw std::runtime_error(
            fmt::format("{} is not a symlink.", dirEntry.path().string()));
      }
      channelBusNames.push_back(fs::read_symlink(dirEntry).filename().string());
    }
  }
  return channelBusNames;
}

std::string PlatformI2cExplorer::getDeviceI2cPath(
    const std::string& busName,
    uint8_t addr) {
  return fmt::format("/sys/bus/i2c/devices/{}-{:04}", getBusNum(busName), addr);
}

} // namespace facebook::fboss::platform::platform_manager
