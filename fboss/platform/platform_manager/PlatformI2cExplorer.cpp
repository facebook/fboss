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

int32_t extractBusNumFromPath(const fs::path& busPath) {
  if (!re2::RE2::FullMatch(busPath.filename().string(), kI2cBusNameRegex)) {
    XLOG(ERR) << "Invalid i2c bus path: " << busPath.string();
    throw std::runtime_error("Invalid i2c bus path: " + busPath.string());
  }
  return folly::to<int32_t>(
      busPath.string().substr(busPath.string().find_last_of("-") + 1));
}

std::string getI2cAdapterName(const fs::path& busPath) {
  auto nameFile = busPath / "name";
  if (!fs::exists(nameFile)) {
    throw std::runtime_error(
        fmt::format("{} does not exist", nameFile.string()));
  }
  std::string i2cAdapterName{};
  if (!folly::readFile(nameFile.string().c_str(), i2cAdapterName)) {
    throw std::runtime_error(
        fmt::format("Could not read {}", nameFile.string()));
  }
  return folly::trimWhitespace(i2cAdapterName).str();
}

} // namespace

namespace facebook::fboss::platform::platform_manager {

std::map<std::string, int16_t> PlatformI2cExplorer::getBusNums(
    const std::vector<std::string>& i2cAdaptersFromCpu) {
  std::map<std::string, int16_t> busNums;
  auto deviceRoot = fs::path("/sys/bus/i2c/devices");
  for (const auto& dirEntry : fs::directory_iterator(deviceRoot)) {
    if (dirEntry.path().filename().string().rfind("i2c-", 0) == 0) {
      auto i2cAdapterName = getI2cAdapterName(dirEntry.path());
      if (std::find(
              i2cAdaptersFromCpu.begin(),
              i2cAdaptersFromCpu.end(),
              i2cAdapterName) != i2cAdaptersFromCpu.end()) {
        busNums[i2cAdapterName] = extractBusNumFromPath(dirEntry.path());
      }
    }
  }
  return busNums;
}

std::string PlatformI2cExplorer::getFruTypeName(const std::string&) {
  throw std::runtime_error("Not implemented yet.");
}

bool PlatformI2cExplorer::isI2cDevicePresent(uint16_t busNum, uint8_t addr) {
  return fs::exists(fs::path(getDeviceI2cPath(busNum, addr)) / "name");
}

std::optional<std::string> PlatformI2cExplorer::getI2cDeviceName(
    uint16_t busNum,
    uint8_t addr) {
  std::string deviceName{};
  auto deviceNameFile = fs::path(getDeviceI2cPath(busNum, addr)) / "name";
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
    uint16_t busNum,
    uint8_t addr) {
  if (isI2cDevicePresent(busNum, addr)) {
    auto existingDeviceName = getI2cDeviceName(busNum, addr);
    if (existingDeviceName && existingDeviceName.value() == deviceName) {
      XLOG(INFO) << fmt::format(
          "Device {} already exists at bus: {}, addr: {:#x}. Skipping creation.",
          deviceName,
          busNum,
          addr);
      return;
    }
    XLOG(ERR) << fmt::format(
        "Creation of i2c device {} at bus: {}, addr: {:#x} failed. "
        "Another device already present",
        deviceName,
        busNum,
        addr);
    throw std::runtime_error("Creation of i2c device failed");
  }
  auto cmd = fmt::format(
      "echo {} {:#x} > /sys/bus/i2c/devices/i2c-{}/new_device",
      deviceName,
      addr,
      busNum);
  auto [exitStatus, standardOut] = platformUtils_->execCommand(cmd);
  XLOG_IF(INFO, !standardOut.empty()) << standardOut;
  if (exitStatus != 0) {
    XLOG(ERR) << fmt::format(
        "Creation of i2c device for {} at bus: {}, addr: {:#x} "
        "failed with exit status {}",
        deviceName,
        busNum,
        addr,
        exitStatus);
    throw std::runtime_error("Creation of i2c device failed");
  }
  XLOG(INFO) << fmt::format(
      "Created i2c device {} at {}",
      deviceName,
      getDeviceI2cPath(busNum, addr));
}

std::vector<uint16_t> PlatformI2cExplorer::getMuxChannelI2CBuses(
    uint16_t busNum,
    uint8_t addr) {
  auto devicePath = fs::path(getDeviceI2cPath(busNum, addr));
  if (!fs::is_directory(devicePath)) {
    throw std::runtime_error(
        fmt::format("{} is not a directory.", devicePath.string()));
  }
  // This is an implementation of
  // find [devicePath] -type l -name channel* -exec readlink -f {} \; |
  // xargs --max-args 1 basename"
  std::vector<uint16_t> channelBusNums;
  for (const auto& dirEntry : fs::directory_iterator(devicePath)) {
    if (dirEntry.path().filename().string().rfind("channel-", 0) == 0) {
      if (!dirEntry.is_symlink()) {
        throw std::runtime_error(
            fmt::format("{} is not a symlink.", dirEntry.path().string()));
      }
      channelBusNums.push_back(
          extractBusNumFromPath(fs::read_symlink(dirEntry).filename()));
    }
  }
  return channelBusNums;
}

std::string PlatformI2cExplorer::getDeviceI2cPath(
    uint16_t busNum,
    uint8_t addr) {
  return fmt::format("/sys/bus/i2c/devices/{}-{:04x}", busNum, addr);
}

} // namespace facebook::fboss::platform::platform_manager
