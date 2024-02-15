// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/platform/platform_manager/ConfigValidator.h"

#include <folly/logging/xlog.h>
#include <re2/re2.h>

#include "fboss/platform/platform_manager/I2cExplorer.h"
#include "fboss/platform/platform_manager/PciExplorer.h"
#include "fboss/platform/platform_manager/gen-cpp2/platform_manager_config_constants.h"

namespace {
const re2::RE2 kPciDevOffsetRegex{"0x[0-9a-f]+"};
const re2::RE2 kSymlinkRegex{"^/run/devmap/(?P<SymlinkDirs>[a-z0-9-]+)/.+"};
const re2::RE2 kDevPathRegex{"/([A-Z]+_SLOT@[0-9]+/)*\\[.+\\]"};
const re2::RE2 kInfoRomDevicePrefixRegex{"^fpga_info_(dom|iob)$"};
constexpr auto kSymlinkDirs = {
    "eeproms",
    "sensors",
    "cplds",
    "fpgas",
    "i2c-busses",
    "gpiochips",
    "xcvrs",
    "flashes"};
// Supported modalias - spidev +
// https://github.com/torvalds/linux/blob/master/drivers/spi/spidev.c#L702
constexpr auto kSpiDevModaliases = {
    "spidev",
    "dh2228fv",
    "ltc2488",
    "sx1301",
    "bk4",
    "dhcom-board",
    "m53cpld",
    "spi-petra",
    "spi-authenta",
    "em3581",
    "si3210"};
constexpr auto kXcvrDeviceName = "xcvr_ctrl";
} // namespace

namespace facebook::fboss::platform::platform_manager {

bool ConfigValidator::isValidSlotTypeConfig(
    const SlotTypeConfig& slotTypeConfig) {
  if (!slotTypeConfig.idpromConfig_ref() && !slotTypeConfig.pmUnitName()) {
    XLOG(ERR) << "SlotTypeConfig must have either IDPROM or PmUnit name";
    return false;
  }
  if (slotTypeConfig.idpromConfig_ref()) {
    try {
      I2cAddr(*slotTypeConfig.idpromConfig_ref()->address_ref());
    } catch (std::invalid_argument& e) {
      XLOG(ERR) << "IDPROM has invalid address " << e.what();
      return false;
    }
  }
  return true;
}

bool ConfigValidator::isValidSlotConfig(const SlotConfig& slotConfig) {
  if (slotConfig.slotType()->empty()) {
    XLOG(ERR) << "SlotType in SlotConfig must be a non-empty string";
    return false;
  }
  if (slotConfig.presenceDetection()) {
    return isValidPresenceDetection(*slotConfig.presenceDetection());
  }
  return true;
}

bool ConfigValidator::isValidFpgaIpBlockConfig(
    const FpgaIpBlockConfig& fpgaIpBlockConfig) {
  if (fpgaIpBlockConfig.pmUnitScopedName()->empty()) {
    XLOG(ERR) << "PmUnitScopedName must be a non-empty string";
    return false;
  }
  if (!fpgaIpBlockConfig.csrOffset()->empty() &&
      !re2::RE2::FullMatch(
          *fpgaIpBlockConfig.csrOffset(), kPciDevOffsetRegex)) {
    XLOG(ERR) << "Invalid CSR Offset : " << *fpgaIpBlockConfig.csrOffset();
    return false;
  }
  if (!fpgaIpBlockConfig.iobufOffset()->empty() &&
      !re2::RE2::FullMatch(
          *fpgaIpBlockConfig.iobufOffset(), kPciDevOffsetRegex)) {
    XLOG(ERR) << "Invalid IOBuf Offset : " << *fpgaIpBlockConfig.iobufOffset();
    return false;
  }
  return true;
}

bool ConfigValidator::isValidPciDeviceConfig(
    const PciDeviceConfig& pciDeviceConfig) {
  if (pciDeviceConfig.pmUnitScopedName()->empty()) {
    XLOG(ERR) << "PmUnitScopedName must be a non-empty string";
    return false;
  }
  if (!re2::RE2::FullMatch(
          *pciDeviceConfig.vendorId(), PciExplorer().kPciIdRegex)) {
    XLOG(ERR) << "Invalid PCI vendor id : " << *pciDeviceConfig.vendorId();
    return false;
  }
  if (!re2::RE2::FullMatch(
          *pciDeviceConfig.deviceId(), PciExplorer().kPciIdRegex)) {
    XLOG(ERR) << "Invalid PCI device id : " << *pciDeviceConfig.deviceId();
    return false;
  }
  if (!pciDeviceConfig.subSystemDeviceId()->empty() &&
      !re2::RE2::FullMatch(
          *pciDeviceConfig.subSystemVendorId(), PciExplorer().kPciIdRegex)) {
    XLOG(ERR) << "Invalid PCI subsystem vendor id : "
              << *pciDeviceConfig.subSystemVendorId();
    return false;
  }
  if (!pciDeviceConfig.subSystemVendorId()->empty() &&
      !re2::RE2::FullMatch(
          *pciDeviceConfig.subSystemDeviceId(), PciExplorer().kPciIdRegex)) {
    XLOG(ERR) << "Invalid PCI subsystem device id : "
              << *pciDeviceConfig.subSystemDeviceId();
    return false;
  }

  std::vector<FpgaIpBlockConfig> fpgaIpBlockConfigs{};
  for (const auto& config : *pciDeviceConfig.i2cAdapterConfigs()) {
    fpgaIpBlockConfigs.push_back(*config.fpgaIpBlockConfig());
  }
  for (const auto& config : *pciDeviceConfig.spiMasterConfigs()) {
    if (!isValidSpiDeviceConfigs(*config.spiDeviceConfigs())) {
      return false;
    }
    fpgaIpBlockConfigs.push_back(*config.fpgaIpBlockConfig());
  }
  for (const auto& config : *pciDeviceConfig.gpioChipConfigs()) {
    fpgaIpBlockConfigs.push_back(config);
  }
  for (const auto& config : *pciDeviceConfig.watchdogConfigs()) {
    fpgaIpBlockConfigs.push_back(config);
  }
  for (const auto& config : *pciDeviceConfig.fanTachoPwmConfigs()) {
    fpgaIpBlockConfigs.push_back(config);
  }
  for (const auto& config : *pciDeviceConfig.ledCtrlConfigs()) {
    fpgaIpBlockConfigs.push_back(*config.fpgaIpBlockConfig());
  }
  for (const auto& config : *pciDeviceConfig.xcvrCtrlConfigs()) {
    if (*config.fpgaIpBlockConfig()->deviceName() != kXcvrDeviceName) {
      XLOG(ERR) << fmt::format(
          "Invalid DeviceName : {} in XcvrCtrlConfig. It must be {}",
          *config.fpgaIpBlockConfig()->deviceName(),
          kXcvrDeviceName);
      return false;
    }
    fpgaIpBlockConfigs.push_back(*config.fpgaIpBlockConfig());
  }
  for (const auto& config : *pciDeviceConfig.infoRomConfigs()) {
    if (!re2::RE2::FullMatch(*config.deviceName(), kInfoRomDevicePrefixRegex)) {
      XLOG(ERR) << fmt::format(
          "Invalid DeviceName : {} in InfoRomConfig. It must follow naming style : fpga_info_[dom|iob]",
          *config.deviceName());
      return false;
    }
    fpgaIpBlockConfigs.push_back(config);
  }
  for (const auto& config : *pciDeviceConfig.miscCtrlConfigs()) {
    fpgaIpBlockConfigs.push_back(config);
  }

  std::set<std::string> uniqueNames{};
  for (const auto& config : fpgaIpBlockConfigs) {
    auto [it, inserted] = uniqueNames.insert(*config.pmUnitScopedName());
    if (!inserted) {
      XLOG(ERR) << "Duplicate pmUnitScopedName: " << *config.pmUnitScopedName();
      return false;
    }
    if (!isValidFpgaIpBlockConfig(config)) {
      return false;
    }
  }

  return true;
}

bool ConfigValidator::isValidI2cDeviceConfig(
    const I2cDeviceConfig& i2cDeviceConfig) {
  try {
    I2cAddr(*i2cDeviceConfig.address());
  } catch (std::invalid_argument& e) {
    XLOG(ERR) << "IDPROM has invalid address " << e.what();
    return false;
  }
  if (i2cDeviceConfig.pmUnitScopedName()->empty()) {
    XLOG(ERR) << "PmUnitScopedName must be a non-empty string";
    return false;
  }
  return true;
}

bool ConfigValidator::isValidSymlink(const std::string& symlink) {
  std::string dir;
  if (!re2::RE2::FullMatch(symlink, kSymlinkRegex, &dir)) {
    XLOG(ERR) << fmt::format("\"{}\" is invalid symlink", symlink);
    return false;
  }
  if (std::find(kSymlinkDirs.begin(), kSymlinkDirs.end(), dir) ==
      kSymlinkDirs.end()) {
    XLOG(ERR) << fmt::format(
        "{} in {} is not predefined symlink directory", dir, symlink);
    return false;
  }
  return true;
}

bool ConfigValidator::isValidDevicePath(const std::string& devicePath) {
  if (!re2::RE2::FullMatch(devicePath, kDevPathRegex)) {
    XLOG(ERR) << fmt::format("Invalid device path {}", devicePath);
    return false;
  }
  return true;
}

bool ConfigValidator::isValid(const PlatformConfig& config) {
  XLOG(INFO) << "Validating the config";

  // Verify presence of platform name
  if (config.platformName()->empty()) {
    XLOG(ERR) << "Platform name cannot be empty";
    return false;
  }

  // Validate SlotTypeConfigs.
  for (const auto& [slotName, slotTypeConfig] : *config.slotTypeConfigs()) {
    if (!isValidSlotTypeConfig(slotTypeConfig)) {
      return false;
    }
  }

  for (const auto& [name, pmUnitConfig] : *config.pmUnitConfigs()) {
    // Validate PciDeviceConfigs
    for (const auto& pciDeviceConfig : *pmUnitConfig.pciDeviceConfigs_ref()) {
      if (!isValidPciDeviceConfig(pciDeviceConfig)) {
        return false;
      }
    }

    // Validate I2cDeviceConfigs
    for (const auto& i2cDeviceConfig : *pmUnitConfig.i2cDeviceConfigs_ref()) {
      if (!isValidI2cDeviceConfig(i2cDeviceConfig)) {
        return false;
      }
    }

    // Validate SlotConfigs
    for (const auto& [_, slotConfig] : *pmUnitConfig.outgoingSlotConfigs()) {
      if (!isValidSlotConfig(slotConfig)) {
        return false;
      }
    }
  }

  for (const auto& [symlink, devicePath] : *config.symbolicLinkToDevicePath()) {
    if (!isValidSymlink(symlink) || !isValidDevicePath(devicePath)) {
      return false;
    }
  }

  return true;
}

bool ConfigValidator::isValidPresenceDetection(
    const PresenceDetection& presenceDetection) {
  if (presenceDetection.gpioLineHandle() &&
      presenceDetection.sysfsFileHandle()) {
    XLOG(ERR)
        << "Only one of GpioLineHandle or SysfsFileHandle must be set for PresenceDetection";
    return false;
  }
  if (!presenceDetection.gpioLineHandle() &&
      !presenceDetection.sysfsFileHandle()) {
    XLOG(ERR)
        << "GpioLineHandle or SysfsFileHandle must be set for PresenceDetection";
    return false;
  }
  if (presenceDetection.gpioLineHandle()) {
    if (presenceDetection.gpioLineHandle()->devicePath()->empty()) {
      XLOG(ERR) << "devicePath for GpioLineHandle cannot be empty";
      return false;
    }
    if (presenceDetection.gpioLineHandle()->desiredValue()->empty()) {
      XLOG(ERR) << "desiredValue for GpioLineHandle cannot be empty";
      return false;
    }
  }
  if (presenceDetection.sysfsFileHandle()) {
    if (presenceDetection.sysfsFileHandle()->devicePath()->empty()) {
      XLOG(ERR) << "devicePath for SysfsFileHandle cannot be empty";
      return false;
    }
    if (*presenceDetection.sysfsFileHandle()->desiredValue() == 0) {
      XLOG(ERR) << "desiredValue for SysfsFileHandle cannot be 0. Typically 1";
      return false;
    }
    if (presenceDetection.sysfsFileHandle()->presenceFileName()->empty()) {
      XLOG(ERR) << "presenceFileName for SysfsFileHandle cannot be empty";
      return false;
    }
  }
  return true;
}

bool ConfigValidator::isValidSpiDeviceConfigs(
    const std::vector<SpiDeviceConfig>& spiDeviceConfigs) {
  if (spiDeviceConfigs.empty()) {
    XLOG(ERR) << "Invalid empty SpiDeviceConfigs";
    return false;
  }
  std::vector<bool> seenChipSelects(spiDeviceConfigs.size(), false);
  for (const auto& spiDeviceConfig : spiDeviceConfigs) {
    if (spiDeviceConfig.pmUnitScopedName()->empty()) {
      XLOG(ERR) << fmt::format(
          "PmUnitScopedName must be non-empty in SpiDeviceConfig");
      return false;
    }
    if (spiDeviceConfig.modalias()->length() >= NAME_MAX) {
      XLOG(ERR) << fmt::format(
          "Modalias exceeded the {} characters limit for SpiDevice {}",
          NAME_MAX,
          *spiDeviceConfig.pmUnitScopedName());
      return false;
    }
    if (std::find(
            kSpiDevModaliases.begin(),
            kSpiDevModaliases.end(),
            *spiDeviceConfig.modalias()) == kSpiDevModaliases.end()) {
      XLOG(ERR) << fmt::format(
          "Unsupported modalias {} for SpiDevice {}",
          *spiDeviceConfig.modalias(),
          *spiDeviceConfig.pmUnitScopedName());
      return false;
    }
    if (*spiDeviceConfig.chipSelect() < 0 ||
        *spiDeviceConfig.chipSelect() >= spiDeviceConfigs.size()) {
      XLOG(ERR) << fmt::format(
          "Out of range chipselect value {} for SpiDevice {}",
          *spiDeviceConfig.chipSelect(),
          *spiDeviceConfig.pmUnitScopedName());
      return false;
    }
    CHECK_EQ(seenChipSelects.size(), spiDeviceConfigs.size());
    if (seenChipSelects[*spiDeviceConfig.chipSelect()]) {
      XLOG(ERR) << fmt::format(
          "Duplicate chipselect value {} for SpiDevice {}",
          *spiDeviceConfig.chipSelect(),
          *spiDeviceConfig.pmUnitScopedName());
      return false;
    }
    seenChipSelects[*spiDeviceConfig.chipSelect()] = true;
    if (*spiDeviceConfig.maxSpeedHz() <= 0) {
      XLOG(ERR) << fmt::format(
          "Invalid maxSpeedHz {} for SpiDevice {}",
          *spiDeviceConfig.maxSpeedHz(),
          *spiDeviceConfig.pmUnitScopedName());
      return false;
    }
  }
  return true;
}
} // namespace facebook::fboss::platform::platform_manager
