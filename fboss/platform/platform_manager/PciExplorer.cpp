// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <filesystem>

#include <folly/FileUtil.h>
#include <folly/String.h>
#include <folly/logging/xlog.h>

#include "fboss/platform/platform_manager/PciExplorer.h"

namespace fs = std::filesystem;
using namespace facebook::fboss::platform::platform_manager;

namespace {
bool isSamePciId(const std::string& id1, const std::string& id2) {
  return RE2::FullMatch(id1, PciExplorer().kPciIdRegex) &&
      RE2::FullMatch(id2, PciExplorer().kPciIdRegex) && id1 == id2;
}
} // namespace

namespace facebook::fboss::platform::platform_manager {

std::optional<std::string> PciExplorer::getDevicePath(
    const std::string& vendorId,
    const std::string& deviceId,
    const std::string& subSystemVendorId,
    const std::string& subSystemDeviceId) {
  for (const auto& dirEntry : fs::directory_iterator("/sys/bus/pci/devices")) {
    std::string vendor, device, subSystemVendor, subSystemDevice;
    auto deviceFilePath = dirEntry.path() / "device";
    auto vendorFilePath = dirEntry.path() / "vendor";
    auto subSystemVendorFilePath = dirEntry.path() / "subsystem_vendor";
    auto subSystemDeviceFilePath = dirEntry.path() / "subsystem_device";
    if (!folly::readFile(vendorFilePath.c_str(), vendor)) {
      XLOG(ERR) << "Failed to read vendor file from " << dirEntry.path();
    }
    if (!folly::readFile(deviceFilePath.c_str(), device)) {
      XLOG(ERR) << "Failed to read device file from " << dirEntry.path();
    }
    if (!folly::readFile(subSystemVendorFilePath.c_str(), subSystemVendor)) {
      XLOG(ERR) << "Failed to read subsystem_vendor file from "
                << dirEntry.path();
    }
    if (!folly::readFile(subSystemDeviceFilePath.c_str(), subSystemDevice)) {
      XLOG(ERR) << "Failed to read subsystem_device file from "
                << dirEntry.path();
    }
    if (isSamePciId(folly::trimWhitespace(vendor).str(), vendorId) &&
        isSamePciId(folly::trimWhitespace(device).str(), deviceId) &&
        isSamePciId(
            folly::trimWhitespace(subSystemVendor).str(), subSystemVendorId) &&
        isSamePciId(
            folly::trimWhitespace(subSystemDevice).str(), subSystemDeviceId)) {
      return fmt::format("/dev/fbiob_{}", dirEntry.path().filename().string());
    }
  }
  return std::nullopt;
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
