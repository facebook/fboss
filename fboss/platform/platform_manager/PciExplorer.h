// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <re2/re2.h>

#include "fboss/platform/platform_manager/gen-cpp2/platform_manager_config_types.h"

namespace facebook::fboss::platform::platform_manager {

struct PciDevice {
 public:
  PciDevice(
      const std::string& name,
      const std::string& vendorId,
      const std::string& deviceId,
      const std::string& subSystemVendorId,
      const std::string& subSystemDeviceId);
  std::string sysfsPath() const;
  std::string charDevPath() const;

 private:
  std::string vendorId_{};
  std::string deviceId_{};
  std::string subSystemVendorId_{};
  std::string subSystemDeviceId_{};
  std::string charDevPath_{};
  std::string sysfsPath_{};
};

class PciExplorer {
 public:
  const re2::RE2 kPciIdRegex{"0x[0-9a-f]{4}"};

  // Create the I2C Adapter based on the given i2cAdapterConfig residing
  // at the given PciDevice path. It returns the the kernel assigned i2c bus
  // number(s) for the created adapter(s). Throw std::runtime_error on failure.
  std::vector<uint16_t> createI2cAdapter(
      const std::string& pciDevPath,
      const I2cAdapterConfig& i2cAdapterConfig,
      uint32_t instanceId);

  // Create the SPI Master based on the given spiMasterConfig residing
  // at the given PciDevice path. Throw std::runtime_error on failure.
  void createSpiMaster(
      const std::string& pciDevPath,
      const SpiMasterConfig& spiMasterConfig,
      uint32_t instanceId);

  // Create the LED Controller based on the given ledCtrlConfig residing
  // at the given PciDevice path. Throw std::runtime_error on failure.
  void createLedCtrl(
      const std::string& pciDevPath,
      const LedCtrlConfig& ledCtrlConfig,
      uint32_t instanceId);

  // Create the Transceiver block based on the given xcvrCtrlConfig residing
  // at the given PciDevice path. Throw std::runtime_error on failure.
  void createXcvrCtrl(
      const std::string& pciDevPath,
      const XcvrCtrlConfig& xcvrCtrlConfig,
      uint32_t instanceId);

  // Create the device based on the given fpgaIpBlockConfig residing
  // at the given PciDevice. Throw std::runtime_error on failure.
  void create(
      const std::string& pciDevPath,
      const FpgaIpBlockConfig& fpgaIpBlock,
      uint32_t instanceId);
};

} // namespace facebook::fboss::platform::platform_manager
