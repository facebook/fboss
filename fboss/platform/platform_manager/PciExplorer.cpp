// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/platform/platform_manager/PciExplorer.h"

namespace facebook::fboss::platform::platform_manager {

std::vector<uint16_t> PciExplorer::createI2cAdapter(
    const PciDevice& /* device */,
    const I2cAdapterConfig& /* i2cAdapterConfig */) {
  throw std::runtime_error("Not implemented");
}

void PciExplorer::createSpiMaster(
    const PciDevice& /* device */,
    const SpiMasterConfig& /* spiMasterConfig */) {
  throw std::runtime_error("Not implemented");
}

void PciExplorer::createLedCtrl(
    const PciDevice& /* device */,
    const LedCtrlConfig& /* ledCtrlConfig */) {
  throw std::runtime_error("Not implemented");
}

void PciExplorer::createXcvrCtrl(
    const PciDevice& /* device */,
    const XcvrCtrlConfig& /* xcvrCtrlConfig */) {
  throw std::runtime_error("Not implemented");
}

void PciExplorer::create(
    const PciDevice& /* device */,
    const FpgaIpBlockConfig& /* fpgaIpBlock */) {
  throw std::runtime_error("Not implemented");
}

} // namespace facebook::fboss::platform::platform_manager
