// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/lib/bsp/BuildFromXcvrLib.h"

#include <optional>
#include <utility>

#include <folly/logging/xlog.h>
#include "fboss/lib/bsp/gen-cpp2/bsp_platform_mapping_types.h"
#include "fboss/lib/led/gen-cpp2/led_mapping_types.h"
#include "fboss/platform/xcvr_lib/XcvrLib.h"

namespace facebook::fboss {

namespace {

constexpr int kResetGpioOffset = 0;
constexpr int kPresenceGpioOffset = 0;
constexpr int kPresenceHoldHi = 0;
constexpr auto kAccessControlType = ResetAndPresenceAccessType::CPLD;
constexpr auto kIOControlType = TransceiverIOType::I2C;
constexpr int kPimId = 1;

std::optional<BspTransceiverAccessControllerInfo> buildTransceiverAccessControl(
    const XcvrLib& xcvrLib,
    int tcvrId) {
  auto resetSysfsPath = xcvrLib.getXcvrResetSysfsPath(tcvrId);
  auto presenceSysfsPath = xcvrLib.getXcvrPresenceSysfsPath(tcvrId);
  if (!resetSysfsPath.has_value() || !presenceSysfsPath.has_value()) {
    XLOG(WARNING) << "Transceiver " << tcvrId
                  << " missing reset or presence sysfs path, skipping";
    return std::nullopt;
  }

  BspTransceiverAccessControllerInfo accessCtrl;
  accessCtrl.controllerId() = std::to_string(tcvrId);
  accessCtrl.type() = kAccessControlType;

  BspResetPinInfo resetInfo;
  resetInfo.sysfsPath() = std::move(*resetSysfsPath);
  resetInfo.mask() = xcvrLib.getResetMask();
  resetInfo.gpioOffset() = kResetGpioOffset;
  resetInfo.resetHoldHi() = xcvrLib.getResetHoldHi();
  accessCtrl.reset() = std::move(resetInfo);

  BspPresencePinInfo presenceInfo;
  presenceInfo.sysfsPath() = std::move(*presenceSysfsPath);
  presenceInfo.mask() = xcvrLib.getPresenceMask();
  presenceInfo.gpioOffset() = kPresenceGpioOffset;
  presenceInfo.presentHoldHi() = kPresenceHoldHi;
  accessCtrl.presence() = std::move(presenceInfo);

  accessCtrl.gpioChip() = "";

  return accessCtrl;
}

std::optional<BspTransceiverIOControllerInfo> buildTransceiverIOControl(
    const XcvrLib& xcvrLib,
    int tcvrId) {
  auto devicePath = xcvrLib.getXcvrIODevicePath(tcvrId);
  if (!devicePath.has_value()) {
    XLOG(WARNING) << "Transceiver " << tcvrId
                  << " missing IO device path, skipping";
    return std::nullopt;
  }

  BspTransceiverIOControllerInfo ioCtrl;
  ioCtrl.controllerId() = std::to_string(tcvrId);
  ioCtrl.type() = kIOControlType;
  ioCtrl.devicePath() = *devicePath;
  return ioCtrl;
}

std::optional<BspTransceiverMapping>
buildTransceiverMapping(const XcvrLib& xcvrLib, int tcvrId, int firstLedId) {
  auto accessCtrl = buildTransceiverAccessControl(xcvrLib, tcvrId);
  auto ioCtrl = buildTransceiverIOControl(xcvrLib, tcvrId);
  if (!accessCtrl.has_value() || !ioCtrl.has_value()) {
    return std::nullopt;
  }

  BspTransceiverMapping mapping;
  mapping.tcvrId() = tcvrId;
  mapping.accessControl() = std::move(*accessCtrl);
  mapping.io() = std::move(*ioCtrl);
  std::map<int, int> laneToLedMap;
  auto numLanes = xcvrLib.getNumLanesForTransceiver(tcvrId);
  auto numLeds = xcvrLib.getNumLedsForTransceiver(tcvrId);
  if (numLanes.has_value() && numLeds.has_value() && *numLeds > 0) {
    int lanesPerLed = *numLanes / *numLeds;
    if (lanesPerLed > 0) {
      for (int lane = 1; lane <= *numLanes; ++lane) {
        int ledIndex = std::min((lane - 1) / lanesPerLed, *numLeds - 1);
        laneToLedMap[lane] = firstLedId + ledIndex;
      }
    }
  }
  mapping.tcvrLaneToLedId() = std::move(laneToLedMap);
  return mapping;
}

std::optional<LedMapping> buildLedMapping(
    const XcvrLib& xcvrLib,
    int ledId,
    int tcvrId,
    int ledNumForTcvr) {
  auto bluePath =
      xcvrLib.getLedSysfsPath(tcvrId, ledNumForTcvr, XcvrLib::LedColor::BLUE);
  auto amberPath =
      xcvrLib.getLedSysfsPath(tcvrId, ledNumForTcvr, XcvrLib::LedColor::AMBER);
  if (!bluePath.has_value() || !amberPath.has_value()) {
    XLOG(WARNING) << "Transceiver " << tcvrId << " LED " << ledNumForTcvr
                  << " missing sysfs path, skipping";
    return std::nullopt;
  }

  LedMapping led;
  led.id() = ledId;
  led.transceiverId() = tcvrId;
  led.bluePath() = std::move(*bluePath);
  led.yellowPath() = std::move(*amberPath);
  return led;
}

// Compute firstLedId for each transceiver. Shared by both transceiver mapping
// (lane-to-LED references) and LED mapping (actual LED entries) to avoid
// duplicating the accumulation logic.
std::map<int, int> computeFirstLedIds(const XcvrLib& xcvrLib) {
  std::map<int, int> firstLedIds;
  int numXcvrs = xcvrLib.getNumTransceivers();
  int nextLedId = 1;
  for (int tcvrId = 1; tcvrId <= numXcvrs; ++tcvrId) {
    firstLedIds[tcvrId] = nextLedId;
    auto numLeds = xcvrLib.getNumLedsForTransceiver(tcvrId);
    if (numLeds.has_value()) {
      nextLedId += *numLeds;
    }
  }
  return firstLedIds;
}

std::map<int, LedMapping> buildAllLedMappings(
    const XcvrLib& xcvrLib,
    const std::map<int, int>& firstLedIds) {
  std::map<int, LedMapping> ledMappings;
  int numXcvrs = xcvrLib.getNumTransceivers();
  for (int tcvrId = 1; tcvrId <= numXcvrs; ++tcvrId) {
    auto numLeds = xcvrLib.getNumLedsForTransceiver(tcvrId);
    if (!numLeds.has_value()) {
      continue;
    }
    int firstLedId = firstLedIds.at(tcvrId);
    for (int ledNum = 1; ledNum <= *numLeds; ++ledNum) {
      int ledId = firstLedId + ledNum - 1;
      auto led = buildLedMapping(xcvrLib, ledId, tcvrId, ledNum);
      if (led.has_value()) {
        ledMappings[ledId] = std::move(*led);
      }
    }
  }
  return ledMappings;
}

} // namespace

BspPlatformMappingThrift buildFromXcvrLib(const std::string& platformName) {
  return buildFromXcvrLib(XcvrLib(platformName));
}

BspPlatformMappingThrift buildFromXcvrLib(const XcvrLib& xcvrLib) {
  int numXcvrs = xcvrLib.getNumTransceivers();

  XLOG(INFO) << "Building BspPlatformMapping for " << numXcvrs
             << " transceivers";

  BspPlatformMappingThrift result;

  auto firstLedIds = computeFirstLedIds(xcvrLib);

  BspPimMapping pimMapping;
  pimMapping.pimID() = kPimId;

  // Build transceiver mappings
  std::map<int, BspTransceiverMapping> tcvrMap;
  for (int tcvrId = 1; tcvrId <= numXcvrs; ++tcvrId) {
    auto mapping =
        buildTransceiverMapping(xcvrLib, tcvrId, firstLedIds.at(tcvrId));
    if (mapping.has_value()) {
      tcvrMap[tcvrId] = std::move(*mapping);
    }
  }
  pimMapping.tcvrMapping() = std::move(tcvrMap);

  // Build LED mappings
  pimMapping.ledMapping() = buildAllLedMappings(xcvrLib, firstLedIds);

  // Empty PHY mappings
  pimMapping.phyMapping() = std::map<int, BspPhyMapping>{};
  pimMapping.phyIOControllers() = std::map<int, BspPhyIOControllerInfo>{};

  result.pimMapping() = {{kPimId, std::move(pimMapping)}};

  XLOG(INFO) << "Built BspPlatformMapping with " << numXcvrs << " transceivers";

  return result;
}

} // namespace facebook::fboss
