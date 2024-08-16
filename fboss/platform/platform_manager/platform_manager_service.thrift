// Copyright (c) Meta Platforms, Inc. and affiliates.

namespace cpp2 facebook.fboss.platform.platform_manager

include "fboss/platform/platform_manager/platform_manager_snapshot.thrift"

enum ExplorationStatus {
  EXPLORATION_SUCCEEDED = 0,
  EXPLORATION_IN_PROGRESS = 1,
  EXPLORATION_FAILED = 2,
}

// `PlatformManagerStatus` contains the last PM exploration status.
//
// `explorationStatus`: Indicates the PM's latest exploration status.
//
// `lastExplorationTime`: The completion time of the last exploration (sec since epoch).
struct PlatformManagerStatus {
  1: ExplorationStatus explorationStatus;
  2: i64 lastExplorationTime;
}

service PlatformManagerService {
  platform_manager_snapshot.PlatformSnapshot getPlatformSnapshot();
  PlatformManagerStatus getLastPMStatus();
}
