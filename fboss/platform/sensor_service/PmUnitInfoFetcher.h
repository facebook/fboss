// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <optional>
#include <string>

#include "fboss/platform/platform_manager/gen-cpp2/platform_manager_config_types.h"
#include "fboss/platform/sensor_service/PmClientFactory.h"

namespace facebook::fboss::platform::sensor_service {

class PmUnitInfoFetcher {
 public:
  explicit PmUnitInfoFetcher(
      const std::shared_ptr<PmClientFactory> pmClientFactory =
          std::make_shared<PmClientFactory>());
  virtual ~PmUnitInfoFetcher() = default;
  virtual std::optional<platform_manager::PmUnitInfo> fetch(
      const std::string& slotPath) const;

 private:
  std::shared_ptr<PmClientFactory> pmClientFactory_;
};

} // namespace facebook::fboss::platform::sensor_service
