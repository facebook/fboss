// Copyright (c) Meta Platforms, Inc. and affiliates.

namespace cpp2 facebook.fboss.platform.platform_manager
namespace hack NetengFbossPlatformManager
namespace py3 fboss.platform.platform_manager

include "thrift/annotation/thrift.thrift"
include "thrift/annotation/cpp.thrift"
include "thrift/annotation/hack.thrift"

include "fboss/platform/platform_manager/platform_manager_snapshot.thrift"
include "fboss/platform/platform_manager/platform_manager_config.thrift"
include "fboss/platform/weutil/if/eeprom_contents.thrift"

@thrift.AllowLegacyMissingUris
package;

@hack.Attributes{
  attributes = [
    "\Oncalls('net_ui')",
    "\JSEnum(shape('flow_enum' => false))",
    "\GraphQLEnum('NetengFbossPlatformManagerExplorationStatus')",
    "\SelfDescriptive",
    "\RelayFlowEnum",
  ],
}
enum ExplorationStatus {
  SUCCEEDED = 0,
  IN_PROGRESS = 1,
  FAILED = 2,
  SUCCEEDED_WITH_EXPECTED_ERRORS = 3,
  UNSTARTED = 4,
}

@hack.Attributes{
  attributes = [
    "\Oncalls('net_ui')",
    "\JSEnum(shape('flow_enum' => false))",
    "\GraphQLEnum('NetengFbossPlatformManagerErrorCode')",
    "\SelfDescriptive",
    "\RelayFlowEnum",
  ],
}
enum PlatformManagerErrorCode {
  NONE = 0,
  EXPLORATION_NOT_SUCCEEDED = 1,
  PM_UNIT_NOT_FOUND = 2,
  EEPROM_CONTENTS_NOT_FOUND = 3,
}

exception PlatformManagerError {
  1: PlatformManagerErrorCode errorCode;
  @thrift.ExceptionMessage
  2: string message;
}

struct ExplorationError {
  1: string errorType;
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
  3: optional map<string, list<ExplorationError>> failedDevices;
}

struct PmUnitInfoResponse {
  1: platform_manager_config.PmUnitInfo pmUnitInfo;
}

struct PmUnitsResponse {
  1: map<string/* slotPath */ , platform_manager_config.PmUnitInfo> pmUnits;
}

struct PmUnitInfoRequest {
  1: string slotPath;
}

struct BspVersionResponse {
  1: string bspBaseName;
  2: string bspVersion;
  3: string kernelVersion;
}

struct EepromContentResponse {
  1: eeprom_contents.EepromContents eepromContents;
}

service PlatformManagerService {
  platform_manager_snapshot.PlatformSnapshot getPlatformSnapshot();
  PlatformManagerStatus getLastPMStatus();
  PmUnitInfoResponse getPmUnitInfo(1: PmUnitInfoRequest req) throws (
    1: PlatformManagerError pmError,
  );
  PmUnitsResponse getAllPmUnits();
  BspVersionResponse getBspVersion();
  string getPlatformName();
  EepromContentResponse getEepromContents(1: PmUnitInfoRequest req) throws (
    1: PlatformManagerError pmError,
  );
}
