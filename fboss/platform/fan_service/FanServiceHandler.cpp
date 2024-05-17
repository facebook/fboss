// Copyright 2021-present Facebook. All Rights Reserved.

#include <folly/logging/xlog.h>

#include "fboss/platform/fan_service/FanServiceHandler.h"

namespace facebook::fboss::platform::fan_service {

FanServiceHandler::FanServiceHandler(
    std::unique_ptr<FanServiceImpl> fanServiceImpl)
    : fanServiceImpl_(std::move(fanServiceImpl)) {
  XLOG(INFO) << "FanServiceHandler Started";
}

void FanServiceHandler::getFanStatuses(FanStatusesResponse& response) {
  response.fanStatuses() = fanServiceImpl_->getFanStatuses();
}

void FanServiceHandler::setHold(std::unique_ptr<HoldRequest> req) {
  std::optional<int> pwm = req->pwm().to_optional();
  if (!pwm.has_value() || (pwm.value() >= 0 && pwm.value() <= 100)) {
    fanServiceImpl_->setFanHold(pwm);
  }
}

void FanServiceHandler::getHold(HoldStatus& status) {
  status.pwm().from_optional(fanServiceImpl_->getFanHold());
}

} // namespace facebook::fboss::platform::fan_service
