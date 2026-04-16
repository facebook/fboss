// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/platform/xcvr_lib/XcvrLib.h"

#include <fmt/format.h>
#include <folly/logging/xlog.h>
#include <thrift/lib/cpp2/protocol/Serializer.h>
#include "fboss/platform/config_lib/ConfigLib.h"

namespace {

facebook::fboss::platform::platform_manager::PlatformConfig getPMConfig(
    const std::string& platformName) {
  std::string platformManagerJson =
      facebook::fboss::platform::ConfigLib().getPlatformManagerConfig(
          platformName);
  facebook::fboss::platform::platform_manager::PlatformConfig pmConfig;
  try {
    apache::thrift::SimpleJSONSerializer::deserialize<
        facebook::fboss::platform::platform_manager::PlatformConfig>(
        platformManagerJson, pmConfig);
  } catch (const std::exception& e) {
    throw std::runtime_error(
        fmt::format(
            "Failed to parse platform_manager.json for {}: {}",
            platformName,
            e.what()));
  }
  return pmConfig;
}

} // namespace

namespace facebook::fboss {

XcvrLib::XcvrLib(const std::string& platformName)
    : XcvrLib(getPMConfig(platformName)) {}

XcvrLib::XcvrLib(const platform::platform_manager::PlatformConfig& pmConfig)
    : pmConfig_(pmConfig) {
  numXcvrs_ = *pmConfig_.numXcvrs();
  xcvrInfos_.resize(numXcvrs_ + 1); // index 0 unused, 1..numXcvrs_
  buildPerTransceiverLedCounts();
  validateXcvrInfos();
}

// --- Platform-level queries ---

int XcvrLib::getNumTransceivers() const {
  return numXcvrs_;
}

bool XcvrLib::isValidXcvrId(int xcvrId) const {
  return xcvrId >= 1 && xcvrId <= numXcvrs_;
}

std::optional<int> XcvrLib::getNumLedsForTransceiver(int xcvrId) const {
  if (!isValidXcvrId(xcvrId)) {
    XLOG(ERR) << fmt::format(
        "Invalid xcvrId {} for getNumLedsForTransceiver (valid: 1-{})",
        xcvrId,
        numXcvrs_);
    return std::nullopt;
  }
  return xcvrInfos_[xcvrId].numLeds;
}

// --- Reset and presence pin info ---

int XcvrLib::getResetMask() const {
  return 1;
}

int XcvrLib::getPresenceMask() const {
  return 1;
}

int XcvrLib::getResetHoldHi() const {
  const auto& platformName = *pmConfig_.platformName();
  if (platformName == "MERU800BFA" || platformName == "MERU800BIA" ||
      platformName == "MORGAN800CC") {
    return 0;
  }
  return 1;
}

// --- Transceiver path queries ---

std::optional<std::string> XcvrLib::getXcvrIODevicePath(int xcvrId) const {
  if (!isValidXcvrId(xcvrId)) {
    XLOG(ERR) << fmt::format(
        "Invalid xcvrId {} for getXcvrIODevicePath (valid: 1-{})",
        xcvrId,
        numXcvrs_);
    return std::nullopt;
  }
  return fmt::format("/run/devmap/xcvrs/xcvr_io_{}", xcvrId);
}

std::optional<std::string> XcvrLib::getXcvrResetSysfsPath(int xcvrId) const {
  if (!isValidXcvrId(xcvrId)) {
    XLOG(ERR) << fmt::format(
        "Invalid xcvrId {} for getXcvrResetSysfsPath (valid: 1-{})",
        xcvrId,
        numXcvrs_);
    return std::nullopt;
  }
  return fmt::format(
      "/run/devmap/xcvrs/xcvr_ctrl_{}/xcvr_reset_{}", xcvrId, xcvrId);
}

std::optional<std::string> XcvrLib::getXcvrPresenceSysfsPath(int xcvrId) const {
  if (!isValidXcvrId(xcvrId)) {
    XLOG(ERR) << fmt::format(
        "Invalid xcvrId {} for getXcvrPresenceSysfsPath (valid: 1-{})",
        xcvrId,
        numXcvrs_);
    return std::nullopt;
  }
  return fmt::format(
      "/run/devmap/xcvrs/xcvr_ctrl_{}/xcvr_present_{}", xcvrId, xcvrId);
}

// --- LED path queries ---

XcvrLib::LedColor XcvrLib::getPrimaryLedColor() const {
  const auto& platformName = *pmConfig_.platformName();
  if (platformName == "DARWIN" || platformName == "DARWIN48V") {
    return LedColor::GREEN;
  }
  return LedColor::BLUE;
}

std::optional<std::string>
XcvrLib::getLedSysfsPath(int xcvrId, int ledNumForXcvr, LedColor color) const {
  if (!isValidXcvrId(xcvrId)) {
    XLOG(ERR) << fmt::format(
        "Invalid xcvrId {} for getLedSysfsPath (valid: 1-{})",
        xcvrId,
        numXcvrs_);
    return std::nullopt;
  }
  auto numLeds = getNumLedsForTransceiver(xcvrId);
  if (!numLeds || ledNumForXcvr < 1 || ledNumForXcvr > *numLeds) {
    XLOG(ERR) << fmt::format(
        "Invalid ledNumForXcvr {} for xcvrId {} (valid: 1-{})",
        ledNumForXcvr,
        xcvrId,
        numLeds.value_or(0));
    return std::nullopt;
  }
  const char* colorStr = (color == LedColor::BLUE) ? "blue"
      : (color == LedColor::GREEN)                 ? "green"
                                                   : "amber";
  return fmt::format(
      "/sys/class/leds/port{}_led{}:{}:status",
      xcvrId,
      ledNumForXcvr,
      colorStr);
}

// --- Lane mapping ---

std::optional<int> XcvrLib::getNumLanesForTransceiver(int xcvrId) const {
  if (!isValidXcvrId(xcvrId)) {
    XLOG(ERR) << fmt::format(
        "Invalid xcvrId {} for getNumLanesForTransceiver (valid: 1-{})",
        xcvrId,
        numXcvrs_);
    return std::nullopt;
  }
  return xcvrInfos_[xcvrId].numLanes;
}

// --- Private helpers ---

void XcvrLib::validateXcvrInfos() {
  const auto& platformName = *pmConfig_.platformName();

  // TODO: Remove once ladakh/lei ledCtrlBlockConfigs cover all xcvrs
  if (platformName == "LADAKH800BCLS" || platformName == "LEH800BCLS") {
    return;
  }

  for (int xcvrId = 1; xcvrId <= numXcvrs_; ++xcvrId) {
    if (!xcvrInfos_[xcvrId].numLeds || !xcvrInfos_[xcvrId].numLanes) {
      throw std::runtime_error(
          fmt::format(
              "Platform {}: xcvr {} missing LED or lane config "
              "(not covered by any ledCtrlBlockConfig)",
              *pmConfig_.platformName(),
              xcvrId));
    }
  }
}

void XcvrLib::buildPerTransceiverLedCounts() {
  // Helper lambda to process PCI device configs from a pmUnitConfig
  auto processPciDevices = [&](const auto& pciDeviceConfigs) {
    for (const auto& pciDevice : pciDeviceConfigs) {
      // Process ledCtrlBlockConfigs (current format)
      for (const auto& block : *pciDevice.ledCtrlBlockConfigs()) {
        int startPort = *block.startPort();
        int numPorts = *block.numPorts();
        int ledPerPort = *block.ledPerPort();
        int lanesPerPort = *block.lanesPerPort();
        for (int port = startPort; port < startPort + numPorts; ++port) {
          if (isValidXcvrId(port)) {
            xcvrInfos_[port].numLeds = ledPerPort;
            xcvrInfos_[port].numLanes = lanesPerPort;
          }
        }
      }
    }
  };

  // Check pmUnitConfigs
  for (const auto& [pmUnitName, pmUnitConfig] : *pmConfig_.pmUnitConfigs()) {
    processPciDevices(*pmUnitConfig.pciDeviceConfigs());
  }
}

} // namespace facebook::fboss
