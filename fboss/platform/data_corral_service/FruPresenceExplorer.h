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

  // Detect whether the frus in the system are present.
  // Program the LEDs appropriately based on their presence.
  void detectFruPresence();

  // Return true if a particular fruType is considered present
  // i.e every frus of the type is present.
  // Return false otherwise or detectFruPresence() hasn't been done.
  bool isPresent(const std::string& fruType) const;

  // Return true if every fruType is present.
  // Return false otherwise or detectFruPresence() hasn't been done.
  bool isAllPresent() const;

 private:
  bool detectFruSysfsPresence(const SysfsFileHandle& handle);
  bool detectFruGpioPresence(const GpioLineHandle& handle);

  const std::vector<FruConfig> fruConfigs_;
  const std::shared_ptr<LedManager> ledManager_;
  std::unordered_map<std::string, bool> fruTypePresence_;
};

} // namespace facebook::fboss::platform::data_corral_service
