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
DEFINE_bool(
    skip_transceiver_programming,
    false,
    "Skip programming transceivers");

namespace facebook::fboss {
SaiPlatformPort::SaiPlatformPort(PortID id, SaiPlatform* platform)
    : PlatformPort(id, platform) {
  auto transceiverLanes = getTransceiverLanes();
  if (!transceiverLanes.empty()) {
    // All the transceiver lanes should use the same transceiver id
    auto chipConfig =
        getPlatform()->getDataPlanePhyChip(transceiverLanes[0].chip);
    if (!chipConfig.has_value()) {
      throw FbossError(
          "Port ",
          getPortID(),
          " is using platform unsupported chip ",
          transceiverLanes[0].chip);
    }
    transceiverID_.emplace(TransceiverID(chipConfig->physicalID));
  }
}

std::vector<phy::PinID> SaiPlatformPort::getTransceiverLanes() const {
  auto platformPortEntry = getPlatformPortEntry();
  if (!platformPortEntry.has_value()) {
    throw FbossError(
        "Platform Port entry does not exist for port: ", getPortID());
  }
  return utility::getTransceiverLanes(
      *platformPortEntry, getPlatform()->getDataPlanePhyChips(), std::nullopt);
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
  auto profileID = getProfileIDBySpeed(speed);
  auto platformPortEntry = getPlatformPortEntry();
  if (!platformPortEntry.has_value()) {
    throw FbossError(
        "Platform Port entry does not exist for port: ", getPortID());
  }
  auto supportedProfilesIter =
      platformPortEntry->supportedProfiles.find(profileID);
  if (supportedProfilesIter == platformPortEntry->supportedProfiles.end()) {
    throw FbossError(
        "Port: ",
        platformPortEntry->mapping.name,
        " doesn't support the speed profile:");
  }
  std::vector<PortID> subsumedPortList;
  for (auto portId : *supportedProfilesIter->second.subsumedPorts_ref()) {
    subsumedPortList.push_back(PortID(portId));
  }
  return subsumedPortList;
}

TransmitterTechnology SaiPlatformPort::getTransmitterTech() {
  // TODO(srikrishnagopu): Copy it from wedge port ?
  return TransmitterTechnology::OPTICAL;
}

} // namespace facebook::fboss
