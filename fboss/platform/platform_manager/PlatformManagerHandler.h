// Copyright (c) 2004-present, Meta Platforms, Inc. and affiliates.
// All Rights Reserved.

#pragma once

#include "fboss/platform/platform_manager/PlatformExplorer.h"
#include "fboss/platform/platform_manager/gen-cpp2/platform_manager_service_handlers.h"

namespace facebook::fboss::platform::platform_manager {
class PlatformManagerHandler
    : public apache::thrift::ServiceHandler<PlatformManagerService> {
 public:
  explicit PlatformManagerHandler(const PlatformExplorer& platformExplorer);
  void getPlatformSnapshot(PlatformSnapshot& response) override;
  void getLastPMStatus(PlatformManagerStatus& response) override;
  void getPmUnitInfo(
      PmUnitInfoResponse& response,
      std::unique_ptr<PmUnitInfoRequest> request) override;

 private:
  const PlatformExplorer& platformExplorer_;
};

} // namespace facebook::fboss::platform::platform_manager
