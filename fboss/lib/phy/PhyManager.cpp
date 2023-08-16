// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/lib/phy/PhyManager.h"

#include "fboss/agent/FbossError.h"
#include "fboss/agent/PhySnapshotManager-defs.h"
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/platforms/common/PlatformMapping.h"
#include "fboss/agent/types.h"
#include "fboss/lib/config/PlatformConfigUtils.h"
#include "fboss/lib/phy/ExternalPhy.h"
#include "fboss/lib/phy/gen-cpp2/phy_types.h"

#include <folly/json.h>
#include <folly/logging/xlog.h>
#include <thrift/lib/cpp/util/EnumUtils.h>
#include <chrono>
#include <cstdlib>
#include <memory>

DEFINE_bool(
    init_pim_xphys,
    true,
    "Initialize pim xphys after creating xphy map");

namespace {
// Key of the portToCacheInfo map in warmboot state cache
constexpr auto kPortToCacheInfoKey = "portToCacheInfo";
constexpr auto kSystemLanesKey = "systemLanes";
constexpr auto kLineLanesKey = "lineLanes";
constexpr auto kPortProfileStrKey = "profile";
constexpr auto kPortSpeedStrKey = "speed";
} // namespace

namespace facebook::fboss {

namespace {
cfg::PortProfileID getProfileIDBySpeed(
    PortID portID,
    cfg::PortSpeed speed,
    const PlatformMapping* platformMapping) {
  auto portEntryIt = platformMapping->getPlatformPorts().find(portID);
  if (portEntryIt == platformMapping->getPlatformPorts().end()) {
    throw FbossError("Can't find port:", portID, " in PlatformMapping");
  }
  for (auto profile : *portEntryIt->second.supportedProfiles()) {
    auto profileID = profile.first;
    if (auto profileCfg = platformMapping->getPortProfileConfig(
            PlatformPortProfileConfigMatcher(profileID, portID))) {
      if (*profileCfg->speed() == speed) {
        return profileID;
      }
    } else {
      throw FbossError(
          "Platform port ",
          portID,
          " has invalid profile ",
          apache::thrift::util::enumNameSafe(profileID));
    }
  }
  throw FbossError(
      "Port:",
      portID,
      " doesn't support speed:",
      apache::thrift::util::enumNameSafe(speed));
}
} // namespace

PhyManager::PhyManager(const PlatformMapping* platformMapping)
    : platformMapping_(platformMapping),
      portToCacheInfo_(setupPortToCacheInfo(platformMapping)),
      portToStatsInfo_(setupPortToStatsInfo(platformMapping)),
      xphySnapshotManager_(
          std::make_unique<
              PhySnapshotManager<kXphySnapshotIntervalSeconds>>()) {}
PhyManager::~PhyManager() {}

PhyManager::PortToCacheInfo PhyManager::setupPortToCacheInfo(
    const PlatformMapping* platformMapping) {
  PortToCacheInfo cacheMap;
  const auto& chips = platformMapping->getChips();
  for (const auto& port : platformMapping->getPlatformPorts()) {
    const auto& xphy = utility::getDataPlanePhyChips(
        port.second, chips, phy::DataPlanePhyChipType::XPHY);
    if (!xphy.empty()) {
      auto cache = std::make_unique<folly::Synchronized<PortCacheInfo>>();
      auto wLockedCache = cache->wlock();
      wLockedCache->xphyID = GlobalXphyID(*xphy.begin()->second.physicalID());
      cacheMap.emplace(PortID(port.first), std::move(cache));
    }
  }
  return cacheMap;
}

PhyManager::PortToStatsInfo PhyManager::setupPortToStatsInfo(
    const PlatformMapping* platformMapping) {
  PortToStatsInfo statsMap;
  const auto& chips = platformMapping->getChips();
  for (const auto& port : platformMapping->getPlatformPorts()) {
    const auto& xphy = utility::getDataPlanePhyChips(
        port.second, chips, phy::DataPlanePhyChipType::XPHY);
    if (!xphy.empty()) {
      statsMap.emplace(
          PortID(port.first),
          std::make_unique<folly::Synchronized<PortStatsInfo>>());
    }
  }
  return statsMap;
}

GlobalXphyID PhyManager::getGlobalXphyIDbyPortID(PortID portID) const {
  return getGlobalXphyIDbyPortIDLocked(getRLockedCache(portID));
}

phy::ExternalPhy* PhyManager::getExternalPhy(GlobalXphyID xphyID) {
  auto pimID = getPhyIDInfo(xphyID).pimID;
  const auto& pimXphyMap = xphyMap_.find(pimID);
  if (pimXphyMap == xphyMap_.end()) {
    throw FbossError(
        "getExternalPhy: Invalid Slot Id ", pimID, " is not in xphyMap_");
  }
  const auto& xphyIt = pimXphyMap->second.find(xphyID);
  if (xphyIt == pimXphyMap->second.end()) {
    throw FbossError(
        "getExternalPhy: Invalid Global Xphy Id ",
        xphyID,
        " is not in xphyMap_ for Pim Id ",
        pimID);
  }
  return xphyIt->second.get();
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

  std::optional<cfg::PlatformPortConfigOverrideFactor> factor;
  if (transceiverInfo) {
    factor = buildPlatformPortConfigOverrideFactor(*transceiverInfo);
  }
  PlatformPortProfileConfigMatcher matcher =
      PlatformPortProfileConfigMatcher(portProfileId, portId, factor);
  const auto& portPinConfig = platformMapping_->getPortXphyPinConfig(matcher);
  const auto& portProfileConfig =
      platformMapping_->getPortProfileConfig(matcher);
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
          platformPortEntry->second, portProfileId, chips, *portProfileConfig));
  phyPortConfig.profile =
      phy::ExternalPhyProfileConfig::fromPortProfileConfig(*portProfileConfig);
  return phyPortConfig;
}

phy::PhyPortConfig PhyManager::getHwPhyPortConfig(PortID portID) {
  return getHwPhyPortConfigLocked(getRLockedCache(portID), portID);
}

void PhyManager::programOnePort(
    PortID portId,
    cfg::PortProfileID portProfileId,
    std::optional<TransceiverInfo> transceiverInfo,
    bool needResetDataPath) {
  const auto& wLockedCache = getWLockedCache(portId);

  // This function will call ExternalPhy::programOnePort(phy::PhyPortConfig).
  auto* xphy = getExternalPhyLocked(wLockedCache);
  const auto& desiredPhyPortConfig =
      getDesiredPhyPortConfig(portId, portProfileId, transceiverInfo);
  xphy->programOnePort(desiredPhyPortConfig, needResetDataPath);

  // Once the port is programmed successfully, update the portToCacheInfo_
  bool isChanged = setPortToPortCacheInfoLocked(
      wLockedCache, portId, portProfileId, desiredPhyPortConfig);
  // Only reset phy port stats when there're changes on the xphy ports
  if (isChanged &&
      (xphy->isSupported(phy::ExternalPhy::Feature::PORT_STATS) ||
       xphy->isSupported(phy::ExternalPhy::Feature::PRBS_STATS))) {
    setPortToExternalPhyPortStats(
        portId, createExternalPhyPortStats(PortID(portId)));
  }
}

folly::EventBase* PhyManager::getPimEventBase(PimID pimID) const {
  if (auto pimEventMultiThread = pimToThread_.find(pimID);
      pimEventMultiThread != pimToThread_.end()) {
    return pimEventMultiThread->second->eventBase.get();
  }
  throw FbossError("Can't find pim EventBase for pim=", pimID);
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

bool PhyManager::setPortToPortCacheInfoLocked(
    const PortCacheWLockedPtr& lockedCache,
    PortID portID,
    cfg::PortProfileID profileID,
    const phy::PhyPortConfig& portConfig) {
  bool isChanged = false;
  if (lockedCache->profile != profileID) {
    lockedCache->profile = profileID;
    isChanged = true;
  }
  if (lockedCache->speed != portConfig.profile.speed) {
    lockedCache->speed = portConfig.profile.speed;
    isChanged = true;
  }

  // Check whether there's a systemLanes and lineLanes cache already
  const auto& systemLanesConfig = portConfig.config.system.lanes;
  const auto& lineLanesConfig = portConfig.config.line.lanes;
  // check whether all the lanes match
  bool matched =
      (lockedCache->systemLanes.size() == systemLanesConfig.size()) &&
      (lockedCache->lineLanes.size() == lineLanesConfig.size());
  // Now check system lane id
  if (matched) {
    for (auto i = 0; i < lockedCache->systemLanes.size(); ++i) {
      if (systemLanesConfig.find(lockedCache->systemLanes[i]) ==
          systemLanesConfig.end()) {
        matched = false;
        break;
      }
    }
  }
  // Now check line lane id
  if (matched) {
    for (auto i = 0; i < lockedCache->lineLanes.size(); ++i) {
      if (lineLanesConfig.find(lockedCache->lineLanes[i]) ==
          lineLanesConfig.end()) {
        matched = false;
        break;
      }
    }
  }
  if (matched) {
    return isChanged;
  }
  // Now reset the cached lane info if there's no match
  lockedCache->systemLanes.clear();
  for (const auto& it : portConfig.config.system.lanes) {
    lockedCache->systemLanes.push_back(it.first);
  }
  lockedCache->lineLanes.clear();
  for (const auto& it : portConfig.config.line.lanes) {
    lockedCache->lineLanes.push_back(it.first);
  }

  if (!lockedCache->speed || lockedCache->systemLanes.empty() ||
      lockedCache->lineLanes.empty()) {
    if (publishPhyCb_) {
      publishPhyCb_(std::string(getPortName(portID)), std::nullopt);
    }
  }

  // There's lane change
  return true;
}

void PhyManager::setPortPrbs(
    PortID portID,
    phy::Side side,
    const phy::PortPrbsState& prbs) {
  const auto& wLockedCache = getWLockedCache(portID);

  auto* xphy = getExternalPhyLocked(wLockedCache);
  if (!xphy->isSupported(phy::ExternalPhy::Feature::PRBS)) {
    throw FbossError("Port:", portID, " xphy can't support PRBS");
  }
  if (wLockedCache->systemLanes.empty() || wLockedCache->lineLanes.empty()) {
    throw FbossError(
        "Port:", portID, " is not programmed and can't find cached lanes");
  }
  const auto& sideLanes = side == phy::Side::SYSTEM ? wLockedCache->systemLanes
                                                    : wLockedCache->lineLanes;
  xphy->setPortPrbs(side, sideLanes, prbs);

  const auto& wLockedStats = getWLockedStats(portID);
  if (*prbs.enabled()) {
    const auto& phyPortConfig = getHwPhyPortConfigLocked(wLockedCache, portID);
    wLockedStats->stats->setupPrbsCollection(
        side, sideLanes, phyPortConfig.getLaneSpeedInMb(side));
  } else {
    wLockedStats->stats->disablePrbsCollection(side);
  }
}

phy::PortPrbsState PhyManager::getPortPrbs(PortID portID, phy::Side side) {
  const auto& rLockedCache = getRLockedCache(portID);

  auto* xphy = getExternalPhyLocked(rLockedCache);
  if (!xphy->isSupported(phy::ExternalPhy::Feature::PRBS)) {
    throw FbossError("Port:", portID, " xphy can't support PRBS");
  }
  if (rLockedCache->systemLanes.empty() || rLockedCache->lineLanes.empty()) {
    throw FbossError(
        "Port:", portID, " is not programmed and can't find cached lanes");
  }
  const auto& sideLanes = side == phy::Side::SYSTEM ? rLockedCache->systemLanes
                                                    : rLockedCache->lineLanes;

  return xphy->getPortPrbs(side, sideLanes);
}

std::vector<phy::PrbsLaneStats> PhyManager::getPortPrbsStats(
    PortID portID,
    phy::Side side) {
  {
    const auto& rLockedCache = getRLockedCache(portID);

    auto* xphy = getExternalPhyLocked(rLockedCache);
    if (!xphy->isSupported(phy::ExternalPhy::Feature::PRBS_STATS)) {
      throw FbossError("Port:", portID, " xphy can't support PRBS_STATS");
    }
  }

  const auto& rLockedStats = getRLockedStats(portID);
  return rLockedStats->stats->getPrbsStats(side);
}

void PhyManager::clearPortPrbsStats(PortID portID, phy::Side side) {
  {
    const auto& rLockedCache = getRLockedCache(portID);

    auto* xphy = getExternalPhyLocked(rLockedCache);
    if (!xphy->isSupported(phy::ExternalPhy::Feature::PRBS_STATS)) {
      throw FbossError("Port:", portID, " xphy can't support PRBS_STATS");
    }
  }

  const auto& wLockedStats = getRLockedStats(portID);
  return wLockedStats->stats->clearPrbsStats(side);
}

folly::dynamic PhyManager::getWarmbootState() const {
  folly::dynamic phyState = folly::dynamic::object;
  folly::dynamic portToCacheInfoCache = folly::dynamic::object;
  // For now, we just need to cache the current system/line lanes
  for (const auto& it : portToCacheInfo_) {
    folly::dynamic portCacheInfo = folly::dynamic::object;
    folly::dynamic systemLanes = folly::dynamic::array;
    const auto& rLockedCache = it.second->rlock();
    for (auto lane : rLockedCache->systemLanes) {
      systemLanes.push_back(static_cast<int>(lane));
    }
    folly::dynamic lineLanes = folly::dynamic::array;
    for (auto lane : rLockedCache->lineLanes) {
      lineLanes.push_back(static_cast<int>(lane));
    }
    portCacheInfo[kSystemLanesKey] = systemLanes;
    portCacheInfo[kLineLanesKey] = lineLanes;
    // Both speed and profile exist
    if (rLockedCache->speed && rLockedCache->profile) {
      portCacheInfo[kPortSpeedStrKey] =
          apache::thrift::util::enumNameSafe(*rLockedCache->speed);
      portCacheInfo[kPortProfileStrKey] =
          apache::thrift::util::enumNameSafe(*rLockedCache->profile);
    }
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
    bool isProgrammed = false;
    PortID portIDStrong = PortID(portID);
    const auto& wLockedCache = it.second->wlock();
    for (auto lane : portCacheInfo[kSystemLanesKey]) {
      wLockedCache->systemLanes.push_back(LaneID(lane.asInt()));
      isProgrammed = true;
    }
    for (auto lane : portCacheInfo[kLineLanesKey]) {
      wLockedCache->lineLanes.push_back(LaneID(lane.asInt()));
    }

    if (isProgrammed) {
      auto* xphy = getExternalPhyLocked(wLockedCache);
      std::optional<phy::PhyPortConfig> hwPortConfig;
      // Restore programmed profile and speed
      if (portCacheInfo.find(kPortProfileStrKey) !=
              portCacheInfo.items().end() &&
          portCacheInfo.find(kPortSpeedStrKey) != portCacheInfo.items().end()) {
        cfg::PortProfileID profile;
        if (!apache::thrift::TEnumTraits<cfg::PortProfileID>::findValue(
                portCacheInfo[kPortProfileStrKey].c_str(), &profile)) {
          throw FbossError(
              "Unrecognized profile:",
              portCacheInfo[kPortProfileStrKey].c_str(),
              " for port:",
              portID);
        }
        wLockedCache->profile = profile;

        cfg::PortSpeed speed;
        if (!apache::thrift::TEnumTraits<cfg::PortSpeed>::findValue(
                portCacheInfo[kPortSpeedStrKey].c_str(), &speed)) {
          throw FbossError(
              "Unrecognized speed:",
              portCacheInfo[kPortSpeedStrKey].c_str(),
              " for port:",
              portID);
        }
        wLockedCache->speed = speed;
      } else {
        // If we can't restore programmed speed + profile from warmboot cache,
        // get from hardware. This should only happen because last version of
        // qsfp_service doesn't cache these two fields in the cache.
        // TODO(joseph5wu) Will remove this after next push
        hwPortConfig = xphy->getConfigOnePort(
            wLockedCache->systemLanes, wLockedCache->lineLanes);
        auto speed = hwPortConfig->profile.speed;
        wLockedCache->speed = speed;
        wLockedCache->profile =
            getProfileIDBySpeed(portIDStrong, speed, platformMapping_);
      }

      // If the port has programmed lane info, we also need to restore the
      // ExternalPhyPortStatsUtils
      if (xphy->isSupported(phy::ExternalPhy::Feature::PORT_STATS) ||
          xphy->isSupported(phy::ExternalPhy::Feature::PRBS_STATS)) {
        setPortToExternalPhyPortStats(
            portIDStrong, createExternalPhyPortStats(portIDStrong));
      }
      // If the prbs is enabled in HW, we also need to setup prbs stats
      if (xphy->isSupported(phy::ExternalPhy::Feature::PRBS_STATS)) {
        const auto& sysPrbsState =
            xphy->getPortPrbs(phy::Side::SYSTEM, wLockedCache->systemLanes);
        const auto& linePrbsState =
            xphy->getPortPrbs(phy::Side::LINE, wLockedCache->lineLanes);
        if (*sysPrbsState.enabled() || *linePrbsState.enabled()) {
          if (!hwPortConfig) {
            hwPortConfig = xphy->getConfigOnePort(
                wLockedCache->systemLanes, wLockedCache->lineLanes);
          }
          hwPortConfig = getHwPhyPortConfigLocked(wLockedCache, portIDStrong);

          const auto& wLockedStats = getWLockedStats(portIDStrong);
          if (*sysPrbsState.enabled()) {
            wLockedStats->stats->setupPrbsCollection(
                phy::Side::SYSTEM,
                wLockedCache->systemLanes,
                hwPortConfig->getLaneSpeedInMb(phy::Side::SYSTEM));
          }
          if (*linePrbsState.enabled()) {
            wLockedStats->stats->setupPrbsCollection(
                phy::Side::LINE,
                wLockedCache->lineLanes,
                hwPortConfig->getLaneSpeedInMb(phy::Side::LINE));
          }
        }
      }
    }

    XLOG(INFO) << "Restore port=" << portID
               << ", systemLanes=" << portCacheInfo[kSystemLanesKey]
               << ", lineLanes=" << portCacheInfo[kLineLanesKey] << ", profile="
               << (wLockedCache->profile ? apache::thrift::util::enumNameSafe(
                                               *wLockedCache->profile)
                                         : "")
               << ", speed="
               << (wLockedCache->speed ? apache::thrift::util::enumNameSafe(
                                             *wLockedCache->speed)
                                       : "")
               << " from the phy warmboot state";
  }
}

void PhyManager::setPortToExternalPhyPortStats(
    PortID portID,
    std::unique_ptr<ExternalPhyPortStatsUtils> stats) {
  const auto& wLockedStats = getWLockedStats(portID);
  wLockedStats->stats = std::move(stats);
}

void PhyManager::updateAllXphyPortsStats() {
  for (const auto& portStatsInfo : portToStatsInfo_) {
    bool supportPortStats = false, supportPrbsStats = false;
    PimID pimID;
    ExternalPhy* xphy;
    {
      const auto& rLockedCache = getRLockedCache(portStatsInfo.first);
      // If the port is not programmed yet, skip updating xphy stats for it
      if (!rLockedCache->speed || rLockedCache->systemLanes.empty() ||
          rLockedCache->lineLanes.empty()) {
        continue;
      }
      xphy = getExternalPhyLocked(rLockedCache);
      supportPortStats =
          (xphy->isSupported(phy::ExternalPhy::Feature::PORT_STATS) ||
           xphy->isSupported(phy::ExternalPhy::Feature::PORT_INFO));
      supportPrbsStats =
          xphy->isSupported(phy::ExternalPhy::Feature::PRBS_STATS);

      auto xphyID = getGlobalXphyIDbyPortIDLocked(rLockedCache);
      pimID = getPhyIDInfo(xphyID).pimID;
    }

    // If xphy doesn't support either of stats collection, skip
    if (!supportPortStats && !supportPrbsStats) {
      continue;
    }

    const auto& wLockedStats = getWLockedStats(portStatsInfo.first);
    auto evb = getPimEventBase(pimID);
    if (supportPortStats) {
      updatePortStats(portStatsInfo.first, xphy, wLockedStats, evb);
    }
    if (supportPrbsStats) {
      updatePrbsStats(portStatsInfo.first, xphy, wLockedStats, evb);
    }
  }
}

using namespace std::chrono;
void PhyManager::updatePortStats(
    PortID portID,
    phy::ExternalPhy* xphy,
    const PhyManager::PortStatsWLockedPtr& wLockedStats,
    folly::EventBase* pimEvb) {
  if (wLockedStats->ongoingStatCollection.has_value() &&
      !wLockedStats->ongoingStatCollection->isReady()) {
    XLOG(DBG4) << "XPHY Port Stat collection for Port:" << portID
               << " still underway...";
    return;
  }

  // Collect xphy port stats
  wLockedStats->ongoingStatCollection =
      folly::via(pimEvb).thenValue([this, portID, xphy](auto&&) {
        // Since this is future job, we need to fetch the cache with lock
        std::vector<LaneID> systemLanes, lineLanes;
        cfg::PortSpeed programmedSpeed;
        {
          const auto& wCache = getWLockedCache(portID);
          if (!wCache->speed || wCache->systemLanes.empty() ||
              wCache->lineLanes.empty()) {
            XLOG(WARN) << "Port:" << portID
                       << " doesn't have programmed speed and lanes";
            return;
          }
          programmedSpeed = *wCache->speed;
          systemLanes = wCache->systemLanes;
          lineLanes = wCache->lineLanes;
        }

        steady_clock::time_point begin = steady_clock::now();
        std::optional<ExternalPhyPortStats> stats;
        // if PORT_INFO feature is supported, use getPortInfo instead
        if (xphy->isSupported(phy::ExternalPhy::Feature::PORT_INFO)) {
          PhyInfo lastPhyInfo;
          if (auto lastXphyInfo = getXphyInfo(portID)) {
            lastPhyInfo = *lastXphyInfo;
          }
          auto xphyPortInfo =
              xphy->getPortInfo(systemLanes, lineLanes, lastPhyInfo);
          xphyPortInfo.state()->name() = getPortName(portID);
          xphyPortInfo.state()->speed() = programmedSpeed;
          stats = ExternalPhyPortStats::fromPhyInfo(xphyPortInfo);
          updateXphyInfo(portID, std::move(xphyPortInfo));
        } else {
          stats = xphy->getPortStats(systemLanes, lineLanes);
        }

        const auto& wLockedStats = getWLockedStats(portID);
        wLockedStats->stats->updateXphyStats(*stats);
        XLOG(DBG3)
            << "Port " << portID << ": xphy port stat collection took "
            << duration_cast<milliseconds>(steady_clock::now() - begin).count()
            << "ms";
      });
}

void PhyManager::updatePrbsStats(
    PortID portID,
    phy::ExternalPhy* xphy,
    const PhyManager::PortStatsWLockedPtr& wLockedStats,
    folly::EventBase* pimEvb) {
  // Only needs to update prbs stats as long as there's one side enabled
  if (!wLockedStats->stats->isPrbsCollectionEnabled(phy::Side::SYSTEM) &&
      !wLockedStats->stats->isPrbsCollectionEnabled(phy::Side::LINE)) {
    return;
  }

  if (wLockedStats->ongoingPrbsStatCollection.has_value() &&
      !wLockedStats->ongoingPrbsStatCollection->isReady()) {
    XLOG(DBG4) << "XPHY PRBS Stat collection for Port:" << portID
               << " still underway...";
    return;
  }

  // Collect xphy prbs stats
  wLockedStats->ongoingPrbsStatCollection =
      folly::via(pimEvb).thenValue([this, portID, xphy](auto&&) {
        // Since this is future job, we need to fetch the cache with lock
        std::vector<LaneID> systemLanes, lineLanes;
        {
          const auto& wCache = getWLockedCache(portID);
          if (wCache->systemLanes.empty() || wCache->lineLanes.empty()) {
            XLOG(WARN) << "Port:" << portID << " doesn't have programmed lanes";
            return;
          }
          systemLanes = wCache->systemLanes;
          lineLanes = wCache->lineLanes;
        }

        steady_clock::time_point begin = steady_clock::now();
        const auto& stats = xphy->getPortPrbsStats(systemLanes, lineLanes);

        const auto& wLockedStats = getWLockedStats(portID);
        wLockedStats->stats->updateXphyPrbsStats(stats);
        XLOG(DBG3)
            << "Port " << portID << ": xphy prbs stat collection took "
            << duration_cast<milliseconds>(steady_clock::now() - begin).count()
            << "ms";
      });
}

const std::string& PhyManager::getPortName(PortID portID) const {
  const auto& portEntry = platformMapping_->getPlatformPorts().find(portID);
  if (portEntry == platformMapping_->getPlatformPorts().end()) {
    throw FbossError(
        "Unrecoginized port=", portID, ", which is not in PlatformMapping");
  }
  return *portEntry->second.mapping()->name();
}

PhyManager::PortCacheRLockedPtr PhyManager::getRLockedCache(
    PortID portID) const {
  const auto& cache = portToCacheInfo_.find(portID);
  if (cache == portToCacheInfo_.end()) {
    throw FbossError(
        "Unrecoginized port=", portID, ", which is not in PlatformMapping");
  }
  return cache->second->rlock();
}
PhyManager::PortCacheWLockedPtr PhyManager::getWLockedCache(
    PortID portID) const {
  const auto& cache = portToCacheInfo_.find(portID);
  if (cache == portToCacheInfo_.end()) {
    throw FbossError(
        "Unrecoginized port=", portID, ", which is not in PlatformMapping");
  }
  return cache->second->wlock();
}

bool PhyManager::isXphyStatsCollectionDone(PortID portID) const {
  const auto& rLockedStats = getRLockedStats(portID);
  return rLockedStats->ongoingStatCollection.has_value() &&
      rLockedStats->ongoingStatCollection->isReady();
}

bool PhyManager::isPrbsStatsCollectionDone(PortID portID) const {
  const auto& rLockedStats = getRLockedStats(portID);
  return rLockedStats->ongoingPrbsStatCollection.has_value() &&
      rLockedStats->ongoingPrbsStatCollection->isReady();
}

std::vector<PortID> PhyManager::getXphyPorts() const {
  std::vector<PortID> xphyPorts;
  std::for_each(
      portToCacheInfo_.begin(),
      portToCacheInfo_.end(),
      [&xphyPorts](auto& portAndInfo) {
        xphyPorts.push_back(portAndInfo.first);
      });
  return xphyPorts;
}

std::vector<PortID> PhyManager::getPortsSupportingFeature(
    phy::ExternalPhy::Feature feature) const {
  std::vector<PortID> ports;
  auto xphysSupportingFeature = getXphysSupportingFeature(feature);
  std::for_each(
      portToCacheInfo_.begin(),
      portToCacheInfo_.end(),
      [&ports, this, &xphysSupportingFeature](auto& portAndInfo) {
        auto portXphy = getRLockedCache(portAndInfo.first)->xphyID;
        if (xphysSupportingFeature.find(portXphy) !=
            xphysSupportingFeature.end()) {
          ports.push_back(portAndInfo.first);
        }
      });
  return ports;
}

std::set<GlobalXphyID> PhyManager::getXphysSupportingFeature(
    phy::ExternalPhy::Feature feature) const {
  std::set<GlobalXphyID> phys;
  for (const auto& pimAndXphy : xphyMap_) {
    for (const auto& idAndXphy : pimAndXphy.second) {
      if (idAndXphy.second->isSupported(feature)) {
        phys.insert(idAndXphy.first);
      }
    }
  }
  return phys;
}

void PhyManager::publishXphyInfoSnapshots(PortID port) const {
  xphySnapshotManager_->publishSnapshots(port);
}

void PhyManager::updateXphyInfo(PortID portID, phy::PhyInfo&& phyInfo) {
  xphySnapshotManager_->updatePhyInfo(portID, phyInfo);
  if (publishPhyCb_) {
    publishPhyCb_(std::string(getPortName(portID)), std::move(phyInfo));
  }
}

std::optional<phy::PhyInfo> PhyManager::getXphyInfo(PortID port) const {
  return xphySnapshotManager_->getPhyInfo(port);
}

// Return programmed profile id for a specific port
std::optional<cfg::PortProfileID> PhyManager::getProgrammedProfile(
    PortID portID) {
  if (auto portCacheInfoIt = portToCacheInfo_.find(portID);
      portCacheInfoIt != portToCacheInfo_.end()) {
    const auto& rLockedCache = portCacheInfoIt->second->rlock();
    return rLockedCache->profile;
  }
  throw FbossError("Can't find port:", portID, " in portToCacheInfo_");
}
// Return programmed port speed for a specific port
std::optional<cfg::PortSpeed> PhyManager::getProgrammedSpeed(PortID portID) {
  if (auto portCacheInfoIt = portToCacheInfo_.find(portID);
      portCacheInfoIt != portToCacheInfo_.end()) {
    const auto& rLockedCache = portCacheInfoIt->second->rlock();
    return rLockedCache->speed;
  }
  throw FbossError("Can't find port:", portID, " in portToCacheInfo_");
}

bool PhyManager::shouldInitializePimXphy(PimID pim) const {
  return xphyMap_.find(pim) != xphyMap_.end();
}

PhyManager::PortStatsRLockedPtr PhyManager::getRLockedStats(
    PortID portID) const {
  const auto& statsInfo = portToStatsInfo_.find(portID);
  if (statsInfo == portToStatsInfo_.end()) {
    throw FbossError(
        "Unrecoginized port=", portID, ", which is not in PlatformMapping");
  }
  return statsInfo->second->rlock();
}

PhyManager::PortStatsWLockedPtr PhyManager::getWLockedStats(
    PortID portID) const {
  const auto& statsInfo = portToStatsInfo_.find(portID);
  if (statsInfo == portToStatsInfo_.end()) {
    throw FbossError(
        "Unrecoginized port=", portID, ", which is not in PlatformMapping");
  }
  return statsInfo->second->wlock();
}
} // namespace facebook::fboss
