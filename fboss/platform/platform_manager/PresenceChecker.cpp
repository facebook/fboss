// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/platform/platform_manager/PresenceChecker.h"
#include "fboss/platform/platform_manager/Utils.h"

using namespace facebook::fboss::platform::platform_manager;

PresenceChecker::PresenceChecker(const DevicePathResolver& devicePathResolver)
    : devicePathResolver_(devicePathResolver) {}

bool PresenceChecker::isPresent(
    const PresenceDetection& presenceDetection,
    const std::string slotPath) {
  if (const auto sysfsFileHandle = presenceDetection.sysfsFileHandle()) {
    auto presencePath = devicePathResolver_.resolvePresencePath(
        *sysfsFileHandle->devicePath(), *sysfsFileHandle->presenceFileName());
    if (!presencePath) {
      throw std::runtime_error(fmt::format(
          "No sysfs file could be found at DevicePath: {} and presenceFileName: {}",
          *sysfsFileHandle->devicePath(),
          *sysfsFileHandle->presenceFileName()));
    }
    XLOG(INFO) << fmt::format(
        "The file {} at DevicePath {} resolves to {}",
        *sysfsFileHandle->presenceFileName(),
        *sysfsFileHandle->devicePath(),
        *presencePath);
    auto presenceFileContent = Utils().getStringFileContent(*presencePath);
    if (!presenceFileContent) {
      throw std::runtime_error(
          fmt::format("Could not read file {}", *presencePath));
    }
    int16_t presenceValue{0};
    try {
      presenceValue = std::stoi(*presenceFileContent, nullptr, 0);
    } catch (const std::exception& ex) {
      throw std::runtime_error(fmt::format(
          "Failed to process file content {}: {}",
          *presenceFileContent,
          folly::exceptionStr(ex)));
    }
    bool isPresent = (presenceValue == *sysfsFileHandle->desiredValue());
    XLOG(INFO) << fmt::format(
        "Value at {} is {}. desiredValue is {}. "
        "Assuming {} of PmUnit at {}",
        *presencePath,
        *presenceFileContent,
        *sysfsFileHandle->desiredValue(),
        isPresent ? "presence" : "absence",
        slotPath);
    return isPresent;
  } else {
    throw std::runtime_error(
        fmt::format("Invalid PresenceDetection for {}", slotPath));
  }
}
