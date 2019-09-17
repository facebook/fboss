/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/platforms/sai/SaiPlatformPort.h"
#include "fboss/agent/FbossError.h"
#include "fboss/agent/platforms/sai/SaiPlatform.h"

namespace facebook {
namespace fboss {

PortID SaiPlatformPort::getPortID() const {
  return id_;
}
void SaiPlatformPort::preDisable(bool /* temporary */) {}
void SaiPlatformPort::postDisable(bool /* temporary */) {}
void SaiPlatformPort::preEnable() {}
void SaiPlatformPort::postEnable() {}
bool SaiPlatformPort::isMediaPresent() {
  return true;
}
void SaiPlatformPort::linkStatusChanged(bool /* up */, bool /* adminUp */) {}
void SaiPlatformPort::linkSpeedChanged(const cfg::PortSpeed& /* speed */) {}
bool SaiPlatformPort::supportsTransceiver() const {
  return false;
}

void SaiPlatformPort::statusIndication(
    bool /* enabled */,
    bool /* link */,
    bool /* ingress */,
    bool /* egress */,
    bool /* discards */,
    bool /* errors */) {}
void SaiPlatformPort::prepareForGracefulExit() {}
bool SaiPlatformPort::shouldDisableFEC() const {
  return true;
}

folly::Optional<cfg::PlatformPortSettings>
SaiPlatformPort::getPlatformPortSettings(cfg::PortSpeed speed) const {
  auto& platformSettings = platform_->config()->thrift.platform;

  auto portsIter = platformSettings.ports.find(id_);
  if (portsIter == platformSettings.ports.end()) {
    throw FbossError("platform port id ", id_, " not found");
  }

  auto& portConfig = portsIter->second;
  auto speedIter = portConfig.supportedSpeeds.find(speed);
  if (speedIter == portConfig.supportedSpeeds.end()) {
    throw FbossError("Port ", id_, " does not support speed ", speed);
  }

  return speedIter->second;
}

std::vector<uint32_t> SaiPlatformPort::getHwPortLanes(
    cfg::PortSpeed speed) const {
  auto platformPortSettings = getPlatformPortSettings(speed);
  if (!platformPortSettings) {
    throw FbossError(
        "platform port settings is empty for port ", id_, "speed ", speed);
  }
  auto& laneMap = platformPortSettings->iphy_ref()->lanes;
  std::vector<uint32_t> hwLaneList;
  for (auto lane : laneMap) {
    hwLaneList.push_back(lane.first);
  }
  return hwLaneList;
}

std::vector<PortID> SaiPlatformPort::getSubsumedPorts(
    cfg::PortSpeed speed) const {
  auto platformPortSettings = getPlatformPortSettings(speed);
  if (!platformPortSettings) {
    throw FbossError(
        "platform port settings is empty for port ", id_, "speed ", speed);
  }
  std::vector<PortID> subsumedPortList;
  for (auto portId : platformPortSettings->subsumedPorts) {
    subsumedPortList.push_back(PortID(portId));
  }
  return subsumedPortList;
}

phy::FecMode SaiPlatformPort::getFecMode(cfg::PortSpeed speed) const {
  auto platformSettings = getPlatformPortSettings(speed);
  if (!platformSettings) {
    throw FbossError(
        "platform port settings is empty for port ", id_, "speed ", speed);
  }
  return platformSettings->iphy_ref()->fec;
}

folly::Optional<TransceiverID> SaiPlatformPort::getTransceiverID() const {
  // TODO (srikrishnagopu): TransceiverID is now part of AgentConfig in
  // front panel resources which is defined per speed. Declaring default
  // speed for now to get the transceiverid.
  auto speed = cfg::PortSpeed::HUNDREDG;
  auto platformSettings = getPlatformPortSettings(speed);
  if (!platformSettings) {
    throw FbossError(
        "platform port settings is empty for port ", id_, "speed ", speed);
  }
  return static_cast<TransceiverID>(
      platformSettings->frontPanelResources_ref()->transceiverId);
}

TransmitterTechnology SaiPlatformPort::getTransmitterTech() {
  // TODO(srikrishnagopu): Copy it from wedge port ?
  return TransmitterTechnology::OPTICAL;
}

} // namespace fboss
} // namespace facebook
