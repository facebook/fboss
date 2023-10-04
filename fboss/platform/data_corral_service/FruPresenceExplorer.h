// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/platform/data_corral_service/LedManager.h"
#include "fboss/platform/data_corral_service/if/gen-cpp2/led_manager_config_types.h"

namespace facebook::fboss::platform::data_corral_service {

class FruPresenceExplorer {
 public:
  explicit FruPresenceExplorer(
      const std::vector<FruConfig>& fruConfigs,
      std::shared_ptr<LedManager> ledManager);

  void detectFruPresence() const;

 private:
  const std::vector<FruConfig> fruConfigs_;
  const std::shared_ptr<LedManager> ledManager_;
};

} // namespace facebook::fboss::platform::data_corral_service
