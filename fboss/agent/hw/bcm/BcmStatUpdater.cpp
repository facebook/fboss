/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/bcm/BcmStatUpdater.h"

#include "fboss/agent/hw/bcm/BcmAclStat.h"
#include "fboss/agent/hw/bcm/BcmAddressFBConvertors.h"
#include "fboss/agent/hw/bcm/BcmError.h"
#include "fboss/agent/hw/bcm/BcmFieldProcessorFBConvertors.h"
#include "fboss/agent/hw/bcm/BcmSwitch.h"

#include "fboss/agent/hw/HwResourceStatsPublisher.h"
#include "fboss/agent/hw/bcm/BcmPortUtils.h"
#include "fboss/agent/state/Port.h"

#include "fboss/lib/config/PlatformConfigUtils.h"

#include <boost/container/flat_map.hpp>

#include <thrift/lib/cpp/util/EnumUtils.h>

extern "C" {
#include <bcm/field.h>
}

namespace {

struct LaneRateMapKey {
  LaneRateMapKey(uint32 speed, uint32 numLanes, bcm_port_phy_fec_t fecType)
      : speed_(speed), numLanes_(numLanes), fecType_(fecType) {}

  bool operator<(const LaneRateMapKey& other) const {
    return speed_ != other.speed_
        ? (speed_ < other.speed_)
        : (numLanes_ != other.numLanes_ ? (numLanes_ < other.numLanes_)
                                        : (fecType_ < other.fecType_));
  }

  uint32 speed_; /* port speed in Mbps */
  uint32 numLanes_; /* number of lanes */
  bcm_port_phy_fec_t fecType_; /* FEC type */
};
using LaneRateMap = std::map<LaneRateMapKey, double>;

static const LaneRateMap kLaneRateMap = {
    {{10000, 1, bcmPortPhyFecNone}, 10.3125},
    {{10000, 1, bcmPortPhyFecBaseR}, 10.3125},
    {{20000, 1, bcmPortPhyFecNone}, 10.3125},
    {{20000, 1, bcmPortPhyFecBaseR}, 10.3125},
    {{40000, 4, bcmPortPhyFecNone}, 10.3125},
    {{40000, 4, bcmPortPhyFecBaseR}, 10.3125},
    {{40000, 2, bcmPortPhyFecNone}, 10.3125},
    {{25000, 1, bcmPortPhyFecNone}, 25.78125},
    {{25000, 1, bcmPortPhyFecBaseR}, 25.78125},
    {{25000, 1, bcmPortPhyFecRsFec}, 25.7812},
    {{50000, 1, bcmPortPhyFecNone}, 51.5625},
    {{50000, 1, bcmPortPhyFecRsFec}, 51.5625},
    {{50000, 1, bcmPortPhyFecRs544}, 53.125},
    {{50000, 1, bcmPortPhyFecRs272}, 53.125},
    {{50000, 2, bcmPortPhyFecNone}, 25.78125},
    {{50000, 2, bcmPortPhyFecRsFec}, 25.78125},
    {{50000, 2, bcmPortPhyFecRs544}, 26.5625},
    {{100000, 2, bcmPortPhyFecNone}, 51.5625},
    {{100000, 2, bcmPortPhyFecRsFec}, 51.5625},
    {{100000, 2, bcmPortPhyFecRs544}, 53.125},
    {{100000, 2, bcmPortPhyFecRs272}, 53.125},
    {{100000, 4, bcmPortPhyFecNone}, 25.78125},
    {{100000, 4, bcmPortPhyFecRsFec}, 25.78125},
    {{100000, 4, bcmPortPhyFecRs544}, 26.5625},
    {{200000, 4, bcmPortPhyFecNone}, 51.5625},
    {{200000, 4, bcmPortPhyFecRs272}, 53.125},
    {{200000, 4, bcmPortPhyFecRs544}, 53.125},
    {{200000, 4, bcmPortPhyFecRs544_2xN}, 53.125},
    {{400000, 8, bcmPortPhyFecRs544_2xN}, 53.125}};
} // namespace

namespace facebook::fboss {

using std::chrono::duration_cast;
using std::chrono::seconds;
using std::chrono::system_clock;

using facebook::fboss::bcmCheckError;

BcmStatUpdater::BcmStatUpdater(BcmSwitch* hw)
    : hw_(hw),
      bcmTableStatsManager_(std::make_unique<BcmHwTableStatManager>(hw)) {}

std::string BcmStatUpdater::counterTypeToString(cfg::CounterType type) {
  switch (type) {
    case cfg::CounterType::PACKETS:
      return "packets";
    case cfg::CounterType::BYTES:
      return "bytes";
  }
  throw FbossError("Unsupported Counter Type");
}

void BcmStatUpdater::toBeAddedAclStat(
    BcmAclStatHandle handle,
    const std::string& name,
    const std::vector<cfg::CounterType>& counterTypes) {
  for (auto type : counterTypes) {
    std::string counterName = name + "." + counterTypeToString(type);
    toBeAddedAclStats_.emplace(counterName, AclCounterDescriptor(handle, type));
  }
}

void BcmStatUpdater::toBeRemovedAclStat(BcmAclStatHandle handle) {
  toBeRemovedAclStats_.emplace(handle);
}

void BcmStatUpdater::refreshPostBcmStateChange(const StateDelta& delta) {
  refreshHwTableStats(delta);
  refreshAclStats();
  refreshPrbsStats(delta);
}

void BcmStatUpdater::updateStats() {
  updateAclStats();
  updateHwTableStats();
  updatePrbsStats();
}

void BcmStatUpdater::updateAclStats() {
  auto now = duration_cast<seconds>(system_clock::now().time_since_epoch());
  auto lockedAclStats = aclStats_.wlock();
  for (auto& entry : *lockedAclStats) {
    updateAclStat(
        hw_->getUnit(),
        entry.first.handle,
        entry.first.counterType,
        now,
        entry.second.get());
  }
}
void BcmStatUpdater::updateHwTableStats() {
  HwResourceStatsPublisher().publish(*resourceStats_.rlock());
}

void BcmStatUpdater::updatePrbsStats() {
  uint32 status;
  auto lockedAsicPrbsStats = portAsicPrbsStats_.wlock();
  for (auto& entry : *lockedAsicPrbsStats) {
    auto& lanePrbsStatsTable = entry.second;

    for (auto& lanePrbsStatsEntry : lanePrbsStatsTable) {
      bcm_gport_t gport = lanePrbsStatsEntry.getGportId();
      bcm_port_phy_control_get(
          hw_->getUnit(), gport, BCM_PORT_PHY_CONTROL_PRBS_RX_STATUS, &status);
      if ((int32_t)status == -1) {
        lanePrbsStatsEntry.lossOfLock();
      } else if ((int32_t)status == -2) {
        lanePrbsStatsEntry.locked();
      } else {
        lanePrbsStatsEntry.updateLaneStats(status);
      }
    }
  }
}

double BcmStatUpdater::calculateLaneRate(std::shared_ptr<Port> swPort) {
  auto profileID = swPort->getProfileID();
  auto platformPortEntry = hw_->getPlatform()
                               ->getPlatformPort(swPort->getID())
                               ->getPlatformPortEntry();
  if (!platformPortEntry.has_value()) {
    throw FbossError("No PlatformPortEntry found for ", swPort->getName());
  }

  auto platformPortConfig =
      platformPortEntry.value().supportedProfiles_ref()->find(profileID);
  if (platformPortConfig ==
      platformPortEntry.value().supportedProfiles_ref()->end()) {
    throw FbossError(
        "No speed profile with id ",
        apache::thrift::util::enumNameSafe(profileID),
        " found in PlatformPortEntry for ",
        swPort->getName());
  }

  const auto portProfileConfig =
      hw_->getPlatform()->getPortProfileConfig(profileID);
  if (!portProfileConfig) {
    throw FbossError(
        "Platform doesn't support speed profile: ",
        apache::thrift::util::enumNameSafe(profileID));
  }

  auto portSpeed = static_cast<int>((*portProfileConfig).get_speed());
  auto fecType = utility::phyFecModeToBcmPortPhyFec(
      (*portProfileConfig).get_iphy().get_fec());
  auto numLanes = platformPortConfig->second.pins_ref()->iphy_ref()->size();

  double laneRateGb;
  auto laneRateGbIter =
      kLaneRateMap.find(LaneRateMapKey(portSpeed, numLanes, fecType));
  if (laneRateGbIter != kLaneRateMap.end()) {
    laneRateGb = laneRateGbIter->second;
  } else {
    laneRateGb = portSpeed / 1000 / numLanes;
  }

  double laneRate = laneRateGb * 1024. * 1024. * 1024.;
  return laneRate;
}

size_t BcmStatUpdater::getCounterCount() const {
  return aclStats_.rlock()->size();
}

MonotonicCounter* FOLLY_NULLABLE
BcmStatUpdater::getCounterIf(BcmAclStatHandle handle, cfg::CounterType type) {
  auto lockedAclStats = aclStats_.rlock();
  auto iter = lockedAclStats->find(AclCounterDescriptor(handle, type));
  return iter != lockedAclStats->end() ? iter->second.get() : nullptr;
}

void BcmStatUpdater::clearPortStats(
    const std::unique_ptr<std::vector<int32_t>>& ports) {
  // XXX clear per queue stats and, BST stats as well.
  for (auto port : *ports) {
    auto ret = bcm_stat_clear(hw_->getUnit(), port);
    if (BCM_FAILURE(ret)) {
      XLOG(ERR) << "Clear Failed for port " << port << " :" << bcm_errmsg(ret);
      return;
    }
  }
}

std::vector<PrbsLaneStats> BcmStatUpdater::getPortAsicPrbsStats(
    int32_t portId) {
  std::vector<PrbsLaneStats> prbsStats;
  auto lockedPortAsicPrbsStats = portAsicPrbsStats_.rlock();
  auto portAsicPrbsStatIter = lockedPortAsicPrbsStats->find(portId);
  if (portAsicPrbsStatIter == lockedPortAsicPrbsStats->end()) {
    throw FbossError(
        "Asic prbs lane error map not initialized for port ", portId);
  }
  auto lanePrbsStatsTable = portAsicPrbsStatIter->second;
  XLOG(DBG3) << "lanePrbsStatsMap size: " << lanePrbsStatsTable.size();

  for (const auto& lanePrbsStats : lanePrbsStatsTable) {
    prbsStats.push_back(lanePrbsStats.getPrbsLaneStats());
  }
  return prbsStats;
}

void BcmStatUpdater::clearPortAsicPrbsStats(int32_t portId) {
  auto lockedPortAsicPrbsStats = portAsicPrbsStats_.wlock();
  auto portAsicPrbsStatIter = lockedPortAsicPrbsStats->find(portId);
  if (portAsicPrbsStatIter == lockedPortAsicPrbsStats->end()) {
    XLOG(ERR) << "Asic prbs lane error map not initialized for port " << portId;
    return;
  }
  auto& lanePrbsStatsTable = portAsicPrbsStatIter->second;
  for (auto& lanePrbsStats : lanePrbsStatsTable) {
    lanePrbsStats.clearLaneStats();
  }
}

void BcmStatUpdater::refreshHwTableStats(const StateDelta& delta) {
  auto stats = resourceStats_.wlock();
  bcmTableStatsManager_->refresh(delta, &(*stats));
}

void BcmStatUpdater::refreshAclStats() {
  if (toBeRemovedAclStats_.empty() && toBeAddedAclStats_.empty()) {
    return;
  }

  auto lockedAclStats = aclStats_.wlock();

  while (!toBeRemovedAclStats_.empty()) {
    BcmAclStatHandle handle = toBeRemovedAclStats_.front();
    auto itr = lockedAclStats->begin();
    while (itr != lockedAclStats->end()) {
      if (itr->first.handle == handle) {
        lockedAclStats->erase(itr++);
      } else {
        ++itr;
      }
    }
    toBeRemovedAclStats_.pop();
  }

  while (!toBeAddedAclStats_.empty()) {
    const std::string& name = toBeAddedAclStats_.front().first;
    auto aclCounterDescriptor = toBeAddedAclStats_.front().second;
    auto inserted = lockedAclStats->emplace(
        aclCounterDescriptor,
        std::make_unique<MonotonicCounter>(name, fb303::SUM, fb303::RATE));
    if (!inserted.second) {
      throw FbossError(
          "Duplicate ACL stat handle, handle=",
          aclCounterDescriptor.handle,
          ", name=",
          name);
    }
    toBeAddedAclStats_.pop();
  }
}

void BcmStatUpdater::refreshPrbsStats(const StateDelta& delta) {
  // Add or remove ports into PrbsStats.
  DeltaFunctions::forEachChanged(
      delta.getPortsDelta(),
      [&](const std::shared_ptr<Port>& oldPort,
          const std::shared_ptr<Port>& newPort) {
        if (oldPort->getAsicPrbs() == newPort->getAsicPrbs()) {
          // nothing changed
          return;
        }

        auto lockedPortAsicPrbsStats = portAsicPrbsStats_.wlock();
        if (!(*newPort->getAsicPrbs().enabled_ref())) {
          lockedPortAsicPrbsStats->erase(oldPort->getID());
          return;
        }

        // Find how many lanes does the port associate with.
        auto profileID = newPort->getProfileID();
        if (profileID == cfg::PortProfileID::PROFILE_DEFAULT) {
          XLOG(WARNING)
              << newPort->getName()
              << " has default profile, skip refreshPrbsStats for now";
          return;
        }

        const auto& platformPortEntry = hw_->getPlatform()
                                            ->getPlatformPort(newPort->getID())
                                            ->getPlatformPortEntry();
        if (!platformPortEntry.has_value()) {
          throw FbossError(
              "No PlatformPortEntry found for ", newPort->getName());
        }

        const auto& platformPortConfig =
            platformPortEntry.value().supportedProfiles_ref()->find(profileID);
        if (platformPortConfig ==
            platformPortEntry.value().supportedProfiles_ref()->end()) {
          throw FbossError(
              "No speed profile with id ",
              apache::thrift::util::enumNameSafe(profileID),
              " found in PlatformPortEntry for ",
              newPort->getName());
        }

        const auto& portProfileConfig =
            hw_->getPlatform()->getPortProfileConfig(profileID);
        if (!portProfileConfig.has_value()) {
          throw FbossError(
              "No port profile with id ",
              apache::thrift::util::enumNameSafe(profileID),
              " found in PlatformConfig for ",
              newPort->getName());
        }

        auto lanePrbsStatsTable = LanePrbsStatsTable();
        for (int lane = 0;
             lane < platformPortConfig->second.pins_ref()->iphy_ref()->size();
             lane++) {
          bcm_gport_t gport;
          BCM_PHY_GPORT_LANE_PORT_SET(gport, lane, newPort->getID());
          lanePrbsStatsTable.push_back(
              LanePrbsStatsEntry(lane, gport, calculateLaneRate(newPort)));
        }
        (*lockedPortAsicPrbsStats)[newPort->getID()] =
            std::move(lanePrbsStatsTable);
      });
}

void BcmStatUpdater::updateAclStat(
    int unit,
    BcmAclStatHandle handle,
    cfg::CounterType counterType,
    std::chrono::seconds now,
    MonotonicCounter* counter) {
  uint64_t value;
  bcm_field_stat_t type = utility::cfgCounterTypeToBcmCounterType(counterType);
  auto rv = bcm_field_stat_get(unit, handle, type, &value);
  bcmCheckError(rv, "Failed to update stat=", handle);
  counter->updateValue(now, value);
}

} // namespace facebook::fboss
