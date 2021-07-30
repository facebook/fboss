// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/lib/phy/PhyManager.h"

#include "fboss/agent/FbossError.h"
#include "fboss/agent/platforms/common/PlatformMapping.h"
#include "fboss/agent/types.h"
#include "fboss/lib/config/PlatformConfigUtils.h"
#include "fboss/lib/phy/ExternalPhy.h"
#include "fboss/lib/phy/gen-cpp2/phy_types.h"

#include <folly/json.h>
#include <folly/logging/xlog.h>
#include <thrift/lib/cpp/util/EnumUtils.h>
#include <cstdlib>

namespace {
// Key of the portToCacheInfo map in warmboot state cache
constexpr auto kPortToCacheInfoKey = "portToCacheInfo";
constexpr auto kSystemLanesKey = "systemLanes";
constexpr auto kLineLanesKey = "lineLanes";
} // namespace

namespace facebook::fboss {

PhyManager::PhyManager(const PlatformMapping* platformMapping)
    : platformMapping_(platformMapping) {
  const auto& chips = platformMapping_->getChips();
  for (const auto& port : platformMapping_->getPlatformPorts()) {
    const auto& xphy = utility::getDataPlanePhyChips(
        port.second, chips, phy::DataPlanePhyChipType::XPHY);
    if (!xphy.empty()) {
      auto cache = std::make_unique<PortCacheInfo>();
      cache->xphyID = GlobalXphyID(*xphy.begin()->second.physicalID_ref());
      portToCacheInfo_.emplace(PortID(port.first), std::move(cache));
    }
  }
}

PhyManager::~PhyManager() {}

GlobalXphyID PhyManager::getGlobalXphyIDbyPortID(PortID portID) const {
  if (auto id = portToCacheInfo_.find(portID); id != portToCacheInfo_.end()) {
    return id->second->xphyID;
  }
  throw FbossError(
      "Unable to get GlobalXphyID for port:",
      portID,
      ", current portToCacheInfo_ size:",
      portToCacheInfo_.size());
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

phy::PhyPortConfig PhyManager::getHwPhyPortConfig(PortID portId) {
  // First check whether we have cached lane inf
  const auto& cacheInfo = portToCacheInfo_.find(portId);
  if (cacheInfo == portToCacheInfo_.end() ||
      cacheInfo->second->systemLanes.empty() ||
      cacheInfo->second->lineLanes.empty()) {
    throw FbossError(
        "Port:", portId, " has not program yet. Can't find the cached info");
  }
  auto* xphy = getExternalPhy(portId);
  return xphy->getConfigOnePort(
      cacheInfo->second->systemLanes, cacheInfo->second->lineLanes);
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
  // Due to PhyManager::PhyManager(), we always create the PortCacheInfo with
  // the global xphy id for possible ports in PlatformMapping.
  const auto& cache = portToCacheInfo_.find(portID);
  if (cache == portToCacheInfo_.end()) {
    throw FbossError(
        "Unrecoginized port=", portID, ", which is not in PlatformMapping");
  }

  // First check whether there's a systemLanes and lineLanes cache already
  const auto& systemLanesConfig = portConfig.config.system.lanes;
  const auto& lineLanesConfig = portConfig.config.line.lanes;
  const auto& cacheInfo = cache->second;
  // check whether all the lanes match
  bool matched = (cacheInfo->systemLanes.size() == systemLanesConfig.size()) &&
      (cacheInfo->lineLanes.size() == lineLanesConfig.size());
  // Now check system lane id
  if (matched) {
    for (auto i = 0; i < cacheInfo->systemLanes.size(); ++i) {
      if (systemLanesConfig.find(cacheInfo->systemLanes[i]) ==
          systemLanesConfig.end()) {
        matched = false;
        break;
      }
    }
  }
  // Now check line lane id
  if (matched) {
    for (auto i = 0; i < cacheInfo->lineLanes.size(); ++i) {
      if (lineLanesConfig.find(cacheInfo->lineLanes[i]) ==
          lineLanesConfig.end()) {
        matched = false;
        break;
      }
    }
  }
  if (matched) {
    return;
  }

  // Now reset the cached lane info if there's no match
  portToCacheInfo_[portID]->systemLanes.clear();
  for (const auto& it : portConfig.config.system.lanes) {
    portToCacheInfo_[portID]->systemLanes.push_back(it.first);
  }
  portToCacheInfo_[portID]->lineLanes.clear();
  for (const auto& it : portConfig.config.line.lanes) {
    portToCacheInfo_[portID]->lineLanes.push_back(it.first);
  }
}

const std::vector<LaneID>& PhyManager::getCachedLanes(
    PortID portID,
    phy::Side side) const {
  // Try to get the cached lanes list
  const auto& cache = portToCacheInfo_.find(portID);
  if (cache == portToCacheInfo_.end() || cache->second->systemLanes.empty() ||
      cache->second->lineLanes.empty()) {
    throw FbossError(
        "Port:", portID, " is not programmed and can't find cached lanes");
  }
  return (
      side == phy::Side::SYSTEM ? cache->second->systemLanes
                                : cache->second->lineLanes);
}

void PhyManager::setPortPrbs(
    PortID portID,
    phy::Side side,
    const phy::PortPrbsState& prbs) {
  auto* xphy = getExternalPhy(portID);
  if (!xphy->isSupported(phy::ExternalPhy::Feature::PRBS)) {
    throw FbossError("Port:", portID, " xphy can't support PRBS");
  }
  xphy->setPortPrbs(side, getCachedLanes(portID, side), prbs);
}

phy::PortPrbsState PhyManager::getPortPrbs(PortID portID, phy::Side side) {
  auto* xphy = getExternalPhy(portID);
  if (!xphy->isSupported(phy::ExternalPhy::Feature::PRBS)) {
    throw FbossError("Port:", portID, " xphy can't support PRBS");
  }
  return xphy->getPortPrbs(side, getCachedLanes(portID, side));
}

folly::dynamic PhyManager::getWarmbootState() const {
  folly::dynamic phyState = folly::dynamic::object;
  folly::dynamic portToCacheInfoCache = folly::dynamic::object;
  // For now, we just need to cache the current system/line lanes
  for (const auto& it : portToCacheInfo_) {
    folly::dynamic portCacheInfo = folly::dynamic::object;
    folly::dynamic systemLanes = folly::dynamic::array;
    for (auto lane : it.second->systemLanes) {
      systemLanes.push_back(static_cast<int>(lane));
    }
    folly::dynamic lineLanes = folly::dynamic::array;
    for (auto lane : it.second->lineLanes) {
      lineLanes.push_back(static_cast<int>(lane));
    }
    portCacheInfo[kSystemLanesKey] = systemLanes;
    portCacheInfo[kLineLanesKey] = lineLanes;
    portToCacheInfoCache[folly::to<std::string>(static_cast<int>(it.first))] =
        portCacheInfo;
  }
  phyState[kPortToCacheInfoKey] = portToCacheInfoCache;
  return phyState;
}

void PhyManager::restoreFromWarmbootState(
    const folly::dynamic& phyWarmbootState) {
  if (phyWarmbootState.find(kPortToCacheInfoKey) ==
      phyWarmbootState.items().end()) {
    throw FbossError(
        "Can't find ",
        kPortToCacheInfoKey,
        " in the phy warmboot state. Skip restoring portToCacheInfo_ map");
  }

  const auto& portToCacheInfoCache = phyWarmbootState[kPortToCacheInfoKey];
  for (const auto& it : portToCacheInfo_) {
    auto portID = static_cast<int>(it.first);
    auto portIDStr = folly::to<std::string>(portID);
    if (portToCacheInfoCache.find(portIDStr) ==
        portToCacheInfoCache.items().end()) {
      XLOG(WARN) << "Can't find port=" << portID
                 << " in the phy warmboot portToCacheInfo map.";
      continue;
    }
    const auto& portCacheInfo = portToCacheInfoCache[portIDStr];
    if (portCacheInfo.find(kSystemLanesKey) == portCacheInfo.items().end() ||
        portCacheInfo.find(kLineLanesKey) == portCacheInfo.items().end()) {
      throw FbossError(
          "Can't find the system and line lanes for port=",
          portID,
          " in the phy warmboot portToCacheInfo map.");
    }
    for (auto lane : portCacheInfo[kSystemLanesKey]) {
      portToCacheInfo_[PortID(portID)]->systemLanes.push_back(
          LaneID(lane.asInt()));
    }
    for (auto lane : portCacheInfo[kLineLanesKey]) {
      portToCacheInfo_[PortID(portID)]->lineLanes.push_back(
          LaneID(lane.asInt()));
    }
    XLOG(INFO) << "Restore port=" << portID
               << ", systemLanes=" << portCacheInfo[kSystemLanesKey]
               << ", lineLanes=" << portCacheInfo[kLineLanesKey]
               << " from the phy warmboot state";
  }
}

void PhyManager::updateStats(PortID portID) {
  auto* xphy = getExternalPhy(portID);
  // If the xphy doesn't support either port or prbs stats, no-op
  if (!xphy->isSupported(phy::ExternalPhy::Feature::PORT_STATS) &&
      !xphy->isSupported(phy::ExternalPhy::Feature::PRBS_STATS)) {
    return;
  }

  // Try to get the cached lanes list
  const auto& cache = portToCacheInfo_.find(portID);
  if (cache == portToCacheInfo_.end() || cache->second->systemLanes.empty() ||
      cache->second->lineLanes.empty()) {
    throw FbossError(
        "Port:", portID, " is not programmed and can't find cached lanes");
  }

  // TODO(joseph5wu) Fetch the pim EventBase to get port and prbs stats
  if (xphy->isSupported(phy::ExternalPhy::Feature::PORT_STATS)) {
    // Collect xphy port stats
  }
  if (xphy->isSupported(phy::ExternalPhy::Feature::PRBS_STATS)) {
    // Collect xphy prbs stats
  }
}

const std::string& PhyManager::getPortName(PortID portID) const {
  const auto& portEntry = platformMapping_->getPlatformPorts().find(portID);
  if (portEntry == platformMapping_->getPlatformPorts().end()) {
    throw FbossError(
        "Unrecoginized port=", portID, ", which is not in PlatformMapping");
  }
  return *portEntry->second.mapping_ref()->name_ref();
}
} // namespace facebook::fboss
