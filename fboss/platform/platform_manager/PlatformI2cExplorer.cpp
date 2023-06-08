// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/platform/platform_manager/PlatformI2cExplorer.h"

namespace facebook::fboss::platform::platform_manager {

std::map<std::string, std::string> PlatformI2cExplorer::getBusesfromBsp(
    const std::vector<std::string>&) {
  throw std::runtime_error("Not implemented yet.");
}

std::string PlatformI2cExplorer::getFruTypeName(const std::string&) {
  throw std::runtime_error("Not implemented yet.");
}

bool PlatformI2cExplorer::createI2cDevice(
    const std::string&,
    const std::string&,
    uint8_t) {
  throw std::runtime_error("Not implemented yet.");
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
