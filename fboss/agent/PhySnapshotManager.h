// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#pragma once

#include "fboss/agent/state/Port.h"
#include "fboss/lib/link_snapshots/SnapshotManager-defs.h"
#include "fboss/lib/phy/gen-cpp2/phy_types.h"

namespace facebook::fboss {

// intervalSeconds is the interval between Phy stat collections
template <size_t intervalSeconds>
class PhySnapshotManager {
  using PhySnapshotCache = SnapshotManager<intervalSeconds>;
  using SnapshotMapRLockedPtr = typename folly::Synchronized<
      std::map<PortID, PhySnapshotCache>>::ConstRLockedPtr;
  using SnapshotMapWLockedPtr = typename folly::Synchronized<
      std::map<PortID, PhySnapshotCache>>::WLockedPtr;

 public:
  void updatePhyInfo(PortID portID, const phy::PhyInfo& phyInfo);
  void updatePhyInfos(const std::map<PortID, phy::PhyInfo>& phyInfo);
  std::optional<phy::PhyInfo> getPhyInfo(PortID portID) const;
  std::map<PortID, const phy::PhyInfo> getPhyInfos(
      const std::vector<PortID>& portIDs) const;
  void publishSnapshots(PortID portID);

 private:
  void updatePhyInfoLocked(
      const SnapshotMapWLockedPtr& lockedSnapshotMap,
      PortID portID,
      const phy::PhyInfo& phyInfo);
  std::optional<phy::PhyInfo> getPhyInfoLocked(
      const SnapshotMapRLockedPtr& lockedSnapshotMap,
      PortID portID) const;

  // Map of portID to last few phy diagnostic snapshots
  folly::Synchronized<std::map<PortID, PhySnapshotCache>> snapshots_;
};

} // namespace facebook::fboss
