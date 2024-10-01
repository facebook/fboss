// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/platform/fan_service/if/gen-cpp2/fan_service_config_types.h"

namespace facebook::fboss::platform::fan_service {

class PidLogic {
 public:
  explicit PidLogic(const PidSetting& pidSetting, unsigned int dT)
      : pidSetting_(pidSetting), dT_(dT) {
    if (dT_ <= 0) {
      throw std::invalid_argument("dT cannot less than or equal to 0");
    }
  }
  int16_t calculatePwm(float measurement);
  float getSetPoint() const {
    return *pidSetting_.setPoint();
  }

  void updateLastPwm(int16_t lastPwm) {
    lastPwm_ = lastPwm;
  }

  int16_t getLastPwm() const {
    return lastPwm_;
  }

  float getLastError() const {
    return lastError_;
  }

 private:
  const PidSetting pidSetting_;
  float integral_{0};
  float lastError_{0};
  int16_t lastPwm_{0};
  const unsigned int dT_;
};

} // namespace facebook::fboss::platform::fan_service
