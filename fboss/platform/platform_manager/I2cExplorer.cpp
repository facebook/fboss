// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/platform/platform_manager/I2cExplorer.h"

#include <folly/FileUtil.h>
#include <folly/String.h>
#include <folly/logging/xlog.h>
#include <filesystem>
#include <stdexcept>

#include "fboss/lib/i2c/I2cDevIo.h"
#include "fboss/platform/platform_manager/Utils.h"

namespace fs = std::filesystem;

namespace {

const re2::RE2 kI2cMuxChannelRegex{"channel-\\d+"};
constexpr auto kI2cDevCreationWaitSecs = std::chrono::seconds(5);
constexpr auto kCpuI2cBusNumsWaitSecs = 1;

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
  const auto deviceRoot = fs::path("/sys/bus/i2c/devices");
  const int maxRetries = 10;
  for (int attempt = 1; attempt <= maxRetries; attempt++) {
    XLOG(INFO) << "Probing CPU I2C BusNums -- Attempt #" << attempt;
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
    if (busNums.size() == i2cAdaptersFromCpu.size()) {
      XLOG(INFO) << "Probed all CPU I2C BusNums";
      return busNums;
    }
    sleep(kCpuI2cBusNumsWaitSecs);
  }
  throw std::runtime_error(fmt::format(
      "Failed to get all CPU I2C BusNums over {}s",
      kCpuI2cBusNumsWaitSecs * maxRetries));
}

bool I2cExplorer::isI2cDevicePresent(uint16_t busNum, const I2cAddr& addr)
    const {
  auto path = fs::path(getDeviceI2cPath(busNum, addr)) / "driver";
  return fs::exists(path) && fs::directory_entry(path).is_symlink();
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

void I2cExplorer::setupI2cDevice(
    uint16_t busNum,
    const I2cAddr& addr,
    const std::vector<I2cRegData>& initRegSettings) {
  if (initRegSettings.size() == 0) {
    return;
  }

  auto path = getI2cBusCharDevPath(busNum);
  std::unique_ptr<I2cDevIo> i2cIoHandle = std::make_unique<I2cDevIo>(path);

  for (const auto& regInfo : initRegSettings) {
    int size = regInfo.ioBuf()->size();
    if (size == 0) {
      continue;
    }

    XLOG(INFO) << fmt::format(
        "Setting up i2c device {}-{}. Writing {} bytes at register {}",
        busNum,
        addr.hex4Str(),
        size,
        *regInfo.regOffset());
    const uint8_t* buf = (const uint8_t*)regInfo.ioBuf()->data();
    i2cIoHandle->write(addr.raw(), regInfo.get_regOffset(), buf, size);
  }
}

void I2cExplorer::createI2cDevice(
    const std::string& pmUnitScopedName,
    const std::string& deviceName,
    uint16_t busNum,
    const I2cAddr& addr) {
  XLOG(INFO) << fmt::format(
      "Creating i2c device {} ({}) at i2c-{}",
      pmUnitScopedName,
      deviceName,
      busNum);
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
    throw std::runtime_error(fmt::format(
        "Creation of i2c device {} ({}) at bus: {}, addr: {} failed. "
        "Another device ({}) is already present",
        pmUnitScopedName,
        deviceName,
        busNum,
        addr.hex2Str(),
        existingDeviceName.value_or("READ_ERROR")));
  }
  auto cmd = fmt::format(
      "echo {} {} > /sys/bus/i2c/devices/i2c-{}/new_device",
      deviceName,
      addr.hex2Str(),
      busNum);
  auto [exitStatus, standardOut] = platformUtils_->execCommand(cmd);
  XLOG_IF(INFO, !standardOut.empty()) << standardOut;
  if (exitStatus != 0) {
    throw std::runtime_error(fmt::format(
        "Failed to create i2c device for {} ({}) at bus: {}, addr: {} with exit status {}",
        pmUnitScopedName,
        deviceName,
        busNum,
        addr.hex2Str(),
        exitStatus));
  }
  if (!Utils().checkDeviceReadiness(
          [&]() -> bool { return isI2cDevicePresent(busNum, addr); },
          fmt::format(
              "I2cDevice at busNum: {} and addr: {} is not yet initialized. Waiting for at most {}s",
              busNum,
              addr.hex4Str(),
              kI2cDevCreationWaitSecs.count()),
          kI2cDevCreationWaitSecs)) {
    throw std::runtime_error(fmt::format(
        "Failed to initialize i2c device for {} ({}) at bus: {}, addr: {}",
        pmUnitScopedName,
        deviceName,
        busNum,
        addr.hex2Str()));
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

std::string I2cExplorer::getI2cBusCharDevPath(uint16_t busNum) {
  return fmt::format("/dev/i2c-{}", busNum);
}
} // namespace facebook::fboss::platform::platform_manager
