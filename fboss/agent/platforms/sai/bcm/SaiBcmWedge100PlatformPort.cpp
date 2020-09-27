// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/platforms/sai/SaiBcmWedge100PlatformPort.h"

#include "fboss/agent/FbossError.h"
#include "fboss/agent/platforms/common/utils/Wedge100LedUtils.h"
#include "fboss/agent/platforms/sai/SaiPlatform.h"

#include <folly/logging/xlog.h>
#include <tuple>

namespace facebook::fboss {

void SaiBcmWedge100PlatformPort::linkStatusChanged(bool up, bool adminUp) {
  uint32_t phyPortId = getPhysicalPortId();
  auto ledAndIndex = getLedAndIndex(phyPortId);
  if (!ledAndIndex) {
    return;
  }

  auto [led, index] = ledAndIndex.value();
  auto lanes = getHwPortLanes(getCurrentProfile());
  try {
    uint32_t status =
        static_cast<uint32_t>(Wedge100LedUtils::getDesiredLEDState(
            static_cast<int>(lanes.size()), up, adminUp));
    if (!useCompactMode()) {
      XLOG(DBG5) << "setting LED status in non-compact mode";
      setLEDState(led, index, status);
    } else {
      // compact mode
      XLOG(DBG5) << "setting LED status in compact mode";
      compactLEDStateChange(status);
    }
  } catch (const FbossError& ex) {
    XLOG(ERR) << "exception while setting LED state for port: "
              << static_cast<PortID>(phyPortId) << ":" << ex.what();
    throw;
  }
}

void SaiBcmWedge100PlatformPort::compactLEDStateChange(uint32_t status) {
  auto quadMode = getLaneCount() == 4;
  auto channel = getChannel();
  if (!channel || *channel == 1 || *channel == 3 ||
      (quadMode && *channel == 2)) {
    // We don't ever do anything for channel 1 & 3 in compact mode, and
    // we shouldn't do anything for lane 2 if in QUAD mode;
    XLOG(DBG5) << "skipping LED status change";
    return;
  }
  std::vector<uint32_t> compactModePorts{};
  if (Wedge100LedUtils::isTop(getTransceiverID())) {
    compactModePorts.push_back(getPhysicalPortId());
    auto neighborPort = static_cast<PortID>(static_cast<int>(getPortID()) + 4);
    auto platformPort = static_cast<SaiBcmWedge100PlatformPort*>(
        getPlatform()->getPlatformPort(neighborPort));
    if (platformPort) {
      compactModePorts.push_back(platformPort->getPhysicalPortId());
    } else {
      XLOG(ERR) << "platform port not found " << neighborPort;
    }
  } else {
    auto target = quadMode ? getPortID() + 2 : getPortID();
    for (auto neighborPort : {target + 1, target - 3}) {
      auto platformPort = static_cast<SaiBcmWedge100PlatformPort*>(
          getPlatform()->getPlatformPort(static_cast<PortID>(neighborPort)));
      if (platformPort) {
        compactModePorts.push_back(platformPort->getPhysicalPortId());
      } else {
        XLOG(ERR) << "platform port not found " << neighborPort;
      }
    }
  }

  for (auto phyPort : compactModePorts) {
    if (auto ledAndIndex = getLedAndIndex(phyPort)) {
      auto [led, index] = ledAndIndex.value();
      XLOG(DBG5) << "compact LED state changed for " << phyPort;
      setLEDState(led, index, status);
    }
  }
}

void SaiBcmWedge100PlatformPort::externalState(PortLedExternalState lfs) {
  uint32_t phyPortId = getPhysicalPortId();
  auto ledAndIndex = getLedAndIndex(phyPortId);
  if (!ledAndIndex) {
    return;
  }

  auto [led, index] = ledAndIndex.value();

  uint32_t status = static_cast<uint32_t>(Wedge100LedUtils::getDesiredLEDState(
      lfs, static_cast<Wedge100LedUtils::LedColor>(getCurrentLedState())));
  setLEDState(led, index, status);
}

std::optional<std::tuple<uint32_t, uint32_t>>
SaiBcmWedge100PlatformPort::getLedAndIndex(uint32_t phyPortId) {
  PortID port = static_cast<PortID>(phyPortId);
  int index = Wedge100LedUtils::getPortIndex(port);
  auto led = Wedge100LedUtils::getLEDProcessorNumber(port);
  if (!led) {
    return std::nullopt;
  }
  return std::make_tuple(led.value(), index);
}

bool SaiBcmWedge100PlatformPort::useCompactMode() const {
  auto tcvrID = getTransceiverID();
  if (!tcvrID) {
    return false;
  }
  // This should return true when the port is part of a qsfp that is
  // configured in a way that it could mirror the leds. This means
  // that the port is configured as either one or two ports
  // (e.g. 1x40, 1x100 or 2x50). If in compact mode, the button should
  // have no effect on these LEDs.

  auto lanesCount = getLaneCount();

  if (lanesCount != 1) {
    // Our port group supports compact mode. Now we need to check
    // our neighboring port group
    auto neighborFpPort = TransceiverID((*tcvrID) ^ 0x1);
    auto neighborPorts = static_cast<SaiPlatform*>(getPlatform())
                             ->getPortsWithTransceiverID(neighborFpPort);
    if (neighborPorts.size() > 0 && neighborPorts[0]->getLaneCount() != 1) {
      return true;
    }
  }
  return false;
}
} // namespace facebook::fboss
