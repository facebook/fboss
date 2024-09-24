// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <re2/re2.h>

#include "fboss/platform/platform_manager/gen-cpp2/platform_manager_config_types.h"
#include "fboss/platform/platform_manager/uapi/fbiob-ioctl.h"

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
  // Create the I2C Adapter based on the given i2cAdapterConfig residing
  // at the given PciDevice path. It returns the the kernel assigned i2c bus
  // number(s) for the created adapter(s). Throw std::runtime_error on failure.
  std::vector<uint16_t> createI2cAdapter(
      const PciDevice& pciDevice,
      const I2cAdapterConfig& i2cAdapterConfig,
      uint32_t instanceId);

  // Create the SPIMaster and SpiSlave(s) based on the given spiMasterConfig
  // residing at the given PciDevice. Return a map from SpiSlave
  // pmUnitScopedName to its charDev path. Throw std::runtime_error on failure.
  std::map<
      std::string /* spiDeviceConfig's pmUnitScopedName */,
      std::string /* charDevPath */>
  createSpiMaster(
      const PciDevice& pciDevice,
      const SpiMasterConfig& spiMasterConfig,
      uint32_t instanceId);

  // Create GPIO chip based on the given gpio's FpgaIpblockConfig residing
  // at the given PciDevice.
  // Return CharDevPath. Throw std::runtime_error on failure.
  std::string createGpioChip(
      const PciDevice& pciDevice,
      const FpgaIpBlockConfig& fpgaIpBlockConfig,
      uint32_t instanceId);

  // Create the LED Controller based on the given ledCtrlConfig residing
  // at the given PciDevice. Throw std::runtime_error on failure.
  void createLedCtrl(
      const PciDevice& pciDevice,
      const LedCtrlConfig& ledCtrlConfig,
      uint32_t instanceId);

  // Create the Transceiver block based on the given xcvrCtrlConfig residing
  // at the given PciDevice.
  // Return the SysfsPath. Throw std::runtime_error on failure.
  std::string createXcvrCtrl(
      const PciDevice& pciDevPath,
      const XcvrCtrlConfig& xcvrCtrlConfig,
      uint32_t instanceId);

  // Create the InfoRom block based on the given InfoRomConfig residing at the
  // given PciDevice.
  // Return the created InfoRom sysfs path. Throw std::runtime_error on failure.
  std::string createInfoRom(
      const PciDevice& pciDevice,
      const FpgaIpBlockConfig& fpgaIpBlockConfig,
      uint32_t instanceId);

  // Create the Watchdog based on the given FpgaIpBlockConfig residing at the
  // given PciDevice.
  // Return the created Watchdog CharDevPath. Throw std::runtime_error on
  // failure.
  std::string createWatchdog(
      const PciDevice& pciDevice,
      const FpgaIpBlockConfig& fpgaIpBlockConfig,
      uint32_t instanceId);

  // Create the FanPwmCtrl based on the given FanPwmCtrlConfig residing at the
  // given PciDevice.
  // Return the created FanPwmCtrl SyfsPath. Throw std::runtime_error on
  // failure.
  std::string createFanPwmCtrl(
      const PciDevice& pciDevice,
      const FanPwmCtrlConfig& fanPwmCtrlConfig,
      uint32_t instanceId);

  // Create the generic device block based on the given FpgaIpBlockConfig
  // residing at the given PciDevice. Throw std::runtime_error on failure.
  void createFpgaIpBlock(
      const PciDevice& pciDevice,
      const FpgaIpBlockConfig& fpgaIpBlockConfig,
      uint32_t instanceId);

  // Create the device based on the given fbiob_aux_data residing
  // at the given PciDevice. Throw std::runtime_error on failure.
  void create(
      const PciDevice& pciDevice,
      const FpgaIpBlockConfig& fpgaIpBlockConfig,
      const struct fbiob_aux_data& auxData);

  std::string getGpioChipCharDevPath(
      const PciDevice& pciDevice,
      const FpgaIpBlockConfig& fpgaIpBlockConfig,
      uint32_t instanceId);

 private:
  std::vector<uint16_t> getI2cAdapterBusNums(
      const PciDevice& pciDevice,
      const I2cAdapterConfig& i2cAdapterConfig,
      uint32_t instanceId);
  std::map<
      std::string /* spiDeviceConfig's pmUnitScopedName */,
      std::string /* charDevPath */>
  getSpiDeviceCharDevPaths(
      const PciDevice& pciDevice,
      const SpiMasterConfig& spiMasterConfig,
      uint32_t instanceId);
  std::string getInfoRomSysfsPath(
      const FpgaIpBlockConfig& infoRomConfig,
      uint32_t instanceId);
  std::string getWatchDogCharDevPath(
      const PciDevice& pciDevice,
      const FpgaIpBlockConfig& fpgaIpBlockConfig,
      uint32_t instanceId);
  std::string getFanPwmCtrlSysfsPath(
      const PciDevice& pciDevice,
      const FpgaIpBlockConfig& fpgaIpBlockConfig,
      uint32_t instanceId);
  std::string getXcvrCtrlSysfsPath(
      const PciDevice& pciDevice,
      const FpgaIpBlockConfig& fpgaIpBlockConfig,
      uint32_t instanceId);
  bool isPciSubDeviceReady(
      const PciDevice& pciDevice,
      const FpgaIpBlockConfig& fpgaIpBlockConfig,
      uint32_t instanceId);
  bool isPciSubDeviceDriverReady(const std::string& devPath);
  std::optional<std::string> getPciSubDeviceIOBlockPath(
      const PciDevice& pciDevice,
      const FpgaIpBlockConfig& fpgaIpBlockConfig,
      uint32_t instanceId);
};
} // namespace facebook::fboss::platform::platform_manager
