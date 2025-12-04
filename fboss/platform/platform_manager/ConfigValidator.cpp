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
#include <thrift/lib/cpp2/op/Get.h>

#include "fboss/platform/platform_manager/I2cAddr.h"
#include "fboss/platform/platform_manager/Utils.h"
#include "fboss/platform/platform_manager/gen-cpp2/platform_manager_validators_constants.h"

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

bool ConfigValidator::isValidLedCtrlBlockConfig(
    const LedCtrlBlockConfig& ledCtrlBlockConfig) {
  if (ledCtrlBlockConfig.pmUnitScopedNamePrefix()->empty()) {
    XLOG(ERR) << "PmUnitScopedNamePrefix must be a non-empty string";
    return false;
  }
  if (ledCtrlBlockConfig.pmUnitScopedNamePrefix()->ends_with('_')) {
    XLOG(ERR) << "PmUnitScopedNamePrefix must not end with an underscore";
    return false;
  }
  if (ledCtrlBlockConfig.deviceName()->empty()) {
    XLOG(ERR) << "deviceName must be a non-empty string";
    return false;
  }
  if (ledCtrlBlockConfig.csrOffsetCalc()->empty()) {
    XLOG(ERR) << "csrOffsetCalc must be a non-empty string";
    return false;
  }
  if (*ledCtrlBlockConfig.numPorts() <= 0) {
    XLOG(ERR) << "numPorts must be a value greater than 0";
    return false;
  }
  if (*ledCtrlBlockConfig.ledPerPort() <= 0) {
    XLOG(ERR) << "ledPerPort must be a value greater than 0";
    return false;
  }
  if (*ledCtrlBlockConfig.ledPerPort() > 4) {
    XLOG(ERR) << "ledPerPort must be a value less than or equal to 4";
    return false;
  }
  if (*ledCtrlBlockConfig.startPort() <= 0) {
    XLOG(ERR) << "startPort must be a value greater than 0";
    return false;
  }
  if (*ledCtrlBlockConfig.numPorts() > numXcvrs_) {
    XLOG(ERR) << fmt::format(
        "numPorts must be less than or equal to {}", numXcvrs_);
    return false;
  }
  if (*ledCtrlBlockConfig.startPort() > numXcvrs_) {
    XLOG(ERR) << fmt::format(
        "startPort must be less than or equal to {}", numXcvrs_);
    return false;
  }
  if (*ledCtrlBlockConfig.startPort() + *ledCtrlBlockConfig.numPorts() - 1 >
      numXcvrs_) {
    XLOG(ERR) << fmt::format(
        "startPort + numPorts - 1 must be must be less than or equal to {}",
        numXcvrs_);
    return false;
  }

  for (int16_t port = *ledCtrlBlockConfig.startPort();
       port < *ledCtrlBlockConfig.startPort() + *ledCtrlBlockConfig.numPorts();
       port++) {
    for (int16_t led = 1; led <= *ledCtrlBlockConfig.ledPerPort(); led++) {
      if (!isValidCsrOffsetCalc(
              *ledCtrlBlockConfig.csrOffsetCalc(),
              port,
              *ledCtrlBlockConfig.startPort(),
              led)) {
        return false;
      }

      if (!ledCtrlBlockConfig.iobufOffsetCalc()->empty()) {
        if (!isValidIobufOffsetCalc(
                *ledCtrlBlockConfig.iobufOffsetCalc(),
                port,
                *ledCtrlBlockConfig.startPort(),
                led)) {
          return false;
        }
      }
    }
  }

  return true;
}

bool ConfigValidator::isValidXcvrCtrlBlockConfig(
    const XcvrCtrlBlockConfig& xcvrCtrlBlockConfig) {
  if (xcvrCtrlBlockConfig.pmUnitScopedNamePrefix()->empty()) {
    XLOG(ERR) << "PmUnitScopedNamePrefix must be a non-empty string";
    return false;
  }
  if (xcvrCtrlBlockConfig.pmUnitScopedNamePrefix()->ends_with('_')) {
    XLOG(ERR) << "PmUnitScopedNamePrefix must not end with an underscore";
    return false;
  }
  if (xcvrCtrlBlockConfig.deviceName()->empty()) {
    XLOG(ERR) << "deviceName must be a non-empty string";
    return false;
  }
  if (xcvrCtrlBlockConfig.csrOffsetCalc()->empty()) {
    XLOG(ERR) << "csrOffsetCalc must be a non-empty string";
    return false;
  }
  if (*xcvrCtrlBlockConfig.numPorts() <= 0) {
    XLOG(ERR) << "numPorts must be a value greater than 0";
    return false;
  }
  if (*xcvrCtrlBlockConfig.startPort() <= 0) {
    XLOG(ERR) << "startPort must be a value greater than 0";
    return false;
  }
  if (*xcvrCtrlBlockConfig.numPorts() > numXcvrs_) {
    XLOG(ERR) << fmt::format(
        "numPorts must be less than or equal to {}", numXcvrs_);
    return false;
  }
  if (*xcvrCtrlBlockConfig.startPort() > numXcvrs_) {
    XLOG(ERR) << fmt::format(
        "startPort must be less than or equal to {}", numXcvrs_);
    return false;
  }
  if (*xcvrCtrlBlockConfig.startPort() + *xcvrCtrlBlockConfig.numPorts() - 1 >
      numXcvrs_) {
    XLOG(ERR) << fmt::format(
        "startPort + numPorts - 1 must be must be less than or equal to {}",
        numXcvrs_);
    return false;
  }

  for (int16_t port = *xcvrCtrlBlockConfig.startPort(); port <
       *xcvrCtrlBlockConfig.startPort() + *xcvrCtrlBlockConfig.numPorts();
       port++) {
    if (!isValidCsrOffsetCalc(
            *xcvrCtrlBlockConfig.csrOffsetCalc(),
            port,
            *xcvrCtrlBlockConfig.startPort())) {
      return false;
    }

    if (!xcvrCtrlBlockConfig.iobufOffsetCalc()->empty()) {
      if (!isValidIobufOffsetCalc(
              *xcvrCtrlBlockConfig.iobufOffsetCalc(),
              port,
              *xcvrCtrlBlockConfig.startPort())) {
        return false;
      }
    }
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
  for (const auto& config : *pciDeviceConfig.sysLedCtrlConfigs()) {
    fpgaIpBlockConfigs.push_back(config);
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

  for (const auto& config : *pciDeviceConfig.ledCtrlBlockConfigs()) {
    if (!isValidLedCtrlBlockConfig(config)) {
      return false;
    }
  }

  std::vector<std::pair<int16_t, int16_t>> ledPortRanges;
  for (const auto& config : *pciDeviceConfig.ledCtrlBlockConfigs()) {
    ledPortRanges.emplace_back(*config.startPort(), *config.numPorts());
  }
  if (!isValidPortRanges(ledPortRanges)) {
    return false;
  }

  for (const auto& config : *pciDeviceConfig.xcvrCtrlBlockConfigs()) {
    if (!isValidXcvrCtrlBlockConfig(config)) {
      return false;
    }
  }

  std::vector<std::pair<int16_t, int16_t>> xcvrPortRanges;
  for (const auto& config : *pciDeviceConfig.xcvrCtrlBlockConfigs()) {
    xcvrPortRanges.emplace_back(*config.startPort(), *config.numPorts());
  }
  if (!isValidPortRanges(xcvrPortRanges)) {
    return false;
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
    // nextSlotName to verify that PmUnit sits between CurrSlot and
    // NextSlot.
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
              *pciDeviceConfig.sysLedCtrlConfigs(),
              Utils::createLedCtrlConfigs(pciDeviceConfig),
              Utils::createXcvrCtrlConfigs(pciDeviceConfig),
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

  // Store numXcvrs for use by other validation methods
  numXcvrs_ = *config.numXcvrs();

  // Verify presence of platform name
  if (config.platformName()->empty()) {
    XLOG(ERR) << "Platform name cannot be empty";
    return false;
  }

  // Verify platformName is in uppercase
  if (containsLower(*config.platformName())) {
    XLOGF(
        ERR,
        "Platform name must be in uppercase; {} contains lowercase characters",
        *config.platformName());
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

  // Validate chassisEepromDevicePath
  if (!isValidChassisEepromDevicePath(
          config, *config.chassisEepromDevicePath())) {
    return false;
  }

  // Validate SlotTypeConfigs.
  for (const auto& [slotName, slotTypeConfig] : *config.slotTypeConfigs()) {
    XLOG(INFO) << fmt::format(
        "Validating SlotTypeConfig for Slot {}...", slotName);
    if (!isValidSlotTypeConfig(slotTypeConfig)) {
      return false;
    }
    // Validate that pmUnitName in slotTypeConfig exists in pmUnitConfigs
    if (slotTypeConfig.pmUnitName() &&
        !config.pmUnitConfigs()->contains(*slotTypeConfig.pmUnitName())) {
      XLOG(ERR) << fmt::format(
          "PMUnit name '{}' in SlotTypeConfig '{}' does not exist in pmUnitConfigs",
          *slotTypeConfig.pmUnitName(),
          slotName);
      return false;
    }
  }

  for (const auto& [pmUnitName, pmUnitConfig] : *config.pmUnitConfigs()) {
    XLOG(INFO) << fmt::format(
        "Validating PmUnitConfig for PmUnit {} in Slot {}...",
        pmUnitName,
        *pmUnitConfig.pluggedInSlotType());

    // Validate that pmUnitName is in the allowed list
    if (std::find(
            platform_manager_validators_constants::ALLOWED_PMUNIT_NAMES()
                .begin(),
            platform_manager_validators_constants::ALLOWED_PMUNIT_NAMES().end(),
            pmUnitName) ==
        platform_manager_validators_constants::ALLOWED_PMUNIT_NAMES().end()) {
      XLOG(ERR) << fmt::format(
          "PMUnit name '{}' is not in the allowed list of PMUnit names",
          pmUnitName);
      return false;
    }

    if (!isValidPmUnitConfig(*config.slotTypeConfigs(), pmUnitConfig)) {
      return false;
    }
  }

  for (const auto& [pmUnitName, versionedPmUnitConfigs] :
       *config.versionedPmUnitConfigs()) {
    XLOG(INFO) << fmt::format(
        "Validating VersionedPmUnitConfigs for PmUnit {}...", pmUnitName);

    auto defaultConfigIt = config.pmUnitConfigs()->find(pmUnitName);

    // Validate that PMUnit name exists in pmUnitConfigs
    if (defaultConfigIt == config.pmUnitConfigs()->end()) {
      XLOG(ERR) << fmt::format(
          "PMUnit name '{}' in versionedPmUnitConfigs does not exist in pmUnitConfigs",
          pmUnitName);
      return false;
    }

    if (!isValidVersionedPmUnitConfig(
            pmUnitName,
            versionedPmUnitConfigs,
            defaultConfigIt->second,
            *config.slotTypeConfigs())) {
      return false;
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

bool ConfigValidator::isValidCsrOffsetCalc(
    const std::string& csrOffsetCalc,
    const int16_t& portNum,
    const int16_t& startPort,
    std::optional<int16_t> ledNum) {
  // Test the expression with sample values to see if it's computable
  try {
    // Use Utils to test if the expression can be compiled and evaluated
    auto result =
        Utils().computeHexExpression(csrOffsetCalc, portNum, startPort, ledNum);

    // Validate the resulting hex value
    if (result.empty()) {
      XLOG(ERR) << "csrOffsetCalc expression resulted in empty value";
      return false;
    }
    if (!result.starts_with("0x")) {
      XLOG(ERR)
          << "csrOffsetCalc expression result is not in valid hex format: "
          << result;
      return false;
    }
    return true;
  } catch (const std::exception& e) {
    XLOG(ERR) << "csrOffsetCalc expression validation failed: " << e.what();
    return false;
  }
}

bool ConfigValidator::isValidIobufOffsetCalc(
    const std::string& iobufOffsetCalc,
    const int16_t& portNum,
    const int16_t& startPort,
    std::optional<int16_t> ledNum) {
  // Test the expression with sample values to see if it's computable
  try {
    // Use Utils to test if the expression can be compiled and evaluated
    auto result = Utils().computeHexExpression(
        iobufOffsetCalc, portNum, startPort, ledNum);

    // Validate the resulting hex value
    if (result.empty()) {
      XLOG(ERR) << "iobufOffsetCalc expression resulted in empty value";
      return false;
    }
    if (!result.starts_with("0x")) {
      XLOG(ERR)
          << "iobufOffsetCalc expression result is not in valid hex format: "
          << result;
      return false;
    }
    return true;
  } catch (const std::exception& e) {
    XLOG(ERR) << "iobufOffsetCalc expression validation failed: " << e.what();
    return false;
  }
}

bool ConfigValidator::isValidVersionedPmUnitConfig(
    const std::string& pmUnitName,
    const std::vector<VersionedPmUnitConfig>& versionedPmUnitConfigs,
    const PmUnitConfig& defaultPmUnitConfig,
    const std::map<std::string, SlotTypeConfig>& slotTypeConfigs) {
  using apache::thrift::op::get;
  using apache::thrift::op::get_name_v;
  using apache::thrift::op::get_value_or_null;
  namespace ident = apache::thrift::ident;

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

    bool fieldMismatch = false;
    apache::thrift::op::for_each_field_id<PmUnitConfig>([&]<class Id>(Id) {
      // i2cDeviceConfigs are allowed to differ between versioned and default
      if constexpr (std::is_same_v<
                        apache::thrift::op::get_ident<PmUnitConfig, Id>,
                        ident::i2cDeviceConfigs>) {
        return;
      }

      const auto& defaultFieldRef = get<Id>(defaultPmUnitConfig);
      const auto& versionedFieldRef =
          get<Id>(*versionedPmUnitConfig.pmUnitConfig());

      const auto* defaultValue = get_value_or_null(defaultFieldRef);
      const auto* versionedValue = get_value_or_null(versionedFieldRef);

      // Check if fields mismatch:
      if ((defaultValue && versionedValue &&
           *defaultValue != *versionedValue) ||
          (defaultValue && !versionedValue) ||
          (!defaultValue && versionedValue)) {
        XLOG(ERR) << fmt::format(
            "The {} of VersionedPmUnitConfig {}, does not match default config",
            get_name_v<PmUnitConfig, Id>,
            pmUnitName);
        fieldMismatch = true;
      }
    });

    if (fieldMismatch) {
      return false;
    }

    if (!isValidPmUnitConfig(
            slotTypeConfigs, *versionedPmUnitConfig.pmUnitConfig())) {
      return false;
    }
  }
  return true;
}

bool ConfigValidator::isValidPortRanges(
    const std::vector<std::pair<int16_t, int16_t>>& startPortAndNumPorts) {
  if (startPortAndNumPorts.empty()) {
    return true; // Empty list is valid
  }

  // Create a vector of port ranges (startPort, endPort) for sorting
  std::vector<std::pair<int, int>> portRanges;
  for (const auto& [startPort, numPorts] : startPortAndNumPorts) {
    int endPort = startPort + numPorts - 1;
    portRanges.emplace_back(startPort, endPort);
  }

  // Check for sorting, overlaps and gaps
  for (size_t i = 1; i < portRanges.size(); i++) {
    int prevStart = portRanges[i - 1].first;
    int prevEnd = portRanges[i - 1].second;
    int currStart = portRanges[i].first;

    // Check if port ranges are sorted by start port in ascending order
    if (currStart < prevStart) {
      XLOG(ERR) << fmt::format(
          "Port ranges are not sorted by start port: found port {} after port {}",
          currStart,
          prevStart);
      return false;
    }

    // Check for overlap
    if (currStart <= prevEnd) {
      XLOG(ERR) << fmt::format(
          "Overlapping port ranges detected: previous range ends at port {}, current range starts at port {}",
          prevEnd,
          currStart);
      return false;
    }
  }

  return true;
}

bool ConfigValidator::isValidChassisEepromDevicePath(
    const PlatformConfig& platformConfig,
    const std::string& chassisEepromDevicePath) {
  if (platformConfig.platformName() == "DARWIN") {
    // Darwin has a special case where the chassis EEPROM is not a real device
    return true;
  }

  // First check if the device path is valid
  if (!isValidDevicePath(platformConfig, chassisEepromDevicePath)) {
    return false;
  }

  auto [_, deviceName] = Utils().parseDevicePath(chassisEepromDevicePath);
  if (deviceName == "IDPROM") {
    // IDPROM is only allowed for certain platforms
    const auto& exceptionPlatforms = platform_manager_validators_constants::
        PLATFORMS_WITH_IDPROM_CHASSIS_EEPROM();
    if (std::find(
            exceptionPlatforms.begin(),
            exceptionPlatforms.end(),
            *platformConfig.platformName()) == exceptionPlatforms.end()) {
      XLOG(ERR) << fmt::format(
          "Platform {} has chassisEepromDevicePath pointing to IDPROM device '{}'. "
          "New platforms must NOT use IDPROM for chassisEepromDevicePath. "
          "Please use a dedicated chassis EEPROM device instead.",
          *platformConfig.platformName(),
          chassisEepromDevicePath);
      return false;
    }
  } else if (deviceName != "CHASSIS_EEPROM") {
    // Device name must be CHASSIS_EEPROM
    XLOG(ERR) << fmt::format(
        "Platform {} has chassisEepromDevicePath pointing to device '{}'. "
        "Device name must be 'CHASSIS_EEPROM'.",
        *platformConfig.platformName(),
        deviceName);
    return false;
  }

  return true;
}

} // namespace facebook::fboss::platform::platform_manager
