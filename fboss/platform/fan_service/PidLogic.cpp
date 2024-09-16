// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <folly/logging/xlog.h>

#include "fboss/platform/fan_service/PidLogic.h"

namespace facebook::fboss::platform::fan_service {

int16_t PidLogic::calculatePwm(float measurement) {
  float newPwm;
  float minVal = *pidSetting_.setPoint() - *pidSetting_.negHysteresis();
  float maxVal = *pidSetting_.setPoint() + *pidSetting_.posHysteresis();
  std::optional<float> error{};

  if (measurement < minVal) {
    error = minVal - measurement;
  } else if (measurement > maxVal) {
    error = maxVal - measurement;
  }

  if (error.has_value()) {
    integral_ = integral_ + *error * dT_;
    auto derivative = (*error - lastError_) / dT_;
    newPwm = (*pidSetting_.kp() * error.value()) +
        (*pidSetting_.ki() * integral_) + (*pidSetting_.kd() * derivative);
  } else {
    newPwm = lastPwm_;
  }

  if (measurement <= maxVal) {
    integral_ = newPwm / *pidSetting_.ki();
  }

  if (newPwm < 0) {
    newPwm = 0;
  } else if (newPwm > 100) {
    newPwm = 100;
  }

  XLOG(DBG1) << fmt::format(
      "Measurement: {}, Error: {}, Last PWM: {}, New PWM: {}",
      measurement,
      error.value_or(0),
      lastPwm_,
      newPwm);

  lastPwm_ = newPwm;
  lastError_ = error.value_or(0);

  return newPwm;
}

} // namespace facebook::fboss::platform::fan_service
