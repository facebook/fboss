// Copyright 2021- Facebook. All rights reserved.

#include "fboss/led_service/LedService.h"
#include "fboss/led_service/LedManagerInit.h"

namespace facebook::fboss {

/*
 * kickStart
 *
 * Start the Led Service by creating platform specific LedManager class and
 * initialize the LED manager
 */
void LedService::kickStart() {
  pLedManager_ = createLedManager();
  if (pLedManager_ == nullptr) {
    throw FbossError("Could not create LED Manager for this platform");
  }

  pLedManager_->initLedManager();
}

} // namespace facebook::fboss
