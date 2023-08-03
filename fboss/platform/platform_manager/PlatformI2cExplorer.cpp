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
} // namespace

namespace facebook::fboss::platform::platform_manager {

std::map<std::string, std::string> PlatformI2cExplorer::getBusesfromBsp(
    const std::vector<std::string>&) {
  throw std::runtime_error("Not implemented yet.");
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

bool PlatformI2cExplorer::createI2cMux(
    const std::string&,
    const std::string&,
    uint8_t,
    uint8_t) {
  throw std::runtime_error("Not implemented yet.");
}

std::vector<std::string> PlatformI2cExplorer::getMuxChannelI2CBuses(
    const std::string&,
    uint8_t) {
  throw std::runtime_error("Not implemented yet.");
}

std::string PlatformI2cExplorer::getDeviceI2cPath(
    const std::string& busName,
    uint8_t addr) {
  return fmt::format("/sys/bus/i2c/devices/{}-{:04}", getBusNum(busName), addr);
}

} // namespace facebook::fboss::platform::platform_manager
