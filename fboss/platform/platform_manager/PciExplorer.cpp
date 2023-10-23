// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.
#include "fboss/platform/platform_manager/PciExplorer.h"

#include <folly/FileUtil.h>
#include <folly/String.h>
#include <folly/logging/xlog.h>

using namespace facebook::fboss::platform::platform_manager;

namespace facebook::fboss::platform::platform_manager {

std::optional<std::string> PciExplorer::getDevicePath(
    const std::string& vendorId,
    const std::string& deviceId,
    const std::string& subSystemVendorId,
    const std::string& subSystemDeviceId) {
  return fmt::format(
      "/dev/fbiob_{}_{}",
      std::string(vendorId, 2, 4),
      std::string(deviceId, 2, 4));
}

std::vector<uint16_t> PciExplorer::createI2cAdapter(
    const std::string& /* pciDevPath */,
    const I2cAdapterConfig& /* i2cAdapterConfig */) {
  throw std::runtime_error("Not implemented");
}

void PciExplorer::createSpiMaster(
    const std::string& /* pciDevPath */,
    const SpiMasterConfig& /* spiMasterConfig */) {
  throw std::runtime_error("Not implemented");
}

void PciExplorer::createLedCtrl(
    const std::string& /* pciDevPath */,
    const LedCtrlConfig& /* ledCtrlConfig */) {
  throw std::runtime_error("Not implemented");
}

void PciExplorer::createXcvrCtrl(
    const std::string& /* pciDevPath */,
    const XcvrCtrlConfig& /* xcvrCtrlConfig */) {
  throw std::runtime_error("Not implemented");
}

void PciExplorer::create(
    const std::string& /* pciDevPath */,
    const FpgaIpBlockConfig& /* fpgaIpBlock */) {
  throw std::runtime_error("Not implemented");
}

} // namespace facebook::fboss::platform::platform_manager
