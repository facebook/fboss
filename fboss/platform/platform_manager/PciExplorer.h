// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <re2/re2.h>

#include "fboss/platform/platform_manager/gen-cpp2/platform_manager_config_types.h"

namespace facebook::fboss::platform::platform_manager {

class PciExplorer {
 public:
  const re2::RE2 kPciIdRegex{"0x[0-9a-f]{4}"};

  // Returns the device path of the given pci device at /sys/bus/pci/devices/.
  // If not found, returns std::nullopt.
  std::optional<std::string> getDevicePath(
      const std::string& vendorId,
      const std::string& deviceId,
      const std::string& subSystemVendorId,
      const std::string& subSystemDeviceId);

  // Create the I2C Adapter based on the given i2cAdapterConfig residing
  // at the given PciDevice path. It returns the the kernel assigned i2c bus
  // number(s) for the created adapter(s). Throw std::runtime_error on failure.
  std::vector<uint16_t> createI2cAdapter(
      const std::string& pciDevPath,
      const I2cAdapterConfig& i2cAdapterConfig);

  // Create the SPI Master based on the given spiMasterConfig residing
  // at the given PciDevice path. Throw std::runtime_error on failure.
  void createSpiMaster(
      const std::string& pciDevPath,
      const SpiMasterConfig& spiMasterConfig);

  // Create the LED Controller based on the given ledCtrlConfig residing
  // at the given PciDevice path. Throw std::runtime_error on failure.
  void createLedCtrl(
      const std::string& pciDevPath,
      const LedCtrlConfig& ledCtrlConfig);

  // Create the Transceiver block based on the given xcvrCtrlConfig residing
  // at the given PciDevice path. Throw std::runtime_error on failure.
  void createXcvrCtrl(
      const std::string& pciDevPath,
      const XcvrCtrlConfig& xcvrCtrlConfig);

  // Create the device based on the given fpgaIpBlockConfig residing
  // at the given PciDevice. Throw std::runtime_error on failure.
  void create(
      const std::string& pciDevPath,
      const FpgaIpBlockConfig& fpgaIpBlock);
};

} // namespace facebook::fboss::platform::platform_manager
