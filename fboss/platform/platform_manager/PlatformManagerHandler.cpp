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
} // namespace facebook::fboss::platform::platform_manager
