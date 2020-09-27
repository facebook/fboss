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
#include "fboss/qsfp_service/lib/QsfpCache.h"

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
        getPlatform()->getDataPlanePhyChip(*transceiverLanes[0].chip_ref());
    if (!chipConfig.has_value()) {
      throw FbossError(
          "Port ",
          getPortID(),
          " is using platform unsupported chip ",
          *transceiverLanes[0].chip_ref());
    }
    transceiverID_.emplace(TransceiverID(*chipConfig->physicalID_ref()));
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
  return !transceiverID_.has_value();
}

bool SaiPlatformPort::checkSupportsTransceiver() const {
  return supportsTransceiver() && !FLAGS_skip_transceiver_programming &&
      transceiverID_.has_value();
}

std::vector<uint32_t> SaiPlatformPort::getHwPortLanes(
    cfg::PortSpeed speed) const {
  auto profileID = getProfileIDBySpeed(speed);
  return getHwPortLanes(profileID);
}

std::vector<uint32_t> SaiPlatformPort::getHwPortLanes(
    cfg::PortProfileID profileID) const {
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
    auto chipIter = dataPlanePhyChips.find(*iphy.chip_ref());
    if (chipIter == dataPlanePhyChips.end()) {
      throw FbossError(
          "dataplane chip does not exist for chip: ", *iphy.chip_ref());
    }
    hwLaneList.push_back(getPhysicalLaneId(
        *chipIter->second.physicalID_ref(), *iphy.lane_ref()));
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
      platformPortEntry->supportedProfiles_ref()->find(profileID);
  if (supportedProfilesIter ==
      platformPortEntry->supportedProfiles_ref()->end()) {
    throw FbossError(
        "Port: ",
        *platformPortEntry->mapping_ref()->name_ref(),
        " doesn't support the speed profile:");
  }
  std::vector<PortID> subsumedPortList;
  for (auto portId : *supportedProfilesIter->second.subsumedPorts_ref()) {
    subsumedPortList.push_back(PortID(portId));
  }
  return subsumedPortList;
}

folly::Future<TransmitterTechnology>
SaiPlatformPort::getTransmitterTechInternal(folly::EventBase* evb) {
  if (!checkSupportsTransceiver()) {
    return folly::makeFuture<TransmitterTechnology>(
        TransmitterTechnology::COPPER);
  }
  int32_t transID = static_cast<int32_t>(getTransceiverID().value());
  auto getTech = [](TransceiverInfo info) {
    if (auto cable = info.cable_ref()) {
      return *cable->transmitterTech_ref();
    }
    return TransmitterTechnology::UNKNOWN;
  };
  auto handleError = [transID](const folly::exception_wrapper& e) {
    XLOG(ERR) << "Error retrieving info for transceiver " << transID
              << " Exception: " << folly::exceptionStr(e);
    return TransmitterTechnology::UNKNOWN;
  };
  folly::Future<TransceiverInfo> transceiverInfo = getTransceiverInfo();
  return transceiverInfo.via(evb).thenValueInline(getTech).thenError(
      std::move(handleError));
}

TransmitterTechnology SaiPlatformPort::getTransmitterTech() {
  folly::EventBase evb;
  return getTransmitterTechInternal(&evb).getVia(&evb);
}

TransceiverIdxThrift SaiPlatformPort::getTransceiverMapping(
    cfg::PortSpeed speed) {
  if (!checkSupportsTransceiver()) {
    return TransceiverIdxThrift();
  }
  auto profileID = getProfileIDBySpeed(speed);
  auto platformPortEntry = getPlatformPortEntry();
  std::vector<int32_t> lanes;
  if (!platformPortEntry.has_value()) {
    throw FbossError(
        "Platform Port entry does not exist for port: ", getPortID());
  }
  auto transceiverLanes = utility::getTransceiverLanes(
      *platformPortEntry, getPlatform()->getDataPlanePhyChips(), profileID);
  for (auto entry : transceiverLanes) {
    lanes.push_back(*entry.lane_ref());
  }
  TransceiverIdxThrift xcvr;
  xcvr.transceiverId_ref() = static_cast<int32_t>(*getTransceiverID());
  xcvr.channels_ref() = lanes;
  return xcvr;
}

std::vector<phy::PinConfig> SaiPlatformPort::getIphyPinConfigs(
    cfg::PortProfileID profileID) const {
  if (!checkSupportsTransceiver()) {
    return {};
  }
  if (auto cable = getCableInfo()) {
    if (auto cableLength = cable->length_ref()) {
      // TODO(pgardideh): this is temporary until we fully remove any
      // dependence on transmitter tech and only rely on the profile ID
      if (*cable->transmitterTech_ref() == TransmitterTechnology::COPPER) {
        auto cableMeters = std::max(1.0, std::min(3.0, *cableLength));
        return getPlatform()->getPlatformMapping()->getPortIphyPinConfigs(
            getPortID(), profileID, cableMeters);
      }
    }
  }
  return getPlatform()->getPlatformMapping()->getPortIphyPinConfigs(
      getPortID(), profileID);
}

folly::Future<std::optional<Cable>> SaiPlatformPort::getCableInfoInternal(
    folly::EventBase* evb) const {
  CHECK(checkSupportsTransceiver());
  int32_t transID = static_cast<int32_t>(getTransceiverID().value());
  auto getCable = [](TransceiverInfo info) -> std::optional<Cable> {
    return info.cable_ref().to_optional();
  };
  auto handleError =
      [transID](const folly::exception_wrapper& e) -> std::optional<Cable> {
    XLOG(ERR) << "Error retrieving cable info for transceiver " << transID
              << " Exception: " << folly::exceptionStr(e);
    return std::nullopt;
  };
  folly::Future<TransceiverInfo> transceiverInfo = getTransceiverInfo();
  return transceiverInfo.via(evb).thenValueInline(getCable).thenError(
      std::move(handleError));
}

std::optional<Cable> SaiPlatformPort::getCableInfo() const {
  folly::EventBase evb;
  return getCableInfoInternal(&evb).getVia(&evb);
}

folly::Future<TransceiverInfo> SaiPlatformPort::getTransceiverInfo() const {
  // use this method to query transceiver info
  // for hw test, it uses a map populated by switch ensemble to return
  // transceiver information
  if (auto transceiver =
          getPlatform()->getOverrideTransceiverInfo(getPortID())) {
    return transceiver.value();
  }
  auto qsfpCache = static_cast<SaiPlatform*>(getPlatform())->getQsfpCache();
  return qsfpCache->futureGet(getTransceiverID().value());
}

std::optional<ChannelID> SaiPlatformPort::getChannel() const {
  auto tcvrList = getTransceiverLanes();
  if (!tcvrList.empty()) {
    // All the transceiver lanes should use the same transceiver id
    return ChannelID(*tcvrList[0].lane_ref());
  }
  return std::nullopt;
}

int SaiPlatformPort::getLaneCount() const {
  auto lanes = getHwPortLanes(getCurrentProfile());
  return lanes.size();
}

} // namespace facebook::fboss
