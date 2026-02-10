// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <string>

#include "fboss/platform/fan_service/if/gen-cpp2/fan_service_config_types.h"

namespace facebook::fboss::platform {

class FanImpl {
 public:
  explicit FanImpl(const fan_service::Fan& fan);

  bool readFanIsPresentOnDevice() const;
  int readFanRpm() const;
  int readFanPwmPercent() const;

 private:
  const fan_service::Fan fan_;

  static float readSysfsAsFloat(const std::string& path);
};

} // namespace facebook::fboss::platform
