// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/platform/platform_manager/ConfigValidator.h"

#include <folly/logging/xlog.h>
#include <range/v3/range/conversion.hpp>
#include <range/v3/view/drop.hpp>
#include <range/v3/view/filter.hpp>
#include <range/v3/view/map.hpp>
#include <range/v3/view/split.hpp>
#include <range/v3/view/transform.hpp>
#include <range/v3/view/unique.hpp>
#include <re2/re2.h>

#include "fboss/platform/platform_manager/I2cAddr.h"

namespace facebook::fboss::platform::platform_manager {
namespace {
const re2::RE2 kRpmVersionRegex{"^[0-9]+\\.[0-9]+\\.[0-9]+\\-[0-9]+$"};
const re2::RE2 kPciIdRegex{"0x[0-9a-f]{4}"};
const re2::RE2 kPciDevOffsetRegex{"0x[0-9a-f]+"};
const re2::RE2 kSymlinkRegex{"^/run/devmap/(?P<SymlinkDirs>[a-z0-9-]+)/.+"};
const re2::RE2 kDevPathRegex{"(?P<SlotPath>.*)\\[(?P<DeviceName>.+)\\]"};
const re2::RE2 kSlotNameRegex{"(?P<SlotType>.([A-Z]+_)+SLOT)@\\d+"};
const re2::RE2 kSlotPathRegex{"/|(/([A-Z]+_)+SLOT@\\d+)+"};
const re2::RE2 kInfoRomDevicePrefixRegex{"^fpga_info_(dom|iob|scm|mcb)$"};
const re2::RE2 kI2cAdapterNameRegex{"(?P<PmUnitScopedName>.+)@(?P<Num>\\d+)"};
const re2::RE2 kRpmNameRegex{"(?P<KEYWORD>[a-z]+)_bsp_kmods"};
constexpr auto kSymlinkDirs = {
    "eeproms",
    "sensors",
    "cplds",
    "fpgas",
    "inforoms",
    "i2c-busses",
    "gpiochips",
    "xcvrs",
    "flashes",
    "watchdogs",
    "mdio-busses"};
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

bool containsLower(const std::string& s) {
  return std::any_of(s.begin(), s.end(), ::islower);
}

// Tokenize the SlotPath by delimiter '/'
std::vector<std::string> split(const std::string& slotPath) {
  return slotPath | ranges::views::split('/') | ranges::views::drop(1) |
      ranges::to<std::vector<std::string>>;
}

// Returns all PmUnitConfigs that has the given slotType.
std::vector<PmUnitConfig> getPmUnitConfigsBySlotType(
    const PlatformConfig& platformConfig,
    const SlotType& slotType) {
  return *platformConfig.pmUnitConfigs() | ranges::views::values |
      ranges::views::filter([&](const auto& pmUnitConfig) {
        return *pmUnitConfig.pluggedInSlotType() == slotType;
      }) |
      ranges::to_vector;
}

std::optional<SlotType> extractSlotType(const std::string& slotName) {
  SlotType slotType;
  if (!re2::RE2::FullMatch(slotName, kSlotNameRegex, &slotType)) {
    return std::nullopt;
  }
  return slotType;
}

std::optional<SlotType> resolveSlotType(
    const PlatformConfig& platformConfig,
    const std::string& slotPath) {
  std::optional<SlotType> slotType;
  if (slotPath == "/") {
    return *platformConfig.rootSlotType();
  }
  const auto lastSlotName = std::move(split(slotPath).back());
  // Find the SlotType of the lastSlotName.
  return extractSlotType(lastSlotName);
}

template <typename T>
  requires requires(T t) {
    { *t.pmUnitScopedName() } -> std::convertible_to<std::string>;
  }
bool hasDeviceName(const T& deviceConfig, const std::string& deviceName) {
  return *deviceConfig.pmUnitScopedName() == deviceName;
}

template <typename T>
  requires requires(T t) {
    { *t.fpgaIpBlockConfig() } -> std::convertible_to<FpgaIpBlockConfig>;
  }
bool hasDeviceName(const T& deviceConfig, const std::string& deviceName) {
  return hasDeviceName(*deviceConfig.fpgaIpBlockConfig(), deviceName);
}

bool hasDeviceName(
    const I2cAdapterConfig& deviceConfig,
    const std::string& deviceName) {
  std::string pmUnitScopedName, adapterNum;
  if (!re2::RE2::FullMatch(
          deviceName, kI2cAdapterNameRegex, &pmUnitScopedName, &adapterNum)) {
    return hasDeviceName(*deviceConfig.fpgaIpBlockConfig(), deviceName);
  }
  return hasDeviceName(*deviceConfig.fpgaIpBlockConfig(), pmUnitScopedName) &&
      0 <= stoi(adapterNum) &&
      stoi(adapterNum) < *deviceConfig.numberOfAdapters();
}

template <typename T>
bool hasDeviceName(
    const std::vector<T>& deviceConfigs,
    const std::string& deviceName) {
  for (const auto& deviceConfig : deviceConfigs) {
    if (hasDeviceName(deviceConfig, deviceName)) {
      return true;
    }
  }
  return false;
}

bool hasDeviceName(
    const std::vector<SpiMasterConfig>& deviceConfigs,
    const std::string& deviceName) {
  for (const auto& deviceConfig : deviceConfigs) {
    if (hasDeviceName(*deviceConfig.fpgaIpBlockConfig(), deviceName) ||
        hasDeviceName(*deviceConfig.spiDeviceConfigs(), deviceName)) {
      return true;
    }
  }
  return false;
}

template <typename... Ts>
bool isDeviceMatch(const std::string& deviceName, Ts&&... deviceConfigs) {
  return (hasDeviceName(deviceConfigs, deviceName) || ...);
}
} // namespace

bool ConfigValidator::isValidSlotTypeConfig(
    const SlotTypeConfig& slotTypeConfig) {
  if (!slotTypeConfig.idpromConfig() && !slotTypeConfig.pmUnitName()) {
    XLOG(ERR) << "SlotTypeConfig must have either IDPROM or PmUnit name";
    return false;
  }
  if (slotTypeConfig.idpromConfig()) {
    try {
      I2cAddr(*slotTypeConfig.idpromConfig()->address());
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
  if (containsLower(*fpgaIpBlockConfig.pmUnitScopedName())) {
    XLOGF(
        ERR,
        "PmUnitScopedName must be in uppercase; {} contains lowercase characters",
        *fpgaIpBlockConfig.pmUnitScopedName());
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
  if (containsLower(*pciDeviceConfig.pmUnitScopedName())) {
    XLOGF(
        ERR,
        "PmUnitScopedName must be in uppercase; {} contains lowercase characters",
        *pciDeviceConfig.pmUnitScopedName());
    return false;
  }
  if (!re2::RE2::FullMatch(*pciDeviceConfig.vendorId(), kPciIdRegex)) {
    XLOG(ERR) << "Invalid PCI vendor id : " << *pciDeviceConfig.vendorId();
    return false;
  }
  if (!re2::RE2::FullMatch(*pciDeviceConfig.deviceId(), kPciIdRegex)) {
    XLOG(ERR) << "Invalid PCI device id : " << *pciDeviceConfig.deviceId();
    return false;
  }
  if (!pciDeviceConfig.subSystemDeviceId()->empty() &&
      !re2::RE2::FullMatch(*pciDeviceConfig.subSystemVendorId(), kPciIdRegex)) {
    XLOG(ERR) << "Invalid PCI subsystem vendor id : "
              << *pciDeviceConfig.subSystemVendorId();
    return false;
  }
  if (!pciDeviceConfig.subSystemVendorId()->empty() &&
      !re2::RE2::FullMatch(*pciDeviceConfig.subSystemDeviceId(), kPciIdRegex)) {
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
    fpgaIpBlockConfigs.push_back(*config.fpgaIpBlockConfig());
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
  for (const auto& config : *pciDeviceConfig.mdioBusConfigs()) {
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
  if (containsLower(*i2cDeviceConfig.pmUnitScopedName())) {
    XLOGF(
        ERR,
        "PmUnitScopedName must be in uppercase; {} contains lowercase characters",
        *i2cDeviceConfig.pmUnitScopedName());
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

bool ConfigValidator::isValidDevicePath(
    const PlatformConfig& platformConfig,
    const std::string& devicePath) {
  std::string slotPath, deviceName;
  if (!re2::RE2::FullMatch(devicePath, kDevPathRegex, &slotPath, &deviceName)) {
    XLOG(ERR) << fmt::format("Invalid device path {}", devicePath);
    return false;
  }
  CHECK_EQ(slotPath.back(), '/');
  if (slotPath.length() > 1) {
    slotPath.pop_back();
  }
  if (!isValidSlotPath(platformConfig, slotPath) ||
      !isValidDeviceName(platformConfig, slotPath, deviceName)) {
    return false;
  }
  return true;
}

bool ConfigValidator::isValidSlotPath(
    const PlatformConfig& platformConfig,
    const std::string& slotPath) {
  // Syntactic validation.
  if (!re2::RE2::FullMatch(slotPath, kSlotPathRegex)) {
    XLOG(ERR) << fmt::format("Invalid SlotPath format {}", slotPath);
    return false;
  }

  // Slot topological validation.
  // Starting from the root, check for a PmUnit from CurrSlot to NextSlot.
  auto slotNames = split(slotPath);
  auto currSlotType = *platformConfig.rootSlotType();
  for (const auto& nextSlotName : slotNames) {
    // Find all pmUnits that can be plug into currSlotType.
    auto pmUnitConfigs =
        getPmUnitConfigsBySlotType(platformConfig, currSlotType);
    if (pmUnitConfigs.empty()) {
      XLOG(ERR) << fmt::format(
          "Couldn't find PmUnitConfigs that can be plug-into {} in SlotPath {}",
          currSlotType,
          slotPath);
      return false;
    }
    // Find next SlotType from the found PmUnits' outgoingSlotConfig of
    // nextSlotName to verify that PmUnit sits between CurrSlot and NextSlot.
    auto nextSlotType =
        pmUnitConfigs | ranges::views::filter([&](const auto& pmUnitConfig) {
          return pmUnitConfig.outgoingSlotConfigs()->contains(nextSlotName);
        }) |
        ranges::views::transform([&](const auto& pmUnitConfig) -> SlotType {
          return *pmUnitConfig.outgoingSlotConfigs()
                      ->at(nextSlotName)
                      .slotType();
        }) |
        ranges::views::unique | ranges::to_vector;
    if (nextSlotType.size() != 1) {
      XLOG(ERR) << fmt::format(
          "Invalid SlotName {}. It maps to {} SlotConfig(s)",
          nextSlotName,
          nextSlotType.size());
      return false;
    }
    currSlotType = nextSlotType.front();
  }
  return true;
}

bool ConfigValidator::isValidDeviceName(
    const PlatformConfig& platformConfig,
    const std::string& slotPath,
    const std::string& deviceName) {
  auto slotType = resolveSlotType(platformConfig, slotPath);
  CHECK(slotType) << "SlotType must be nonnull";
  // If the device is IDPROM, search from the SlotTypeConfigs.
  if (deviceName == "IDPROM") {
    if (!platformConfig.slotTypeConfigs()->contains(*slotType)) {
      XLOG(ERR) << fmt::format(
          "Found no SlotTypeConfig for SlotType {}", *slotType);
      return false;
    }
    const auto& slotTypeConfig =
        platformConfig.slotTypeConfigs()->at(*slotType);
    if (!slotTypeConfig.idpromConfig()) {
      XLOG(ERR) << fmt::format(
          "Unexpected IDPROM at SlotPath {}. IdpromConfig is not defined at {}",
          slotPath,
          *slotType);
      return false;
    }
    return true;
  }
  // Otherwise, find all plugable PmUnitConfigs to the last Slot of the
  // SlotPath. eagerly check if any has the given deviceName in its device
  // configurations.
  for (const auto& pmUnitConfig :
       getPmUnitConfigsBySlotType(platformConfig, *slotType)) {
    if (isDeviceMatch(
            deviceName,
            *pmUnitConfig.pciDeviceConfigs(),
            *pmUnitConfig.i2cDeviceConfigs(),
            *pmUnitConfig.embeddedSensorConfigs())) {
      return true;
    }
    for (const auto& pciDeviceConfig : *pmUnitConfig.pciDeviceConfigs()) {
      if (isDeviceMatch(
              deviceName,
              *pciDeviceConfig.i2cAdapterConfigs(),
              *pciDeviceConfig.spiMasterConfigs(),
              *pciDeviceConfig.fanTachoPwmConfigs(),
              *pciDeviceConfig.ledCtrlConfigs(),
              *pciDeviceConfig.xcvrCtrlConfigs(),
              *pciDeviceConfig.spiMasterConfigs(),
              *pciDeviceConfig.gpioChipConfigs(),
              *pciDeviceConfig.watchdogConfigs(),
              *pciDeviceConfig.infoRomConfigs(),
              *pciDeviceConfig.miscCtrlConfigs(),
              *pciDeviceConfig.mdioBusConfigs())) {
        return true;
      }
    }
  }
  // Couldn't find the matching deviceName.
  XLOG(ERR) << fmt::format(
      "Invalid DeviceName {} at SlotPath {}", deviceName, slotPath);
  return false;
}

bool ConfigValidator::isValid(const PlatformConfig& config) {
  XLOG(INFO) << "Validating platform_manager config";

  // Verify presence of platform name
  if (config.platformName()->empty()) {
    XLOG(ERR) << "Platform name cannot be empty";
    return false;
  }

  if (config.rootSlotType()->empty()) {
    XLOG(ERR) << "Platform rootSlotType cannot be empty";
    return false;
  }

  if (config.slotTypeConfigs()->find(*config.rootSlotType()) ==
      config.slotTypeConfigs()->end()) {
    XLOG(ERR) << fmt::format(
        "Invalid rootSlotType {}. Not found in slotTypeConfigs",
        *config.rootSlotType());
    return false;
  }

  // Validate SlotTypeConfigs.
  for (const auto& [slotName, slotTypeConfig] : *config.slotTypeConfigs()) {
    XLOG(INFO) << fmt::format(
        "Validating SlotTypeConfig for Slot {}...", slotName);
    if (!isValidSlotTypeConfig(slotTypeConfig)) {
      return false;
    }
  }

  for (const auto& [pmUnitName, pmUnitConfig] : *config.pmUnitConfigs()) {
    XLOG(INFO) << fmt::format(
        "Validating PmUnitConfig for PmUnit {} in Slot {}...",
        pmUnitName,
        *pmUnitConfig.pluggedInSlotType());
    if (!isValidPmUnitConfig(*config.slotTypeConfigs(), pmUnitConfig)) {
      return false;
    }
  }

  for (const auto& [pmUnitName, versionedPmUnitConfigs] :
       *config.versionedPmUnitConfigs()) {
    XLOG(INFO) << fmt::format(
        "Validating VersionedPmUnitConfigs for PmUnit {}...", pmUnitName);
    if (versionedPmUnitConfigs.empty()) {
      XLOG(ERR) << fmt::format(
          "VersionedPmUnitConfigs for {} must not be empty", pmUnitName);
      return false;
    }
    for (const auto& versionedPmUnitConfig : versionedPmUnitConfigs) {
      if (*versionedPmUnitConfig.productSubVersion() < 0) {
        XLOG(ERR) << fmt::format(
            "One of PmUnit {}'s VersionedPmUnitConfig has a negative ProductSubVersion",
            pmUnitName);
        return false;
      }
      if (!isValidPmUnitConfig(
              *config.slotTypeConfigs(),
              *versionedPmUnitConfig.pmUnitConfig())) {
        return false;
      }
    }
  }

  XLOG(INFO) << "Validating Symbolic links...";
  for (const auto& [symlink, devicePath] : *config.symbolicLinkToDevicePath()) {
    if (!isValidSymlink(symlink) || !isValidDevicePath(config, devicePath)) {
      return false;
    }
  }

  XLOG(INFO) << "Validating Transceiver symbolic links...";
  auto symlinks = *config.symbolicLinkToDevicePath() | ranges::views::keys |
      ranges::to<std::vector<std::string>>();
  if (!isValidXcvrSymlinks(*config.numXcvrs(), symlinks)) {
    return false;
  }

  if (!isValidBspKmodsRpmVersion(*config.bspKmodsRpmVersion())) {
    return false;
  }

  if (!isValidBspKmodsRpmName(*config.bspKmodsRpmName())) {
    return false;
  }

  return true;
}

bool ConfigValidator::isValidPmUnitConfig(
    const std::map<std::string, SlotTypeConfig>& slotTypeConfigs,
    const PmUnitConfig& pmUnitConfig) {
  if (!slotTypeConfigs.contains(*pmUnitConfig.pluggedInSlotType())) {
    XLOG(ERR) << fmt::format(
        "Plugged-into Slot {} which has a missing SlotTypeConfig definition",
        *pmUnitConfig.pluggedInSlotType());
    return false;
  }

  // Validate PciDeviceConfigs
  for (const auto& pciDeviceConfig : *pmUnitConfig.pciDeviceConfigs()) {
    if (!isValidPciDeviceConfig(pciDeviceConfig)) {
      return false;
    }
  }

  // Validate I2cDeviceConfigs
  for (const auto& i2cDeviceConfig : *pmUnitConfig.i2cDeviceConfigs()) {
    if (!isValidI2cDeviceConfig(i2cDeviceConfig)) {
      return false;
    }
  }

  // Validate SlotConfigs
  for (const auto& [slotName, slotConfig] :
       *pmUnitConfig.outgoingSlotConfigs()) {
    auto slotType = extractSlotType(slotName);
    if (!slotType) {
      XLOG(ERR) << fmt::format(
          "Invalid SlotName format {}. Must follow <SlotType>@<Num>", slotName);
      return false;
    }
    if (*slotType != *slotConfig.slotType()) {
      XLOG(ERR) << fmt::format(
          "SlotName must contain the SlotType {} instead contains {}",
          *slotConfig.slotType(),
          *slotType);
      return false;
    }
    if (!isValidSlotConfig(slotConfig)) {
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
    if (presenceDetection.gpioLineHandle()->desiredValue() < 0) {
      XLOG(ERR)
          << "desiredValue for GpioLineHandle cannot be < 0. Typically 0 or 1";
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
          "PmUnitScopedName must be a non-empty string in SpiDeviceConfig");
      return false;
    }
    if (containsLower(*spiDeviceConfig.pmUnitScopedName())) {
      XLOGF(
          ERR,
          "PmUnitScopedName must be in uppercase; {} contains lowercase characters",
          *spiDeviceConfig.pmUnitScopedName());
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

bool ConfigValidator::isValidBspKmodsRpmVersion(
    const std::string& bspKmodsRpmVersion) {
  if (bspKmodsRpmVersion.empty()) {
    XLOG(ERR) << "BspKmodsRpmVersion cannot be empty";
    return false;
  }
  if (!re2::RE2::FullMatch(bspKmodsRpmVersion, kRpmVersionRegex)) {
    XLOG(ERR) << fmt::format(
        "Invalid BspKmodsRpmVersion : {}", bspKmodsRpmVersion);
    return false;
  }
  return true;
}

bool ConfigValidator::isValidPmUnitName(
    const PlatformConfig& platformConfig,
    const std::string& slotPath,
    const std::string& pmUnitName) {
  if (!platformConfig.pmUnitConfigs()->contains(pmUnitName)) {
    XLOG(ERR) << fmt::format("Undefined PmUnitConfig for {}", pmUnitName);
    return false;
  }
  const auto& pmUnitConfig = platformConfig.pmUnitConfigs()->at(pmUnitName);
  const auto slotType = resolveSlotType(platformConfig, slotPath);
  if (!slotType || *slotType != *pmUnitConfig.pluggedInSlotType()) {
    XLOG(ERR) << fmt::format(
        "Unexpected SlotType {} for PmUnit {}. Expected SlotType {} ",
        *slotType,
        pmUnitName,
        *pmUnitConfig.pluggedInSlotType());
    return false;
  }
  return true;
}

bool ConfigValidator::isValidBspKmodsRpmName(
    const std::string& bspKmodsRpmName) {
  if (bspKmodsRpmName.empty()) {
    XLOG(ERR) << "BspKmodsRpmName cannot be empty";
    return false;
  }
  std::string keyword{};
  if (!re2::RE2::FullMatch(bspKmodsRpmName, kRpmNameRegex, &keyword)) {
    XLOG(ERR) << fmt::format("Invalid BspKmodsRpmName : {}", bspKmodsRpmName);
    return false;
  }
  return true;
}

bool ConfigValidator::isValidXcvrSymlinks(
    int16_t numXcvrs,
    const std::vector<std::string>& symlinks) {
  std::set<std::string> xcvrCtrlSymlinks, xcvrIoSymlinks;
  for (const auto& symlink : symlinks) {
    if (symlink.starts_with("/run/devmap/xcvrs/xcvr_ctrl_")) {
      xcvrCtrlSymlinks.insert(symlink);
    }
    if (symlink.starts_with("/run/devmap/xcvrs/xcvr_io_")) {
      xcvrIoSymlinks.insert(symlink);
    }
  }
  if (xcvrCtrlSymlinks.size() != numXcvrs) {
    XLOG(ERR) << fmt::format(
        "Expected {} xcvr control symlinks, but found {}",
        numXcvrs,
        xcvrCtrlSymlinks.size());
    return false;
  }
  if (xcvrIoSymlinks.size() != numXcvrs) {
    XLOG(ERR) << fmt::format(
        "Expected {} xcvr IO symlinks, but found {}",
        numXcvrs,
        xcvrIoSymlinks.size());
    return false;
  }

  for (int16_t xcvrId = 1; xcvrId <= numXcvrs; xcvrId++) {
    auto xcvrCtrlSymlink =
        fmt::format("/run/devmap/xcvrs/xcvr_ctrl_{}", xcvrId);
    if (!xcvrCtrlSymlinks.contains(xcvrCtrlSymlink)) {
      XLOG(ERR) << fmt::format(
          "Missing xcvr control symlink for xcvr {}", xcvrId);
      return false;
    }
    auto xcvrIoSymlink = fmt::format("/run/devmap/xcvrs/xcvr_io_{}", xcvrId);
    if (!xcvrIoSymlinks.contains(xcvrIoSymlink)) {
      XLOG(ERR) << fmt::format("Missing xcvr IO symlink for xcvr {}", xcvrId);
      return false;
    }
  }
  return true;
}
} // namespace facebook::fboss::platform::platform_manager
