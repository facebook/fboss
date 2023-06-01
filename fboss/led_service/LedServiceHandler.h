// Copyright 2021-present Facebook. All Rights Reserved.

#pragma once

#include <folly/futures/Future.h>
#include "common/fb303/cpp/FacebookBase2.h"
#include "fboss/led_service/LedService.h"

namespace facebook::fboss {
class LedServiceHandler
    : public ::facebook::fb303::FacebookBase2DeprecationMigration {
 public:
  explicit LedServiceHandler(std::unique_ptr<LedService> ledService);
  ~LedServiceHandler() override = default;

  facebook::fb303::cpp2::fb_status getStatus() override;

  LedService* getLedService() const {
    return service_.get();
  }

 private:
  // Internal pointer for LedService
  std::unique_ptr<LedService> service_{nullptr};
};
} // namespace facebook::fboss
