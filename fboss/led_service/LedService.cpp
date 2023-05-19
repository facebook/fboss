// Copyright 2021- Facebook. All rights reserved.

#include "fboss/led_service/LedService.h"

namespace facebook::fboss {

/*
 * kickStart
 *
 * Start the Led Service by creating platform specific LedManager class and
 * initialize the LED manager
 */
void LedService::kickStart() {
  pLedManager_ = std::make_unique<LedManager>();
  pLedManager_->initLedManager();
}

} // namespace facebook::fboss
