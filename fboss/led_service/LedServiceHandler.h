// Copyright 2021-present Facebook. All Rights Reserved.

#pragma once

#include <folly/futures/Future.h>
#include "common/fb303/cpp/FacebookBase2.h"
#include "fboss/led_service/LedService.h"
#include "fboss/led_service/if/gen-cpp2/LedService.h"

namespace facebook::fboss {
class LedServiceHandler : public facebook::fboss::led_service::LedServiceSvIf {
 public:
  explicit LedServiceHandler(std::unique_ptr<LedService> ledService);
  ~LedServiceHandler() override = default;

  LedService* getLedService() const {
    return service_.get();
  }

  void setExternalLedState(
      int32_t /* portNum */,
      PortLedExternalState /* ledState */) override {}

 private:
  // Internal pointer for LedService
  std::unique_ptr<LedService> service_{nullptr};
};
} // namespace facebook::fboss
