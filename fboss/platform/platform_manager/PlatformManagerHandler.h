// Copyright (c) 2004-present, Meta Platforms, Inc. and affiliates.
// All Rights Reserved.

#pragma once

#include "fboss/platform/platform_manager/PkgManager.h"
#include "fboss/platform/platform_manager/PlatformExplorer.h"
#include "fboss/platform/platform_manager/gen-cpp2/platform_manager_service_handlers.h"

namespace facebook::fboss::platform::platform_manager {
class PlatformManagerHandler
    : public apache::thrift::ServiceHandler<PlatformManagerService> {
 public:
  explicit PlatformManagerHandler(
      const PlatformExplorer& platformExplorer,
      const DataStore& dataStore,
      const PlatformConfig& config);
  void getPlatformSnapshot(PlatformSnapshot& response) override;
  void getLastPMStatus(PlatformManagerStatus& response) override;
  void getPmUnitInfo(
      PmUnitInfoResponse& response,
      std::unique_ptr<PmUnitInfoRequest> request) override;
  void getBspVersion(BspVersionResponse& response) override;
  void getPlatformName(std::string& response) override;

 private:
  const PlatformExplorer& platformExplorer_;
  const DataStore dataStore_;
  const package_manager::SystemInterface system_;
  const PlatformConfig& platformConfig_;
};

} // namespace facebook::fboss::platform::platform_manager
