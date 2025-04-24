// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.
#pragma once
#include "fboss/platform/fw_util/if/gen-cpp2/fw_util_config_types.h"

namespace facebook::fboss::platform::fw_util {

class FwUtilVersionHandler {
 public:
  explicit FwUtilVersionHandler(
      const std::vector<std::pair<std::string, int>>& fwDeviceNamesByPrio,
      const fw_util_config::NewFwUtilConfig& fwUtilConfig)
      : fwDeviceNamesByPrio_(fwDeviceNamesByPrio),
        fwUtilConfig_(fwUtilConfig){};
  ~FwUtilVersionHandler() = default;

 private:
  std::vector<std::pair<std::string, int>> fwDeviceNamesByPrio_;
  fw_util_config::NewFwUtilConfig fwUtilConfig_{};
};

} // namespace facebook::fboss::platform::fw_util
