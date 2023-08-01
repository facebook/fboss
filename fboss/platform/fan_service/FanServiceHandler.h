// Copyright 2021-present Facebook. All Rights Reserved.

// Handler class is the interface between FanService and Thrift call handlers
#pragma once
#include <folly/futures/Future.h>
#include "common/fb303/cpp/FacebookBase2.h"
#include "fboss/platform/fan_service/FanService.h"

namespace facebook::fboss::platform::fan_service {
class FanServiceHandler
    : public ::facebook::fb303::FacebookBase2DeprecationMigration {
 public:
  FanServiceHandler(std::unique_ptr<FanService> fanService);
  // Make compiler happy
  ~FanServiceHandler() override = default;
  facebook::fb303::cpp2::fb_status getStatus() override;
  // Simple functions are just implemented here.
  FanService* getFanService() const {
    return service_.get();
  }

 private:
  // Internal pointer for FanService.
  std::unique_ptr<FanService> service_{nullptr};
};
} // namespace facebook::fboss::platform::fan_service
