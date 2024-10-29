// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/platform/platform_manager/gen-cpp2/platform_manager_config_types.h"

namespace facebook::fboss::platform::platform_manager {

class ConfigValidator {
 public:
  bool isValid(const PlatformConfig& platformConfig);
  bool isValidPmUnitConfig(
      const std::map<std::string, SlotTypeConfig>& slotTypeConfigs,
      const PmUnitConfig& pmUnitConfig);
  bool isValidSlotTypeConfig(const SlotTypeConfig& slotTypeConfig);
  bool isValidSlotConfig(const SlotConfig& slotConfig);
  bool isValidFpgaIpBlockConfig(const FpgaIpBlockConfig& fpgaIpBlockConfig);
  bool isValidPciDeviceConfig(const PciDeviceConfig& pciDeviceConfig);
  bool isValidI2cDeviceConfig(const I2cDeviceConfig& i2cDeviceConfig);
  bool isValidDevicePath(
      const PlatformConfig& platformConfig,
      const std::string& devicePath);
  bool isValidSlotPath(
      const PlatformConfig& platformConfig,
      const std::string& slotPath);
  bool isValidSymlink(const std::string& symlink);
  bool isValidPresenceDetection(const PresenceDetection& presenceDetection);
  bool isValidSpiDeviceConfigs(
      const std::vector<SpiDeviceConfig>& spiDeviceConfig);
  bool isValidBspKmodsRpmVersion(const std::string& bspKmodsRpmVersion);
  bool isValidVersionedPmConfigs(
      const std::map<std::string, std::vector<VersionedPmUnitConfig>>&
          versionedPmUnitConfigs);
};

} // namespace facebook::fboss::platform::platform_manager
