// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/lib/phy/PhyManager.h"

#include "fboss/agent/FbossError.h"
#include "fboss/agent/platforms/common/PlatformMapping.h"
#include "fboss/agent/types.h"
#include "fboss/lib/config/PlatformConfigUtils.h"

#include <folly/logging/xlog.h>
#include <thrift/lib/cpp/util/EnumUtils.h>

namespace facebook::fboss {
PhyManager::PhyManager(const PlatformMapping* platformMapping)
    : platformMapping_(platformMapping) {
  const auto& chips = platformMapping_->getChips();
  for (const auto& port : platformMapping_->getPlatformPorts()) {
    const auto& xphy = utility::getDataPlanePhyChips(
        port.second, chips, phy::DataPlanePhyChipType::XPHY);
    if (!xphy.empty()) {
      portToGlobalXphyID_.emplace(
          PortID(port.first),
          GlobalXphyID(*xphy.begin()->second.physicalID_ref()));
    }
  }
}

GlobalXphyID PhyManager::getGlobalXphyIDbyPortID(PortID portID) const {
  if (auto id = portToGlobalXphyID_.find(portID);
      id != portToGlobalXphyID_.end()) {
    return id->second;
  }
  throw FbossError(
      "Unable to get GlobalXphyID for port:",
      portID,
      ", current portToGlobalXphyID_ size:",
      portToGlobalXphyID_.size());
}

phy::ExternalPhy* PhyManager::getExternalPhy(GlobalXphyID xphyID) {
  auto pimID = getPhyIDInfo(xphyID).pimID;
  const auto& pimXphyMap = xphyMap_.find(pimID);
  if (pimXphyMap == xphyMap_.end()) {
    throw FbossError(
        "getExternalPhy: Invalid Slot Id ", pimID, " is not in xphyMap_");
  }
  if (pimXphyMap->second.find(xphyID) == pimXphyMap->second.end()) {
    throw FbossError(
        "getExternalPhy: Invalid Global Xphy Id ",
        xphyID,
        " is not in xphyMap_ for Pim Id ",
        pimID);
  }
  // Return the externalPhy object for this slot, mdio, phy
  return xphyMap_[pimID][xphyID].get();
}

phy::PhyPortConfig PhyManager::getDesiredPhyPortConfig(
    PortID portId,
    cfg::PortProfileID portProfileId,
    std::optional<TransceiverInfo> transceiverInfo) {
  auto platformPortEntry = platformMapping_->getPlatformPorts().find(portId);
  if (platformPortEntry == platformMapping_->getPlatformPorts().end()) {
    throw FbossError(
        "Can't find the platform port entry in platform mapping for port:",
        portId);
  }
  const auto& chips = platformMapping_->getChips();
  if (chips.empty()) {
    throw FbossError("No DataPlanePhyChips found");
  }

  std::optional<PlatformPortProfileConfigMatcher> matcher;
  if (transceiverInfo) {
    matcher = PlatformPortProfileConfigMatcher(
        portProfileId, portId, *transceiverInfo);
  } else {
    matcher = PlatformPortProfileConfigMatcher(portProfileId, portId);
  }
  const auto& portPinConfig = platformMapping_->getPortXphyPinConfig(*matcher);
  const auto& portProfileConfig =
      platformMapping_->getPortProfileConfig(*matcher);
  if (!portProfileConfig.has_value()) {
    throw FbossError(
        "No port profile with id ",
        apache::thrift::util::enumNameSafe(portProfileId),
        " found in PlatformConfig for port ",
        portId);
  }

  phy::PhyPortConfig phyPortConfig;
  phyPortConfig.config = phy::ExternalPhyConfig::fromConfigeratorTypes(
      portPinConfig,
      utility::getXphyLinePolaritySwapMap(
          *platformPortEntry->second.mapping_ref()->pins_ref(), chips));
  phyPortConfig.profile =
      phy::ExternalPhyProfileConfig::fromPortProfileConfig(*portProfileConfig);
  return phyPortConfig;
}

void PhyManager::programOnePort(
    PortID portId,
    cfg::PortProfileID portProfileId,
    std::optional<TransceiverInfo> transceiverInfo) {
  // This function will call ExternalPhy::programOnePort(phy::PhyPortConfig).
  auto* xphy = getExternalPhy(portId);
  xphy->programOnePort(
      getDesiredPhyPortConfig(portId, portProfileId, transceiverInfo));
}

} // namespace facebook::fboss
