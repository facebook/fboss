// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#pragma once

#include "fboss/agent/state/Port.h"
#include "fboss/lib/SnapshotManager-defs.h"

namespace facebook::fboss {

class PhySnapshotManager {
  static constexpr auto kNumCachedSnapshots = 20;
  using IPhySnapshotCache = SnapshotManager<kNumCachedSnapshots>;

 public:
  void updateIPhyInfo(std::map<PortID, phy::PhyInfo> phyInfo);

  // Map of portID to last few Internal phy diagnostic snapshots
  std::map<PortID, folly::Synchronized<IPhySnapshotCache>> snapshots_;
};

} // namespace facebook::fboss
