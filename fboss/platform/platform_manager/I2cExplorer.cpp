// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/platform/platform_manager/I2cExplorer.h"

#include <folly/FileUtil.h>
#include <folly/String.h>
#include <folly/logging/xlog.h>

namespace fs = std::filesystem;

namespace {

const re2::RE2 kI2cMuxChannelRegex{"channel-\\d+"};

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

uint16_t I2cExplorer::extractBusNumFromPath(const fs::path& busPath) {
  if (!re2::RE2::FullMatch(busPath.filename().string(), kI2cBusNameRegex)) {
    throw std::runtime_error(
        fmt::format("Invalid path to i2c bus: {}", busPath.string()));
  }
  return folly::to<uint16_t>(
      busPath.string().substr(busPath.string().find_last_of('-') + 1));
}

std::map<std::string, uint16_t> I2cExplorer::getBusNums(
    const std::vector<std::string>& i2cAdaptersFromCpu) {
  std::map<std::string, uint16_t> busNums;
  auto deviceRoot = fs::path("/sys/bus/i2c/devices");
  for (const auto& dirEntry : fs::directory_iterator(deviceRoot)) {
    if (re2::RE2::FullMatch(
            dirEntry.path().filename().string(), kI2cBusNameRegex)) {
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

bool I2cExplorer::isI2cDevicePresent(uint16_t busNum, const I2cAddr& addr)
    const {
  return fs::exists(fs::path(getDeviceI2cPath(busNum, addr)) / "name");
}

std::optional<std::string> I2cExplorer::getI2cDeviceName(
    uint16_t busNum,
    const I2cAddr& addr) {
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

void I2cExplorer::createI2cDevice(
    const std::string& pmUnitScopedName,
    const std::string& deviceName,
    uint16_t busNum,
    const I2cAddr& addr) {
  if (isI2cDevicePresent(busNum, addr)) {
    auto existingDeviceName = getI2cDeviceName(busNum, addr);
    if (existingDeviceName && existingDeviceName.value() == deviceName) {
      XLOG(INFO) << fmt::format(
          "Device {} ({}) already exists at bus: {}, addr: {}. "
          "Skipping creation.",
          pmUnitScopedName,
          deviceName,
          busNum,
          addr.hex2Str());
      return;
    }
    XLOG(ERR) << fmt::format(
        "Creation of i2c device {} ({}) at bus: {}, addr: {} failed. "
        "Another device ({}) is already present",
        pmUnitScopedName,
        deviceName,
        busNum,
        addr.hex2Str(),
        existingDeviceName.value_or("READ_ERROR"));
    throw std::runtime_error("Creation of i2c device failed");
  }
  auto cmd = fmt::format(
      "echo {} {} > /sys/bus/i2c/devices/i2c-{}/new_device",
      deviceName,
      addr.hex2Str(),
      busNum);
  auto [exitStatus, standardOut] = platformUtils_->execCommand(cmd);
  XLOG_IF(INFO, !standardOut.empty()) << standardOut;
  if (exitStatus != 0) {
    XLOG(ERR) << fmt::format(
        "Creation of i2c device for {} ({}) at bus: {}, addr: {} "
        "failed with exit status {}",
        pmUnitScopedName,
        deviceName,
        busNum,
        addr.hex2Str(),
        exitStatus);
    throw std::runtime_error("Creation of i2c device failed");
  }
  XLOG(INFO) << fmt::format(
      "Created i2c device {} ({}) at {}",
      pmUnitScopedName,
      deviceName,
      getDeviceI2cPath(busNum, addr));
}

std::map<uint16_t, uint16_t> I2cExplorer::getMuxChannelI2CBuses(
    uint16_t busNum,
    const I2cAddr& addr) {
  auto devicePath = fs::path(getDeviceI2cPath(busNum, addr));
  if (!fs::is_directory(devicePath)) {
    throw std::runtime_error(
        fmt::format("{} is not a directory.", devicePath.string()));
  }
  // This is an implementation of
  // find [devicePath] -type l -name channel* -exec readlink -f {} \; |
  // xargs --max-args 1 basename"
  std::map<uint16_t, uint16_t> channelBusNums;
  for (const auto& dirEntry : fs::directory_iterator(devicePath)) {
    if (re2::RE2::FullMatch(
            dirEntry.path().filename().string(), kI2cMuxChannelRegex)) {
      if (!dirEntry.is_symlink()) {
        throw std::runtime_error(
            fmt::format("{} is not a symlink.", dirEntry.path().string()));
      }
      auto channelNum =
          folly::to<uint16_t>(dirEntry.path().filename().string().substr(
              dirEntry.path().filename().string().find_last_of('-') + 1));
      auto channelBusNum =
          extractBusNumFromPath(fs::read_symlink(dirEntry).filename());
      channelBusNums[channelNum] = channelBusNum;
    }
  }
  return channelBusNums;
}

std::string I2cExplorer::getDeviceI2cPath(
    uint16_t busNum,
    const I2cAddr& addr) {
  return fmt::format("/sys/bus/i2c/devices/{}-{}", busNum, addr.hex4Str());
}

} // namespace facebook::fboss::platform::platform_manager
