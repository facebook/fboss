// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/platform/fan_service/if/gen-cpp2/fan_service_config_types.h"

namespace facebook::fboss::platform::fan_service {

class PidLogicBase {
 public:
  explicit PidLogicBase(const PidSetting& pidSetting)
      : pidSetting_(pidSetting) {}
  virtual ~PidLogicBase() = default;
  virtual int16_t calculatePwm(float measurement) = 0;
  float getSetPoint() const {
    return *pidSetting_.setPoint();
  }

  void updateLastPwm(int16_t lastPwm) {
    lastPwm_ = lastPwm;
  }

  int16_t getLastPwm() const {
    return lastPwm_;
  }

 protected:
  const PidSetting pidSetting_;
  int16_t lastPwm_{0};
};

class PidLogic : public PidLogicBase {
 public:
  explicit PidLogic(const PidSetting& pidSetting, unsigned int dT)
      : PidLogicBase(pidSetting), dT_(dT) {
    if (dT_ == 0) {
      throw std::invalid_argument("dT cannot be less than or equal to 0");
    }
  }
  int16_t calculatePwm(float measurement) override;
  float getLastError() const {
    return lastError_;
  }

 private:
  float integral_{0};
  float lastError_{0};
  const unsigned int dT_;
};

} // namespace facebook::fboss::platform::fan_service
