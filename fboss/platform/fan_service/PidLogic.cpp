// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <folly/logging/xlog.h>

#include "fboss/platform/fan_service/PidLogic.h"

namespace facebook::fboss::platform::fan_service {

int16_t PidLogic::calculatePwm(float measurement) {
  float newPwm, pwmDelta{0};
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
    pwmDelta = (*pidSetting_.kp() * error.value()) +
        (*pidSetting_.ki() * integral_) + (*pidSetting_.kd() * derivative);
    newPwm = lastPwm_ + pwmDelta;
  } else {
    newPwm = lastPwm_;
  }

  if (newPwm < 0) {
    newPwm = 0;
  } else if (newPwm > 100) {
    newPwm = 100;
  }

  XLOG(DBG2) << fmt::format(
      "Measurement: {}, Error: {}, Last PWM: {}, PWM Delta: {}, New PWM: {}",
      measurement,
      error.value_or(0),
      lastPwm_,
      pwmDelta,
      newPwm);

  lastPwm_ = newPwm;
  lastError_ = error.value_or(0);

  return newPwm;
}

} // namespace facebook::fboss::platform::fan_service
