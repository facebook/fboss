// Copyright (c) 2004-present, Meta Platforms, Inc. and affiliates.
// All Rights Reserved.

#include "fboss/platform/platform_manager/PlatformManagerHandler.h"

namespace facebook::fboss::platform::platform_manager {
PlatformManagerHandler::PlatformManagerHandler(
    const PlatformExplorer& platformExplorer)
    : platformExplorer_(platformExplorer) {}

void PlatformManagerHandler::getPlatformSnapshot(PlatformSnapshot&) {}

void PlatformManagerHandler::getLastPMStatus(PlatformManagerStatus& pmStatus) {
  pmStatus = platformExplorer_.getPMStatus();
}

void PlatformManagerHandler::getPmUnitInfo(
    PmUnitInfoResponse& pmUnitInfoResp,
    std::unique_ptr<PmUnitInfoRequest> pmUnitInfoReq) {
  auto pmStatus = platformExplorer_.getPMStatus();
  if (*pmStatus.explorationStatus() == ExplorationStatus::FAILED ||
      // TODO: 1) Retry on in progress 2) Ensure PM isn't running forever.
      *pmStatus.explorationStatus() == ExplorationStatus::IN_PROGRESS) {
    auto error = PlatformManagerError();
    error.errorCode() = PlatformManagerErrorCode::EXPLORATION_NOT_SUCCEEDED;
    error.message() = fmt::format(
        "PM exploration status is {}, not SUCCEEDED",
        static_cast<int>(*pmStatus.explorationStatus()));
    throw error;
  }
  try {
    auto pmUnitInfo =
        platformExplorer_.getPmUnitInfo(*pmUnitInfoReq->slotPath());
    pmUnitInfoResp.pmUnitInfo() = pmUnitInfo;
  } catch (std::exception& e) {
    auto error = PlatformManagerError();
    error.errorCode() = PlatformManagerErrorCode::PM_UNIT_NOT_FOUND;
    error.message() = fmt::format(
        "Unable to get PmUnitInfo. Reason: Invalid SlotPath {}. No PmUnit was explored",
        *pmUnitInfoReq->slotPath());
    throw error;
  }
}
} // namespace facebook::fboss::platform::platform_manager
