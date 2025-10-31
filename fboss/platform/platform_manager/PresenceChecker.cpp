// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/platform/platform_manager/PresenceChecker.h"

using namespace facebook::fboss::platform::platform_manager;

PresenceChecker::PresenceChecker(
    const DevicePathResolver& devicePathResolver,
    const std::shared_ptr<Utils> utils,
    const std::shared_ptr<PlatformFsUtils> platformFsUtils)
    : devicePathResolver_(devicePathResolver),
      utils_(utils),
      platformFsUtils_(platformFsUtils) {}

bool PresenceChecker::isPresent(
    const PresenceDetection& presenceDetection,
    const std::string& slotPath) {
  if (const auto sysfsFileHandle = presenceDetection.sysfsFileHandle()) {
    return sysfsPresent(*sysfsFileHandle, slotPath);
  } else if (const auto gpioLineHandle = presenceDetection.gpioLineHandle()) {
    return gpioPresent(*gpioLineHandle, slotPath);
  } else {
    throw std::runtime_error(
        fmt::format("Invalid PresenceDetection for {}", slotPath));
  }
}

int PresenceChecker::getPresenceValue(
    const PresenceDetection& presenceDetection,
    const std::string& slotPath) {
  if (const auto sysfsFileHandle = presenceDetection.sysfsFileHandle()) {
    return sysfsValue(*sysfsFileHandle);
  } else if (const auto gpioLineHandle = presenceDetection.gpioLineHandle()) {
    return gpioValue(*gpioLineHandle);
  } else {
    throw std::runtime_error(
        fmt::format("Invalid PresenceDetection for {}", slotPath));
  }
}

int PresenceChecker::sysfsValue(const SysfsFileHandle& handle) {
  auto presencePath = devicePathResolver_.resolvePresencePath(
      *handle.devicePath(), *handle.presenceFileName());
  if (!presencePath) {
    throw std::runtime_error(
        fmt::format(
            "No sysfs file could be found at DevicePath: {} and presenceFileName: {}",
            *handle.devicePath(),
            *handle.presenceFileName()));
  }
  XLOG(INFO) << fmt::format(
      "The file {} at DevicePath {} resolves to {}",
      *handle.presenceFileName(),
      *handle.devicePath(),
      *presencePath);
  auto presenceFileContent =
      platformFsUtils_->getStringFileContent(*presencePath);
  if (!presenceFileContent) {
    throw std::runtime_error(
        fmt::format("Could not read file {}", *presencePath));
  }
  int16_t presenceValue{0};
  try {
    presenceValue = std::stoi(*presenceFileContent, nullptr, 0);
  } catch (const std::exception& ex) {
    throw std::runtime_error(
        fmt::format(
            "Failed to process file content {}: {}",
            *presenceFileContent,
            folly::exceptionStr(ex)));
  }
  return presenceValue;
}

int PresenceChecker::gpioValue(const GpioLineHandle& handle) {
  auto presencePath =
      devicePathResolver_.resolvePciSubDevCharDevPath(*handle.devicePath());
  XLOG(INFO) << fmt::format(
      "The gpio line {} at DevicePath {} resolves to {}",
      *handle.lineIndex(),
      *handle.devicePath(),
      presencePath);
  return utils_->getGpioLineValue(presencePath, *handle.lineIndex());
}

bool PresenceChecker::sysfsPresent(
    const SysfsFileHandle& handle,
    const std::string& slotPath) {
  auto presencePath = devicePathResolver_.resolvePresencePath(
      *handle.devicePath(), *handle.presenceFileName());
  int presenceValue = sysfsValue(handle);
  bool isPresent = (presenceValue == *handle.desiredValue());
  XLOG(INFO) << fmt::format(
      "Value at {} is {}. desiredValue is {}. "
      "Assuming {} of PmUnit at {}",
      *presencePath,
      presenceValue,
      *handle.desiredValue(),
      isPresent ? "presence" : "absence",
      slotPath);
  return isPresent;
}

bool PresenceChecker::gpioPresent(
    const GpioLineHandle& handle,
    const std::string& slotPath) {
  auto presencePath =
      devicePathResolver_.resolvePciSubDevCharDevPath(*handle.devicePath());
  auto presenceValue = gpioValue(handle);
  bool isPresent = (presenceValue == *handle.desiredValue());
  XLOG(INFO) << fmt::format(
      "Value at {} is {}. desiredValue is {}. "
      "Assuming {} of PmUnit at {}",
      presencePath,
      presenceValue,
      *handle.desiredValue(),
      isPresent ? "presence" : "absence",
      slotPath);
  return isPresent;
}
