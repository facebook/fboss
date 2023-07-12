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
#include <optional>
#include "fboss/agent/FbossError.h"
#include "fboss/agent/platforms/sai/SaiPlatform.h"
#include "fboss/lib/config/PlatformConfigUtils.h"

DEFINE_bool(
    skip_transceiver_programming,
    false,
    "Skip programming transceivers");

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

void SaiPlatformPort::statusIndication(
    bool /* enabled */,
    bool /* link */,
    bool /* ingress */,
    bool /* egress */,
    bool /* discards */,
    bool /* errors */) {}
void SaiPlatformPort::prepareForGracefulExit() {}
bool SaiPlatformPort::shouldDisableFEC() const {
  // disable for backplane port for galaxy switches
  return !getTransceiverID().has_value();
}

bool SaiPlatformPort::checkSupportsTransceiver() const {
  if (getPlatform()->getOverrideTransceiverInfo(getPortID())) {
    return true;
  }
  return supportsTransceiver() && !FLAGS_skip_transceiver_programming &&
      getTransceiverID().has_value();
}

std::vector<uint32_t> SaiPlatformPort::getHwPortLanes(
    cfg::PortSpeed speed) const {
  auto profileID = getProfileIDBySpeed(speed);
  return getHwPortLanes(profileID);
}

std::vector<uint32_t> SaiPlatformPort::getHwPortLanes(
    cfg::PortProfileID profileID) const {
  const auto& platformPortEntry = getPlatformPortEntry();
  auto& dataPlanePhyChips = getPlatform()->getDataPlanePhyChips();
  auto iphys = utility::getOrderedIphyLanes(
      platformPortEntry, dataPlanePhyChips, profileID);
  std::vector<uint32_t> hwLaneList;
  for (auto iphy : iphys) {
    auto chipIter = dataPlanePhyChips.find(*iphy.chip());
    if (chipIter == dataPlanePhyChips.end()) {
      throw FbossError(
          "dataplane chip does not exist for chip: ", *iphy.chip());
    }
    hwLaneList.push_back(
        getPhysicalLaneId(*chipIter->second.physicalID(), *iphy.lane()));
  }
  return hwLaneList;
}

std::vector<PortID> SaiPlatformPort::getSubsumedPorts(
    cfg::PortSpeed speed) const {
  auto profileID = getProfileIDBySpeed(speed);
  const auto& platformPortEntry = getPlatformPortEntry();
  auto supportedProfilesIter =
      platformPortEntry.supportedProfiles()->find(profileID);
  if (supportedProfilesIter == platformPortEntry.supportedProfiles()->end()) {
    throw FbossError(
        "Port: ",
        *platformPortEntry.mapping()->name(),
        " doesn't support the speed profile:");
  }
  std::vector<PortID> subsumedPortList;
  for (auto portId : *supportedProfilesIter->second.subsumedPorts()) {
    subsumedPortList.push_back(PortID(portId));
  }
  return subsumedPortList;
}

TransceiverIdxThrift SaiPlatformPort::getTransceiverMapping(
    cfg::PortSpeed speed) {
  if (!checkSupportsTransceiver()) {
    return TransceiverIdxThrift();
  }
  auto profileID = getProfileIDBySpeed(speed);
  const auto& platformPortEntry = getPlatformPortEntry();
  std::vector<int32_t> lanes;
  auto transceiverLanes = utility::getTransceiverLanes(
      platformPortEntry, getPlatform()->getDataPlanePhyChips(), profileID);
  for (auto entry : transceiverLanes) {
    lanes.push_back(*entry.lane());
  }
  TransceiverIdxThrift xcvr;
  xcvr.transceiverId() = static_cast<int32_t>(*getTransceiverID());
  xcvr.channels() = lanes;
  return xcvr;
}

std::shared_ptr<TransceiverSpec> SaiPlatformPort::getTransceiverSpec() const {
  // use this method to query transceiver info
  // for hw test, it uses a map populated by switch ensemble to return
  // transceiver information
  if (auto overrideTransceiverInfo =
          getPlatform()->getOverrideTransceiverInfo(getPortID())) {
    auto overrideTransceiverSpec = TransceiverSpec::createPresentTransceiver(
        overrideTransceiverInfo.value());
    return overrideTransceiverSpec;
  }
  auto transceiverMaps = static_cast<SaiPlatform*>(getPlatform())
                             ->getHwSwitch()
                             ->getProgrammedState()
                             ->getTransceivers();
  auto transceiverSpec = transceiverMaps->getNodeIf(getTransceiverID().value());
  return transceiverSpec;
}

std::optional<ChannelID> SaiPlatformPort::getChannel() const {
  auto tcvrList = getTransceiverLanes();
  if (!tcvrList.empty()) {
    // All the transceiver lanes should use the same transceiver id
    return ChannelID(*tcvrList[0].lane());
  }
  return std::nullopt;
}

int SaiPlatformPort::getLaneCount() const {
  auto lanes = getHwPortLanes(getCurrentProfile());
  return lanes.size();
}

} // namespace facebook::fboss
