// Copyright (c) Meta Platforms, Inc. and affiliates.

namespace cpp2 facebook.fboss.platform.platform_manager

include "thrift/annotation/thrift.thrift"

include "fboss/platform/platform_manager/platform_manager_snapshot.thrift"
include "fboss/platform/platform_manager/platform_manager_config.thrift"

enum ExplorationStatus {
  SUCCEEDED = 0,
  IN_PROGRESS = 1,
  FAILED = 2,
  SUCCEEDED_WITH_EXPECTED_ERRORS = 3,
  UNSTARTED = 4,
}

enum PlatformManagerErrorCode {
  NONE = 0,
  EXPLORATION_NOT_SUCCEEDED = 1,
  PM_UNIT_NOT_FOUND = 2,
}

exception PlatformManagerError {
  1: PlatformManagerErrorCode errorCode;
  @thrift.ExceptionMessage
  2: string message;
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

struct PmUnitInfoResponse {
  1: platform_manager_config.PmUnitInfo pmUnitInfo;
}

struct PmUnitInfoRequest {
  1: string slotPath;
}

service PlatformManagerService {
  platform_manager_snapshot.PlatformSnapshot getPlatformSnapshot();
  PlatformManagerStatus getLastPMStatus();
  PmUnitInfoResponse getPmUnitInfo(1: PmUnitInfoRequest req) throws (
    1: PlatformManagerError pmError,
  );
}
