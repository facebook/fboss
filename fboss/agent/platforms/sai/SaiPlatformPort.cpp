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
#include "fboss/lib/config/PlatformConfigUtils.h"

namespace facebook::fboss {
SaiPlatformPort::SaiPlatformPort(PortID id, SaiPlatform* platform)
    : PlatformPort(id, platform) {}

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

std::optional<cfg::PlatformPortSettings>
SaiPlatformPort::getPlatformPortSettings(cfg::PortSpeed speed) const {
  auto& platformSettings = getPlatform()->config()->thrift.platform;

  auto portsIter = platformSettings.ports.find(getPortID());
  if (portsIter == platformSettings.ports.end()) {
    throw FbossError("platform port id ", getPortID(), " not found");
  }

  auto& portConfig = portsIter->second;
  auto speedIter = portConfig.supportedSpeeds.find(speed);
  if (speedIter == portConfig.supportedSpeeds.end()) {
    throw FbossError("Port ", getPortID(), " does not support speed ", speed);
  }

  return speedIter->second;
}

std::vector<uint32_t> SaiPlatformPort::getHwPortLanes(
    cfg::PortSpeed speed) const {
  auto profileID = getProfileIDBySpeed(speed);
  auto platformPortEntry = getPlatformPortEntry();
  if (!platformPortEntry.has_value()) {
    throw FbossError(
        "Platform Port entry does not exist for port: ", getPortID());
  }
  auto& dataPlanePhyChips = getPlatform()->getDataPlanePhyChips();
  auto iphys = utility::getOrderedIphyLanes(
      *platformPortEntry, dataPlanePhyChips, profileID);
  std::vector<uint32_t> hwLaneList;
  for (auto iphy : iphys) {
    auto chipIter = dataPlanePhyChips.find(iphy.chip);
    if (chipIter == dataPlanePhyChips.end()) {
      throw FbossError("dataplane chip does not exist for chip: ", iphy.chip);
    }
    hwLaneList.push_back(
        getPhysicalLaneId(chipIter->second.physicalID, iphy.lane));
  }
  return hwLaneList;
}

std::vector<PortID> SaiPlatformPort::getSubsumedPorts(
    cfg::PortSpeed speed) const {
  auto platformPortSettings = getPlatformPortSettings(speed);
  if (!platformPortSettings) {
    throw FbossError(
        "platform port settings is empty for port ",
        getPortID(),
        "speed ",
        speed);
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
        "platform port settings is empty for port ",
        getPortID(),
        "speed ",
        speed);
  }
  return platformSettings->iphy_ref()->fec;
}

std::optional<TransceiverID> SaiPlatformPort::getTransceiverID() const {
  // TODO (srikrishnagopu): TransceiverID is now part of AgentConfig in
  // front panel resources which is defined per speed. Declaring default
  // speed for now to get the transceiverid.
  auto speed = cfg::PortSpeed::HUNDREDG;
  auto platformSettings = getPlatformPortSettings(speed);
  if (!platformSettings) {
    throw FbossError(
        "platform port settings is empty for port ",
        getPortID(),
        "speed ",
        speed);
  }
  return static_cast<TransceiverID>(
      platformSettings->frontPanelResources_ref()->transceiverId);
}

TransmitterTechnology SaiPlatformPort::getTransmitterTech() {
  // TODO(srikrishnagopu): Copy it from wedge port ?
  return TransmitterTechnology::OPTICAL;
}

} // namespace facebook::fboss
