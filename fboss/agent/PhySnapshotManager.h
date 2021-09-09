// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#pragma once

#include "fboss/agent/state/Port.h"
#include "fboss/lib/link_snapshots/SnapshotManager-defs.h"
#include "fboss/lib/phy/gen-cpp2/phy_types.h"

namespace facebook::fboss {

class PhySnapshotManager {
  static constexpr auto kNumCachedSnapshots = 20;
  using IPhySnapshotCache = SnapshotManager<kNumCachedSnapshots>;

 public:
  void updateIPhyInfo(const std::map<PortID, phy::PhyInfo>& phyInfo);
  std::map<PortID, const phy::PhyInfo> getIPhyInfo(
      const std::vector<PortID>& portIDs);

 private:
  // Map of portID to last few Internal phy diagnostic snapshots
  folly::Synchronized<std::map<PortID, IPhySnapshotCache>> snapshots_;
};

} // namespace facebook::fboss
