// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/platform/platform_manager/gen-cpp2/platform_manager_config_types.h"

namespace facebook::fboss::platform::platform_manager {

struct PciDevice {
  std::string pmUnitScopedName{};
  std::string vendorId{};
  std::string deviceId{};
  std::string subSystemVendorId{};
  std::string subSystemDeviceId{};
};

class PciExplorer {
 public:
  // Create the I2C Adapter based on the given i2cAdapterConfig residing
  // at the given PciDevice. It returns the the kernel assigned i2c bus
  // number(s) for the created adapter(s). Throw std::runtime_error on failure.
  std::vector<uint16_t> createI2cAdapter(
      const PciDevice& device,
      const I2cAdapterConfig& i2cAdapterConfig);

  // Create the SPI Master based on the given spiMasterConfig residing
  // at the given PciDevice. Throw std::runtime_error on failure.
  void createSpiMaster(
      const PciDevice& device,
      const SpiMasterConfig& spiMasterConfig);

  // Create the LED Controller based on the given ledCtrlConfig residing
  // at the given PciDevice. Throw std::runtime_error on failure.
  void createLedCtrl(
      const PciDevice& device,
      const LedCtrlConfig& ledCtrlConfig);

  // Create the Transceiver block based on the given xcvrCtrlConfig residing
  // at the given PciDevice. Throw std::runtime_error on failure.
  void createXcvrCtrl(
      const PciDevice& device,
      const XcvrCtrlConfig& xcvrCtrlConfig);

  // Create the device based on the given fpgaIpBlockConfig residing
  // at the given PciDevice. Throw std::runtime_error on failure.
  void create(const PciDevice& device, const FpgaIpBlockConfig& fpgaIpBlock);
};

} // namespace facebook::fboss::platform::platform_manager
