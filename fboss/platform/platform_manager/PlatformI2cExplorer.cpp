// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/platform/platform_manager/PlatformI2cExplorer.h"

#include <folly/logging/xlog.h>

namespace facebook::fboss::platform::platform_manager {

std::map<std::string, std::string> PlatformI2cExplorer::getBusesfromBsp(
    const std::vector<std::string>&) {
  throw std::runtime_error("Not implemented yet.");
}

std::string PlatformI2cExplorer::getFruTypeName(const std::string&) {
  throw std::runtime_error("Not implemented yet.");
}

void PlatformI2cExplorer::createI2cDevice(
    const std::string& deviceName,
    const std::string& busName,
    uint8_t addr) {
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
  XLOG(INFO) << fmt::format("Created i2c device {} at {}", deviceName, busName);
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

std::string PlatformI2cExplorer::getI2cPath(const std::string&, uint8_t) {
  throw std::runtime_error("Not implemented yet.");
}

} // namespace facebook::fboss::platform::platform_manager
