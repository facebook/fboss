// Copyright 2021-present Facebook. All Rights Reserved.

#include <folly/logging/xlog.h>

#include "fboss/platform/fan_service/FanServiceHandler.h"

namespace facebook::fboss::platform::fan_service {

FanServiceHandler::FanServiceHandler(std::shared_ptr<ControlLogic> controlLogic)
    : controlLogic_(std::move(controlLogic)) {
  XLOG(INFO) << "FanServiceHandler Started";
}

void FanServiceHandler::getFanStatuses(FanStatusesResponse& response) {
  response.fanStatuses() = controlLogic_->getFanStatuses();
}

void FanServiceHandler::setPwmHold(std::unique_ptr<PwmHoldRequest> req) {
  std::optional<int> pwm = req->pwm().to_optional();
  if (!pwm.has_value() || (pwm.value() >= 0 && pwm.value() <= 100)) {
    controlLogic_->setFanHold(pwm);
  } else {
    // pwm has_value() or we'd be in the `if` part
    XLOG(ERR) << "Bad PWM hold request: " << pwm.value();
  }
}

void FanServiceHandler::getPwmHold(PwmHoldStatus& status) {
  status.pwm().from_optional(controlLogic_->getFanHold());
}

} // namespace facebook::fboss::platform::fan_service
