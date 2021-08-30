// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include "fboss/agent/PhySnapshotManager.h"
#include "fboss/agent/state/Port.h"

namespace facebook::fboss {

void PhySnapshotManager::updateIPhyInfo(
    std::map<PortID, phy::PhyInfo> phyInfo) {
  for (auto info : phyInfo) {
    phy::LinkSnapshot snapshot;
    snapshot.phyInfo_ref() = info.second;
    snapshots_[info.first].wlock()->addSnapshot(snapshot);
  }
}
} // namespace facebook::fboss
