// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/lib/phy/PhyManager.h"

#include "fboss/agent/FbossError.h"
#include "fboss/agent/platforms/common/PlatformMapping.h"
#include "fboss/agent/types.h"
#include "fboss/lib/config/PlatformConfigUtils.h"

#include <folly/logging/xlog.h>
#include <thrift/lib/cpp/util/EnumUtils.h>
#include <cstdlib>

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

PhyManager::~PhyManager() {}

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
  const auto& desiredPhyPortConfig =
      getDesiredPhyPortConfig(portId, portProfileId, transceiverInfo);
  xphy->programOnePort(desiredPhyPortConfig);
  // Once the port is programmed successfully, update the portToLanesInfo_
  setPortToLanesInfo(portId, desiredPhyPortConfig);
}

folly::EventBase* FOLLY_NULLABLE
PhyManager::getPimEventBase(PimID pimID) const {
  if (auto pimEventMultiThread = pimToThread_.find(pimID);
      pimEventMultiThread != pimToThread_.end()) {
    return pimEventMultiThread->second->eventBase.get();
  }
  return nullptr;
}

PhyManager::PimEventMultiThreading::PimEventMultiThreading(PimID pimID) {
  pim = pimID;
  eventBase = std::make_unique<folly::EventBase>();
  // start pim thread
  thread = std::make_unique<std::thread>([=] { eventBase->loopForever(); });
  XLOG(DBG2) << "Created PimEventMultiThreading for pim="
             << static_cast<int>(pimID);
}

PhyManager::PimEventMultiThreading::~PimEventMultiThreading() {
  eventBase->terminateLoopSoon();
  thread->join();
  XLOG(DBG2) << "Terminated multi-threading for pim=" << static_cast<int>(pim);
}

void PhyManager::setupPimEventMultiThreading(PimID pimID) {
  pimToThread_.emplace(pimID, std::make_unique<PimEventMultiThreading>(pimID));
}

void PhyManager::setPortToLanesInfo(
    PortID portID,
    const phy::PhyPortConfig& portConfig) {
  // First check whether there's a cache already
  bool matched = false;
  if (const auto& cached = portToLanesInfo_.find(portID);
      cached != portToLanesInfo_.end()) {
    const auto& systemLanesConfig = portConfig.config.system.lanes;
    const auto& lineLanesConfig = portConfig.config.line.lanes;
    const auto& cachedLanesInfo = cached->second;
    // check whether all the lanes match
    matched = (cachedLanesInfo->system.size() == systemLanesConfig.size()) &&
        (cachedLanesInfo->line.size() == lineLanesConfig.size());
    // Now check system lane id
    if (matched) {
      for (auto i = 0; i < cachedLanesInfo->system.size(); ++i) {
        if (systemLanesConfig.find(cachedLanesInfo->system[i]) ==
            systemLanesConfig.end()) {
          matched = false;
          break;
        }
      }
    }
    // Now check line lane id
    if (matched) {
      for (auto i = 0; i < cachedLanesInfo->line.size(); ++i) {
        if (lineLanesConfig.find(cachedLanesInfo->line[i]) ==
            lineLanesConfig.end()) {
          matched = false;
          break;
        }
      }
    }
  }

  if (matched) {
    return;
  }

  auto portLanesInfo = std::make_unique<PortLanesInfo>();
  for (const auto& it : portConfig.config.system.lanes) {
    portLanesInfo->system.push_back(it.first);
  }
  for (const auto& it : portConfig.config.line.lanes) {
    portLanesInfo->line.push_back(it.first);
  }
  portToLanesInfo_[portID] = std::move(portLanesInfo);
}

} // namespace facebook::fboss
