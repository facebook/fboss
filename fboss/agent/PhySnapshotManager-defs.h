// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#pragma once

#include "fboss/agent/PhySnapshotManager.h"
#include "fboss/agent/state/Port.h"

namespace facebook::fboss {

template <size_t intervalSeconds>
void PhySnapshotManager<intervalSeconds>::updatePhyInfoLocked(
    const SnapshotMapWLockedPtr& lockedSnapshotMap,
    PortID portID,
    const phy::PhyInfo& phyInfo) {
  phy::LinkSnapshot snapshot;
  snapshot.phyInfo_ref() = phyInfo;
  CHECK(phyInfo.state().has_value());

  CHECK(!phyInfo.state()->get_name().empty());
  auto result = lockedSnapshotMap->try_emplace(
      portID, std::set<std::string>({phyInfo.state()->get_name()}));
  auto iter = result.first;
  auto& value = iter->second;
  value.addSnapshot(snapshot);
}

template <size_t intervalSeconds>
void PhySnapshotManager<intervalSeconds>::updatePhyInfo(
    PortID portID,
    const phy::PhyInfo& phyInfo) {
  auto lockedSnapshotMap = snapshots_.wlock();
  updatePhyInfoLocked(lockedSnapshotMap, portID, phyInfo);
}

template <size_t intervalSeconds>
void PhySnapshotManager<intervalSeconds>::updatePhyInfos(
    const std::map<PortID, phy::PhyInfo>& phyInfos) {
  auto lockedSnapshotMap = snapshots_.wlock();
  for (const auto& [portID, phyInfo] : phyInfos) {
    updatePhyInfoLocked(lockedSnapshotMap, portID, phyInfo);
  }
}

template <size_t intervalSeconds>
std::optional<phy::PhyInfo>
PhySnapshotManager<intervalSeconds>::getPhyInfoLocked(
    const SnapshotMapRLockedPtr& lockedSnapshotMap,
    PortID portID) const {
  std::optional<phy::PhyInfo> phyInfo;

  if (auto it = lockedSnapshotMap->find(portID);
      it != lockedSnapshotMap->end()) {
    auto& snapshots = it->second.getSnapshots();
    if (!snapshots.empty()) {
      phyInfo = snapshots.last().snapshot_.get_phyInfo();
    }
  }

  return phyInfo;
}

template <size_t intervalSeconds>
std::optional<phy::PhyInfo> PhySnapshotManager<intervalSeconds>::getPhyInfo(
    PortID portID) const {
  const auto& lockedSnapshotMap = snapshots_.rlock();
  return getPhyInfoLocked(lockedSnapshotMap, portID);
}

template <size_t intervalSeconds>
std::map<PortID, const phy::PhyInfo>
PhySnapshotManager<intervalSeconds>::getPhyInfos(
    const std::vector<PortID>& portIDs) const {
  std::map<PortID, const phy::PhyInfo> infoMap;
  auto lockedSnapshotMap = snapshots_.rlock();
  for (auto portID : portIDs) {
    auto snapshot = getPhyInfoLocked(lockedSnapshotMap, portID);
    if (snapshot) {
      infoMap.emplace(portID, *snapshot);
    }
  }
  return infoMap;
}

template <size_t intervalSeconds>
void PhySnapshotManager<intervalSeconds>::publishSnapshots(PortID port) {
  auto lockedSnapshotMap = snapshots_.wlock();
  if (auto it = lockedSnapshotMap->find(port); it != lockedSnapshotMap->end()) {
    it->second.publishAllSnapshots();
    it->second.publishFutureSnapshots();
  }
}

} // namespace facebook::fboss
