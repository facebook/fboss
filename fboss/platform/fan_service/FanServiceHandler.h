// Copyright 2021-present Facebook. All Rights Reserved.

// Handler class handles Fan Service thrift calls.
#pragma once
#include <folly/futures/Future.h>
#include "common/fb303/cpp/FacebookBase2.h"
#include "fboss/platform/fan_service/FanServiceImpl.h"

namespace facebook::fboss::platform::fan_service {
class FanServiceHandler
    : public ::facebook::fb303::FacebookBase2DeprecationMigration {
 public:
  explicit FanServiceHandler(std::unique_ptr<FanServiceImpl> fanServiceImpl);
  // Make compiler happy
  ~FanServiceHandler() override = default;
  facebook::fb303::cpp2::fb_status getStatus() override;
  // Simple functions are just implemented here.
  FanServiceImpl* getFanServiceImpl() const {
    return fanServiceImpl_.get();
  }

 private:
  // Internal pointer for FanServiceImpl.
  std::unique_ptr<FanServiceImpl> fanServiceImpl_{nullptr};
};
} // namespace facebook::fboss::platform::fan_service
